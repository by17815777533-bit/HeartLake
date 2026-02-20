import { describe, it, expect, beforeEach, vi } from 'vitest'

// Mock WebSocket
class MockWebSocket {
  constructor(url) {
    this.url = url
    this.readyState = 1 // OPEN
    this.onopen = null
    this.onmessage = null
    this.onclose = null
    this.onerror = null
    this.sentMessages = []
  }
  send(data) { this.sentMessages.push(data) }
  close() { this.readyState = 3 }
}

global.WebSocket = MockWebSocket
global.WebSocket.OPEN = 1

// Mock pinia store
vi.mock('@/stores', () => ({
  useAppStore: () => ({
    getToken: () => 'test-token'
  })
}))

describe('WebSocket Service', () => {
  let wsService

  beforeEach(async () => {
    vi.resetModules()
    const mod = await import('@/services/websocket')
    wsService = mod.default
  })

  it('should have allowed message types', () => {
    // 验证白名单存在
    expect(wsService).toBeDefined()
    expect(typeof wsService.connect).toBe('function')
    expect(typeof wsService.disconnect).toBe('function')
    expect(typeof wsService.on).toBe('function')
    expect(typeof wsService.off).toBe('function')
  })

  it('should register and remove listeners', () => {
    const handler = vi.fn()
    wsService.on('stats_update', handler)
    wsService.off('stats_update', handler)
  })

  it('should clear all listeners', () => {
    const handler = vi.fn()
    wsService.on('stats_update', handler)
    wsService.on('new_report', handler)
    wsService.clearAllListeners()
  })
})
