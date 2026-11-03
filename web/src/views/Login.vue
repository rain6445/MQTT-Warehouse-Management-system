<template>
  <div class="login-fullscreen">
    <!-- 左侧装饰区域（深浅模式下保持不变） -->
    <div class="login-left">
      <div class="left-content">
        <svg viewBox="0 0 128 128" width="64" height="64">
          <path fill="#fff" d="M78.8,10L64,35.4L49.2,10H0l64,110l64-110H78.8z" opacity="0.9"/>
          <path fill="#fff" d="M78.8,10L64,35.4L49.2,10H25.6L64,76.5l38.4-66.5H78.8z" opacity="0.5"/>
        </svg>
        <h1 class="brand-name">MYWEB</h1>
        <p class="brand-desc">统一管理您的设备与账户</p>

        <ul class="feature-list">
          <li>
            <svg viewBox="0 0 24 24" width="18" height="18"><path fill="currentColor" d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>
            安全的设备管理平台
          </li>
          <li>
            <svg viewBox="0 0 24 24" width="18" height="18"><path fill="currentColor" d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>
            实时数据监控与统计
          </li>
          <li>
            <svg viewBox="0 0 24 24" width="18" height="18"><path fill="currentColor" d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>
            灵活的权限管理系统
          </li>
        </ul>
      </div>
    </div>

    <!-- 右侧登录表单 -->
    <div class="login-right">
      <div class="login-card">
        <div class="login-header">
          <h2 class="login-title">欢迎回来</h2>
          <p class="login-subtitle">请输入您的账号信息以继续</p>
        </div>

        <form @submit.prevent="handleLogin" class="login-form">
          <div class="form-item">
            <label for="username">用户名</label>
            <div class="input-wrapper">
              <svg class="input-icon" viewBox="0 0 24 24" width="18" height="18">
                <path fill="currentColor" d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z"/>
              </svg>
              <input
                  id="username"
                  v-model="form.username"
                  type="text"
                  placeholder="请输入用户名"
                  autocomplete="username"
                  :disabled="loading"
              />
            </div>
          </div>

          <div class="form-item">
            <label for="password">密码</label>
            <div class="input-wrapper">
              <svg class="input-icon" viewBox="0 0 24 24" width="18" height="18">
                <path fill="currentColor" d="M18 8h-1V6c0-2.76-2.24-5-5-5S7 3.24 7 6v2H6c-1.1 0-2 .9-2 2v10c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V10c0-1.1-.9-2-2-2zm-6 9c-1.1 0-2-.9-2-2s.9-2 2-2 2 .9 2 2-.9 2-2 2zm3.1-9H8.9V6c0-1.71 1.39-3.1 3.1-3.1 1.71 0 3.1 1.39 3.1 3.1v2z"/>
              </svg>
              <input
                  id="password"
                  v-model="form.password"
                  :type="showPassword ? 'text' : 'password'"
                  placeholder="请输入密码"
                  autocomplete="current-password"
                  :disabled="loading"
              />
              <button
                  type="button"
                  class="eye-btn"
                  @click="showPassword = !showPassword"
                  :aria-label="showPassword ? '隐藏密码' : '显示密码'"
              >
                <svg v-if="!showPassword" viewBox="0 0 24 24" width="18" height="18">
                  <path fill="currentColor" d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>
                </svg>
                <svg v-else viewBox="0 0 24 24" width="18" height="18">
                  <path fill="currentColor" d="M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46A11.804 11.804 0 0 0 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z"/>
                </svg>
              </button>
            </div>
          </div>

          <transition name="error-fade">
            <p v-if="errorMsg" class="error-msg" role="alert">{{ errorMsg }}</p>
          </transition>

          <button type="submit" class="login-btn" :disabled="loading">
            <span v-if="loading" class="loading-spinner"></span>
            <span>{{ loading ? '登录中...' : '登 录' }}</span>
          </button>
        </form>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'

const form = ref({ username: '', password: '' })
const errorMsg = ref('')
const loading = ref(false)
const showPassword = ref(false)

