<template>
  <div class="user-page">

    <!-- ==================== 用户信息卡 ==================== -->
    <div class="profile-card">
      <!-- 头像区域：点击触发上传 -->
      <div class="avatar-wrap">
        <div
            class="avatar-circle"
            :style="!user.avatar ? { backgroundColor: avatarColor(user.username) } : {}"
            @click="triggerUpload"
            role="button"
            tabindex="0"
            aria-label="点击更换头像"
            @keyup.enter="triggerUpload"
        >
          <img
              v-if="user.avatar"
              :src="user.avatar"
              :alt="`${user.username} 的头像`"
              class="avatar-img"
          />
          <span v-else class="avatar-letter">{{ user.username.charAt(0).toUpperCase() }}</span>

          <!-- hover 遮罩 -->
          <div class="avatar-overlay" :class="{ 'is-uploading': uploading }">
            <div v-if="uploading" class="upload-spinner"></div>
            <template v-else>
              <svg viewBox="0 0 24 24" width="22" height="22">
                <path fill="currentColor" d="M9 16h6v-6h4l-7-7-7 7h4zm-4 2h14v2H5z"/>
              </svg>
              <span>更换头像</span>
            </template>
          </div>
        </div>

        <!-- 隐藏的文件输入 -->
        <input
            ref="fileInputRef"
            type="file"
            accept="image/jpeg,image/png,image/webp,image/gif"
            style="display:none"
            @change="handleFileChange"
            aria-hidden="true"
        />
      </div>

      <div class="profile-info">
        <div class="username-row">
          <h2 class="username">{{ user.username }}</h2>
          <span :class="['role-badge', user.is_admin ? 'badge-admin' : 'badge-user']">
            {{ user.is_admin ? '管理员' : '普通用户' }}
          </span>
        </div>
        <p class="user-id">UID：{{ user.id }}</p>
        <p class="avatar-hint">点击头像可更换（JPG / PNG / WebP / GIF，最大 2MB）</p>
      </div>
    </div>

    <!-- ==================== 修改密码卡 ==================== -->
    <div class="form-card">
      <div class="card-header">
        <svg viewBox="0 0 24 24" width="20" height="20" class="card-icon">
          <path fill="currentColor" d="M18 8h-1V6c0-2.76-2.24-5-5-5S7 3.24 7 6v2H6c-1.1 0-2 .9-2 2v10c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V10c0-1.1-.9-2-2-2zm-6 9c-1.1 0-2-.9-2-2s.9-2 2-2 2 .9 2 2-.9 2-2 2zm3.1-9H8.9V6c0-1.71 1.39-3.1 3.1-3.1 1.71 0 3.1 1.39 3.1 3.1v2z"/>
        </svg>
        <span>修改密码</span>
      </div>

      <form class="pw-form" @submit.prevent="submitChange" novalidate>

        <div class="form-item">
          <label for="old-pw">旧密码</label>
          <div class="input-wrap">
            <input
                id="old-pw"
                v-model="form.oldPassword"
                :type="show.old ? 'text' : 'password'"
                placeholder="请输入当前密码"
                class="form-input"
                :class="{ 'input-error': errors.oldPassword }"
                autocomplete="current-password"
            />
            <button type="button" class="eye-btn" @click="show.old = !show.old" :aria-label="show.old ? '隐藏密码' : '显示密码'">
              <svg v-if="show.old" viewBox="0 0 24 24" width="18" height="18"><path fill="currentColor" d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/></svg>
              <svg v-else viewBox="0 0 24 24" width="18" height="18"><path fill="currentColor" d="M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46A11.804 11.804 0 0 0 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z"/></svg>
            </button>
          </div>
          <p v-if="errors.oldPassword" class="field-error" role="alert">{{ errors.oldPassword }}</p>
        </div>

        <div class="form-item">
          <label for="new-pw">新密码</label>
          <div class="input-wrap">
            <input
                id="new-pw"
                v-model="form.newPassword"
                :type="show.new ? 'text' : 'password'"
                placeholder="至少 6 位"
                class="form-input"
                :class="{ 'input-error': errors.newPassword }"
                autocomplete="new-password"
            />
            <button type="button" class="eye-btn" @click="show.new = !show.new" :aria-label="show.new ? '隐藏密码' : '显示密码'">
              <svg v-if="show.new" viewBox="0 0 24 24" width="18" height="18"><path fill="currentColor" d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/></svg>
              <svg v-else viewBox="0 0 24 24" width="18" height="18"><path fill="currentColor" d="M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46A11.804 11.804 0 0 0 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z"/></svg>
            </button>
          </div>
          <div v-if="form.newPassword" class="strength-bar">
            <div v-for="i in 4" :key="i" class="strength-seg" :class="strengthClass(i)"></div>
            <span class="strength-label" :class="`label-${strength.level}`">{{ strength.text }}</span>
          </div>
          <p v-if="errors.newPassword" class="field-error" role="alert">{{ errors.newPassword }}</p>
        </div>

        <div class="form-item">
          <label for="confirm-pw">确认新密码</label>
          <div class="input-wrap">
            <input
                id="confirm-pw"
                v-model="form.confirmPassword"
                :type="show.confirm ? 'text' : 'password'"
                placeholder="再次输入新密码"
                class="form-input"
                :class="{ 'input-error': errors.confirmPassword }"
                autocomplete="new-password"
            />
            <button type="button" class="eye-btn" @click="show.confirm = !show.confirm" :aria-label="show.confirm ? '隐藏密码' : '显示密码'">
              <svg v-if="show.confirm" viewBox="0 0 24 24" width="18" height="18"><path fill="currentColor" d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/></svg>
              <svg v-else viewBox="0 0 24 24" width="18" height="18"><path fill="currentColor" d="M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46A11.804 11.804 0 0 0 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z"/></svg>
            </button>
          </div>
          <p v-if="errors.confirmPassword" class="field-error" role="alert">{{ errors.confirmPassword }}</p>
        </div>

        <div class="form-footer">
          <p v-if="submitError" class="submit-error" role="alert">
            <svg viewBox="0 0 24 24" width="16" height="16"><path fill="currentColor" d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-2h2v2zm0-4h-2V7h2v6z"/></svg>
            {{ submitError }}
          </p>
          <button type="submit" class="submit-btn" :disabled="submitting">
            <span v-if="submitting" class="btn-spinner"></span>
            {{ submitting ? '提交中...' : '修改密码' }}
          </button>
        </div>

      </form>
    </div>

    <!-- ==================== Toast ==================== -->
    <teleport to="body">
      <transition name="toast-fade">
        <div v-if="toast.show" :class="['toast', `toast--${toast.type}`]" role="status">
          <svg v-if="toast.type === 'success'" viewBox="0 0 24 24" width="18" height="18"><path fill="currentColor" d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>
          <svg v-else viewBox="0 0 24 24" width="18" height="18"><path fill="currentColor" d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-2h2v2zm0-4h-2V7h2v6z"/></svg>
          {{ toast.message }}
        </div>
      </transition>
    </teleport>

  </div>
