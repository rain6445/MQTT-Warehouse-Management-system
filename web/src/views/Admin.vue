<template>
  <div class="admin-page">

    <div class="page-header">
      <div class="header-left">
        <h1 class="page-title">网站管理</h1>
        <p class="page-subtitle">管理所有注册用户</p>
      </div>
      <div class="header-right">
        <div class="header-stats" v-if="!loading">
          <div class="stat-card">
            <span class="stat-num">{{ users.length }}</span>
            <span class="stat-label">总用户数</span>
          </div>
          <div class="stat-card admin-stat">
            <span class="stat-num">{{ adminCount }}</span>
            <span class="stat-label">管理员</span>
          </div>
        </div>
        <button class="add-btn" @click="openAddDialog" :disabled="loading">
          <svg viewBox="0 0 24 24" width="16" height="16">
            <path fill="currentColor" d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z"/>
          </svg>
          添加用户
        </button>
      </div>
    </div>

    <div class="toolbar">
      <div class="search-wrapper">
        <svg class="search-icon" viewBox="0 0 24 24" width="18" height="18">
          <path fill="currentColor" d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0 0 16 9.5 6.5 6.5 0 1 0 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z"/>
        </svg>
        <input
            v-model="searchText"
            type="text"
            placeholder="搜索用户名..."
            class="search-input"
            aria-label="搜索用户"
        />
        <button v-if="searchText" class="search-clear" @click="searchText = ''" aria-label="清除搜索">
          <svg viewBox="0 0 24 24" width="16" height="16">
            <path fill="currentColor" d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/>
          </svg>
        </button>
      </div>
      <span class="result-count" v-if="!loading">
        共 <b>{{ filteredUsers.length }}</b> 条结果
      </span>
    </div>

    <div v-if="loading" class="status-area">
      <div class="spinner"></div>
      <p>加载中...</p>
    </div>

    <div v-else-if="error" class="status-area">
      <svg viewBox="0 0 24 24" width="48" height="48">
        <path fill="#f56c6c" d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-2h2v2zm0-4h-2V7h2v6z"/>
      </svg>
      <p>{{ error }}</p>
      <button class="retry-btn" @click="fetchUsers">重试</button>
    </div>

    <template v-else>
      <div class="table-wrapper">
        <table class="user-table" aria-label="用户列表">
          <thead>
          <tr>
            <th scope="col">ID</th>
            <th scope="col">用户名</th>
            <th scope="col">角色</th>
            <th scope="col">注册时间</th>
            <th scope="col">NFC卡</th>
            <th scope="col">操作</th>
          </tr>
          </thead>
          <tbody>
          <tr v-for="user in pagedUsers" :key="user.id" class="user-row">
            <td class="td-id">{{ user.id }}</td>
            <td class="td-username">
              <div class="user-cell">
                <div class="avatar" :style="{ backgroundColor: avatarColor(user.username) }">
                  {{ user.username.charAt(0).toUpperCase() }}
                </div>
                <span>{{ user.username }}</span>
              </div>
            </td>
            <td class="td-role">
                <span :class="['role-badge', user.is_admin ? 'badge-admin' : 'badge-user']">
                  {{ user.is_admin ? '管理员' : '普通用户' }}
                </span>
            </td>
            <td class="td-date">{{ formatDate(user.created_at) }}</td>
            <td class="td-nfc">
              <span v-if="user.uid" class="nfc-bound" :title="user.uid">
                <svg viewBox="0 0 24 24" width="14" height="14">
                  <path fill="currentColor" d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/>
                </svg>
                {{ user.uid }}
              </span>
              <span v-else class="nfc-unbound">未绑定</span>
            </td>
            <td class="td-action">
              <button
                  class="bind-btn"
                  @click="openBindDialog(user)"
                  :disabled="actionLoading || binding"
                  :aria-label="`为 ${user.username} 绑定NFC卡`"
              >
                <svg viewBox="0 0 24 24" width="14" height="14">
                  <path fill="currentColor" d="M20 4H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2zm0 14H4v-6h16v6zm0-10H4V6h16v2z"/>
                </svg>
                绑卡
              </button>
              <button
                  class="reset-btn"
                  @click="openResetConfirm(user)"
                  :disabled="actionLoading"
                  :aria-label="`重置 ${user.username} 的密码`"
              >
                <svg viewBox="0 0 24 24" width="14" height="14">
                  <path fill="currentColor" d="M12 5V1L7 6l5 5V7c3.31 0 6 2.69 6 6s-2.69 6-6 6-6-2.69-6-6H4c0 4.42 3.58 8 8 8s8-3.58 8-8-3.58-8-8-8z"/>
                </svg>
                重置
              </button>
              <button
                  v-if="!user.is_admin"
                  class="delete-btn"
                  @click="openDeleteConfirm(user)"
                  :disabled="actionLoading"
                  :aria-label="`删除用户 ${user.username}`"
              >
                <svg viewBox="0 0 24 24" width="14" height="14">
                  <path fill="currentColor" d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/>
                </svg>
                删除
              </button>
              <span v-else class="admin-lock" title="管理员账号受保护">
                  <svg viewBox="0 0 24 24" width="15" height="15">
                    <path fill="currentColor" d="M18 8h-1V6c0-2.76-2.24-5-5-5S7 3.24 7 6v2H6c-1.1 0-2 .9-2 2v10c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V10c0-1.1-.9-2-2-2zm-6 9c-1.1 0-2-.9-2-2s.9-2 2-2 2 .9 2 2-.9 2-2 2zm3.1-9H8.9V6c0-1.71 1.39-3.1 3.1-3.1 1.71 0 3.1 1.39 3.1 3.1v2z"/>
                  </svg>
                </span>
            </td>
          </tr>

          <tr v-if="filteredUsers.length === 0">
            <td colspan="6" class="empty-cell">
              <div class="empty-state">
                <svg viewBox="0 0 24 24" width="40" height="40">
                  <path fill="#c0c4cc" d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0 0 16 9.5 6.5 6.5 0 1 0 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z"/>
                </svg>
                <p>没有找到用户名包含 "<b>{{ searchText }}</b>" 的用户</p>
              </div>
            </td>
          </tr>
          </tbody>
        </table>
      </div>

      <!-- ==================== 翻页 ==================== -->
      <div v-if="totalPages > 1" class="pagination" role="navigation" aria-label="翻页">
        <!-- 上一页 -->
        <button
            class="page-btn page-arrow"
            @click="currentPage--"
            :disabled="currentPage === 1"
            aria-label="上一页"
        >
          <svg viewBox="0 0 24 24" width="16" height="16">
            <path fill="currentColor" d="M15.41 7.41L14 6l-6 6 6 6 1.41-1.41L10.83 12z"/>
          </svg>
        </button>

        <!-- 页码 -->
        <template v-for="p in pageRange" :key="p">
          <span v-if="p === '...'" class="page-ellipsis">…</span>
          <button
              v-else
              class="page-btn"
              :class="{ 'page-btn--active': p === currentPage }"
              @click="currentPage = p"
              :aria-label="`第 ${p} 页`"
              :aria-current="p === currentPage ? 'page' : undefined"
          >
            {{ p }}
          </button>
        </template>

        <!-- 下一页 -->
        <button
            class="page-btn page-arrow"
            @click="currentPage++"
            :disabled="currentPage === totalPages"
            aria-label="下一页"
        >
          <svg viewBox="0 0 24 24" width="16" height="16">
            <path fill="currentColor" d="M10 6L8.59 7.41 13.17 12l-4.58 4.59L10 18l6-6z"/>
          </svg>
        </button>

        <span class="page-info">{{ currentPage }} / {{ totalPages }} 页</span>
      </div>
    </template>

    <!-- ==================== 添加用户弹窗 ==================== -->
    <teleport to="body">
      <transition name="modal-fade">
        <div v-if="showAddDialog" class="modal-overlay" @click.self="closeAddDialog" role="dialog" aria-modal="true" aria-labelledby="add-dialog-title">
          <div class="modal-box">
            <div class="modal-header">
              <h2 id="add-dialog-title">添加用户</h2>
              <button class="modal-close" @click="closeAddDialog" aria-label="关闭">
                <svg viewBox="0 0 24 24" width="20" height="20">
                  <path fill="currentColor" d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/>
                </svg>
              </button>
            </div>
            <div class="modal-body">
              <div class="form-item">
                <label for="new-username">用户名</label>
                <input
                    id="new-username"
                    v-model="newUsername"
                    type="text"
                    placeholder="请输入用户名（2~20 个字符）"
                    class="form-input"
                    :class="{ 'input-error': addError }"
                    @keyup.enter="submitAdd"
                    maxlength="20"
                    ref="usernameInputRef"
                />
                <p v-if="addError" class="form-error" role="alert">{{ addError }}</p>
              </div>
              <div class="form-hint">
                <svg viewBox="0 0 24 24" width="14" height="14">
                  <path fill="currentColor" d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z"/>
                </svg>
                默认密码为 <b>123456</b>，用户登录后可自行修改
              </div>
            </div>
            <div class="modal-footer">
              <button class="btn-cancel" @click="closeAddDialog" :disabled="actionLoading">取消</button>
              <button class="btn-confirm" @click="submitAdd" :disabled="actionLoading || !newUsername.trim()">
                <span v-if="actionLoading" class="btn-spinner"></span>
                {{ actionLoading ? '创建中...' : '确认创建' }}
              </button>
            </div>
          </div>
        </div>
      </transition>
    </teleport>

    <!-- ==================== 重置密码确认弹窗 ==================== -->
    <teleport to="body">
      <transition name="modal-fade">
        <div v-if="showResetConfirm" class="modal-overlay" @click.self="closeResetConfirm" role="dialog" aria-modal="true" aria-labelledby="reset-dialog-title">
          <div class="modal-box modal-box--sm">
            <div class="modal-header">
              <h2 id="reset-dialog-title">重置密码</h2>
              <button class="modal-close" @click="closeResetConfirm" aria-label="关闭">
                <svg viewBox="0 0 24 24" width="20" height="20">
                  <path fill="currentColor" d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/>
                </svg>
              </button>
            </div>
            <div class="modal-body">
              <div class="reset-warning">
                <svg viewBox="0 0 24 24" width="40" height="40">
                  <path fill="#409eff" d="M12 5V1L7 6l5 5V7c3.31 0 6 2.69 6 6s-2.69 6-6 6-6-2.69-6-6H4c0 4.42 3.58 8 8 8s8-3.58 8-8-3.58-8-8-8z"/>
                </svg>
                <p>将 <b>{{ resetTarget?.username }}</b> 的密码重置为</p>
                <div class="reset-password-preview">123456</div>
                <p class="reset-sub">用户下次登录需使用新密码。</p>
              </div>
              <p v-if="resetError" class="form-error" role="alert">{{ resetError }}</p>
            </div>
            <div class="modal-footer">
              <button class="btn-cancel" @click="closeResetConfirm" :disabled="actionLoading">取消</button>
              <button class="btn-reset" @click="submitReset" :disabled="actionLoading">
                <span v-if="actionLoading" class="btn-spinner"></span>
                {{ actionLoading ? '重置中...' : '确认重置' }}
              </button>
            </div>
          </div>
        </div>
      </transition>
    </teleport>

    <!-- ==================== 删除确认弹窗 ==================== -->
    <teleport to="body">
      <transition name="modal-fade">
        <div v-if="showDeleteConfirm" class="modal-overlay" @click.self="closeDeleteConfirm" role="dialog" aria-modal="true" aria-labelledby="delete-dialog-title">
          <div class="modal-box modal-box--sm">
            <div class="modal-header">
              <h2 id="delete-dialog-title">确认删除</h2>
              <button class="modal-close" @click="closeDeleteConfirm" aria-label="关闭">
                <svg viewBox="0 0 24 24" width="20" height="20">
                  <path fill="currentColor" d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/>
                </svg>
              </button>
            </div>
            <div class="modal-body">
              <div class="delete-warning">
                <svg viewBox="0 0 24 24" width="40" height="40">
                  <path fill="#e6a23c" d="M1 21h22L12 2 1 21zm12-3h-2v-2h2v2zm0-4h-2v-4h2v4z"/>
                </svg>
                <p>确定要删除用户 <b>{{ deleteTarget?.username }}</b> 吗？</p>
                <p class="delete-sub">此操作不可恢复。</p>
              </div>
              <p v-if="deleteError" class="form-error" role="alert">{{ deleteError }}</p>
            </div>
            <div class="modal-footer">
              <button class="btn-cancel" @click="closeDeleteConfirm" :disabled="actionLoading">取消</button>
              <button class="btn-danger" @click="submitDelete" :disabled="actionLoading">
                <span v-if="actionLoading" class="btn-spinner"></span>
                {{ actionLoading ? '删除中...' : '确认删除' }}
              </button>
            </div>
          </div>
        </div>
      </transition>
    </teleport>

    <!-- ==================== 绑定 NFC 弹窗 ==================== -->
    <teleport to="body">
      <transition name="modal-fade">
        <div v-if="showBindDialog" class="modal-overlay" @click.self="closeBindDialog" role="dialog" aria-modal="true" aria-labelledby="bind-dialog-title">
          <div class="modal-box modal-box--sm">
            <div class="modal-header">
              <h2 id="bind-dialog-title">绑定 NFC 卡</h2>
              <button class="modal-close" @click="closeBindDialog" aria-label="关闭">
                <svg viewBox="0 0 24 24" width="20" height="20">
                  <path fill="currentColor" d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/>
                </svg>
              </button>
            </div>
            <div class="modal-body">
              <div class="bind-area">
                <!-- 扫描中 -->
                <template v-if="bindState === 'scanning'">
                  <div class="nfc-scan-icon">
                    <svg viewBox="0 0 24 24" width="48" height="48">
                      <path fill="#409eff" d="M20 4H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2zm0 14H4v-6h16v6zm0-10H4V6h16v2z"/>
                    </svg>
                    <span class="scan-ring"></span>
                  </div>
                  <p class="bind-title">请将 NFC 卡靠近读卡器</p>
                  <p class="bind-sub">正在为 <b>{{ bindTarget?.username }}</b> 绑定（{{ countdown }} 秒）</p>
                </template>

                <!-- 成功 -->
                <template v-else-if="bindState === 'success'">
                  <svg viewBox="0 0 24 24" width="48" height="48">
                    <path fill="#67c23a" d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z"/>
                  </svg>
                  <p class="bind-title">绑定成功</p>
                  <div class="bind-uid">{{ boundUid }}</div>
                  <p class="bind-sub">已绑定到 <b>{{ bindTarget?.username }}</b></p>
                </template>

                <!-- 失败 -->
                <template v-else>
                  <svg viewBox="0 0 24 24" width="48" height="48">
                    <path fill="#f56c6c" d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-2h2v2zm0-4h-2V7h2v6z"/>
                  </svg>
                  <p class="bind-title">绑定失败</p>
                  <p class="bind-sub">{{ bindError }}</p>
                </template>
              </div>
            </div>
            <div class="modal-footer">
              <button class="btn-cancel" @click="closeBindDialog">
                {{ bindState === 'scanning' ? '取消' : '关闭' }}
              </button>
              <button
                  v-if="bindState === 'error'"
                  class="btn-reset"
                  @click="startBind"
              >
                重试
              </button>
            </div>
          </div>
        </div>
      </transition>
    </teleport>

    <!-- ==================== Toast ==================== -->
    <teleport to="body">
      <transition name="toast-fade">
        <div v-if="toast.show" :class="['toast', `toast--${toast.type}`]" role="status">
          <svg v-if="toast.type === 'success'" viewBox="0 0 24 24" width="18" height="18">
            <path fill="currentColor" d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/>
          </svg>
          <svg v-else viewBox="0 0 24 24" width="18" height="18">
            <path fill="currentColor" d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-2h2v2zm0-4h-2V7h2v6z"/>
          </svg>
          {{ toast.message }}
        </div>
      </transition>
    </teleport>

  </div>
