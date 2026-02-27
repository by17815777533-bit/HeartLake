/**
 * WebSocket 服务 - 安全认证 + 心跳保活 + 消息类型白名单
 */

import { useAppStore } from '@/stores'
import type { WSListener } from '@/types'

const listeners: Record<string, WSListener[]> = {}
let ws: WebSocket | null = null
let reconnectTimer: ReturnType<typeof setTimeout> | null = null
let reconnectAttempts = 0
let heartbeatTimer: ReturnType<typeof setInterval> | null = null
let lastPongTime = Date.now()
const MAX_RECONNECT_ATTEMPTS = 10
const RECONNECT_BASE_INTERVAL = 1000 // 基础重连间隔1秒，指数退避
const HEARTBEAT_INTERVAL = 30000 // 30秒心跳间隔

// 允许的 WebSocket 消息类型白名单
// 新增消息类型时在此处统一维护，避免硬编码散落在业务逻辑中
const WS_MESSAGE_TYPE_LIST = [
  'stats_update',
  'new_report',
  'new_moderation',
  'broadcast',
  'user_online',
  'user_offline',
  'pong',
  'auth_success',
  'auth_fail',
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

type WSMessageType = typeof WS_MESSAGE_TYPE_LIST[number]

export const WS_MESSAGE_TYPES: ReadonlySet<string> = new Set<string>(WS_MESSAGE_TYPE_LIST)

// M-2: 启动心跳定时器
function startHeartbeat() {
  stopHeartbeat()
  heartbeatTimer = setInterval(() => {
    if (ws && ws.readyState === WebSocket.OPEN) {
      // 检测 pong 超时：超过2个心跳周期未收到 pong，判定连接已死
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
  connect() {
    if (ws && ws.readyState === WebSocket.OPEN) return
    // 清理残留连接
    this._cleanup()

    // C-2: 统一从 Pinia store 读取 token
    const appStore = useAppStore()
    const token = appStore.getToken()
    if (!token) return

    // S-2: 连接时不带 token，避免凭据暴露在 URL / 日志中
    const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:'
    ws = new WebSocket(`${protocol}//${location.host}/ws/broadcast`)

    ws.onopen = () => {
      // 连接后发送认证消息，避免 token 暴露在 URL 中
      ws!.send(JSON.stringify({ type: 'auth', token }))
      reconnectAttempts = 0
      lastPongTime = Date.now()
      // M-2: 启动心跳保活
      startHeartbeat()
    }

    ws.onmessage = e => {
      try {
        const { type, data } = JSON.parse(e.data)
        // 收到 pong 时更新时间戳（心跳存活检测用）
        if (type === 'pong') {
          lastPongTime = Date.now()
        }
        // M-3: 消息类型白名单校验
        if (!WS_MESSAGE_TYPES.has(type)) {
          console.warn('收到未知 WebSocket 消息类型:', type)
          return
        }
        listeners[type]?.forEach(fn => fn(data))
      } catch (err) {
        console.error('WebSocket 消息解析失败:', err)
      }
    }

    ws.onerror = (e) => {
      console.warn('WebSocket 连接错误:', e)
    }

    ws.onclose = (e: CloseEvent) => {
      ws = null
      stopHeartbeat()
      // 非主动关闭时自动重连（code 1000 为正常关闭）
      // 使用指数退避策略，最大间隔30秒
      if (e.code !== 1000 && reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        const baseDelay = Math.min(RECONNECT_BASE_INTERVAL * Math.pow(2, reconnectAttempts), 30000)
        // L-23: 乘性随机抖动，延迟在 [0.5x, 1.5x] 范围内波动，避免惊群效应
        const jitteredDelay = Math.round(baseDelay * (0.5 + Math.random()))
        reconnectTimer = setTimeout(() => {
          reconnectAttempts++
          this.connect()
        }, jitteredDelay)
      }
    }
  },

  disconnect() {
    // 停止重连和心跳
    if (reconnectTimer) {
      clearTimeout(reconnectTimer)
      reconnectTimer = null
    }
    stopHeartbeat()
    reconnectAttempts = MAX_RECONNECT_ATTEMPTS // 阻止自动重连
    this._cleanup()
  },

  _cleanup() {
    if (ws) {
      ws.onclose = null // 防止触发自动重连
      ws.onerror = null
      ws.onmessage = null
      ws.close()
      ws = null
    }
  },

  on(type: string, fn: WSListener) {
    (listeners[type] ||= []).push(fn)
  },

  off(type: string, fn: WSListener) {
    listeners[type] = listeners[type]?.filter(f => f !== fn)
  },

  // 清理所有监听器（用于完全断开时）
  clearAllListeners() {
    Object.keys(listeners).forEach(key => delete listeners[key])
  }
}