</template>

<script setup>
import { ref, reactive, computed } from 'vue'

// ==================== 当前用户（响应式，方便头像更新后立刻刷新） ====================
const userRaw = ref(JSON.parse(localStorage.getItem('user') || '{}'))
const user = computed(() => userRaw.value)

// ==================== 头像上传 ====================
const fileInputRef = ref(null)
const uploading    = ref(false)

function triggerUpload() {
  if (uploading.value) return
  fileInputRef.value?.click()
}

async function handleFileChange(e) {
  const file = e.target.files?.[0]
  if (!file) return

  // 前端先做一次大小校验
  if (file.size > 2 * 1024 * 1024) {
    showToast('图片不能超过 2MB', 'error')
    e.target.value = ''
    return
  }

  const fd = new FormData()
  fd.append('avatar', file)

  uploading.value = true
  try {
    const res = await fetch('/api/auth/avatar', {
      method: 'PATCH',
      headers: { Authorization: `Bearer ${localStorage.getItem('token')}` },
      body: fd
    })

    const data = await res.json()
    if (!res.ok) {
      showToast(data.message || '上传失败', 'error')
      return
    }

    // 更新内存和 localStorage
    const updated = { ...userRaw.value, avatar: data.avatar }
    userRaw.value = updated
    localStorage.setItem('user', JSON.stringify(updated))

    showToast('头像更新成功', 'success')
  } catch {
    showToast('网络错误，请重试', 'error')
  } finally {
    uploading.value = false
    e.target.value = '' // 允许重复选同一张图
  }
}