</template>

<script setup>
import { ref, computed, onMounted, nextTick, watch } from 'vue'

const PAGE_SIZE = 5

const users         = ref([])
const loading       = ref(true)
const error         = ref('')
const searchText    = ref('')
const actionLoading = ref(false)
const currentPage   = ref(1)

// 添加弹窗
const showAddDialog    = ref(false)
const newUsername      = ref('')
const addError         = ref('')
const usernameInputRef = ref(null)

// 重置弹窗
const showResetConfirm = ref(false)
const resetTarget      = ref(null)
const resetError       = ref('')

// 删除弹窗
const showDeleteConfirm = ref(false)
const deleteTarget      = ref(null)
const deleteError       = ref('')

// 绑卡弹窗
const showBindDialog = ref(false)
const bindTarget     = ref(null)
const bindState      = ref('scanning')   // scanning | success | error
const bindError      = ref('')
const boundUid       = ref('')
const binding        = ref(false)         // 全局是否正在绑卡（禁用其它绑卡按钮）
const countdown      = ref(30)
let bindController   = null
let countdownTimer   = null

// Toast
const toast = ref({ show: false, message: '', type: 'success' })
let toastTimer = null

// ==================== 翻页计算 ====================
const filteredUsers = computed(() => {
  const keyword = searchText.value.trim().toLowerCase()
  if (!keyword) return users.value
  return users.value.filter(u => u.username.toLowerCase().includes(keyword))
})

