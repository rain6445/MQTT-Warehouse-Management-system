<template>
  <div class="setting-page">
    <div class="setting-card">
      <div class="card-header">
        <svg viewBox="0 0 24 24" width="22" height="22" class="card-icon">
          <path fill="currentColor" d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z"/>
        </svg>
        <span>外观设置</span>
      </div>

      <div class="setting-row">
        <div class="setting-info">
          <!-- 根据当前模式显示对应图标 -->
          <div class="mode-icon-wrap">
            <transition name="icon-flip" mode="out-in">
              <svg v-if="isDark" key="moon" viewBox="0 0 24 24" width="24" height="24" class="mode-icon">
                <path fill="currentColor" d="M12 3a9 9 0 1 0 9 9c0-.46-.04-.92-.1-1.36a5.389 5.389 0 0 1-4.4 2.26 5.403 5.403 0 0 1-3.14-9.8c-.44-.06-.9-.1-1.36-.1z"/>
              </svg>
              <svg v-else key="sun" viewBox="0 0 24 24" width="24" height="24" class="mode-icon">
                <path fill="currentColor" d="M12 7c-2.76 0-5 2.24-5 5s2.24 5 5 5 5-2.24 5-5-2.24-5-5-5zM2 13h2c.55 0 1-.45 1-1s-.45-1-1-1H2c-.55 0-1 .45-1 1s.45 1 1 1zm18 0h2c.55 0 1-.45 1-1s-.45-1-1-1h-2c-.55 0-1 .45-1 1s.45 1 1 1zM11 2v2c0 .55.45 1 1 1s1-.45 1-1V2c0-.55-.45-1-1-1s-1 .45-1 1zm0 18v2c0 .55.45 1 1 1s1-.45 1-1v-2c0-.55-.45-1-1-1s-1 .45-1 1zM5.99 4.58a.996.996 0 0 0-1.41 0 .996.996 0 0 0 0 1.41l1.06 1.06c.39.39 1.03.39 1.41 0s.39-1.03 0-1.41L5.99 4.58zm12.37 12.37a.996.996 0 0 0-1.41 0 .996.996 0 0 0 0 1.41l1.06 1.06c.39.39 1.03.39 1.41 0a.996.996 0 0 0 0-1.41l-1.06-1.06zm1.06-12.37l-1.06 1.06a.996.996 0 0 0 0 1.41c.39.39 1.03.39 1.41 0l1.06-1.06a.996.996 0 0 0 0-1.41.996.996 0 0 0-1.41 0zM7.05 18.36l-1.06 1.06a.996.996 0 0 0 0 1.41c.39.39 1.03.39 1.41 0l1.06-1.06a.996.996 0 0 0 0-1.41.996.996 0 0 0-1.41 0z"/>
              </svg>
            </transition>
          </div>
          <div class="setting-text">
            <span class="setting-label">深色模式</span>
            <span class="setting-desc">
              {{ isDark ? '当前：深色主题' : '当前：浅色主题' }}
            </span>
          </div>
        </div>

        <!-- 拨动开关 -->
        <button
            class="toggle"
            :class="{ 'toggle--on': isDark }"
            role="switch"
            :aria-checked="isDark"
            aria-label="切换深浅模式"
            @click="toggleTheme"
        >
          <span class="toggle-thumb">
            <transition name="thumb-icon" mode="out-in">
              <!-- 深色时显示月亮，浅色时显示太阳 -->
              <svg v-if="isDark" key="m" viewBox="0 0 24 24" width="12" height="12">
                <path fill="currentColor" d="M12 3a9 9 0 1 0 9 9c0-.46-.04-.92-.1-1.36a5.389 5.389 0 0 1-4.4 2.26 5.403 5.403 0 0 1-3.14-9.8c-.44-.06-.9-.1-1.36-.1z"/>
              </svg>
              <svg v-else key="s" viewBox="0 0 24 24" width="12" height="12">
                <path fill="currentColor" d="M12 7c-2.76 0-5 2.24-5 5s2.24 5 5 5 5-2.24 5-5-2.24-5-5-5zM2 13h2c.55 0 1-.45 1-1s-.45-1-1-1H2c-.55 0-1 .45-1 1s.45 1 1 1zm18 0h2c.55 0 1-.45 1-1s-.45-1-1-1h-2c-.55 0-1 .45-1 1s.45 1 1 1zM11 2v2c0 .55.45 1 1 1s1-.45 1-1V2c0-.55-.45-1-1-1s-1 .45-1 1zm0 18v2c0 .55.45 1 1 1s1-.45 1-1v-2c0-.55-.45-1-1-1s-1 .45-1 1zM5.99 4.58a.996.996 0 0 0-1.41 0 .996.996 0 0 0 0 1.41l1.06 1.06c.39.39 1.03.39 1.41 0s.39-1.03 0-1.41L5.99 4.58zm12.37 12.37a.996.996 0 0 0-1.41 0 .996.996 0 0 0 0 1.41l1.06 1.06c.39.39 1.03.39 1.41 0a.996.996 0 0 0 0-1.41l-1.06-1.06zm1.06-12.37l-1.06 1.06a.996.996 0 0 0 0 1.41c.39.39 1.03.39 1.41 0l1.06-1.06a.996.996 0 0 0 0-1.41.996.996 0 0 0-1.41 0zM7.05 18.36l-1.06 1.06a.996.996 0 0 0 0 1.41c.39.39 1.03.39 1.41 0l1.06-1.06a.996.996 0 0 0 0-1.41.996.996 0 0 0-1.41 0z"/>
              </svg>
            </transition>
          </span>
        </button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { useTheme } from '@/composables/useTheme'

