/**
 * WebSocket 服务 -- 安全认证、心跳保活、消息类型白名单
 *
 * 连接策略：
 * - 握手时通过 URL query 传递 token 完成鉴权
 * - 心跳间隔 30s，超过 2 个周期未收到 pong 判定连接已死并强制重连
 * - 非正常关闭时指数退避重连（基础 1s，最大 30s），叠加随机抖动避免惊群
 * - 最多重连 10 次，disconnect() 后不再自动重连
 *
 * 安全设计：
 * - 消息类型白名单校验，未知类型直接丢弃并 warn
 * - token 从 Pinia store 读取，不持久化在 WebSocket 层
 */

import { useAppStore } from '@/stores'
import type { WSListener } from '@/types'

/** 按消息类型分组的监听器注册表 */
const listeners: Record<string, WSListener[]> = {}
let ws: WebSocket | null = null
let reconnectTimer: ReturnType<typeof setTimeout> | null = null
let reconnectAttempts = 0
let heartbeatTimer: ReturnType<typeof setInterval> | null = null
/** 上次收到 pong 的时间戳，用于心跳超时检测 */
let lastPongTime = Date.now()
let hasEverConnected = false
const MAX_RECONNECT_ATTEMPTS = 10
const RECONNECT_BASE_INTERVAL = 1000
const HEARTBEAT_INTERVAL = 30000

/**
 * 允许的消息类型白名单。
 * 新增消息类型时在此追加，onmessage 中会校验，不在白名单内的消息直接丢弃。
 */
const WS_MESSAGE_TYPE_LIST = [
  'stats_update',
  'new_report',
  'new_moderation',
  'broadcast',
  'user_online',
  'user_offline',
  'pong',
  'auth_success',
  'new_stone',
  'ripple_update',
  'ripple_deleted',
  'boat_update',
  'boat_deleted',
  'new_boat',
  'stone_deleted',
  'friend_accepted',
  'friend_removed',
  'friend_request',
  'new_notification',
  'ping',
] as const

type WSMessageType = (typeof WS_MESSAGE_TYPE_LIST)[number]

export const WS_MESSAGE_TYPES: ReadonlySet<string> = new Set<string>(WS_MESSAGE_TYPE_LIST)

function resolveWsEndpoint(token: string): URL {
  const explicitUrl = import.meta.env.VITE_WS_URL?.trim()
  const rawUrl =
    explicitUrl ||
    `${location.protocol === 'https:' ? 'wss:' : 'ws:'}//${location.host}/ws/broadcast`
  const wsUrl = new URL(
    rawUrl,
    `${location.protocol === 'https:' ? 'wss:' : 'ws:'}//${location.host}`,
  )
  // 低配机的代理链路更稳定地支持握手期 query 鉴权；连接后仍兼容发送 auth 首包。
  wsUrl.searchParams.set('token', token)
  return wsUrl
}

/** 启动心跳定时器，定期发送 ping 并检测 pong 超时 */
function startHeartbeat() {
  stopHeartbeat()
  heartbeatTimer = setInterval(() => {
    if (ws && ws.readyState === WebSocket.OPEN) {
      // 超过2个心跳周期未收到 pong，判定连接已死
      if (Date.now() - lastPongTime > HEARTBEAT_INTERVAL * 2) {
        console.warn('WebSocket pong 超时，强制重连')
        ws.close(4000, 'pong timeout')
        return
      }
      ws.send(JSON.stringify({ type: 'ping' }))
    }
  }, HEARTBEAT_INTERVAL)
}

// 停止心跳定时器
function stopHeartbeat() {
  if (heartbeatTimer) {
    clearInterval(heartbeatTimer)
    heartbeatTimer = null
  }
}

export default {
  /** 建立 WebSocket 连接，已连接时跳过 */
  connect() {
    if (ws && ws.readyState === WebSocket.OPEN) return
    // 清理残留连接
    this._cleanup()

    // 从 Pinia store 读取 token
    const appStore = useAppStore()
    const token = appStore.getToken()
    if (!token) return

    ws = new WebSocket(resolveWsEndpoint(token).toString())

    ws.onopen = () => {
      hasEverConnected = true
      reconnectAttempts = 0
      lastPongTime = Date.now()
      // 启动心跳保活
      startHeartbeat()
    }

    ws.onmessage = (e) => {
      try {
        const parsed = JSON.parse(e.data) as Record<string, unknown>
        const type = String(parsed.type ?? '')
        // 收到 pong 时更新时间戳（心跳存活检测用）
        if (type === 'pong') {
          lastPongTime = Date.now()
        }
        // 消息类型白名单校验
        if (!WS_MESSAGE_TYPES.has(type)) {
          console.warn('收到未知 WebSocket 消息类型:', type)
          return
        }
        listeners[type]?.forEach((fn) => fn(parsed))
      } catch (err) {
        console.error('WebSocket 消息解析失败:', err)
      }
    }

    ws.onerror = () => {}

    ws.onclose = (e: CloseEvent) => {
      ws = null
      stopHeartbeat()
      // 非主动关闭时自动重连（code 1000 为正常关闭），指数退避最大30秒
      if (e.code !== 1000 && reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        const baseDelay = Math.min(RECONNECT_BASE_INTERVAL * Math.pow(2, reconnectAttempts), 30000)
        // 乘性随机抖动 [0.5x, 1.5x]，避免多客户端同时重连的惊群效应
        const jitteredDelay = Math.round(baseDelay * (0.5 + Math.random()))
        reconnectTimer = setTimeout(() => {
          reconnectAttempts++
          this.connect()
        }, jitteredDelay)
      } else if (!hasEverConnected) {
        // 轮询已经覆盖后台关键统计，不在控制台刷无意义告警。
        reconnectAttempts = MAX_RECONNECT_ATTEMPTS
      }
    }
  },

  /** 主动断开连接，停止重连和心跳 */
  disconnect() {
    // 停止重连和心跳
    if (reconnectTimer) {
      clearTimeout(reconnectTimer)
      reconnectTimer = null
    }
    stopHeartbeat()
    reconnectAttempts = MAX_RECONNECT_ATTEMPTS // 阻止自动重连
    hasEverConnected = false
    this._cleanup()
  },

  /** 清理底层 WebSocket 实例，移除事件监听后关闭连接 */
  _cleanup() {
    if (ws) {
      ws.onclose = null // 防止触发自动重连
      ws.onerror = null
      ws.onmessage = null
      ws.close()
      ws = null
    }
  },

  /** 注册指定消息类型的监听器 */
  on(type: string, fn: WSListener) {
    ;(listeners[type] ||= []).push(fn)
  },

  /** 移除指定消息类型的监听器 */
  off(type: string, fn: WSListener) {
    listeners[type] = listeners[type]?.filter((f) => f !== fn)
  },

  /** 清理所有监听器（用于完全断开时） */
  clearAllListeners() {
    Object.keys(listeners).forEach((key) => delete listeners[key])
  },
}