// ==================== 修改密码 ====================
const form = reactive({ oldPassword: '', newPassword: '', confirmPassword: '' })
const show = reactive({ old: false, new: false, confirm: false })
const errors = reactive({ oldPassword: '', newPassword: '', confirmPassword: '' })
const submitError = ref('')
const submitting  = ref(false)

const strength = computed(() => {
  const pw = form.newPassword
  if (!pw) return { score: 0, level: 0, text: '' }
  let score = 0
  if (pw.length >= 8)            score++
  if (/[A-Z]/.test(pw))         score++
  if (/[0-9]/.test(pw))         score++
  if (/[^A-Za-z0-9]/.test(pw))  score++
  const labels = ['', '弱', '一般', '强', '很强']
  return { score, level: score, text: labels[score] || '弱' }
})

function strengthClass(seg) {
  const s = strength.value.score
  if (!s || seg > s) return ''
  if (s === 1) return 'seg-weak'
  if (s === 2) return 'seg-fair'
  if (s === 3) return 'seg-strong'
  return 'seg-great'
}

function validate() {
  errors.oldPassword = errors.newPassword = errors.confirmPassword = ''
  let ok = true
  if (!form.oldPassword)                         { errors.oldPassword = '请输入旧密码'; ok = false }
  if (!form.newPassword)                         { errors.newPassword = '请输入新密码'; ok = false }
  else if (form.newPassword.length < 6)          { errors.newPassword = '新密码至少 6 位'; ok = false }
  else if (form.newPassword === form.oldPassword) { errors.newPassword = '新密码不能与旧密码相同'; ok = false }
  if (!form.confirmPassword)                     { errors.confirmPassword = '请确认新密码'; ok = false }
  else if (form.confirmPassword !== form.newPassword) { errors.confirmPassword = '两次输入的密码不一致'; ok = false }
  return ok
}

async function submitChange() {
  submitError.value = ''
  if (!validate()) return

  submitting.value = true
  try {
    const res = await fetch('/api/auth/change-password', {
      method: 'PATCH',
      headers: {
        'Content-Type': 'application/json',
        Authorization: `Bearer ${localStorage.getItem('token')}`
      },
      body: JSON.stringify({ oldPassword: form.oldPassword, newPassword: form.newPassword })
    })

    const data = await res.json()
    if (!res.ok) {
      if (res.status === 401) errors.oldPassword = data.message || '旧密码错误'
      else submitError.value = data.message || '修改失败'
      return
    }

    form.oldPassword = form.newPassword = form.confirmPassword = ''
    show.old = show.new = show.confirm = false
    showToast('密码修改成功', 'success')
  } catch {
    submitError.value = '网络错误，请重试'
  } finally {
    submitting.value = false
  }
}

// ==================== 工具 ====================
const toast = ref({ show: false, message: '', type: 'success' })
let toastTimer = null

function showToast(message, type = 'success') {
  clearTimeout(toastTimer)
  toast.value = { show: true, message, type }
  toastTimer  = setTimeout(() => { toast.value.show = false }, 3000)
}

function avatarColor(username) {
  const colors = ['#42b883','#409eff','#e6a23c','#f56c6c','#9c27b0','#00bcd4','#ff5722','#607d8b']
  let hash = 0
  for (let i = 0; i < username.length; i++) hash = username.charCodeAt(i) + ((hash << 5) - hash)
  return colors[Math.abs(hash) % colors.length]
}
</script>

<style scoped>
.user-page {
  flex: 1;
  width: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24px;
  padding: 40px 24px;
  box-sizing: border-box;
  overflow-y: auto;
  background-color: var(--bg-page);
  transition: background-color 0.3s ease;
}