const { isDark, toggleTheme } = useTheme()
</script>

<style scoped>
/* ==================== 页面容器 ==================== */
.setting-page {
  flex: 1;
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 40px 24px;
  box-sizing: border-box;
  background-color: var(--bg-page);
  transition: background-color 0.3s ease;
}

/* ==================== 卡片 ==================== */
.setting-card {
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

/* ==================== 设置行 ==================== */
.setting-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24px;
  gap: 16px;
}

.setting-info {
  display: flex;
  align-items: center;
  gap: 16px;
}

.mode-icon-wrap {
  width: 44px;
  height: 44px;
  border-radius: 50%;
  background-color: var(--bg-hover);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  transition: background-color 0.3s ease;
}

.mode-icon { color: var(--text-primary); transition: color 0.3s ease; }

.setting-text {
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.setting-label {
  font-size: 1rem;
  font-weight: 600;
  color: var(--text-primary);
  transition: color 0.3s ease;
}

.setting-desc {
  font-size: 0.82rem;
  color: var(--text-muted);
  transition: color 0.3s ease;
}

/* ==================== 拨动开关 ==================== */
.toggle {
  position: relative;
  width: 56px;
  height: 30px;
  border-radius: 15px;
  border: none;
  cursor: pointer;
  background-color: var(--border-input);
  transition: background-color 0.3s ease;
  flex-shrink: 0;
  padding: 0;
}

.toggle--on {
  background-color: #42b883;
}

.toggle-thumb {
  position: absolute;
  top: 3px;
  left: 3px;
  width: 24px;
  height: 24px;
  border-radius: 50%;
  background-color: #ffffff;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.2);
  transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  color: #e6a23c;
}

.toggle--on .toggle-thumb {
  transform: translateX(26px);
  color: #35495e;
}

/* ==================== 动画 ==================== */
.icon-flip-enter-active,
.icon-flip-leave-active {
  transition: all 0.25s ease;
}
.icon-flip-enter-from {
  opacity: 0;
  transform: rotate(-90deg) scale(0.6);
}
.icon-flip-leave-to {
  opacity: 0;
  transform: rotate(90deg) scale(0.6);
}

.thumb-icon-enter-active,
.thumb-icon-leave-active {
  transition: all 0.2s ease;
}
.thumb-icon-enter-from,
.thumb-icon-leave-to {
  opacity: 0;
  transform: scale(0.4);
}
</style>
