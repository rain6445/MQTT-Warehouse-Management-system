import express from 'express'
import jwt from 'jsonwebtoken'
import {
    getLatestMessages,
    addSseClient,
    removeSseClient
} from '../server/mqttService.js'

const router = express.Router()

// ==================== GET /api/mqtt/stream — SSE 实时消息流 ====================
router.get('/stream', (req, res) => {
    // EventSource 无法带 header，token 通过 query 传递
    const token = req.query.token
    if (!token) {
        return res.status(401).json({ message: '缺少认证 token' })
    }
    try {
        jwt.verify(token, process.env.JWT_SECRET)
    } catch {
        return res.status(401).json({ message: 'token 无效或已过期' })
    }

    // SSE 响应头
    res.setHeader('Content-Type', 'text/event-stream')
    res.setHeader('Cache-Control', 'no-cache')
    res.setHeader('Connection', 'keep-alive')
    res.flushHeaders()

    // 连接建立后，先把缓存的历史消息一次性发过去
    res.write(`data: ${JSON.stringify({
        type: 'history',
        data: getLatestMessages()
    })}\n\n`)

    // 注册为在线客户端
    addSseClient(res)

    // 心跳：每 25 秒发一个注释行，防止代理/网关掐断空闲连接
    const heartbeat = setInterval(() => {
        res.write(': ping\n\n')
    }, 25000)

    // 客户端断开时清理
    req.on('close', () => {
        clearInterval(heartbeat)
        removeSseClient(res)
    })
})

export default router