const totalPages = computed(() => Math.max(1, Math.ceil(filteredUsers.value.length / PAGE_SIZE)))

const pagedUsers = computed(() => {
  const start = (currentPage.value - 1) * PAGE_SIZE
  return filteredUsers.value.slice(start, start + PAGE_SIZE)
})

// 搜索词变化时跳回第一页
watch(searchText, () => { currentPage.value = 1 })

// 构建页码序列，超过 7 页时中间用省略号
const pageRange = computed(() => {
  const total = totalPages.value
  const cur   = currentPage.value
  if (total <= 7) return Array.from({ length: total }, (_, i) => i + 1)

  const pages = []
  // 始终显示第 1、最后一页，以及当前页附近各 1 页
  const around = new Set([1, total, cur - 1, cur, cur + 1].filter(p => p >= 1 && p <= total))
  const sorted = [...around].sort((a, b) => a - b)

  for (let i = 0; i < sorted.length; i++) {
    if (i > 0 && sorted[i] - sorted[i - 1] > 1) pages.push('...')
    pages.push(sorted[i])
  }
  return pages
})

const adminCount = computed(() => users.value.filter(u => u.is_admin).length)

// ==================== 接口 ====================
function authHeaders() {
  return { Authorization: `Bearer ${localStorage.getItem('token')}` }
}

