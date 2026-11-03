<template>
  <div class="device-page">
    <div class="page-header">
      <h2 class="page-title">设备实时数据</h2>
      <div class="status-row">
        <span class="status-dot" :class="statusClass"></span>
        <span class="status-text">{{ statusText }}</span>
        <button class="clear-btn" @click="clearMessages" :disabled="!messages.length">
          清空
        </button>
      </div>
    </div>

    <!-- ==================== 库房温湿度 ==================== -->
    <div class="env-panel">
      <div class="env-card env-temp">
        <div class="env-icon">
          <svg viewBox="0 0 24 24" width="28" height="28">
            <path fill="currentColor" d="M15 13V5c0-1.66-1.34-3-3-3S9 3.34 9 5v8c-1.21.91-2 2.37-2 4 0 2.76 2.24 5 5 5s5-2.24 5-5c0-1.63-.79-3.09-2-4zm-4-8c0-.55.45-1 1-1s1 .45 1 1h-1v1h1v2h-1v1h1v2h-2V5z"/>
          </svg>
        </div>
        <div class="env-info">
          <span class="env-label">温度</span>
          <span class="env-value" v-if="env.temp !== null">
            {{ env.temp.toFixed(1) }}<small>°C</small>
          </span>
          <span class="env-value env-empty" v-else>--<small>°C</small></span>
        </div>
      </div>

      <div class="env-card env-humi">
        <div class="env-icon">
          <svg viewBox="0 0 24 24" width="28" height="28">
            <path fill="currentColor" d="M12 2C8 7 6 10.5 6 14c0 3.31 2.69 6 6 6s6-2.69 6-6c0-3.5-2-7-6-12zm0 16c-2.21 0-4-1.79-4-4 0-2.2 1.2-4.78 4-8.43 2.8 3.65 4 6.23 4 8.43 0 2.21-1.79 4-4 4z"/>
          </svg>
        </div>
        <div class="env-info">
          <span class="env-label">湿度</span>
          <span class="env-value" v-if="env.humi !== null">
            {{ env.humi.toFixed(1) }}<small>%</small>
          </span>
          <span class="env-value env-empty" v-else>--<small>%</small></span>
        </div>
      </div>

      <div class="env-meta">
        <div class="meta-row" v-if="env.seq !== null">
          <span class="meta-label">序号</span>
          <span class="meta-val">#{{ env.seq }}</span>
        </div>
        <div class="meta-row" v-if="env.uptime_ms !== null">
          <span class="meta-label">运行</span>
          <span class="meta-val">{{ formatUptime(env.uptime_ms) }}</span>
        </div>
        <div class="meta-row" v-if="env.updatedAt">
          <span class="meta-label">更新</span>
          <span class="meta-val">{{ formatTime(env.updatedAt) }}</span>
        </div>
        <div class="meta-row meta-empty" v-if="env.seq === null && !env.updatedAt">
          等待数据上报...
        </div>
      </div>
    </div>

    <!-- ==================== 原始消息列表 ==================== -->
    <div class="message-panel">
      <div class="panel-title">消息记录</div>

      <div v-if="!messages.length" class="empty-state">
        <svg viewBox="0 0 24 24" width="48" height="48">
          <path fill="currentColor" d="M20 2H4c-1.1 0-2 .9-2 2v18l4-4h14c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm0 14H5.17L4 17.17V4h16v12z"/>
        </svg>
        <p>暂无消息，等待设备上报...</p>
      </div>

      <transition-group v-else name="msg-list" tag="ul" class="message-list">
        <li v-for="msg in messages" :key="msg._key" class="message-item">
          <div class="msg-head">
            <span class="msg-topic">{{ msg.topic }}</span>
            <span class="msg-time">
              <span v-if="msg.decrypted === false" class="msg-cipher-tag">密文</span>
              {{ formatTime(msg.timestamp) }}
            </span>
          </div>
          <pre class="msg-body">{{ msg.message }}</pre>
        </li>
      </transition-group>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, reactive, onMounted, onBeforeUnmount } from 'vue'

// 温湿度数据上报的主题
const ENV_TOPIC = '1138_MQTT_Pub_JSON'

const messages = ref([])
const connected = ref(false)
const connecting = ref(true)

// 库房环境数据（最新值）
const env = reactive({
  temp: null,
  humi: null,
  seq: null,
  uptime_ms: null,
  student_id: null,
  updatedAt: null
})

let eventSource = null
let keyCounter = 0

const statusClass = computed(() => {
  if (connected.value) return 'dot-online'
  return connecting.value ? 'dot-connecting' : 'dot-offline'
})

