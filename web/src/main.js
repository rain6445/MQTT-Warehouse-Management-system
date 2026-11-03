import './assets/main.css'

import { createApp } from 'vue'
import App from './App.vue'
import router from './router' // 引入刚才创建的路由

const app = createApp(App)

app.use(router) // 注册路由插件
app.mount('#app')