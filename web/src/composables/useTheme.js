import { ref } from 'vue'

// 模块级单例，所有组件共享同一个状态
const isDark = ref(false)

export function useTheme() {
    function applyTheme(dark) {
        document.documentElement.classList.toggle('dark', dark)
        localStorage.setItem('theme', dark ? 'dark' : 'light')
    }

    function toggleTheme() {
        isDark.value = !isDark.value
        applyTheme(isDark.value)
    }

    // 在 App.vue 的 onMounted 里调用一次
    function initTheme() {
        const saved = localStorage.getItem('theme')
        if (saved) {
            isDark.value = saved === 'dark'
        } else {
            // 跟随系统偏好
            isDark.value = window.matchMedia('(prefers-color-scheme: dark)').matches
        }
        applyTheme(isDark.value)
    }

    return { isDark, toggleTheme, initTheme }
}
