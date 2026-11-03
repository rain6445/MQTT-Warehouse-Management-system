import mysql from 'mysql2/promise'
import dotenv from 'dotenv'
import { fileURLToPath } from 'url'
import { dirname, resolve } from 'path'

// ES 模块里没有 __dirname，需要手动构造
const __filename = fileURLToPath(import.meta.url)
const __dirname = dirname(__filename)

// 显式指定 .env 路径（从 src/server 向上两级到项目根目录）
dotenv.config({ path: resolve(__dirname, '../../.env') })

const pool = mysql.createPool({
    host: process.env.DB_HOST,
    port: process.env.DB_PORT,
    user: process.env.DB_USER,
    password: process.env.DB_PASSWORD,
    database: process.env.DB_NAME,
    waitForConnections: true,
    connectionLimit: 10,
    queueLimit: 0
})

export default pool
