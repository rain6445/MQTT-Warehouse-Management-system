import express from 'express'
import bcrypt from 'bcryptjs'
import jwt from 'jsonwebtoken'
import pool from '../server/db.js'
import multer from 'multer'
import path from 'path'
import fs from 'fs'
import { fileURLToPath } from 'url'
import { startNfcBinding, cancelNfcBinding } from '../server/mqttService.js'

const __dirname = path.dirname(fileURLToPath(import.meta.url))

// 头像保存目录：项目根目录 / public / uploads / avatars
const AVATAR_DIR = path.join(__dirname, '../../public/uploads/avatars')
if (!fs.existsSync(AVATAR_DIR)) {
    fs.mkdirSync(AVATAR_DIR, { recursive: true })
}

// multer 配置
const storage = multer.diskStorage({
    destination: (_req, _file, cb) => cb(null, AVATAR_DIR),
    filename: (req, file, cb) => {
        const ext = path.extname(file.originalname).toLowerCase()
        cb(null, `${req.user.id}-${Date.now()}${ext}`)
    }
})

const ALLOWED_TYPES = ['image/jpeg', 'image/png', 'image/webp', 'image/gif']

const upload = multer({
    storage,
    limits: { fileSize: 2 * 1024 * 1024 }, // 最大 2MB
    fileFilter: (_req, file, cb) => {
        if (ALLOWED_TYPES.includes(file.mimetype)) {
            cb(null, true)
        } else {
            cb(new Error('只支持 JPG、PNG、WebP、GIF 格式'))
        }
    }
})

const router = express.Router()

// ==================== 中间件：验证 JWT ====================
function authenticateToken(req, res, next) {
    const authHeader = req.headers['authorization']
    const token = authHeader && authHeader.split(' ')[1]

    if (!token) return res.status(401).json({ message: '未提供 Token' })

    try {
        req.user = jwt.verify(token, process.env.JWT_SECRET)
        next()
    } catch {
        res.status(401).json({ message: 'Token 无效或已过期' })
    }
}

// 中间件：验证管理员身份
function requireAdmin(req, res, next) {
    if (!req.user?.is_admin) {
        return res.status(403).json({ message: '权限不足' })
    }
    next()
}

// ==================== POST /api/auth/login ====================
router.post('/login', async (req, res) => {
    const { username, password } = req.body

    if (!username || !password) {
        return res.status(400).json({ message: '用户名和密码不能为空' })
    }

    try {
        const [rows] = await pool.execute(
            'SELECT id, username, password, is_admin, avatar FROM users WHERE username = ?',
            [username]
        )

        if (rows.length === 0) {
            return res.status(401).json({ message: '用户名或密码错误' })
        }

        const user = rows[0]
        const isMatch = await bcrypt.compare(password, user.password)
        if (!isMatch) {
            return res.status(401).json({ message: '用户名或密码错误' })
        }

        const token = jwt.sign(
            { id: user.id, username: user.username, is_admin: user.is_admin === 1 },
            process.env.JWT_SECRET,
            { expiresIn: '8h' }
        )

        res.json({
            message: '登录成功',
            token,
            user: {
                id: user.id,
                username: user.username,
                is_admin: user.is_admin === 1,
                avatar: user.avatar || null
            }
        })
    } catch (err) {
        console.error('登录接口错误:', err)
        res.status(500).json({ message: '服务器内部错误' })
    }
})

// ==================== GET /api/auth/me ====================
router.get('/me', authenticateToken, async (req, res) => {
    try {
        const [rows] = await pool.execute(
            'SELECT id, username, is_admin, avatar FROM users WHERE id = ?',
            [req.user.id]
        )

        if (rows.length === 0) {
            return res.status(404).json({ message: '用户不存在' })
        }

        const user = rows[0]
        res.json({
            user: {
                id: user.id,
                username: user.username,
                is_admin: user.is_admin === 1,
                avatar: user.avatar || null
            }
        })
    } catch (err) {
        console.error('获取用户信息错误:', err)
        res.status(500).json({ message: '服务器内部错误' })
    }
})

// ==================== GET /api/auth/users — 获取所有用户（仅管理员）====================
router.get('/users', authenticateToken, requireAdmin, async (req, res) => {
    try {
        const [rows] = await pool.execute(
            'SELECT id, username, is_admin, uid, created_at FROM users ORDER BY id ASC'
        )
        res.json({ users: rows })
    } catch (err) {
        console.error('获取用户列表错误:', err)
        res.status(500).json({ message: '服务器内部错误' })
    }
})