const statusText = computed(() => {
  if (connected.value) return '已连接'
  return connecting.value ? '连接中...' : '已断开'
})

// 给每条消息打一个唯一 key，供 transition-group 使用
function withKey(msg) {
  return { ...msg, _key: ++keyCounter }
}

// 解析温湿度消息，更新 env
function applyEnv(msg) {
  if (msg.topic !== ENV_TOPIC) return
  try {
    const d = JSON.parse(msg.message)
    if (typeof d.temp === 'number') env.temp = d.temp
    if (typeof d.humi === 'number') env.humi = d.humi
    if (typeof d.seq === 'number') env.seq = d.seq
    if (typeof d.uptime_ms === 'number') env.uptime_ms = d.uptime_ms
    if (d.student_id) env.student_id = d.student_id
    env.updatedAt = msg.timestamp
  } catch {
    // 非 JSON 或解析失败，忽略
  }
}

function connect() {
  const token = localStorage.getItem('token')
  if (!token) return

  connecting.value = true
  eventSource = new EventSource(`/api/mqtt/stream?token=${encodeURIComponent(token)}`)

  eventSource.onopen = () => {
    connected.value = true
    connecting.value = false
  }

  eventSource.onmessage = (e) => {
    try {
      const payload = JSON.parse(e.data)
      if (payload.type === 'history') {
        // 历史消息一次性填充（已是「新在前」的顺序）
        messages.value = payload.data.map(withKey)
        // 用历史里最新的一条温湿度初始化面板（数组首个匹配项即最新）
        const latestEnv = payload.data.find(m => m.topic === ENV_TOPIC)
        if (latestEnv) applyEnv(latestEnv)
      } else if (payload.type === 'message') {
        // 新消息插到最前
        messages.value.unshift(withKey(payload.data))
        if (messages.value.length > 100) messages.value.pop()
        // 如果是温湿度主题，更新面板
        applyEnv(payload.data)
      }
    } catch {
      // 忽略心跳等非 JSON 数据
    }
  }

  eventSource.onerror = () => {
    connected.value = false
    connecting.value = true
    // EventSource 会自动重连，这里只更新状态
  }
}

function clearMessages() {
  messages.value = []
}

function formatTime(iso) {
  const d = new Date(iso)
  return d.toLocaleTimeString('zh-CN', { hour12: false }) +
      '.' + String(d.getMilliseconds()).padStart(3, '0')
}

// 毫秒运行时长转为易读格式
function formatUptime(ms) {
  const totalSec = Math.floor(ms / 1000)
  const h = Math.floor(totalSec / 3600)
  const m = Math.floor((totalSec % 3600) / 60)
  const s = totalSec % 60
  if (h > 0) return `${h}时${m}分`
  if (m > 0) return `${m}分${s}秒`
  return `${s}秒`
}

onMounted(connect)

onBeforeUnmount(() => {
  if (eventSource) {
    eventSource.close()
    eventSource = null
  }
})
</script>

<style scoped>
.device-page {
  flex: 1;
  width: 100%;
  display: flex;
  flex-direction: column;
  padding: 32px 24px;
  box-sizing: border-box;
  gap: 20px;
  background-color: var(--bg-page);
  transition: background-color 0.3s ease;
}

.page-header {
  width: 100%;
  max-width: 800px;
  margin: 0 auto;
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 12px;
}

.page-title {
  margin: 0;
  font-size: 1.4rem;
  font-weight: 700;
  color: var(--text-primary);
  transition: color 0.3s ease;
}

.status-row {
  display: flex;
  align-items: center;
  gap: 10px;
}