async function fetchUsers() {
  loading.value = true
  error.value   = ''
  try {
    const res = await fetch('/api/auth/users', { headers: authHeaders() })
    if (!res.ok) throw new Error((await res.json()).message || '获取用户列表失败')
    users.value = (await res.json()).users
  } catch (err) {
    error.value = err.message
  } finally {
    loading.value = false
  }
}

// ==================== 添加 ====================
function openAddDialog() {
  newUsername.value = ''
  addError.value    = ''
  showAddDialog.value = true
  nextTick(() => usernameInputRef.value?.focus())
}

function closeAddDialog() { showAddDialog.value = false }

async function submitAdd() {
  addError.value = ''
  const name = newUsername.value.trim()
  if (!name)           { addError.value = '用户名不能为空'; return }
  if (name.length < 2) { addError.value = '用户名至少 2 个字符'; return }

  actionLoading.value = true
  try {
    const res = await fetch('/api/auth/users', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', ...authHeaders() },
      body: JSON.stringify({ username: name })
    })
    const data = await res.json()
    if (!res.ok) { addError.value = data.message || '创建失败'; return }
    users.value.push({ ...data.user, uid: null, created_at: new Date().toISOString() })
    // 跳到最后一页让新用户可见
    currentPage.value = totalPages.value
    closeAddDialog()
    showToast(`用户 ${name} 创建成功`, 'success')
  } catch {
    addError.value = '网络错误，请重试'
  } finally {
    actionLoading.value = false
  }
}

// ==================== 重置密码 ====================
function openResetConfirm(user) {
  resetTarget.value      = user
  resetError.value       = ''
  showResetConfirm.value = true
}

function closeResetConfirm() {
  showResetConfirm.value = false
  resetTarget.value = null
}

async function submitReset() {
  if (!resetTarget.value) return
  resetError.value    = ''
  actionLoading.value = true
  try {
    const res = await fetch(`/api/auth/users/${resetTarget.value.id}/reset-password`, {
      method: 'PATCH',
      headers: authHeaders()
    })
    const data = await res.json()
    if (!res.ok) { resetError.value = data.message || '重置失败'; return }
    const name = resetTarget.value.username
    closeResetConfirm()
    showToast(`${name} 的密码已重置为 123456`, 'success')
  } catch {
    resetError.value = '网络错误，请重试'
  } finally {
    actionLoading.value = false
  }
}

