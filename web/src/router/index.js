import { createRouter, createWebHistory } from 'vue-router'

const routes = [
    { path: '/', redirect: '/home' },
    {
        path: '/login',
        name: 'Login',
        component: () => import('../views/Login.vue'),
        meta: { requiresAuth: false }
    },
    {
        path: '/home',
        name: 'Home',
        component: () => import('../views/Home.vue'),
        meta: { requiresAuth: false }
    },
    {
        path: '/device',
        name: 'Device',
        component: () => import('../views/Device.vue'),
        meta: { requiresAuth: true }
    },
    {
        path: '/about',
        name: 'About',
        component: () => import('../views/About.vue'),
        meta: { requiresAuth: false }
    },
    {
        path: '/user',
        name: 'User',
        component: () => import('../views/User.vue'),
        meta: { requiresAuth: true }
    },
    {
        path: '/setting',
        name: 'Setting',
        component: () => import('../views/Setting.vue'),
        meta: { requiresAuth: false }
    },
    {
        path: '/admin',
        name: 'Admin',
        component: () => import('../views/Admin.vue'),
        meta: { requiresAuth: true, requiresAdmin: true }
    }
]

const router = createRouter({
    history: createWebHistory(),
    routes
})

router.beforeEach((to, from, next) => {
    const token = localStorage.getItem('token')

    // 已登录用户访问登录页 → 回首页
    if (to.path === '/login' && token) {
        next('/home')
        return
    }

    // 只有显式标记 requiresAuth: true 的页面才强制登录
    if (to.meta.requiresAuth === true && !token) {
        next({ path: '/login', query: { redirect: to.fullPath } })
        return
    }

    // 管理员路由额外校验
    if (to.meta.requiresAdmin) {
        const user = localStorage.getItem('user')
        const parsed = user ? JSON.parse(user) : null
        if (!parsed?.is_admin) {
            next('/home')
            return
        }
    }

    next()
})

export default router
