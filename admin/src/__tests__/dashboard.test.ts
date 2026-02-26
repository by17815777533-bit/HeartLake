/**
 * @file dashboard.test.ts
 * @brief Dashboard 相关 API 调用测试
 */
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

const statsData = {
  data: {
    total_users: 1234,
    total_stones: 5678,
    total_boats: 890,
    total_ripples: 3456,
    today_users: 56,
    today_stones: 123,
  },
}

const growthData = {
  data: [
    { date: '2025-01-01', count: 10 },
    { date: '2025-01-02', count: 15 },
    { date: '2025-01-03', count: 20 },
  ],
}

const moodData = {
  data: [
    { mood: 'joyful', count: 100 },
    { mood: 'calm', count: 80 },
    { mood: 'melancholy', count: 60 },
    { mood: 'anxious', count: 40 },
  ],
}

const privacyData = {
  data: {
    epsilon_used: 3.5,
    epsilon_budget: 10.0,
    queries_today: 128,
  },
}

const pulseData = {
  data: {
    avgScore: 0.65,
    stddev: 0.12,
    trend: 'stable',
    sampleCount: 50,
    windowSeconds: 300,
  },
}

describe('Dashboard API', () => {
  let mock: MockAdapter

  beforeEach(() => {
    localStorage.clear()
    sessionStorage.clear()
    vi.clearAllMocks()
    setActivePinia(createPinia())
    mock = new MockAdapter(http)
  })

  afterEach(() => { mock.restore() })

  describe('仪表盘统计', () => {
    it('应正确获取统计数据', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, statsData)
      const res = await api.getDashboardStats()
      expect(res.data.data.total_users).toBe(1234)
      expect(res.data.data.total_stones).toBe(5678)
    })

    it('应包含今日数据', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, statsData)
      const res = await api.getDashboardStats()
      expect(res.data.data.today_users).toBe(56)
      expect(res.data.data.today_stones).toBe(123)
    })

    it('接口错误应抛出异常', async () => {
      mock.onGet('/admin/dashboard/stats').reply(400)
      await expect(api.getDashboardStats()).rejects.toThrow()
    })

    it('网络错误应抛出异常', async () => {
      mock.onGet('/admin/dashboard/stats').networkError()
      try { await api.getDashboardStats(); expect.unreachable() } catch (e: any) { expect(e.message).toContain('Network Error') }
    })

    it('应包含 total_boats', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, statsData)
      const res = await api.getDashboardStats()
      expect(res.data.data.total_boats).toBe(890)
    })

    it('应包含 total_ripples', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, statsData)
      const res = await api.getDashboardStats()
      expect(res.data.data.total_ripples).toBe(3456)
    })

    it('空响应体不崩溃', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, {})
      const res = await api.getDashboardStats()
      expect(res.status).toBe(200)
    })

    it('应传递 range 参数 7d', async () => {
      mock.onGet(/\/admin\/dashboard\/user-growth/).reply(200, growthData)
      await api.getUserGrowthStats('7d')
      expect(mock.history.get[0].url).toContain('range=7d')
    })
  })

  describe('用户增长趋势', () => {
    it('应正确获取增长数据', async () => {
      mock.onGet(/\/admin\/dashboard\/user-growth/).reply(200, growthData)
      const res = await api.getUserGrowthStats('7d')
      expect(res.data.data).toHaveLength(3)
      expect(res.data.data[0].date).toBe('2025-01-01')
    })

    it('应传递 range 参数', async () => {
      mock.onGet(/\/admin\/dashboard\/user-growth/).reply(200, growthData)
      await api.getUserGrowthStats('30d')
      expect(mock.history.get[0].url).toContain('range=30d')
    })
  })

  describe('心情分布', () => {
    it('应正确获取心情分布', async () => {
      mock.onGet('/admin/dashboard/mood-distribution').reply(200, moodData)
      const res = await api.getMoodDistribution()
      expect(res.data.data).toHaveLength(4)
      expect(res.data.data[0].mood).toBe('joyful')
    })
  })

  describe('隐私统计', () => {
    it('应正确获取隐私预算数据', async () => {
      mock.onGet('/admin/edge-ai/privacy-budget').reply(200, privacyData)
      const res = await api.getPrivacyBudget()
      expect(res.data.data.epsilon_used).toBe(3.5)
      expect(res.data.data.epsilon_budget).toBe(10.0)
    })

    it('接口错误应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/privacy-budget').reply(500)
      await expect(api.getPrivacyBudget()).rejects.toThrow()
    }, 15000)
  })

  describe('情绪脉搏', () => {
    it('应正确获取脉搏数据', async () => {
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(200, pulseData)
      const res = await api.getEmotionPulse()
      expect(res.data.data.avgScore).toBe(0.65)
      expect(res.data.data.trend).toBe('stable')
      expect(res.data.data.sampleCount).toBe(50)
    })

    it('接口错误应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(500)
      await expect(api.getEmotionPulse()).rejects.toThrow()
    }, 15000)
  })

  describe('数据完整性', () => {
    it('应包含所有心情类型', async () => {
      mock.onGet('/admin/dashboard/mood-distribution').reply(200, moodData)
      const res = await api.getMoodDistribution()
      const moods = res.data.data.map((d: any) => d.mood)
      expect(moods).toContain('joyful')
      expect(moods).toContain('anxious')
    })

    it('隐私统计应包含 queries_today', async () => {
      mock.onGet('/admin/edge-ai/privacy-budget').reply(200, privacyData)
      const res = await api.getPrivacyBudget()
      expect(res.data.data.queries_today).toBe(128)
    })

    it('脉搏应包含 stddev 和 windowSeconds', async () => {
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(200, pulseData)
      const res = await api.getEmotionPulse()
      expect(res.data.data.stddev).toBe(0.12)
      expect(res.data.data.windowSeconds).toBe(300)
    })

    it('getRealtimeStats 成功', async () => {
      mock.onGet('/admin/realtime-stats').reply(200, { data: { online: 42 } })
      const res = await api.getRealtimeStats()
      expect(res.data.data.online).toBe(42)
    })

    it('getActiveTimeStats 成功', async () => {
      mock.onGet('/admin/dashboard/active-time').reply(200, { data: [{ hour: 0, count: 10 }] })
      const res = await api.getActiveTimeStats()
      expect(res.data.data[0].hour).toBe(0)
    })

    it('getTrendingTopics 成功', async () => {
      mock.onGet('/admin/dashboard/trending-topics').reply(200, { data: [{ topic: '心湖', count: 99 }] })
      const res = await api.getTrendingTopics()
      expect(res.data.data[0].topic).toBe('心湖')
    })

    it('getMoodTrend 成功', async () => {
      mock.onGet(/\/admin\/dashboard\/mood-trend/).reply(200, { data: [{ date: '2025-01-01', score: 0.7 }] })
      const res = await api.getMoodTrend('7d')
      expect(res.data.data[0].score).toBe(0.7)
    })

    it('部分接口失败不影响其他', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, statsData)
      mock.onGet('/admin/dashboard/mood-distribution').reply(400)
      const results = await Promise.allSettled([api.getDashboardStats(), api.getMoodDistribution()])
      expect(results[0].status).toBe('fulfilled')
      expect(results[1].status).toBe('rejected')
    })

    it('所有统计接口应能并发调用', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, statsData)
      mock.onGet(/\/admin\/dashboard\/user-growth/).reply(200, growthData)
      mock.onGet('/admin/dashboard/mood-distribution').reply(200, moodData)
      mock.onGet('/admin/edge-ai/privacy-budget').reply(200, privacyData)
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(200, pulseData)

      const results = await Promise.all([
        api.getDashboardStats(),
        api.getUserGrowthStats('7d'),
        api.getMoodDistribution(),
        api.getPrivacyBudget(),
        api.getEmotionPulse(),
      ])

      expect(results).toHaveLength(5)
      results.forEach((r) => expect(r.status).toBe(200))
    })
  })
})
