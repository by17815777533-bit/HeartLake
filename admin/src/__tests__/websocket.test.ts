import { describe, it, expect, beforeEach, vi } from 'vitest'

// Mock WebSocket
class MockWebSocket {
  static instances: MockWebSocket[] = []
  url: string
  readyState: number
  onopen: ((ev: Event) => void) | null
  onmessage: ((ev: MessageEvent) => void) | null
  onclose: ((ev: CloseEvent) => void) | null
  onerror: ((ev: Event) => void) | null
  sentMessages: string[]

  constructor(url: string) {
    this.url = url
    this.readyState = 1 // OPEN
    this.onopen = null
    this.onmessage = null
    this.onclose = null
    this.onerror = null
    this.sentMessages = []
    MockWebSocket.instances.push(this)
  }
  send(data: string): void {
    this.sentMessages.push(data)
  }
  close(): void {
    this.readyState = 3
  }
}

Object.assign(globalThis, {
  WebSocket: MockWebSocket,
})
;(globalThis as unknown as Record<string, unknown>).WebSocket = MockWebSocket
;(MockWebSocket as unknown as Record<string, number>).OPEN = 1

// Mock pinia store
vi.mock('@/stores', () => ({
  useAppStore: () => ({
    getToken: (): string => 'test-token',
  }),
}))

interface WSService {
  connect: () => void
  disconnect: () => void
  on: (type: string, fn: (data: unknown) => void) => void
  off: (type: string, fn: (data: unknown) => void) => void
  clearAllListeners: () => void
}

describe('WebSocket Service', () => {
  let wsService: WSService

  beforeEach(async () => {
    vi.resetModules()
    MockWebSocket.instances = []
    const mod = await import('@/services/websocket')
    wsService = mod.default as unknown as WSService
  })

  it('should have allowed message types', () => {
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

  it('should normalize flat websocket payloads', () => {
    const handler = vi.fn()
    wsService.on('new_stone', handler)
    wsService.connect()

    const ws = MockWebSocket.instances.at(-1)
    expect(ws).toBeTruthy()

    ws?.onmessage?.({
      data: JSON.stringify({ type: 'new_stone', stone_id: 'stone-1', triggered_by: 'user-1' }),
    } as MessageEvent)

    expect(handler).toHaveBeenCalledWith(
      expect.objectContaining({ type: 'new_stone', stone_id: 'stone-1', triggered_by: 'user-1' }),
    )
  })
})
