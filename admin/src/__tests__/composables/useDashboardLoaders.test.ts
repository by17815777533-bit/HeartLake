/**
 * useDashboardLoaders composable 测试
 */
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { ref, reactive } from 'vue'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), success: vi.fn(), warning: vi.fn() },
}))

const mockApi = vi.hoisted(() => ({
  getDashboardStats: vi.fn(),
  getRealtimeStats: vi.fn(),
  getUserGrowthStats: vi.fn(),
  getMoodDistribution: vi.fn(),
  getMoodTrend: vi.fn(),
  getTrendingTopics: vi.fn(),
  getActiveTimeStats: vi.fn(),
  getPrivacyBudget: vi.fn(),
  getEmotionPulse: vi.fn(),
  getEmotionTrends: vi.fn(),
  getTrendingContent: vi.fn(),
  getStones: vi.fn(),
}))

vi.mock('@/api', () => ({ default: mockApi }))

import { useDashboardLoaders } from '@/composables/useDashboardLoaders'

function createDeps() {
  return {
    stats: reactive({ totalUsers: 0, todayStones: 0, onlineCount: 0, pendingReports: 0 }),
    chartRange: ref(7),
    moodTrendRange: ref(7),
    trendingTopics: ref([]),
    aiTrendingContent: ref([]),
    privacyStats: reactive({ queryCount: 0, epsilonUsed: 0, epsilonTotal: 1.0, protectedUsers: 0 }),
    privacyLoading: ref(false),
    resonanceStats: reactive({ todayMatches: 0, avgScore: 0, topMood: '', successRate: 0 }),
    resonanceLoading: ref(false),
    userGrowthOption: ref({ xAxis: { data: [] }, series: [{ data: [] }] }),
    moodDistributionOption: ref({ series: [{ data: [] }] }),
    moodTrendOption: ref({ xAxis: { data: [] }, series: [{ data: [] }, { data: [] }, { data: [] }, { data: [] }, { data: [] }] }),
    activeTimeOption: ref({ series: [{ data: [] }] }),
    emotionPulseOption: ref({ series: [{ data: [{ value: 50, name: '情绪温度' }] }] }),
    emotionTrendsOption: ref({ xAxis: { data: [] }, series: [{ data: [] }, { data: [] }, { data: [] }] }),
  }
}
describe('useDashboardLoaders', () => {
  beforeEach(() => {
    vi.clearAllMocks()
  })

  describe('loadStats', () => {
    it('成功加载统计数据', async () => {
      mockApi.getDashboardStats.mockResolvedValue({ data: { data: { total_users: 100, today_stones: 50, pending_reports: 3 } } })
      mockApi.getRealtimeStats.mockResolvedValue({ data: { data: { total_users: 110, online_users: 20 } } })
      const deps = createDeps()
      const { loadStats } = useDashboardLoaders(deps)
      await loadStats()
      expect(deps.stats.totalUsers).toBe(110)
      expect(deps.stats.onlineCount).toBe(20)
    })

    it('getDashboardStats 失败时使用 catch 兜底', async () => {
      mockApi.getDashboardStats.mockRejectedValue(new Error('fail'))
      mockApi.getRealtimeStats.mockResolvedValue({ data: { data: { online_users: 5 } } })
      const deps = createDeps()
      const { loadStats } = useDashboardLoaders(deps)
      await loadStats()
      expect(deps.stats.onlineCount).toBe(5)
    })

    it('getRealtimeStats 失败时使用 catch 兜底', async () => {
      mockApi.getDashboardStats.mockResolvedValue({ data: { data: { total_users: 100, today_stones: 50 } } })
      mockApi.getRealtimeStats.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadStats } = useDashboardLoaders(deps)
      await loadStats()
      expect(deps.stats.totalUsers).toBe(100)
      expect(deps.stats.todayStones).toBe(50)
    })

    it('两个接口都失败时 stats 保持默认', async () => {
      mockApi.getDashboardStats.mockRejectedValue(new Error('fail'))
      mockApi.getRealtimeStats.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadStats } = useDashboardLoaders(deps)
      await loadStats()
      expect(deps.stats.totalUsers).toBe(0)
      expect(deps.stats.onlineCount).toBe(0)
    })

    it('空数据时使用默认值 0', async () => {
      mockApi.getDashboardStats.mockResolvedValue({ data: { data: {} } })
      mockApi.getRealtimeStats.mockResolvedValue({ data: { data: {} } })
      const deps = createDeps()
      const { loadStats } = useDashboardLoaders(deps)
      await loadStats()
      expect(deps.stats.totalUsers).toBe(0)
      expect(deps.stats.todayStones).toBe(0)
    })
  })

  describe('loadGrowthData', () => {
    it('成功加载增长数据', async () => {
      const list = [{ date: '2025-01-01', count: 10 }, { date: '2025-01-02', count: 20 }]
      mockApi.getUserGrowthStats.mockResolvedValue({ data: { data: list } })
      const deps = createDeps()
      const { loadGrowthData } = useDashboardLoaders(deps)
      await loadGrowthData()
      expect(deps.userGrowthOption.value.xAxis.data).toEqual(['2025-01-01', '2025-01-02'])
      expect(deps.userGrowthOption.value.series[0].data).toEqual([10, 20])
    })

    it('空数据不更新图表', async () => {
      mockApi.getUserGrowthStats.mockResolvedValue({ data: { data: [] } })
      const deps = createDeps()
      const { loadGrowthData } = useDashboardLoaders(deps)
      await loadGrowthData()
      expect(deps.userGrowthOption.value.xAxis.data).toEqual([])
    })

    it('接口失败时不崩溃', async () => {
      mockApi.getUserGrowthStats.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadGrowthData } = useDashboardLoaders(deps)
      await expect(loadGrowthData()).resolves.toBeUndefined()
    })

    it('使用 chartRange 作为参数', async () => {
      mockApi.getUserGrowthStats.mockResolvedValue({ data: { data: [] } })
      const deps = createDeps()
      deps.chartRange.value = 30
      const { loadGrowthData } = useDashboardLoaders(deps)
      await loadGrowthData()
      expect(mockApi.getUserGrowthStats).toHaveBeenCalledWith('30')
    })
  })
  describe('loadMoodDistribution', () => {
    it('成功加载心情分布', async () => {
      const data = [{ mood_type: '开心', count: 100 }, { mood_type: '平静', count: 80 }]
      mockApi.getMoodDistribution.mockResolvedValue({ data: { data } })
      const deps = createDeps()
      const { loadMoodDistribution } = useDashboardLoaders(deps)
      await loadMoodDistribution()
      expect(deps.moodDistributionOption.value.series[0].data.length).toBeGreaterThan(0)
    })

    it('空数据不更新', async () => {
      mockApi.getMoodDistribution.mockResolvedValue({ data: { data: [] } })
      const deps = createDeps()
      const { loadMoodDistribution } = useDashboardLoaders(deps)
      await loadMoodDistribution()
      expect(deps.moodDistributionOption.value.series[0].data).toEqual([])
    })

    it('接口失败时不崩溃', async () => {
      mockApi.getMoodDistribution.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadMoodDistribution } = useDashboardLoaders(deps)
      await expect(loadMoodDistribution()).resolves.toBeUndefined()
    })
  })

  describe('loadMoodTrend', () => {
    it('成功加载心情趋势', async () => {
      const data = [{ date: '01-01', moods: [10, 20, 30, 40, 50] }]
      mockApi.getMoodTrend.mockResolvedValue({ data: { data } })
      const deps = createDeps()
      const { loadMoodTrend } = useDashboardLoaders(deps)
      await loadMoodTrend()
      // 不崩溃即可
    })

    it('接口失败时不崩溃', async () => {
      mockApi.getMoodTrend.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadMoodTrend } = useDashboardLoaders(deps)
      await expect(loadMoodTrend()).resolves.toBeUndefined()
    })
  })

  describe('loadTrendingTopics', () => {
    it('成功加载热门话题', async () => {
      const data = [{ keyword: '心情', count: 50 }, { keyword: '天气', count: 30 }]
      mockApi.getTrendingTopics.mockResolvedValue({ data: { data } })
      const deps = createDeps()
      const { loadTrendingTopics } = useDashboardLoaders(deps)
      await loadTrendingTopics()
      expect(deps.trendingTopics.value).toHaveLength(2)
    })

    it('空数据设为空数组', async () => {
      mockApi.getTrendingTopics.mockResolvedValue({ data: { data: [] } })
      const deps = createDeps()
      const { loadTrendingTopics } = useDashboardLoaders(deps)
      await loadTrendingTopics()
      expect(deps.trendingTopics.value).toEqual([])
    })

    it('接口失败时不崩溃', async () => {
      mockApi.getTrendingTopics.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadTrendingTopics } = useDashboardLoaders(deps)
      await expect(loadTrendingTopics()).resolves.toBeUndefined()
    })
  })

  describe('loadActiveTimeStats', () => {
    it('成功加载活跃时间', async () => {
      const data = Array.from({ length: 24 }, (_, i) => ({ hour: i, count: i * 10 }))
      mockApi.getActiveTimeStats.mockResolvedValue({ data: { data } })
      const deps = createDeps()
      const { loadActiveTimeStats } = useDashboardLoaders(deps)
      await loadActiveTimeStats()
      expect(deps.activeTimeOption.value.series[0].data.length).toBeGreaterThan(0)
    })

    it('接口失败时不崩溃', async () => {
      mockApi.getActiveTimeStats.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadActiveTimeStats } = useDashboardLoaders(deps)
      await expect(loadActiveTimeStats()).resolves.toBeUndefined()
    })
  })
  describe('loadPrivacyStats', () => {
    it('成功加载隐私统计', async () => {
      mockApi.getPrivacyBudget.mockResolvedValue({ data: { data: { query_count: 100, epsilon_used: 3.5, epsilon_total: 10, protected_users: 50 } } })
      const deps = createDeps()
      const { loadPrivacyStats } = useDashboardLoaders(deps)
      await loadPrivacyStats()
      expect(deps.privacyStats.queryCount).toBe(100)
    })

    it('接口失败时不崩溃', async () => {
      mockApi.getPrivacyBudget.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadPrivacyStats } = useDashboardLoaders(deps)
      await expect(loadPrivacyStats()).resolves.toBeUndefined()
    })

    it('加载时设置 privacyLoading', async () => {
      mockApi.getPrivacyBudget.mockResolvedValue({ data: { data: {} } })
      const deps = createDeps()
      const { loadPrivacyStats } = useDashboardLoaders(deps)
      await loadPrivacyStats()
      expect(deps.privacyLoading.value).toBe(false)
    })
  })

  describe('loadResonanceStats', () => {
    it('成功加载共鸣统计', async () => {
      mockApi.getEmotionPulse.mockResolvedValue({ data: { data: { today_matches: 20, avg_score: 0.8, top_mood: '开心', success_rate: 0.9 } } })
      const deps = createDeps()
      const { loadResonanceStats } = useDashboardLoaders(deps)
      await loadResonanceStats()
      expect(deps.resonanceStats.todayMatches).toBe(20)
    })

    it('接口失败时不崩溃', async () => {
      mockApi.getEmotionPulse.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadResonanceStats } = useDashboardLoaders(deps)
      await expect(loadResonanceStats()).resolves.toBeUndefined()
    })

    it('加载时设置 resonanceLoading', async () => {
      mockApi.getEmotionPulse.mockResolvedValue({ data: { data: {} } })
      const deps = createDeps()
      const { loadResonanceStats } = useDashboardLoaders(deps)
      await loadResonanceStats()
      expect(deps.resonanceLoading.value).toBe(false)
    })
  })

  describe('loadEmotionPulse', () => {
    it('成功加载情绪温度', async () => {
      mockApi.getEmotionPulse.mockResolvedValue({ data: { data: { temperature: 75 } } })
      const deps = createDeps()
      const { loadEmotionPulse } = useDashboardLoaders(deps)
      await loadEmotionPulse()
      expect(deps.emotionPulseOption.value.series[0].data[0].value).toBe(75)
    })

    it('接口失败时不崩溃', async () => {
      mockApi.getEmotionPulse.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadEmotionPulse } = useDashboardLoaders(deps)
      await expect(loadEmotionPulse()).resolves.toBeUndefined()
    })
  })

  describe('loadEmotionTrends', () => {
    it('成功加载情绪趋势', async () => {
      const list = [
        { date: '01-01', mood_type: '开心', count: 10 },
        { date: '01-01', mood_type: '平静', count: 20 },
        { date: '01-01', mood_type: '难过', count: 5 },
      ]
      mockApi.getMoodTrend.mockResolvedValue({ data: { data: list } })
      const deps = createDeps()
      const { loadEmotionTrends } = useDashboardLoaders(deps)
      await loadEmotionTrends()
      expect(deps.emotionTrendsOption.value.xAxis.data).toEqual(['01-01'])
      expect(deps.emotionTrendsOption.value.series[0].data).toEqual([10])
      expect(deps.emotionTrendsOption.value.series[1].data).toEqual([20])
      expect(deps.emotionTrendsOption.value.series[2].data).toEqual([5])
    })

    it('按情绪名称归并到积极/中性/消极', async () => {
      const list = [
        { date: '01-02', mood_type: '开心', count: 15 },
        { date: '01-02', mood_type: '其他', count: 25 },
        { date: '01-02', mood_type: '焦虑', count: 8 },
      ]
      mockApi.getMoodTrend.mockResolvedValue({ data: { data: list } })
      const deps = createDeps()
      const { loadEmotionTrends } = useDashboardLoaders(deps)
      await loadEmotionTrends()
      expect(deps.emotionTrendsOption.value.xAxis.data).toEqual(['01-02'])
      expect(deps.emotionTrendsOption.value.series[0].data).toEqual([15])
      expect(deps.emotionTrendsOption.value.series[1].data).toEqual([25])
      expect(deps.emotionTrendsOption.value.series[2].data).toEqual([8])
    })

    it('接口失败时不崩溃', async () => {
      mockApi.getMoodTrend.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadEmotionTrends } = useDashboardLoaders(deps)
      await expect(loadEmotionTrends()).resolves.toBeUndefined()
    })
  })

  describe('loadAITrendingContent', () => {
    it('成功加载热门内容', async () => {
      const data = Array.from({ length: 15 }, (_, i) => ({
        stone_id: `s${i}`,
        content: `内容${i}`,
        mood_type: i % 2 === 0 ? '开心' : '平静',
        ripple_count: i,
        boat_count: i,
      }))
      mockApi.getStones.mockResolvedValue({ data: { data: { list: data } } })
      const deps = createDeps()
      const { loadAITrendingContent } = useDashboardLoaders(deps)
      await loadAITrendingContent()
      expect(deps.aiTrendingContent.value).toHaveLength(10)
    })

    it('少于 10 条时全部显示', async () => {
      const data = [{ stone_id: 's1', content: '内容1', mood_type: '开心', ripple_count: 2, boat_count: 1 }]
      mockApi.getStones.mockResolvedValue({ data: { data: { list: data } } })
      const deps = createDeps()
      const { loadAITrendingContent } = useDashboardLoaders(deps)
      await loadAITrendingContent()
      expect(deps.aiTrendingContent.value).toHaveLength(1)
    })

    it('接口失败时不崩溃', async () => {
      mockApi.getStones.mockRejectedValue(new Error('fail'))
      const deps = createDeps()
      const { loadAITrendingContent } = useDashboardLoaders(deps)
      await expect(loadAITrendingContent()).resolves.toBeUndefined()
    })
  })

  describe('返回值完整性', () => {
    it('返回所有 11 个 loader 函数', () => {
      const deps = createDeps()
      const loaders = useDashboardLoaders(deps)
      expect(typeof loaders.loadStats).toBe('function')
      expect(typeof loaders.loadGrowthData).toBe('function')
      expect(typeof loaders.loadMoodDistribution).toBe('function')
      expect(typeof loaders.loadMoodTrend).toBe('function')
      expect(typeof loaders.loadTrendingTopics).toBe('function')
      expect(typeof loaders.loadActiveTimeStats).toBe('function')
      expect(typeof loaders.loadPrivacyStats).toBe('function')
      expect(typeof loaders.loadResonanceStats).toBe('function')
      expect(typeof loaders.loadEmotionPulse).toBe('function')
      expect(typeof loaders.loadEmotionTrends).toBe('function')
      expect(typeof loaders.loadAITrendingContent).toBe('function')
    })
  })
})