// ==================== POST /api/auth/users — 创建普通用户（仅管理员）====================
router.post('/users', authenticateToken, requireAdmin, async (req, res) => {
    const { username } = req.body

    if (!username || !username.trim()) {
        return res.status(400).json({ message: '用户名不能为空' })
    }

    const trimmed = username.trim()

    if (trimmed.length < 2 || trimmed.length > 20) {
        return res.status(400).json({ message: '用户名长度须在 2~20 个字符之间' })
    }

    try {
        const [exists] = await pool.execute(
            'SELECT id FROM users WHERE username = ?',
            [trimmed]
        )
        if (exists.length > 0) {
            return res.status(409).json({ message: '用户名已存在' })
        }

        const hashed = await bcrypt.hash('123456', 10)

        const [result] = await pool.execute(
            'INSERT INTO users (username, password, is_admin) VALUES (?, ?, 0)',
            [trimmed, hashed]
        )

        res.status(201).json({
            message: '用户创建成功',
            user: { id: result.insertId, username: trimmed, is_admin: 0 }
        })
    } catch (err) {
        console.error('创建用户错误:', err)
        res.status(500).json({ message: '服务器内部错误' })
    }
})

// ==================== DELETE /api/auth/users/:id — 删除普通用户（仅管理员）====================
router.delete('/users/:id', authenticateToken, requireAdmin, async (req, res) => {
    const targetId = parseInt(req.params.id, 10)

    if (isNaN(targetId)) {
        return res.status(400).json({ message: '无效的用户 ID' })
    }

    if (targetId === req.user.id) {
        return res.status(400).json({ message: '不能删除自己的账号' })
    }

    try {
        const [rows] = await pool.execute(
            'SELECT id, is_admin, avatar FROM users WHERE id = ?',
            [targetId]
        )

        if (rows.length === 0) {
            return res.status(404).json({ message: '用户不存在' })
        }

        if (rows[0].is_admin) {
            return res.status(403).json({ message: '不能删除管理员账号' })
        }

        // 删除用户头像文件
        if (rows[0].avatar) {
            const avatarFile = path.join(__dirname, '../../public', rows[0].avatar)
            if (fs.existsSync(avatarFile)) {
                fs.unlink(avatarFile, () => {})
            }
        }

        await pool.execute('DELETE FROM users WHERE id = ?', [targetId])

        res.json({ message: '用户已删除' })
    } catch (err) {
        console.error('删除用户错误:', err)
        res.status(500).json({ message: '服务器内部错误' })
    }
})

// ==================== PATCH /api/auth/users/:id/reset-password ====================
router.patch('/users/:id/reset-password', authenticateToken, requireAdmin, async (req, res) => {
    const targetId = parseInt(req.params.id, 10)
    if (isNaN(targetId)) return res.status(400).json({ message: '无效的用户 ID' })

    try {
        const [rows] = await pool.execute('SELECT id FROM users WHERE id = ?', [targetId])
        if (rows.length === 0) return res.status(404).json({ message: '用户不存在' })

        const hashed = await bcrypt.hash('123456', 10)
        await pool.execute('UPDATE users SET password = ? WHERE id = ?', [hashed, targetId])
        res.json({ message: '密码已重置为 123456' })
    } catch (err) {
        console.error('重置密码错误:', err)
        res.status(500).json({ message: '服务器内部错误' })
    }
})

// ==================== POST /api/auth/users/:id/bind-nfc — 绑定 NFC 卡（仅管理员）====================
// 长轮询：开始监听 NFC 卡，刷卡成功后写入 uid。同一时间只允许一个绑卡会话。
router.post('/users/:id/bind-nfc', authenticateToken, requireAdmin, async (req, res) => {
    const targetId = parseInt(req.params.id, 10)
    if (isNaN(targetId)) return res.status(400).json({ message: '无效的用户 ID' })

    // 确认目标用户存在
    let username
    try {
        const [rows] = await pool.execute(
            'SELECT id, username FROM users WHERE id = ?',
            [targetId]
        )
        if (rows.length === 0) return res.status(404).json({ message: '用户不存在' })
        username = rows[0].username
    } catch (err) {
        console.error('绑卡查询用户错误:', err)
        return res.status(500).json({ message: '服务器内部错误' })
    }

    // 开始绑卡会话（可能因已有会话进行中而失败）
    let bindingPromise
    try {
        bindingPromise = startNfcBinding({ userId: targetId, username, timeoutMs: 30000 })
    } catch (err) {
        if (err.code === 'BINDING_BUSY') {
            return res.status(409).json({
                message: `正在为「${err.currentUsername}」绑卡，请稍后再试`
            })
        }
        return res.status(500).json({ message: '无法开始绑卡' })
    }

    // 客户端中途断开（关闭弹窗）时取消会话
    let responded = false
    req.on('close', () => {
        if (!responded) cancelNfcBinding(targetId)
    })

    // 等待刷卡
    let card
    try {
        card = await bindingPromise
    } catch (err) {
        responded = true
        if (err.code === 'BINDING_TIMEOUT') {
            return res.status(408).json({ message: '绑卡超时，未检测到卡片' })
        }
        if (err.code === 'BINDING_CANCELLED') {
            return res.status(499).json({ message: '绑卡已取消' })
        }
        return res.status(500).json({ message: '绑卡失败' })
    }

    // 检查该 uid 是否已被其他用户占用
    try {
        const [dup] = await pool.execute(
            'SELECT id, username FROM users WHERE uid = ? AND id != ?',
            [card.uid, targetId]
        )
        if (dup.length > 0) {
            responded = true
            return res.status(409).json({
                message: `该卡已绑定到用户「${dup[0].username}」`
            })
        }

        await pool.execute(
            'UPDATE users SET uid = ? WHERE id = ?',
            [card.uid, targetId]
        )

        responded = true
        res.json({
            message: '绑卡成功',
            uid: card.uid,
            student_id: card.student_id,
            device: card.device
        })
    } catch (err) {
        responded = true
        // 唯一索引兜底
        if (err.code === 'ER_DUP_ENTRY') {
            return res.status(409).json({ message: '该卡已被其他用户绑定' })
        }
        console.error('写入 uid 错误:', err)
        res.status(500).json({ message: '服务器内部错误' })
    }
})

