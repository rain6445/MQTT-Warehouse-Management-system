<template>
  <div class="app-container">
    <header v-if="showNavbar" class="top-navbar">
      <div class="nav-left">
        <svg viewBox="0 0 128 128" width="24" height="24">
          <path fill="#42b883" d="M78.8,10L64,35.4L49.2,10H0l64,110l64-110H78.8z"/>
          <path fill="#35495e" d="M78.8,10L64,35.4L49.2,10H25.6L64,76.5l38.4-66.5H78.8z"/>
        </svg>
        <span class="site-title">MYWEB</span>
      </div>

      <nav class="nav-right pc-menu">
        <router-link to="/home" class="nav-link" active-class="active">首页</router-link>
        <router-link to="/device" class="nav-link" active-class="active">设备</router-link>
        <router-link to="/about" class="nav-link" active-class="active">关于我们</router-link>

        <div
            class="dropdown-wrapper"
            @mouseenter="showUserDropdown = true"
            @mouseleave="showUserDropdown = false"
        >
          <a href="#" class="user-trigger">
            <svg viewBox="0 0 24 24" width="16" height="16" class="trigger-icon">
              <path fill="currentColor" d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z"/>
            </svg>
            {{ currentUser?.username || '未登录' }}
            <svg viewBox="0 0 24 24" width="12" height="12" class="caret" :class="{ 'caret-up': showUserDropdown }">
              <path fill="currentColor" d="M7 10l5 5 5-5z"/>
            </svg>
          </a>

          <transition name="dropdown-fade">
            <div v-if="showUserDropdown" class="dropdown-menu">
              <router-link
                  v-if="currentUser?.is_admin"
                  to="/admin"
                  class="dropdown-item admin-item"
                  @click="showUserDropdown = false"
              >
                <svg class="icon" viewBox="0 0 24 24" width="16" height="16">
                  <path fill="currentColor" d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/>
                </svg>
                网站管理
              </router-link>

              <router-link to="/user" class="dropdown-item" @click="showUserDropdown = false">
                <svg class="icon" viewBox="0 0 24 24" width="16" height="16">
                  <path fill="currentColor" d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z"/>
                </svg>
                用户中心
              </router-link>

              <router-link to="/setting" class="dropdown-item" @click="showUserDropdown = false">
                <svg class="icon" viewBox="0 0 24 24" width="16" height="16">
                  <path fill="currentColor" d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z"/>
                </svg>
                设置
              </router-link>

              <button
                  v-if="isLoggedIn"
                  class="dropdown-item logout-item"
                  @click="handleLogout"
              >
                <svg class="icon" viewBox="0 0 24 24" width="16" height="16">
                  <path fill="currentColor" d="M17 7l-1.41 1.41L18.17 11H8v2h10.17l-2.58 2.58L17 17l5-5zM4 5h8V3H4c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h8v-2H4V5z"/>
                </svg>
                退出登录
              </button>

              <router-link
                  v-else
                  to="/login"
                  class="dropdown-item admin-item"
                  @click="showUserDropdown = false"
              >
                <svg class="icon" viewBox="0 0 24 24" width="16" height="16">
                  <path fill="currentColor" d="M11 7L9.6 8.4l2.6 2.6H2v2h10.2l-2.6 2.6L11 17l5-5zM20 19h-8v2h8c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2h-8v2h8v14z"/>
                </svg>
                登录
              </router-link>
            </div>
          </transition>
        </div>
      </nav>

      <button
          class="mobile-menu-btn"
          @click="showMobileMenu = !showMobileMenu"
          :class="{ 'is-active': showMobileMenu }"
          aria-label="菜单"
      >
        <span class="bar"></span>
        <span class="bar"></span>
        <span class="bar"></span>
      </button>

      <transition name="mobile-menu-fade">
        <div v-if="showMobileMenu" class="mobile-menu">
          <router-link to="/home" class="mobile-nav-link" @click="showMobileMenu = false">首页</router-link>
          <router-link to="/device" class="mobile-nav-link" @click="showMobileMenu = false">设备</router-link>
          <router-link to="/about" class="mobile-nav-link" @click="showMobileMenu = false">关于我们</router-link>

          <div class="mobile-divider"></div>

          <router-link
              v-if="currentUser?.is_admin"
              to="/admin"
              class="mobile-nav-link admin-mobile-link"
              @click="showMobileMenu = false"
          >
            <svg class="icon" viewBox="0 0 24 24" width="16" height="16">
              <path fill="currentColor" d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/>
            </svg>
            网站管理
          </router-link>

          <router-link to="/user" class="mobile-nav-link" @click="showMobileMenu = false">
            <svg class="icon" viewBox="0 0 24 24" width="16" height="16">
              <path fill="currentColor" d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z"/>
            </svg>
            用户中心
          </router-link>

          <router-link to="/setting" class="mobile-nav-link" @click="showMobileMenu = false">
            <svg class="icon" viewBox="0 0 24 24" width="16" height="16">
              <path fill="currentColor" d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z"/>
            </svg>
            设置
          </router-link>

          <div class="mobile-divider"></div>

          <button
              v-if="isLoggedIn"
              class="mobile-nav-link mobile-logout"
              @click="handleLogout"
          >
            <svg class="icon" viewBox="0 0 24 24" width="16" height="16">
              <path fill="currentColor" d="M17 7l-1.41 1.41L18.17 11H8v2h10.17l-2.58 2.58L17 17l5-5zM4 5h8V3H4c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h8v-2H4V5z"/>
            </svg>
            退出登录
          </button>

          <router-link
              v-else
              to="/login"
              class="mobile-nav-link admin-mobile-link"
              @click="showMobileMenu = false"
          >
            <svg class="icon" viewBox="0 0 24 24" width="16" height="16">
              <path fill="currentColor" d="M11 7L9.6 8.4l2.6 2.6H2v2h10.2l-2.6 2.6L11 17l5-5zM20 19h-8v2h8c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2h-8v2h8v14z"/>
            </svg>
            登录
          </router-link>
        </div>
      </transition>
    </header>

    <main class="main-content" :class="{ 'no-navbar': !showNavbar }">
      <router-view />
    </main>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { useTheme } from '@/composables/useTheme'

