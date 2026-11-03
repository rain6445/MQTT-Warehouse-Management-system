import express from 'express'
import cors from 'cors'
import dotenv from 'dotenv'
import path from 'path'
import { fileURLToPath } from 'url'
import authRoutes from '../router/authRoutes.js'
import mqttRoutes from '../router/mqttRoutes.js'
import { initMqtt } from './mqttService.js'

dotenv.config()

const __dirname = path.dirname(fileURLToPath(import.meta.url))

const app = express()
const PORT = process.env.SERVER_PORT || 3000

// 中间件
app.use(cors({ origin: 'http://localhost:5173' }))
app.use(express.json())

// 静态文件：暴露 public/uploads 目录，供前端访问头像图片
app.use('/uploads', express.static(path.join(__dirname, '../../public/uploads')))

// 挂载路由
app.use('/api/auth', authRoutes)
app.use('/api/mqtt', mqttRoutes)

// 健康检查接口
app.get('/api/health', (req, res) => {
    res.json({ status: 'ok', message: '后端服务运行正常' })
})

// 启动时连接 MQTT broker 并订阅主题
initMqtt()

app.listen(PORT, () => {
    console.log(`后端服务已启动 → http://localhost:${PORT}`)
})