// ==================== 删除 ====================
function openDeleteConfirm(user) {
  deleteTarget.value      = user
  deleteError.value       = ''
  showDeleteConfirm.value = true
}

function closeDeleteConfirm() {
  showDeleteConfirm.value = false
  deleteTarget.value = null
}

async function submitDelete() {
  if (!deleteTarget.value) return
  deleteError.value   = ''
  actionLoading.value = true
  try {
    const res = await fetch(`/api/auth/users/${deleteTarget.value.id}`, {
      method: 'DELETE',
      headers: authHeaders()
    })
    const data = await res.json()
    if (!res.ok) { deleteError.value = data.message || '删除失败'; return }
    const name = deleteTarget.value.username
    users.value = users.value.filter(u => u.id !== deleteTarget.value.id)
    // 删完后如果当前页已空，退到上一页
    if (currentPage.value > totalPages.value) currentPage.value = totalPages.value
    closeDeleteConfirm()
    showToast(`用户 ${name} 已删除`, 'success')
  } catch {
    deleteError.value = '网络错误，请重试'
  } finally {
    actionLoading.value = false
  }
}

// ==================== 绑定 NFC ====================
function openBindDialog(user) {
  bindTarget.value     = user
  showBindDialog.value = true
  startBind()
}

async function startBind() {
  if (!bindTarget.value) return
  bindState.value = 'scanning'
  bindError.value = ''
  boundUid.value  = ''
  binding.value   = true

  // 倒计时显示
  countdown.value = 30
  clearInterval(countdownTimer)
  countdownTimer = setInterval(() => {
    if (countdown.value > 0) countdown.value--
  }, 1000)

  bindController = new AbortController()
  try {
    const res = await fetch(`/api/auth/users/${bindTarget.value.id}/bind-nfc`, {
      method: 'POST',
      headers: authHeaders(),
      signal: bindController.signal
    })
    const data = await res.json()

    if (!res.ok) {
      bindState.value = 'error'
      bindError.value = data.message || '绑定失败'
      return
    }

    // 成功：更新本地列表里该用户的 uid
    boundUid.value = data.uid
    const u = users.value.find(x => x.id === bindTarget.value.id)
    if (u) u.uid = data.uid
    bindState.value = 'success'
    showToast(`已为 ${bindTarget.value.username} 绑定卡片`, 'success')
  } catch (err) {
    // 主动取消（关闭弹窗）会触发 AbortError，忽略
    if (err.name === 'AbortError') return
    bindState.value = 'error'
    bindError.value = '网络错误，请重试'
  } finally {
    binding.value = false
    clearInterval(countdownTimer)
  }
}

function closeBindDialog() {
  // 扫描中关闭：中止请求并通知后端取消会话
  if (bindState.value === 'scanning' && bindTarget.value) {
    if (bindController) bindController.abort()
    fetch(`/api/auth/users/${bindTarget.value.id}/bind-nfc/cancel`, {
      method: 'POST',
      headers: authHeaders()
    }).catch(() => {})
  }
  clearInterval(countdownTimer)
  showBindDialog.value = false
  bindTarget.value = null
  binding.value = false
}

// ==================== 工具 ====================
function showToast(message, type = 'success') {
  clearTimeout(toastTimer)
  toast.value = { show: true, message, type }
  toastTimer  = setTimeout(() => { toast.value.show = false }, 3000)
}

function avatarColor(username) {
  const colors = ['#42b883','#409eff','#e6a23c','#f56c6c','#9c27b0','#00bcd4','#ff5722','#607d8b']
  let hash = 0
  for (let i = 0; i < username.length; i++) {
    hash = username.charCodeAt(i) + ((hash << 5) - hash)
  }
  return colors[Math.abs(hash) % colors.length]
}

function formatDate(dateStr) {
  if (!dateStr) return '—'
  return new Date(dateStr).toLocaleDateString('zh-CN', { year: 'numeric', month: '2-digit', day: '2-digit' })
}

onMounted(fetchUsers)
</script>

<style scoped>
.admin-page {
  flex: 1;
  width: 100%;
  display: flex;
  flex-direction: column;
  gap: 24px;
  padding: 24px;
  box-sizing: border-box;
  overflow-y: auto;
  background-color: var(--bg-page);
  transition: background-color 0.3s ease;
}

.page-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 16px;
}

.page-title {
  font-size: 1.6rem;
  font-weight: 700;
  color: var(--text-primary);
  margin: 0 0 4px;
  transition: color 0.3s ease;
}

.page-subtitle {
  color: var(--text-muted);
  font-size: 0.9rem;
  margin: 0;
  transition: color 0.3s ease;
}

.header-right { display: flex; align-items: center; gap: 16px; flex-wrap: wrap; }
.header-stats { display: flex; gap: 12px; }

.stat-card {
  background-color: var(--bg-card);
  border-radius: 8px;
  padding: 12px 24px;
  text-align: center;
  box-shadow: var(--shadow-sm);
  border: 1px solid var(--border-color);
  min-width: 80px;
  transition: background-color 0.3s ease, border-color 0.3s ease;
}

.stat-num {
  display: block;
  font-size: 1.6rem;
  font-weight: 700;
  color: var(--text-primary);
  line-height: 1.2;
  transition: color 0.3s ease;
}