/* ==================== 用户信息卡 ==================== */
.profile-card {
  width: 100%;
  max-width: 480px;
  background-color: var(--bg-card);
  border: 1px solid var(--border-color);
  border-radius: 12px;
  box-shadow: var(--shadow-sm);
  padding: 32px 28px;
  display: flex;
  align-items: center;
  gap: 24px;
  transition: background-color 0.3s ease, border-color 0.3s ease;
}

/* ==================== 头像 ==================== */
.avatar-wrap { flex-shrink: 0; }

.avatar-circle {
  position: relative;
  width: 80px;
  height: 80px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  overflow: hidden;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  transition: box-shadow 0.2s;
  outline: none;
}
.avatar-circle:hover { box-shadow: 0 6px 18px rgba(0, 0, 0, 0.22); }
.avatar-circle:focus-visible { box-shadow: 0 0 0 3px rgba(66, 184, 131, 0.5); }

.avatar-img {
  width: 100%;
  height: 100%;
  object-fit: cover;
  border-radius: 50%;
  display: block;
}

.avatar-letter {
  color: #fff;
  font-size: 2rem;
  font-weight: 700;
  line-height: 1;
  user-select: none;
}

/* hover 遮罩 */
.avatar-overlay {
  position: absolute;
  inset: 0;
  border-radius: 50%;
  background-color: rgba(0, 0, 0, 0.45);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 4px;
  opacity: 0;
  transition: opacity 0.2s ease;
  color: #fff;
  font-size: 0.7rem;
  font-weight: 500;
}
.avatar-overlay.is-uploading { opacity: 1; }
.avatar-circle:hover .avatar-overlay { opacity: 1; }

.upload-spinner {
  width: 24px;
  height: 24px;
  border: 3px solid rgba(255, 255, 255, 0.35);
  border-top-color: #fff;
  border-radius: 50%;
  animation: spin 0.7s linear infinite;
}

/* ==================== 用户信息文字 ==================== */
.profile-info { flex: 1; min-width: 0; }

.username-row {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}

.username {
  margin: 0;
  font-size: 1.4rem;
  font-weight: 700;
  color: var(--text-primary);
  word-break: break-all;
  transition: color 0.3s ease;
}

