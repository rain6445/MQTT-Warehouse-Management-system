import axios from 'axios'

const request = axios.create({
    baseURL: '/api',
    timeout: 10000
})

// 请求拦截器：自动把 JWT token 加到请求头
request.interceptors.request.use(config => {
    const token = localStorage.getItem('token')
    if (token) {
        config.headers['Authorization'] = `Bearer ${token}`
    }
    return config
})

// 响应拦截器：统一处理 401（未登录）跳转
request.interceptors.response.use(
    response => response.data,
    error => {
        if (error.response?.status === 401 || error.response?.status === 403) {
            localStorage.removeItem('token')
            localStorage.removeItem('user')
            window.location.href = '/login'
        }
        return Promise.reject(error)
    }
)

export default request
