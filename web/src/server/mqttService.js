import mqtt from 'mqtt'
import crypto from 'crypto'
import fs from 'fs'
import path from 'path'
import { fileURLToPath } from 'url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))

// 内存缓存最近的消息
const latestMessages = []
const MAX_CACHE = 50

// 所有正在监听的 SSE 客户端
const sseClients = new Set()

// NFC 绑卡主题
const NFC_TOPIC = process.env.MQTT_NFC_TOPIC || '1138_NFC_Card'

// 全局唯一的绑卡会话（同一时间只能为一个用户绑卡）
let bindingSession = null

let client = null
let privateKey = null

// ==================== 加载私钥 ====================
function loadPrivateKey() {
    // 私钥位于 src/keys/private_key.pem
    const keyPath = process.env.MQTT_PRIVATE_KEY_PATH
        || path.join(__dirname, '../keys/private_key.pem')
    try {
        privateKey = fs.readFileSync(keyPath, 'utf8')
        console.log(`已加载私钥: ${keyPath}`)
    } catch (err) {
        console.error(`私钥加载失败 (${keyPath}):`, err.message)
        console.error('消息将无法解密，会以原始密文显示')
    }
}

// ==================== RSA 解密（OAEP + SHA-256）====================
// 设备端：用公钥 RSA-OAEP(SHA-256) 加密，密文做 base64 编码后发布
function decryptMessage(payloadBuffer) {
    if (!privateKey) return null

    try {
        // 密文是 base64 字符串，先还原成二进制
        const cipher = Buffer.from(payloadBuffer.toString('utf8').trim(), 'base64')

        const decrypted = crypto.privateDecrypt(
            {
                key: privateKey,
                padding: crypto.constants.RSA_PKCS1_OAEP_PADDING,
                oaepHash: 'sha256'
            },
            cipher
        )
        return decrypted.toString('utf8')
    } catch (err) {
        console.error('解密失败:', err.message)
        return null
    }
}

// ==================== NFC 绑卡会话 ====================
// 开始一次绑卡：返回 Promise，刷卡成功 resolve({uid,...})，超时/取消 reject
export function startNfcBinding({ userId, username, timeoutMs = 30000 }) {
    if (bindingSession) {
        const err = new Error('已有正在进行的绑卡操作')
        err.code = 'BINDING_BUSY'
        err.currentUsername = bindingSession.username
        throw err
    }
    return new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
            bindingSession = null
            const e = new Error('绑卡超时，未检测到卡片')
            e.code = 'BINDING_TIMEOUT'
            reject(e)
        }, timeoutMs)
        bindingSession = { userId, username, resolve, reject, timer }
        console.log(`开始为用户 ${username}(id=${userId}) 绑卡，等待刷卡...`)
    })
}

// 取消当前绑卡会话（仅当 userId 匹配时）
export function cancelNfcBinding(userId) {
    if (bindingSession && bindingSession.userId === userId) {
        clearTimeout(bindingSession.timer)
        const e = new Error('绑卡已取消')
        e.code = 'BINDING_CANCELLED'
        bindingSession.reject(e)
        bindingSession = null
        return true
    }
    return false
}

// 查询当前绑卡会话状态
export function getNfcBindingStatus() {
    return bindingSession
        ? { userId: bindingSession.userId, username: bindingSession.username }
        : null
}

// 收到 NFC 卡消息时尝试完成绑卡
function handleNfcMessage(text) {
    if (!bindingSession) return
    try {
        const data = JSON.parse(text)
        if (data && data.uid) {
            const session = bindingSession
            clearTimeout(session.timer)
            bindingSession = null
            console.log(`检测到卡片 uid=${data.uid}，绑定到 ${session.username}`)
            session.resolve({
                uid: data.uid,
                student_id: data.student_id || null,
                device: data.device || null
            })
        }
    } catch {
        // 非 JSON 或缺字段，忽略
    }
}

// ==================== 初始化 MQTT 连接 ====================
export function initMqtt() {
    loadPrivateKey()

    const url = process.env.MQTT_URL || 'mqtt://localhost:1883'

    const topics = (process.env.MQTT_TOPICS || '1138_NFC_Card,1138_MQTT_Pub_JSON')
        .split(',')
        .map(t => t.trim())
        .filter(Boolean)

    const options = {
        clientId: process.env.MQTT_CLIENT_ID || 'Web', // 客户端 ID 设为 Web
        reconnectPeriod: 3000,
        connectTimeout: 10000
    }
    if (process.env.MQTT_USERNAME) options.username = process.env.MQTT_USERNAME
    if (process.env.MQTT_PASSWORD) options.password = process.env.MQTT_PASSWORD

    client = mqtt.connect(url, options)

    client.on('connect', () => {
        console.log(`MQTT 已连接 → ${url} (clientId=${options.clientId})`)
        client.subscribe(topics, (err, granted) => {
            if (err) {
                console.error('MQTT 订阅失败:', err.message)
            } else {
                granted.forEach(g => console.log(`已订阅主题: ${g.topic}`))
            }
        })
    })

    client.on('message', (topic, payload) => {
        // 用私钥 RSA-OAEP(SHA-256) 解密
        const plain = decryptMessage(payload)
        const text = plain !== null ? plain : payload.toString()

        const msg = {
            topic,
            // 解密成功显示明文；失败保留原始密文并标记
            message: text,
            decrypted: plain !== null,
            timestamp: new Date().toISOString()
        }

        latestMessages.unshift(msg)
        if (latestMessages.length > MAX_CACHE) latestMessages.pop()

        broadcast({ type: 'message', data: msg })

        // 如果是 NFC 卡主题且正在绑卡，尝试完成绑定
        if (topic === NFC_TOPIC) {
            handleNfcMessage(text)
        }
    })

    client.on('reconnect', () => console.log('MQTT 重连中...'))
    client.on('error', (err) => console.error('MQTT 错误:', err.message))
    client.on('offline', () => console.log('MQTT 离线'))
}

// ==================== SSE 工具 ====================
function broadcast(payload) {
    const data = `data: ${JSON.stringify(payload)}\n\n`
    for (const res of sseClients) {
        res.write(data)
    }
}

export function getLatestMessages() {
    return latestMessages
}

export function addSseClient(res) {
    sseClients.add(res)
}

export function removeSseClient(res) {
    sseClients.delete(res)
}