.role-badge {
  display: inline-flex;
  align-items: center;
  padding: 3px 10px;
  border-radius: 20px;
  font-size: 0.78rem;
  font-weight: 500;
  white-space: nowrap;
  flex-shrink: 0;
}
.badge-admin { background-color: #ecf5ff; color: #409eff; border: 1px solid #b3d8ff; }
.badge-user  { background-color: #f0f9eb; color: #67c23a; border: 1px solid #c2e7b0; }

.user-id {
  margin: 6px 0 2px;
  font-size: 0.85rem;
  color: var(--text-muted);
  transition: color 0.3s ease;
}

.avatar-hint {
  margin: 0;
  font-size: 0.78rem;
  color: var(--text-light);
  transition: color 0.3s ease;
}

/* ==================== 表单卡 ==================== */
.form-card {
  width: 100%;
  max-width: 480px;
  background-color: var(--bg-card);
  border: 1px solid var(--border-color);
  border-radius: 12px;
  box-shadow: var(--shadow-sm);
  overflow: hidden;
  transition: background-color 0.3s ease, border-color 0.3s ease;
}

.card-header {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 18px 24px;
  border-bottom: 1px solid var(--border-color);
  font-size: 0.95rem;
  font-weight: 600;
  color: var(--text-primary);
  transition: border-color 0.3s ease, color 0.3s ease;
}
.card-icon { color: #42b883; flex-shrink: 0; }

.pw-form { padding: 24px; display: flex; flex-direction: column; gap: 20px; }

.form-item { display: flex; flex-direction: column; gap: 6px; }

.form-item label {
  font-size: 0.875rem;
  font-weight: 500;
  color: var(--text-secondary);
  transition: color 0.3s ease;
}

.input-wrap { position: relative; display: flex; align-items: center; }

.form-input {
  width: 100%;
  padding: 10px 44px 10px 14px;
  border: 1px solid var(--border-input);
  border-radius: 6px;
  font-size: 0.95rem;
  color: var(--text-regular);
  background-color: var(--bg-input);
  outline: none;
  box-sizing: border-box;
  transition: border-color 0.2s, box-shadow 0.2s, background-color 0.3s ease, color 0.3s ease;
}
.form-input:focus { border-color: #42b883; box-shadow: 0 0 0 3px rgba(66,184,131,0.1); }
.form-input.input-error { border-color: #f56c6c; }
.form-input.input-error:focus { box-shadow: 0 0 0 3px rgba(245,108,108,0.1); }
.form-input::placeholder { color: var(--text-light); }

.eye-btn {
  position: absolute; right: 12px;
  background: none; border: none; cursor: pointer; padding: 0;
  color: var(--text-muted);
  display: flex; align-items: center;
  transition: color 0.2s;
  flex-shrink: 0;
}
.eye-btn:hover { color: var(--text-secondary); }

.field-error { margin: 0; font-size: 0.8rem; color: #f56c6c; }

/* ==================== 强度条 ==================== */
.strength-bar {
  display: flex;
  align-items: center;
  gap: 4px;
  margin-top: 2px;
}
.strength-seg {
  flex: 1; height: 4px; border-radius: 2px;
  background-color: var(--border-color);
  transition: background-color 0.3s ease;
}
.seg-weak   { background-color: #f56c6c; }
.seg-fair   { background-color: #e6a23c; }
.seg-strong { background-color: #42b883; }
.seg-great  { background-color: #409eff; }

.strength-label {
  font-size: 0.78rem; margin-left: 4px;
  white-space: nowrap; font-weight: 500;
}
.label-1 { color: #f56c6c; }
.label-2 { color: #e6a23c; }
.label-3 { color: #42b883; }
.label-4 { color: #409eff; }

/* ==================== 提交 ==================== */
.form-footer { display: flex; flex-direction: column; gap: 12px; padding-top: 4px; }

.submit-error {
  display: flex; align-items: center; gap: 6px;
  margin: 0; font-size: 0.875rem; color: #f56c6c;
  background-color: #fef0f0; border: 1px solid #fbc4c4;
  padding: 10px 14px; border-radius: 6px;
}

.submit-btn {
  display: inline-flex; align-items: center; justify-content: center; gap: 8px;
  width: 100%; padding: 12px;
  background-color: #42b883; color: white;
  border: none; border-radius: 6px;
  font-size: 1rem; font-weight: 600; cursor: pointer;
  transition: background-color 0.2s, transform 0.1s;
}
.submit-btn:hover { background-color: #33a06f; }
.submit-btn:active { transform: scale(0.98); }
.submit-btn:disabled { opacity: 0.6; cursor: not-allowed; transform: none; }

.btn-spinner {
  width: 16px; height: 16px;
  border: 2px solid rgba(255,255,255,0.4);
  border-top-color: white; border-radius: 50%;
  animation: spin 0.7s linear infinite; flex-shrink: 0;
}
@keyframes spin { to { transform: rotate(360deg); } }

/* ==================== Toast ==================== */
.toast {
  position: fixed; bottom: 32px; left: 50%;
  transform: translateX(-50%);
  display: inline-flex; align-items: center; gap: 8px;
  padding: 12px 20px; border-radius: 8px;
  font-size: 0.9rem; font-weight: 500;
  box-shadow: var(--shadow-md); z-index: 3000; white-space: nowrap;
}
.toast--success { background-color: #f0f9eb; color: #67c23a; border: 1px solid #c2e7b0; }
.toast--error   { background-color: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; }

.toast-fade-enter-active, .toast-fade-leave-active { transition: all 0.3s ease; }
.toast-fade-enter-from, .toast-fade-leave-to { opacity: 0; transform: translate(-50%, 12px); }

/* ==================== 响应式 ==================== */
@media (max-width: 480px) {
  .user-page { padding: 24px 16px; }
  .profile-card { flex-direction: column; text-align: center; padding: 24px 20px; }
  .username-row { justify-content: center; }
  .pw-form { padding: 20px 16px; }
}
</style>