const route = useRoute()
const { initTheme } = useTheme()

const showNavbar = computed(() => route.path !== '/login')
const showUserDropdown = ref(false)
const showMobileMenu = ref(false)

const currentUser = computed(() => {
  const user = localStorage.getItem('user')
  return user ? JSON.parse(user) : null
})

const isLoggedIn = computed(() => !!localStorage.getItem('token'))

function handleLogout() {
  localStorage.removeItem('token')
  localStorage.removeItem('user')
  showUserDropdown.value = false
  showMobileMenu.value = false
  window.location.href = '/home'
}

onMounted(() => {
  initTheme()
})
</script>

<style>
/* ==================== CSS 主题变量 ==================== */
:root {
  --bg-page:        #f6f8fa;
  --bg-card:        #ffffff;
  --bg-navbar:      #ffffff;
  --bg-hover:       #f5f7fa;
  --bg-input:       #ffffff;
  --border-color:   #ebeef5;
  --border-input:   #dcdfe6;
  --text-primary:   #2c3e50;
  --text-regular:   #303133;
  --text-secondary: #606266;
  --text-muted:     #909399;
  --text-light:     #c0c4cc;
  --shadow-sm:      0 2px 8px rgba(0, 0, 0, 0.06);
  --shadow-md:      0 4px 16px rgba(0, 0, 0, 0.12);
  --shadow-lg:      0 8px 32px rgba(0, 0, 0, 0.12);
}

html.dark {
  --bg-page:        #0d1117;
  --bg-card:        #161b22;
  --bg-navbar:      #161b22;
  --bg-hover:       #21262d;
  --bg-input:       #0d1117;
  --border-color:   #30363d;
  --border-input:   #30363d;
  --text-primary:   #e6edf3;
  --text-regular:   #c9d1d9;
  --text-secondary: #8b949e;
  --text-muted:     #6e7681;
  --text-light:     #484f58;
  --shadow-sm:      0 2px 8px rgba(0, 0, 0, 0.4);
  --shadow-md:      0 4px 16px rgba(0, 0, 0, 0.5);
  --shadow-lg:      0 8px 32px rgba(0, 0, 0, 0.6);
}

/* ==================== 全局重置 ==================== */
html, body {
  margin: 0;
  padding: 0;
  width: 100%;
  height: 100%;
  box-sizing: border-box;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  background-color: var(--bg-page);
  overflow-x: hidden;
  transition: background-color 0.3s ease, color 0.3s ease;
}

*, *::before, *::after { box-sizing: inherit; }

#app { width: 100%; height: 100%; }

.app-container {
  position: fixed;
  top: 0; left: 0; right: 0; bottom: 0;
  display: flex;
  flex-direction: column;
}

/* ==================== 导航栏 ==================== */
.top-navbar {
  position: relative;
  flex-shrink: 0;
  width: 100%;
  height: 64px;
  background-color: var(--bg-navbar);
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 40px;
  box-shadow: var(--shadow-sm);
  border-bottom: 1px solid var(--border-color);
  z-index: 1000;
  transition: background-color 0.3s ease, border-color 0.3s ease;
}

.nav-left { display: flex; align-items: center; gap: 12px; }

.site-title {
  font-size: 1.3rem;
  font-weight: 700;
  color: var(--text-primary);
  letter-spacing: 1px;
  transition: color 0.3s ease;
}

/* ==================== 电脑端菜单 ==================== */
.nav-right.pc-menu { display: flex; align-items: center; height: 100%; }

.nav-link {
  color: var(--text-secondary);
  text-decoration: none;
  font-size: 1rem;
  font-weight: 500;
  padding: 0 20px;
  height: 64px;
  line-height: 64px;
  transition: color 0.25s ease;
  border-bottom: 3px solid transparent;
  white-space: nowrap;
}