async function handleLogin() {
  errorMsg.value = ''

  if (!form.value.username || !form.value.password) {
    errorMsg.value = '请输入用户名和密码'
    return
  }

  loading.value = true

  try {
    const res = await fetch('/api/auth/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(form.value)
    })

    const data = await res.json()

    if (!res.ok) {
      errorMsg.value = data.message || '登录失败，请重试'
      return
    }

    localStorage.setItem('token', data.token)
    localStorage.setItem('user', JSON.stringify(data.user))

    window.location.href = '/home'
  } catch {
    errorMsg.value = '网络错误，无法连接到服务器'
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
/* ==================== 全屏布局 ==================== */
.login-fullscreen {
  display: flex;
  min-height: 100vh;
  width: 100vw;
}

/* ==================== 左侧装饰区（固定绿色，不随主题变化） ==================== */
.login-left {
  flex: 1;
  background: linear-gradient(135deg, #42b883 0%, #2c7a57 100%);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 60px 40px;
  position: relative;
  overflow: hidden;
}

.login-left::before {
  content: '';
  position: absolute;
  width: 400px;
  height: 400px;
  border-radius: 50%;
  background: rgba(255, 255, 255, 0.05);
  top: -100px;
  right: -100px;
}

.login-left::after {
  content: '';
  position: absolute;
  width: 300px;
  height: 300px;
  border-radius: 50%;
  background: rgba(255, 255, 255, 0.05);
  bottom: -80px;
  left: -80px;
}

.left-content {
  position: relative;
  z-index: 1;
  color: white;
}

.brand-name {
  font-size: 2.8rem;
  font-weight: 800;
  letter-spacing: 4px;
  margin: 20px 0 10px;
  color: white;
}

.brand-desc {
  font-size: 1.1rem;
  color: rgba(255, 255, 255, 0.8);
  margin: 0 0 40px;
}

.feature-list {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.feature-list li {
  display: flex;
  align-items: center;
  gap: 10px;
  font-size: 1rem;
  color: rgba(255, 255, 255, 0.9);
}

.feature-list svg { flex-shrink: 0; }

/* ==================== 右侧表单区 ==================== */
.login-right {
  width: 480px;
  display: flex;
  align-items: center;
  justify-content: center;
  /* 跟随主题的页面背景色 */
  background-color: var(--bg-page);
  padding: 40px 24px;
  transition: background-color 0.3s ease;
}

.login-card {
  width: 100%;
  max-width: 380px;
  /* 跟随主题的卡片背景色 */
  background-color: var(--bg-card);
  border: 1px solid var(--border-color);
  border-radius: 12px;
  padding: 48px 40px;
  box-shadow: var(--shadow-md);
  transition: background-color 0.3s ease, border-color 0.3s ease, box-shadow 0.3s ease;
}

.login-header { margin-bottom: 36px; }

.login-title {
  font-size: 1.8rem;
  font-weight: 700;
  color: var(--text-primary);
  margin: 0 0 8px;
  transition: color 0.3s ease;
}

.login-subtitle {
  color: var(--text-muted);
  font-size: 0.9rem;
  margin: 0;
  transition: color 0.3s ease;
}

/* ==================== 表单样式 ==================== */
.login-form {
  display: flex;
  flex-direction: column;
  gap: 22px;
}

.form-item {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.form-item label {
  font-size: 0.875rem;
  color: var(--text-secondary);
  font-weight: 500;
  transition: color 0.3s ease;
}

.input-wrapper {
  position: relative;
  display: flex;
  align-items: center;
}

.input-icon {
  position: absolute;
  left: 12px;
  color: var(--text-light);
  pointer-events: none;
  flex-shrink: 0;
  transition: color 0.3s ease;
}

.input-wrapper input {
  width: 100%;
  padding: 12px 40px 12px 40px;
  border: 1px solid var(--border-input);
  border-radius: 6px;
  font-size: 1rem;
  color: var(--text-regular);
  background-color: var(--bg-input);
  outline: none;
  transition: border-color 0.2s, box-shadow 0.2s, background-color 0.3s ease, color 0.3s ease;
}

.input-wrapper input::placeholder {
  color: var(--text-light);
}

.input-wrapper input:focus {
  border-color: #42b883;
  box-shadow: 0 0 0 3px rgba(66, 184, 131, 0.12);
}

.input-wrapper input:disabled {
  background-color: var(--bg-hover);
  cursor: not-allowed;
  opacity: 0.7;
}

.eye-btn {
  position: absolute;
  right: 12px;
  background: none;
  border: none;
  cursor: pointer;
  padding: 0;
  color: var(--text-light);
  display: flex;
  align-items: center;
  transition: color 0.2s;
}
.eye-btn:hover { color: #42b883; }

/* ==================== 错误提示 ==================== */
.error-msg {
  color: #f56c6c;
  font-size: 0.875rem;
  text-align: center;
  margin: 0;
  padding: 10px 12px;
  background-color: #fef0f0;
  border-radius: 6px;
  border: 1px solid #fde2e2;
}

/* 深色模式下错误提示也需要适配 */
:global(.dark) .error-msg {
  background-color: rgba(245, 108, 108, 0.1);
  border-color: rgba(245, 108, 108, 0.3);
}

/* ==================== 登录按钮 ==================== */
.login-btn {
  width: 100%;
  padding: 14px;
  background-color: #42b883;
  color: white;
  border: none;
  border-radius: 6px;
  font-size: 1rem;
  font-weight: 600;
  cursor: pointer;
  transition: background-color 0.2s, transform 0.1s;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  margin-top: 4px;
}

.login-btn:hover:not(:disabled) { background-color: #33a06f; }
.login-btn:active:not(:disabled) { transform: scale(0.98); }
.login-btn:disabled { background-color: #a8d8c2; cursor: not-allowed; }

/* ==================== 加载动画 ==================== */
.loading-spinner {
  width: 16px;
  height: 16px;
  border: 2px solid rgba(255, 255, 255, 0.4);
  border-top-color: white;
  border-radius: 50%;
  animation: spin 0.7s linear infinite;
  flex-shrink: 0;
}

@keyframes spin { to { transform: rotate(360deg); } }

/* 错误动画 */
.error-fade-enter-active, .error-fade-leave-active { transition: all 0.25s ease; }
.error-fade-enter-from, .error-fade-leave-to { opacity: 0; transform: translateY(-4px); }

/* ==================== 响应式 ==================== */
@media (max-width: 768px) {
  .login-left { display: none; }
  .login-right {
    width: 100%;
    padding: 40px 20px;
  }
}
</style>