.stat-label {
  font-size: 0.8rem;
  color: var(--text-muted);
  transition: color 0.3s ease;
}

.admin-stat .stat-num { color: #409eff; }

.add-btn {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 10px 20px;
  background-color: #42b883;
  color: white;
  border: none;
  border-radius: 6px;
  font-size: 0.95rem;
  font-weight: 500;
  cursor: pointer;
  transition: background-color 0.2s, transform 0.1s;
  white-space: nowrap;
}
.add-btn:hover { background-color: #33a06f; }
.add-btn:active { transform: scale(0.97); }
.add-btn:disabled { opacity: 0.6; cursor: not-allowed; }

.toolbar {
  display: flex;
  align-items: center;
  gap: 16px;
  background-color: var(--bg-card);
  padding: 16px 20px;
  border-radius: 8px;
  box-shadow: var(--shadow-sm);
  border: 1px solid var(--border-color);
  transition: background-color 0.3s ease, border-color 0.3s ease;
}

.search-wrapper { position: relative; flex: 1; display: flex; align-items: center; }

.search-icon { position: absolute; left: 12px; color: var(--text-light); pointer-events: none; }

.search-input {
  width: 100%;
  padding: 10px 36px 10px 40px;
  border: 1px solid var(--border-input);
  border-radius: 6px;
  font-size: 0.95rem;
  color: var(--text-regular);
  background-color: var(--bg-input);
  outline: none;
  transition: border-color 0.2s, box-shadow 0.2s, background-color 0.3s ease, color 0.3s ease;
}
.search-input:focus { border-color: #42b883; box-shadow: 0 0 0 3px rgba(66,184,131,0.1); }
.search-input::placeholder { color: var(--text-light); }

.search-clear {
  position: absolute; right: 10px;
  background: none; border: none; cursor: pointer;
  color: var(--text-light); padding: 0;
  display: flex; align-items: center;
  transition: color 0.2s;
}
.search-clear:hover { color: var(--text-secondary); }

.result-count { font-size: 0.875rem; color: var(--text-muted); white-space: nowrap; transition: color 0.3s ease; }
.result-count b { color: #42b883; }

.status-area {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 12px;
  padding: 80px 0;
  color: var(--text-muted);
  background-color: var(--bg-card);
  border-radius: 8px;
  box-shadow: var(--shadow-sm);
  border: 1px solid var(--border-color);
  transition: background-color 0.3s ease, border-color 0.3s ease;
}

.spinner {
  width: 36px; height: 36px;
  border: 3px solid var(--border-color);
  border-top-color: #42b883;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}
@keyframes spin { to { transform: rotate(360deg); } }

.retry-btn {
  padding: 8px 20px;
  background-color: #42b883; color: white;
  border: none; border-radius: 6px;
  cursor: pointer; font-size: 0.9rem;
  transition: background-color 0.2s;
}
.retry-btn:hover { background-color: #33a06f; }

.table-wrapper {
  background-color: var(--bg-card);
  border-radius: 8px;
  box-shadow: var(--shadow-sm);
  border: 1px solid var(--border-color);
  overflow: hidden;
  overflow-x: auto;
  transition: background-color 0.3s ease, border-color 0.3s ease;
}

.user-table { width: 100%; border-collapse: collapse; }
.user-table thead { background-color: var(--bg-hover); transition: background-color 0.3s ease; }

.user-table th {
  padding: 14px 20px;
  text-align: left;
  font-size: 0.875rem;
  font-weight: 600;
  color: var(--text-muted);
  border-bottom: 1px solid var(--border-color);
  white-space: nowrap;
  transition: color 0.3s ease, border-color 0.3s ease;
}

.user-row { transition: background-color 0.15s; }
.user-row:hover { background-color: var(--bg-hover); }
.user-row:not(:last-child) td { border-bottom: 1px solid var(--border-color); }

.user-table td {
  padding: 14px 20px;
  font-size: 0.95rem;
  color: var(--text-regular);
  vertical-align: middle;
  transition: color 0.3s ease, border-color 0.3s ease;
}

.td-id { color: var(--text-muted); font-size: 0.875rem; width: 60px; }
.user-cell { display: flex; align-items: center; gap: 10px; }

.avatar {
  width: 34px; height: 34px;
  border-radius: 50%;
  display: flex; align-items: center; justify-content: center;
  color: white; font-weight: 600; font-size: 0.9rem; flex-shrink: 0;
}

.role-badge {
  display: inline-flex; align-items: center;
  padding: 4px 10px; border-radius: 20px;
  font-size: 0.8rem; font-weight: 500; white-space: nowrap;
}
.badge-admin { background-color: #ecf5ff; color: #409eff; border: 1px solid #b3d8ff; }
.badge-user  { background-color: #f0f9eb; color: #67c23a; border: 1px solid #c2e7b0; }

.td-date { color: var(--text-muted); font-size: 0.875rem; }

/* NFC 列 */
.td-nfc { font-size: 0.85rem; }
.nfc-bound {
  display: inline-flex; align-items: center; gap: 4px;
  color: #67c23a; font-family: monospace;
  max-width: 120px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;
}
.nfc-unbound { color: var(--text-light); }

.td-action { width: 230px; white-space: nowrap; }
.td-action > * + * { margin-left: 8px; }

.bind-btn {
  display: inline-flex; align-items: center; gap: 5px;
  padding: 6px 12px;
  background-color: #909399; border: none; border-radius: 5px;
  color: #fff; font-size: 0.85rem; cursor: pointer;
  transition: background-color 0.2s, transform 0.1s;
  white-space: nowrap;
}
.bind-btn:hover { background-color: #767a82; }
.bind-btn:active { transform: scale(0.96); }
.bind-btn:disabled { opacity: 0.5; cursor: not-allowed; }

.reset-btn {
  display: inline-flex; align-items: center; gap: 5px;
  padding: 6px 12px;
  background-color: #409eff; border: none; border-radius: 5px;
  color: #fff; font-size: 0.85rem; cursor: pointer;
  transition: background-color 0.2s, transform 0.1s;
  white-space: nowrap;
}
.reset-btn:hover { background-color: #337ecc; }
.reset-btn:active { transform: scale(0.96); }
.reset-btn:disabled { opacity: 0.5; cursor: not-allowed; }

.delete-btn {
  display: inline-flex; align-items: center; gap: 5px;
  padding: 6px 12px;
  background-color: #f56c6c; border: none; border-radius: 5px;
  color: #fff; font-size: 0.85rem; cursor: pointer;
  transition: background-color 0.2s, transform 0.1s;
  white-space: nowrap;
}
.delete-btn:hover { background-color: #e64040; }
.delete-btn:active { transform: scale(0.96); }
.delete-btn:disabled { opacity: 0.5; cursor: not-allowed; }

.admin-lock { display: inline-flex; align-items: center; padding: 6px; }
.admin-lock svg path { fill: var(--text-light); }

.empty-cell { padding: 48px 0 !important; }
.empty-state { display: flex; flex-direction: column; align-items: center; gap: 8px; color: var(--text-light); }
.empty-state p { margin: 0; font-size: 0.9rem; }
.empty-state b { color: var(--text-secondary); }

/* ==================== 翻页 ==================== */
.pagination {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 4px 0 8px;
  flex-wrap: wrap;
}

.page-btn {
  min-width: 36px;
  height: 36px;
  padding: 0 10px;
  border-radius: 6px;
  border: 1px solid var(--border-color);
  background-color: var(--bg-card);
  color: var(--text-secondary);
  font-size: 0.9rem;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  transition: all 0.2s ease;
  user-select: none;
}
.page-btn:hover:not(:disabled):not(.page-btn--active) {
  border-color: #42b883;
  color: #42b883;
  background-color: var(--bg-hover);
}
.page-btn--active {
  background-color: #42b883;
  border-color: #42b883;
  color: #fff;
  font-weight: 600;
  cursor: default;
}
.page-btn:disabled {
  opacity: 0.35;
  cursor: not-allowed;
}

.page-arrow { color: var(--text-muted); }

.page-ellipsis {
  min-width: 36px;
  height: 36px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  color: var(--text-light);
  font-size: 0.9rem;
  user-select: none;
}

.page-info {
  font-size: 0.85rem;
  color: var(--text-muted);
  margin-left: 8px;
  white-space: nowrap;
}

/* ==================== 弹窗 ==================== */
.modal-overlay {
  position: fixed; inset: 0;
  background-color: rgba(0,0,0,0.5);
  display: flex; align-items: center; justify-content: center;
  z-index: 2000; padding: 16px;
}

.modal-box {
  background-color: var(--bg-card);
  border: 1px solid var(--border-color);
  border-radius: 10px;
  width: 100%; max-width: 420px;
  box-shadow: var(--shadow-lg);
  overflow: hidden;
  transition: background-color 0.3s ease, border-color 0.3s ease;
}
.modal-box--sm { max-width: 360px; }

.modal-header { display: flex; align-items: center; justify-content: space-between; padding: 18px 24px 0; }
.modal-header h2 { margin: 0; font-size: 1.1rem; font-weight: 600; color: var(--text-primary); transition: color 0.3s ease; }

.modal-close {
  background: none; border: none; cursor: pointer;
  color: var(--text-light); padding: 4px;
  display: flex; align-items: center; border-radius: 4px;
  transition: color 0.2s, background 0.2s;
}
.modal-close:hover { color: var(--text-secondary); background: var(--bg-hover); }

.modal-body  { padding: 20px 24px; }
.modal-footer { display: flex; justify-content: flex-end; gap: 10px; padding: 0 24px 20px; }

.reset-warning {
  display: flex; flex-direction: column; align-items: center;
  gap: 8px; text-align: center; padding: 8px 0 4px;
}
.reset-warning p { margin: 0; color: var(--text-regular); font-size: 0.95rem; }
.reset-warning b { color: var(--text-primary); }
.reset-sub { color: var(--text-muted) !important; font-size: 0.85rem !important; }

.reset-password-preview {
  font-size: 1.4rem; font-weight: 700; letter-spacing: 4px;
  color: #409eff;
  background-color: var(--bg-hover);
  padding: 8px 24px; border-radius: 8px;
  border: 1px dashed var(--border-color);
  font-family: monospace;
}

.delete-warning {
  display: flex; flex-direction: column; align-items: center;
  gap: 8px; text-align: center; padding: 8px 0 4px;
}
.delete-warning p { margin: 0; color: var(--text-regular); font-size: 0.95rem; }
.delete-warning b { color: var(--text-primary); }
.delete-sub { color: var(--text-muted) !important; font-size: 0.85rem !important; }

/* 绑卡弹窗内容 */
.bind-area {
  display: flex; flex-direction: column; align-items: center;
  gap: 10px; text-align: center; padding: 12px 0;
}
.bind-title { margin: 0; font-size: 1rem; font-weight: 600; color: var(--text-primary); }
.bind-sub   { margin: 0; font-size: 0.85rem; color: var(--text-muted); }
.bind-sub b { color: var(--text-primary); }

.bind-uid {
  font-size: 1.2rem; font-weight: 700; letter-spacing: 2px;
  color: #67c23a; font-family: monospace;
  background-color: var(--bg-hover);
  padding: 6px 20px; border-radius: 8px;
  border: 1px dashed var(--border-color);
}

.nfc-scan-icon { position: relative; display: inline-flex; }
.scan-ring {
  position: absolute; inset: -8px;
  border: 2px solid #409eff; border-radius: 50%;
  opacity: 0; animation: scan-pulse 1.5s ease-out infinite;
}
@keyframes scan-pulse {
  0%   { transform: scale(0.8); opacity: 0.6; }
  100% { transform: scale(1.4); opacity: 0; }
}

.form-item { display: flex; flex-direction: column; gap: 6px; margin-bottom: 12px; }
.form-item label { font-size: 0.875rem; font-weight: 500; color: var(--text-secondary); transition: color 0.3s ease; }

.form-input {
  padding: 10px 14px;
  border: 1px solid var(--border-input); border-radius: 6px;
  font-size: 0.95rem; color: var(--text-regular); background-color: var(--bg-input);
  outline: none;
  transition: border-color 0.2s, box-shadow 0.2s, background-color 0.3s ease, color 0.3s ease;
}
.form-input:focus { border-color: #42b883; box-shadow: 0 0 0 3px rgba(66,184,131,0.1); }
.form-input.input-error { border-color: #f56c6c; }
.form-input.input-error:focus { box-shadow: 0 0 0 3px rgba(245,108,108,0.1); }

.form-error { margin: 0; font-size: 0.8rem; color: #f56c6c; }

.form-hint { display: flex; align-items: center; gap: 5px; font-size: 0.82rem; color: var(--text-muted); }
.form-hint b { color: var(--text-secondary); }

.btn-cancel {
  padding: 9px 20px; background: none;
  border: 1px solid var(--border-input); border-radius: 6px;
  color: var(--text-secondary); font-size: 0.9rem; cursor: pointer;
  transition: all 0.2s;
}
.btn-cancel:hover { background: var(--bg-hover); }
.btn-cancel:disabled { opacity: 0.5; cursor: not-allowed; }

.btn-confirm {
  display: inline-flex; align-items: center; gap: 6px;
  padding: 9px 20px; background-color: #42b883; color: white;
  border: none; border-radius: 6px; font-size: 0.9rem; cursor: pointer;
  transition: background-color 0.2s;
}
.btn-confirm:hover { background-color: #33a06f; }
.btn-confirm:disabled { opacity: 0.6; cursor: not-allowed; }

.btn-reset {
  display: inline-flex; align-items: center; gap: 6px;
  padding: 9px 20px; background-color: #409eff; color: white;
  border: none; border-radius: 6px; font-size: 0.9rem; cursor: pointer;
  transition: background-color 0.2s;
}
.btn-reset:hover { background-color: #337ecc; }
.btn-reset:disabled { opacity: 0.6; cursor: not-allowed; }

.btn-danger {
  display: inline-flex; align-items: center; gap: 6px;
  padding: 9px 20px; background-color: #f56c6c; color: white;
  border: none; border-radius: 6px; font-size: 0.9rem; cursor: pointer;
  transition: background-color 0.2s;
}
.btn-danger:hover { background-color: #e64040; }
.btn-danger:disabled { opacity: 0.6; cursor: not-allowed; }

.btn-spinner {
  width: 14px; height: 14px;
  border: 2px solid rgba(255,255,255,0.4);
  border-top-color: white; border-radius: 50%;
  animation: spin 0.7s linear infinite; flex-shrink: 0;
}

.toast {
  position: fixed; bottom: 32px; left: 50%; transform: translateX(-50%);
  display: inline-flex; align-items: center; gap: 8px;
  padding: 12px 20px; border-radius: 8px;
  font-size: 0.9rem; font-weight: 500;
  box-shadow: var(--shadow-md); z-index: 3000; white-space: nowrap;
}
.toast--success { background-color: #f0f9eb; color: #67c23a; border: 1px solid #c2e7b0; }
.toast--error   { background-color: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; }

.modal-fade-enter-active, .modal-fade-leave-active { transition: all 0.2s ease; }
.modal-fade-enter-from, .modal-fade-leave-to { opacity: 0; }
.modal-fade-enter-from .modal-box, .modal-fade-leave-to .modal-box { transform: scale(0.95); }

.toast-fade-enter-active, .toast-fade-leave-active { transition: all 0.3s ease; }
.toast-fade-enter-from, .toast-fade-leave-to { opacity: 0; transform: translate(-50%, 12px); }

@media (max-width: 600px) {
  .page-header  { flex-direction: column; }
  .header-right { width: 100%; justify-content: space-between; }
  .toolbar      { flex-direction: column; align-items: flex-start; }
  .search-wrapper { width: 100%; }
  .td-action { width: auto; }
}
</style>
