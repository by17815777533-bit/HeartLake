#!/usr/bin/env node

import { randomUUID } from 'node:crypto'
import process from 'node:process'
import { createRequire } from 'node:module'

const require = createRequire(import.meta.url)

let WebSocketImpl
try {
  ({ WebSocket: WebSocketImpl } = require('../admin/node_modules/ws'))
} catch (error) {
  try {
    ({ WebSocket: WebSocketImpl } = require('ws'))
  } catch (innerError) {
    console.error('[smoke-client] 缺少 ws 依赖，无法执行 WebSocket 联调')
    console.error(`[smoke-client] 详细错误: ${innerError.message}`)
    process.exit(1)
  }
}

const baseUrl = normalizeBaseUrl(process.argv[2] || process.env.SMOKE_BASE_URL || 'http://127.0.0.1:3000')
const wsUrl = process.env.SMOKE_WS_URL?.trim() || toWsUrl(baseUrl)
const timeoutMs = Number(process.env.SMOKE_TIMEOUT_MS || 15000)

async function main() {
  console.log(`[smoke-client] base=${baseUrl}`)
  console.log(`[smoke-client] ws=${wsUrl}`)

  const health = await fetchJson(`${baseUrl}/api/health`)
  assertOk(health.ok, `健康检查失败: HTTP ${health.status}`)
  console.log('[smoke-client] HTTP 健康检查通过')

  const deviceId = `smoke_${randomUUID()}`
  const auth = await fetchJson(`${baseUrl}/api/auth/anonymous`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ device_id: deviceId }),
  })

  assertOk(auth.ok, `匿名登录失败: HTTP ${auth.status}`)
  assertOk(auth.body?.code === 0 || auth.body?.success === true, `匿名登录返回异常: ${JSON.stringify(auth.body)}`)

  const data = auth.body?.data ?? auth.body
  const token = data?.token
  const userId = data?.user_id
  assertOk(typeof token === 'string' && token.length > 0, '匿名登录未返回 token')
  assertOk(typeof userId === 'string' && userId.length > 0, '匿名登录未返回 user_id')
  console.log(`[smoke-client] 匿名登录通过 user_id=${userId}`)

  await connectWebSocket({ token, userId })
  console.log('[smoke-client] WebSocket 握手、鉴权和 ping/pong 通过')
}

async function fetchJson(url, init) {
  const response = await fetch(url, init)
  const text = await response.text()
  let body = null
  if (text) {
    try {
      body = JSON.parse(text)
    } catch {
      throw new Error(`响应不是合法 JSON: ${text}`)
    }
  }
  return { ok: response.ok, status: response.status, body }
}

async function connectWebSocket({ token, userId }) {
  const endpoint = new URL(wsUrl)
  endpoint.searchParams.set('token', token)

  await new Promise((resolve, reject) => {
    const socket = new WebSocketImpl(endpoint.toString(), {
      handshakeTimeout: timeoutMs,
    })

    const timer = setTimeout(() => {
      socket.terminate?.()
      reject(new Error(`WebSocket 超时，${timeoutMs}ms 内未完成握手`))
    }, timeoutMs)

    let settled = false
    const finish = (error) => {
      if (settled) return
      settled = true
      clearTimeout(timer)
      if (error) {
        socket.terminate?.()
        reject(error)
        return
      }
      socket.close(1000, 'smoke-test-complete')
      resolve()
    }

    socket.on('open', () => {
      socket.send(JSON.stringify({ type: 'join', room: 'lake' }))
      socket.send(JSON.stringify({ type: 'ping' }))
    })

    socket.on('message', (raw) => {
      const payload = parseMessage(raw)
      if (!payload || typeof payload.type !== 'string') {
        return
      }

      if (payload.type === 'ping') {
        socket.send(JSON.stringify({ type: 'pong' }))
        return
      }

      if (payload.type === 'pong') {
        finish()
        return
      }

      if (payload.type === 'error') {
        finish(new Error(`WebSocket 返回错误: ${JSON.stringify(payload)}`))
      }
    })

    socket.on('error', (error) => finish(error))
    socket.on('close', (code, reason) => {
      if (!settled) {
        finish(new Error(`WebSocket 提前关闭: code=${code} reason=${String(reason)}`))
      }
    })
  })

  console.log(`[smoke-client] 已验证用户 ${userId} 的实时链路`)
}

function parseMessage(raw) {
  const text = typeof raw === 'string' ? raw : raw.toString('utf8')
  if (!text) return null
  try {
    return JSON.parse(text)
  } catch {
    return null
  }
}

function normalizeBaseUrl(url) {
  return url.trim().replace(/\/+$/, '')
}

function toWsUrl(url) {
  return `${url.replace(/^https:\/\//, 'wss://').replace(/^http:\/\//, 'ws://')}/ws/broadcast`
}

function assertOk(condition, message) {
  if (!condition) {
    throw new Error(message)
  }
}

main().catch((error) => {
  console.error(`[smoke-client] ${error.message}`)
  process.exit(1)
})