.nav-link:hover { color: #42b883; }

.nav-link.active {
  color: #42b883;
  font-weight: 600;
  border-bottom: 3px solid #42b883;
}

.dropdown-wrapper { position: relative; height: 64px; }

.user-trigger {
  display: flex;
  align-items: center;
  gap: 6px;
  color: var(--text-secondary);
  text-decoration: none;
  font-size: 1rem;
  font-weight: 500;
  padding: 0 20px;
  height: 64px;
  white-space: nowrap;
  transition: color 0.25s;
}

.dropdown-wrapper:hover .user-trigger { color: #42b883; }
.trigger-icon { flex-shrink: 0; }
.caret { flex-shrink: 0; transition: transform 0.25s ease; }
.caret-up { transform: rotate(180deg); }

.dropdown-menu {
  position: absolute;
  top: 60px;
  right: 0;
  background-color: var(--bg-card);
  min-width: 150px;
  border-radius: 6px;
  box-shadow: var(--shadow-md);
  border: 1px solid var(--border-color);
  padding: 6px 0;
  z-index: 1001;
  transition: background-color 0.3s ease, border-color 0.3s ease;
}

.dropdown-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 10px 16px;
  color: var(--text-secondary);
  text-decoration: none;
  font-size: 0.95rem;
  transition: all 0.2s ease;
  cursor: pointer;
  background: none;
  border: none;
  width: 100%;
}

.dropdown-item:hover { background-color: var(--bg-hover); color: #42b883; }
.dropdown-item .icon { color: var(--text-muted); transition: color 0.2s; }
.dropdown-item:hover .icon { color: #42b883; }

.admin-item { color: #409eff; }
.admin-item .icon { color: #409eff; }
.admin-item:hover { background-color: #ecf5ff; color: #337ecc; }
.admin-item:hover .icon { color: #337ecc; }

.logout-item {
  border-top: 1px solid var(--border-color);
  margin-top: 4px;
  color: #f56c6c !important;
}
.logout-item:hover { background-color: #fef0f0 !important; }
.logout-item .icon { color: #f56c6c !important; }

/* ==================== 手机端汉堡按钮 ==================== */
.mobile-menu-btn {
  display: none;
  flex-direction: column;
  justify-content: space-between;
  width: 26px;
  height: 18px;
  background: transparent;
  border: none;
  cursor: pointer;
  padding: 0;
  z-index: 1002;
}

.mobile-menu-btn .bar {
  width: 100%;
  height: 3px;
  background-color: var(--text-primary);
  border-radius: 2px;
  transition: all 0.3s ease;
}

.mobile-menu-btn.is-active .bar:nth-child(1) { transform: translateY(7.5px) rotate(45deg); }
.mobile-menu-btn.is-active .bar:nth-child(2) { opacity: 0; }
.mobile-menu-btn.is-active .bar:nth-child(3) { transform: translateY(-7.5px) rotate(-45deg); }

/* ==================== 手机端下拉菜单 ==================== */
.mobile-menu {
  display: none;
  position: absolute;
  top: 64px;
  left: 0;
  width: 100%;
  background-color: var(--bg-card);
  box-shadow: var(--shadow-md);
  padding: 10px 0;
  border-top: 1px solid var(--border-color);
  z-index: 999;
  transition: background-color 0.3s ease;
}

.mobile-nav-link {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 14px 40px;
  color: var(--text-primary);
  text-decoration: none;
  font-size: 1.05rem;
  font-weight: 500;
  transition: background 0.2s;
  background: none;
  border: none;
  width: 100%;
  cursor: pointer;
}

.mobile-nav-link:hover { background-color: var(--bg-hover); color: #42b883; }
.mobile-nav-link .icon { color: var(--text-muted); }

.admin-mobile-link { color: #409eff; }
.admin-mobile-link .icon { color: #409eff; }
.admin-mobile-link:hover { background-color: var(--bg-hover); color: #337ecc; }

.mobile-logout { color: #f56c6c !important; }
.mobile-logout .icon { color: #f56c6c !important; }
.mobile-logout:hover { background-color: var(--bg-hover) !important; }

.mobile-divider { height: 1px; background-color: var(--border-color); margin: 6px 0; }

/* ==================== 响应式 ==================== */
@media (max-width: 768px) {
  .top-navbar { padding: 0 24px; }
  .nav-right.pc-menu { display: none; }
  .mobile-menu-btn { display: flex; }
  .mobile-menu { display: block; }
}

/* ==================== 动画 ==================== */
.dropdown-fade-enter-active, .dropdown-fade-leave-active { transition: all 0.25s ease-out; }
.dropdown-fade-enter-from, .dropdown-fade-leave-to { opacity: 0; transform: translateY(-8px); }

.mobile-menu-fade-enter-active, .mobile-menu-fade-leave-active { transition: all 0.25s ease; }
.mobile-menu-fade-enter-from, .mobile-menu-fade-leave-to { opacity: 0; transform: translateY(-10px); }

/* ==================== 主内容区 ==================== */
.main-content {
  flex: 1;
  overflow: auto;
  width: 100%;
  min-height: 0;
  background-color: var(--bg-page);
  transition: background-color 0.3s ease;
}

.main-content.no-navbar { }

.main-content > * {
  flex: 1;
  width: 100%;
  min-height: 0;
}
</style>