.status-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  flex-shrink: 0;
}
.dot-online     { background-color: #42b883; box-shadow: 0 0 0 3px rgba(66,184,131,0.2); }
.dot-connecting { background-color: #e6a23c; animation: pulse 1.2s infinite; }
.dot-offline    { background-color: #f56c6c; }

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50%      { opacity: 0.35; }
}

.status-text {
  font-size: 0.9rem;
  color: var(--text-secondary);
  transition: color 0.3s ease;
}

.clear-btn {
  margin-left: 8px;
  padding: 5px 14px;
  border: 1px solid var(--border-input);
  border-radius: 6px;
  background-color: var(--bg-card);
  color: var(--text-secondary);
  font-size: 0.85rem;
  cursor: pointer;
  transition: all 0.2s;
}
.clear-btn:hover:not(:disabled) { border-color: #f56c6c; color: #f56c6c; }
.clear-btn:disabled { opacity: 0.5; cursor: not-allowed; }

/* ==================== 温湿度面板 ==================== */
.env-panel {
  width: 100%;
  max-width: 800px;
  margin: 0 auto;
  display: grid;
  grid-template-columns: 1fr 1fr auto;
  gap: 16px;
}

.env-card {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 20px 24px;
  border-radius: 12px;
  border: 1px solid var(--border-color);
  box-shadow: var(--shadow-sm);
  transition: background-color 0.3s ease, border-color 0.3s ease;
}

.env-temp {
  background: linear-gradient(135deg, rgba(245,108,108,0.08), rgba(245,108,108,0.02));
}
.env-temp .env-icon { color: #f56c6c; }

.env-humi {
  background: linear-gradient(135deg, rgba(64,158,255,0.08), rgba(64,158,255,0.02));
}
.env-humi .env-icon { color: #409eff; }

.env-icon {
  width: 52px; height: 52px;
  border-radius: 50%;
  display: flex; align-items: center; justify-content: center;
  background-color: var(--bg-card);
  flex-shrink: 0;
}

.env-info { display: flex; flex-direction: column; gap: 2px; }

.env-label {
  font-size: 0.85rem;
  color: var(--text-muted);
  transition: color 0.3s ease;
}

.env-value {
  font-size: 2rem;
  font-weight: 700;
  color: var(--text-primary);
  line-height: 1.1;
  font-variant-numeric: tabular-nums;
  transition: color 0.3s ease;
}
.env-value small { font-size: 1rem; font-weight: 500; margin-left: 2px; color: var(--text-muted); }
.env-empty { color: var(--text-light); }

.env-meta {
  display: flex;
  flex-direction: column;
  justify-content: center;
  gap: 6px;
  padding: 16px 20px;
  border-radius: 12px;
  background-color: var(--bg-card);
  border: 1px solid var(--border-color);
  box-shadow: var(--shadow-sm);
  min-width: 150px;
  transition: background-color 0.3s ease, border-color 0.3s ease;
}

.meta-row { display: flex; align-items: center; justify-content: space-between; gap: 12px; }
.meta-label { font-size: 0.8rem; color: var(--text-muted); }
.meta-val {
  font-size: 0.85rem;
  color: var(--text-regular);
  font-family: monospace;
  transition: color 0.3s ease;
}
.meta-empty { color: var(--text-light); font-size: 0.85rem; justify-content: center; }

/* ==================== 消息列表 ==================== */
.message-panel {
  width: 100%;
  max-width: 800px;
  margin: 0 auto;
  flex: 1;
  background-color: var(--bg-card);
  border: 1px solid var(--border-color);
  border-radius: 12px;
  box-shadow: var(--shadow-sm);
  overflow: hidden;
  display: flex;
  flex-direction: column;
  transition: background-color 0.3s ease, border-color 0.3s ease;
}

.panel-title {
  padding: 14px 18px;
  font-size: 0.9rem;
  font-weight: 600;
  color: var(--text-secondary);
  border-bottom: 1px solid var(--border-color);
  transition: color 0.3s ease, border-color 0.3s ease;
}

.empty-state {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 12px;
  color: var(--text-light);
  padding: 60px 20px;
}
.empty-state p { margin: 0; font-size: 0.95rem; }

.message-list {
  list-style: none;
  margin: 0;
  padding: 8px;
  overflow-y: auto;
  flex: 1;
}

.message-item {
  padding: 12px 14px;
  border-radius: 8px;
  background-color: var(--bg-hover);
  margin-bottom: 8px;
  transition: background-color 0.3s ease;
}

.msg-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 6px;
}

.msg-topic {
  font-size: 0.85rem;
  font-weight: 600;
  color: #42b883;
  word-break: break-all;
}

.msg-time {
  font-size: 0.78rem;
  color: var(--text-muted);
  white-space: nowrap;
  flex-shrink: 0;
}

.msg-cipher-tag {
  display: inline-block;
  margin-right: 8px;
  padding: 1px 6px;
  border-radius: 4px;
  background-color: #fef0f0;
  color: #f56c6c;
  font-size: 0.72rem;
}

.msg-body {
  margin: 0;
  font-family: 'SFMono-Regular', Consolas, 'Liberation Mono', monospace;
  font-size: 0.85rem;
  color: var(--text-regular);
  white-space: pre-wrap;
  word-break: break-all;
  transition: color 0.3s ease;
}

/* 新消息插入动画 */
.msg-list-enter-active { transition: all 0.3s ease; }
.msg-list-enter-from   { opacity: 0; transform: translateY(-10px); }
.msg-list-move         { transition: transform 0.3s ease; }

@media (max-width: 768px) {
  .device-page { padding: 24px 16px; }
  .env-panel { grid-template-columns: 1fr; }
  .env-meta { flex-direction: row; flex-wrap: wrap; min-width: 0; }
}
</style>