// ==================== POST /api/auth/users/:id/bind-nfc/cancel — 取消绑卡（仅管理员）====================
router.post('/users/:id/bind-nfc/cancel', authenticateToken, requireAdmin, (req, res) => {
    const targetId = parseInt(req.params.id, 10)
    if (isNaN(targetId)) return res.status(400).json({ message: '无效的用户 ID' })

    const cancelled = cancelNfcBinding(targetId)
    res.json({ message: cancelled ? '已取消绑卡' : '当前没有该用户的绑卡会话' })
})

// ==================== PATCH /api/auth/change-password — 修改自己的密码 ====================
router.patch('/change-password', authenticateToken, async (req, res) => {
    const { oldPassword, newPassword } = req.body

    if (!oldPassword || !newPassword) {
        return res.status(400).json({ message: '旧密码和新密码不能为空' })
    }

    if (newPassword.length < 6) {
        return res.status(400).json({ message: '新密码至少 6 位' })
    }

    if (oldPassword === newPassword) {
        return res.status(400).json({ message: '新密码不能与旧密码相同' })
    }

    try {
        const [rows] = await pool.execute(
            'SELECT id, password FROM users WHERE id = ?',
            [req.user.id]
        )

        if (rows.length === 0) {
            return res.status(404).json({ message: '用户不存在' })
        }

        const isMatch = await bcrypt.compare(oldPassword, rows[0].password)
        if (!isMatch) {
            return res.status(401).json({ message: '旧密码错误' })
        }

        const hashed = await bcrypt.hash(newPassword, 10)
        await pool.execute('UPDATE users SET password = ? WHERE id = ?', [hashed, req.user.id])

        res.json({ message: '密码修改成功' })
    } catch (err) {
        console.error('修改密码错误:', err)
        res.status(500).json({ message: '服务器内部错误' })
    }
})

// ==================== PATCH /api/auth/avatar — 上传头像 ====================
router.patch('/avatar', authenticateToken, (req, res) => {
    upload.single('avatar')(req, res, async (err) => {
        if (err) {
            if (err.code === 'LIMIT_FILE_SIZE') {
                return res.status(400).json({ message: '图片不能超过 2MB' })
            }
            return res.status(400).json({ message: err.message || '上传失败' })
        }

        if (!req.file) {
            return res.status(400).json({ message: '请选择图片文件' })
        }

        const newAvatarUrl = `/uploads/avatars/${req.file.filename}`

        try {
            // 查旧头像，上传成功后删除旧文件
            const [rows] = await pool.execute(
                'SELECT avatar FROM users WHERE id = ?',
                [req.user.id]
            )

            if (rows.length > 0 && rows[0].avatar) {
                const oldFile = path.join(__dirname, '../../public', rows[0].avatar)
                if (fs.existsSync(oldFile)) {
                    fs.unlink(oldFile, () => {})
                }
            }

            await pool.execute(
                'UPDATE users SET avatar = ? WHERE id = ?',
                [newAvatarUrl, req.user.id]
            )

            res.json({ message: '头像更新成功', avatar: newAvatarUrl })
        } catch (dbErr) {
            console.error('头像更新数据库错误:', dbErr)
            // 数据库失败时删掉刚上传的文件，避免垃圾文件堆积
            fs.unlink(req.file.path, () => {})
            res.status(500).json({ message: '服务器内部错误' })
        }
    })
})

export default router
