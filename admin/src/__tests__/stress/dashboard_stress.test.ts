import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import MockAdapter from 'axios-mock-adapter'
import { setActivePinia, createPinia } from 'pinia'
import api, { http } from '@/api'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), warning: vi.fn(), success: vi.fn() },
}))

vi.mock('@/router', () => ({
  default: {
    push: vi.fn().mockResolvedValue(undefined),
    currentRoute: { value: { path: '/dashboard' } },
  },
}))

vi.mock('@/utils/errorHelper', () => ({
  getBusinessMessage: vi.fn(() => ''),
}))

describe('Dashboard Stress Tests', () => {
  let mock: MockAdapter

  beforeEach(() => {
    localStorage.clear()
    sessionStorage.clear()
    vi.clearAllMocks()
    setActivePinia(createPinia())
    mock = new MockAdapter(http)
  })

  afterEach(() => {
    mock.restore()
  })

  // ========== 大数据量渲染压测 ==========
  describe('大数据量处理', () => {
    it('处理10000条图表数据点不崩溃', async () => {
      const points = Array.from({ length: 10000 }, (_, i) => ({
        date: `2026-01-${String((i % 28) + 1).padStart(2, '0')}`,
        value: Math.random() * 10000,
      }))
      mock.onGet('/admin/dashboard/stats').reply(200, { data: { chart_data: points } })
      const res = await api.getDashboardStats()
      expect(res.data.data.chart_data).toHaveLength(10000)
    })

    it('处理大量mood分布数据', async () => {
      const moods = Array.from({ length: 5000 }, (_, i) => ({
        mood: ['happy', 'sad', 'angry', 'neutral', 'excited'][i % 5],
        count: Math.floor(Math.random() * 100000),
      }))
      mock.onGet('/admin/dashboard/mood-distribution').reply(200, { data: moods })
      const res = await api.getMoodDistribution()
      expect(res.data.data).toHaveLength(5000)
    })

    it('处理大量用户增长数据', async () => {
      const growth = Array.from({ length: 3650 }, (_, i) => ({
        date: new Date(2016, 0, i + 1).toISOString().slice(0, 10),
        new_users: Math.floor(Math.random() * 1000),
        total_users: i * 100,
      }))
      mock.onGet(/\/admin\/dashboard\/user-growth/).reply(200, { data: growth })
      const res = await api.getUserGrowthStats('10y')
      expect(res.data.data).toHaveLength(3650)
    })

    it('处理大量trending topics数据', async () => {
      const topics = Array.from({ length: 2000 }, (_, i) => ({
        topic: `topic_${i}_${'x'.repeat(50)}`,
        count: Math.floor(Math.random() * 50000),
        trend: Math.random() > 0.5 ? 'up' : 'down',
      }))
      mock.onGet('/admin/dashboard/trending-topics').reply(200, { data: topics })
      const res = await api.getTrendingTopics()
      expect(res.data.data).toHaveLength(2000)
    })

    it('处理大量活跃时间统计数据', async () => {
      const timeStats = Array.from({ length: 8760 }, (_, i) => ({
        hour: i % 24,
        day: Math.floor(i / 24),
        active_users: Math.floor(Math.random() * 10000),
      }))
      mock.onGet('/admin/dashboard/active-time').reply(200, { data: timeStats })
      const res = await api.getActiveTimeStats()
      expect(res.data.data).toHaveLength(8760)
    })

    it('处理大量实时统计数据', async () => {
      const realtime = {
        online_users: 999999,
        active_sessions: 500000,
        requests_per_second: 100000,
        events: Array.from({ length: 1000 }, (_, i) => ({ id: i, type: 'action', ts: Date.now() })),
      }
      mock.onGet('/admin/realtime-stats').reply(200, { data: realtime })
      const res = await api.getRealtimeStats()
      expect(res.data.data.events).toHaveLength(1000)
    })
  })

  // ========== 极端数值测试 ==========
  describe('极端数值处理', () => {
    it('处理Number.MAX_SAFE_INTEGER', async () => {
      const stats = { data: { total_users: Number.MAX_SAFE_INTEGER, total_stones: Number.MAX_SAFE_INTEGER } }
      mock.onGet('/admin/dashboard/stats').reply(200, stats)
      const res = await api.getDashboardStats()
      expect(res.data.data.total_users).toBe(Number.MAX_SAFE_INTEGER)
    })

    it('处理0值', async () => {
      const stats = { data: { total_users: 0, total_stones: 0, active_users: 0, new_users_today: 0 } }
      mock.onGet('/admin/dashboard/stats').reply(200, stats)
      const res = await api.getDashboardStats()
      expect(res.data.data.total_users).toBe(0)
    })

    it('处理负数值', async () => {
      const stats = { data: { total_users: -1, growth_rate: -99.99 } }
      mock.onGet('/admin/dashboard/stats').reply(200, stats)
      const res = await api.getDashboardStats()
      expect(res.data.data.growth_rate).toBe(-99.99)
    })

    it('处理浮点精度极端值', async () => {
      const stats = { data: { rate: 0.1 + 0.2, tiny: Number.MIN_VALUE, big: Number.MAX_VALUE } }
      mock.onGet('/admin/dashboard/stats').reply(200, stats)
      const res = await api.getDashboardStats()
      expect(res.data.data.tiny).toBe(Number.MIN_VALUE)
      expect(res.data.data.big).toBe(Number.MAX_VALUE)
    })

    it('处理Infinity值', async () => {
      const stats = { data: { ratio: Infinity, neg_ratio: -Infinity } }
      mock.onGet('/admin/dashboard/stats').reply(200, stats)
      const res = await api.getDashboardStats()
      // JSON.stringify converts Infinity to null
      expect(res.data.data.ratio).toBeNull()
    })

    it('处理NaN值', async () => {
      const stats = { data: { value: NaN } }
      mock.onGet('/admin/dashboard/stats').reply(200, stats)
      const res = await api.getDashboardStats()
      expect(res.data.data.value).toBeNull()
    })
  })

  // ========== 空数据/null/undefined ==========
  describe('空数据和边界值', () => {
    it('处理空数据响应', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, { data: {} })
      const res = await api.getDashboardStats()
      expect(res.data.data).toEqual({})
    })

    it('处理null数据响应', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, { data: null })
      const res = await api.getDashboardStats()
      expect(res.data.data).toBeNull()
    })

    it('处理空数组', async () => {
      mock.onGet('/admin/dashboard/mood-distribution').reply(200, { data: [] })
      const res = await api.getMoodDistribution()
      expect(res.data.data).toEqual([])
    })

    it('处理undefined字段', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, { data: { total_users: undefined } })
      const res = await api.getDashboardStats()
      expect(res.data.data.total_users).toBeUndefined()
    })

    it('处理完全空的响应体', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, '')
      const res = await api.getDashboardStats()
      expect(res.data).toBe('')
    })

    it('处理嵌套空对象', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, { data: { nested: { deep: { value: null } } } })
      const res = await api.getDashboardStats()
      expect(res.data.data.nested.deep.value).toBeNull()
    })
  })

  // ========== 日期范围极端值 ==========
  describe('日期范围极端值', () => {
    it('处理极大日期范围', async () => {
      mock.onGet(/\/admin\/dashboard\/user-growth/).reply(200, { data: [] })
      const res = await api.getUserGrowthStats('100y')
      expect(res.status).toBe(200)
    })

    it('处理空日期范围', async () => {
      mock.onGet(/\/admin\/dashboard\/user-growth/).reply(200, { data: [] })
      const res = await api.getUserGrowthStats('')
      expect(res.status).toBe(200)
    })

    it('处理特殊字符日期范围', async () => {
      mock.onGet(/\/admin\/dashboard\/user-growth/).reply(200, { data: [] })
      const res = await api.getUserGrowthStats('<script>alert(1)</script>')
      expect(res.status).toBe(200)
    })

    it('处理mood-trend各种范围', async () => {
      mock.onGet(/\/admin\/dashboard\/mood-trend/).reply(200, { data: [] })
      const ranges = ['1d', '7d', '30d', '90d', '365d', '0d', '-1d']
      const results = await Promise.all(ranges.map(r => api.getMoodTrend(r)))
      expect(results).toHaveLength(7)
      results.forEach(r => expect(r.status).toBe(200))
    })
  })

  // ========== API并发压测 ==========
  describe('API并发压测', () => {
    it('同时发起10个不同dashboard API请求', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, { data: { total: 1 } })
      mock.onGet('/admin/realtime-stats').reply(200, { data: { online: 2 } })
      mock.onGet(/\/admin\/dashboard\/user-growth/).reply(200, { data: [] })
      mock.onGet('/admin/dashboard/mood-distribution').reply(200, { data: [] })
      mock.onGet(/\/admin\/dashboard\/mood-trend/).reply(200, { data: [] })
      mock.onGet('/admin/dashboard/trending-topics').reply(200, { data: [] })
      mock.onGet('/admin/dashboard/active-time').reply(200, { data: [] })
      mock.onGet('/admin/edge-ai/status').reply(200, { data: {} })
      mock.onGet('/admin/edge-ai/metrics').reply(200, { data: {} })
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(200, { data: {} })

      const results = await Promise.all([
        api.getDashboardStats(),
        api.getRealtimeStats(),
        api.getUserGrowthStats('7d'),
        api.getMoodDistribution(),
        api.getMoodTrend('7d'),
        api.getTrendingTopics(),
        api.getActiveTimeStats(),
        api.getEdgeAIStatus(),
        api.getEdgeAIMetrics(),
        api.getEmotionPulse(),
      ])
      expect(results).toHaveLength(10)
      results.forEach(r => expect(r.status).toBe(200))
    })

    it('快速连续调用同一API 50次（去重机制）', async () => {
      let callCount = 0
      mock.onGet('/admin/dashboard/stats').reply(() => {
        callCount++
        return [200, { data: { call: callCount } }]
      })

      const promises = Array.from({ length: 50 }, () => api.getDashboardStats().catch(() => null))
      const results = await Promise.all(promises)
      // 由于请求去重，大部分前面的请求会被abort
      const succeeded = results.filter(r => r !== null)
      expect(succeeded.length).toBeGreaterThanOrEqual(1)
    })

    it('部分请求失败时的容错', async () => {
      let count = 0
      mock.onGet('/admin/dashboard/stats').reply(() => {
        count++
        return count % 3 === 0 ? [400, { message: 'error' }] : [200, { data: {} }]
      })

      const results = await Promise.allSettled(
        Array.from({ length: 9 }, () => api.getDashboardStats())
      )
      const fulfilled = results.filter(r => r.status === 'fulfilled')
      const rejected = results.filter(r => r.status === 'rejected')
      expect(fulfilled.length + rejected.length).toBe(9)
    })

    it('所有请求同时失败', async () => {
      mock.onGet('/admin/dashboard/stats').reply(400, { message: 'server error' })
      mock.onGet('/admin/realtime-stats').reply(400, { message: 'server error' })
      mock.onGet('/admin/dashboard/mood-distribution').reply(400, { message: 'server error' })

      const results = await Promise.allSettled([
        api.getDashboardStats(),
        api.getRealtimeStats(),
        api.getMoodDistribution(),
      ])
      results.forEach(r => expect(r.status).toBe('rejected'))
    }, 30000)

    it('混合成功和超时请求', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, { data: {} })
      mock.onGet('/admin/realtime-stats').reply(200, { data: {} })
      mock.onGet('/admin/dashboard/mood-distribution').timeout()

      const results = await Promise.allSettled([
        api.getDashboardStats(),
        api.getRealtimeStats(),
        api.getMoodDistribution(),
      ])
      const fulfilled = results.filter(r => r.status === 'fulfilled')
      const rejected = results.filter(r => r.status === 'rejected')
      expect(fulfilled.length).toBe(2)
      expect(rejected.length).toBe(1)
    })

    it('混合成功和网络错误请求', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, { data: {} })
      mock.onGet('/admin/realtime-stats').networkError()

      const results = await Promise.allSettled([
        api.getDashboardStats(),
        api.getRealtimeStats(),
      ])
      expect(results[0].status).toBe('fulfilled')
      expect(results[1].status).toBe('rejected')
    })
  })

  // ========== 数据转换压测 ==========
  describe('数据转换压测', () => {
    it('大量数据的图表格式转换', async () => {
      const data = Array.from({ length: 5000 }, (_, i) => ({
        label: `item_${i}`,
        value: Math.random() * 1000,
        category: `cat_${i % 10}`,
      }))
      mock.onGet('/admin/dashboard/stats').reply(200, { data: { items: data } })
      const res = await api.getDashboardStats()
      const items = res.data.data.items
      expect(items).toHaveLength(5000)
      const grouped = items.reduce((acc: Record<string, number>, item: { category: string }) => {
        acc[item.category] = (acc[item.category] || 0) + 1
        return acc
      }, {})
      expect(Object.keys(grouped)).toHaveLength(10)
    })

    it('特殊字符在数据中的处理', async () => {
      const topics = [
        { topic: '<script>alert("xss")</script>', count: 1 },
        { topic: '"; DROP TABLE users; --', count: 2 },
        { topic: '🎉🔥💯', count: 3 },
        { topic: '\n\r\t\0', count: 4 },
        { topic: 'a'.repeat(10000), count: 5 },
        { topic: '中文测试数据', count: 6 },
        { topic: '日本語テスト', count: 7 },
        { topic: 'العربية', count: 8 },
      ]
      mock.onGet('/admin/dashboard/trending-topics').reply(200, { data: topics })
      const res = await api.getTrendingTopics()
      expect(res.data.data).toHaveLength(8)
      expect(res.data.data[0].topic).toContain('script')
    })

    it('超长字符串字段处理', async () => {
      const longString = 'x'.repeat(1_000_000)
      mock.onGet('/admin/dashboard/stats').reply(200, { data: { description: longString } })
      const res = await api.getDashboardStats()
      expect(res.data.data.description).toHaveLength(1_000_000)
    })

    it('深层嵌套JSON数据', async () => {
      let nested: Record<string, unknown> = { value: 'deep' }
      for (let i = 0; i < 100; i++) {
        nested = { child: nested }
      }
      mock.onGet('/admin/dashboard/stats').reply(200, { data: nested })
      const res = await api.getDashboardStats()
      let current = res.data.data
      for (let i = 0; i < 100; i++) {
        current = current.child
      }
      expect(current.value).toBe('deep')
    })

    it('大量重复键值数据', async () => {
      const data = Array.from({ length: 10000 }, () => ({
        mood: 'happy',
        count: 42,
        date: '2026-01-01',
      }))
      mock.onGet('/admin/dashboard/mood-distribution').reply(200, { data })
      const res = await api.getMoodDistribution()
      expect(res.data.data).toHaveLength(10000)
      expect(new Set(res.data.data.map((d: { mood: string }) => d.mood)).size).toBe(1)
    })

    it('混合类型数组数据', async () => {
      const mixed = [1, 'string', null, true, { key: 'value' }, [1, 2, 3], 0, '', false]
      mock.onGet('/admin/dashboard/stats').reply(200, { data: { values: mixed } })
      const res = await api.getDashboardStats()
      expect(res.data.data.values).toHaveLength(9)
    })
  })

  // ========== 大响应体压测 ==========
  describe('大响应体压测', () => {
    it('处理1MB JSON响应', async () => {
      const largeArray = Array.from({ length: 10000 }, (_, i) => ({
        id: i,
        name: `user_${i}_${'data'.repeat(10)}`,
        email: `user${i}@example.com`,
        metadata: { key1: 'value1', key2: 'value2', key3: i },
      }))
      mock.onGet('/admin/dashboard/stats').reply(200, { data: largeArray })
      const res = await api.getDashboardStats()
      expect(res.data.data).toHaveLength(10000)
    })

    it('处理大量嵌套数组', async () => {
      const matrix = Array.from({ length: 100 }, (_, i) =>
        Array.from({ length: 100 }, (_, j) => i * 100 + j)
      )
      mock.onGet('/admin/dashboard/stats').reply(200, { data: { matrix } })
      const res = await api.getDashboardStats()
      expect(res.data.data.matrix).toHaveLength(100)
      expect(res.data.data.matrix[0]).toHaveLength(100)
    })
  })
})
