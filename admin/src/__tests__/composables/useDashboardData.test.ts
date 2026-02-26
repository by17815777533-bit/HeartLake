/**
 * @file useDashboardData.test.ts
 * @brief useDashboardData composable 测试
 */
import { describe, it, expect, vi, beforeEach } from 'vitest'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), success: vi.fn(), warning: vi.fn() },
}))

vi.mock('@/api', () => ({
  default: {
    getDashboardStats: vi.fn().mockResolvedValue({ data: { data: {} } }),
    getRealtimeStats: vi.fn().mockResolvedValue({ data: { data: {} } }),
    getUserGrowthStats: vi.fn().mockResolvedValue({ data: { data: [] } }),
    getMoodDistribution: vi.fn().mockResolvedValue({ data: { data: [] } }),
    getMoodTrend: vi.fn().mockResolvedValue({ data: { data: [] } }),
    getTrendingTopics: vi.fn().mockResolvedValue({ data: { data: [] } }),
    getActiveTimeStats: vi.fn().mockResolvedValue({ data: { data: [] } }),
    getPrivacyStats: vi.fn().mockResolvedValue({ data: { data: {} } }),
    getResonanceStats: vi.fn().mockResolvedValue({ data: { data: {} } }),
    getEmotionPulse: vi.fn().mockResolvedValue({ data: { data: {} } }),
    getEmotionTrends: vi.fn().mockResolvedValue({ data: { data: [] } }),
    getTrendingContent: vi.fn().mockResolvedValue({ data: { data: [] } }),
    getRecommendationStats: vi.fn().mockResolvedValue({ data: { data: {} } }),
  },
}))

import { useDashboardData } from '@/composables/useDashboardData'

describe('useDashboardData', () => {
  beforeEach(() => {
    vi.clearAllMocks()
  })

  describe('初始状态', () => {
    it('loading 初始为 false', () => {
      const { loading } = useDashboardData()
      expect(loading.value).toBe(false)
    })

    it('privacyLoading 初始为 false', () => {
      const { privacyLoading } = useDashboardData()
      expect(privacyLoading.value).toBe(false)
    })
    it('resonanceLoading 初始为 false', () => {
      const { resonanceLoading } = useDashboardData()
      expect(resonanceLoading.value).toBe(false)
    })

    it('chartRange 初始为 7', () => {
      const { chartRange } = useDashboardData()
      expect(chartRange.value).toBe(7)
    })

    it('moodTrendRange 初始为 7', () => {
      const { moodTrendRange } = useDashboardData()
      expect(moodTrendRange.value).toBe(7)
    })

    it('trendingTopics 初始为空数组', () => {
      const { trendingTopics } = useDashboardData()
      expect(trendingTopics.value).toEqual([])
    })

    it('aiTrendingContent 初始为空数组', () => {
      const { aiTrendingContent } = useDashboardData()
      expect(aiTrendingContent.value).toEqual([])
    })

    it('stats 初始值全为 0', () => {
      const { stats } = useDashboardData()
      expect(stats.totalUsers).toBe(0)
      expect(stats.todayStones).toBe(0)
      expect(stats.onlineCount).toBe(0)
      expect(stats.pendingReports).toBe(0)
    })
  })

  describe('computed 属性', () => {
    it('statsCards 返回 4 张卡片', () => {
      const { statsCards } = useDashboardData()
      expect(statsCards.value).toHaveLength(4)
    })

    it('statsCards 包含正确标题', () => {
      const { statsCards } = useDashboardData()
      const titles = statsCards.value.map((c: any) => c.title)
      expect(titles).toContain('总用户数')
      expect(titles).toContain('今日投石')
      expect(titles).toContain('在线人数')
      expect(titles).toContain('待处理举报')
    })

    it('privacyBudgetPercent 初始为 0', () => {
      const { privacyBudgetPercent } = useDashboardData()
      expect(privacyBudgetPercent.value).toBe(0)
    })

    it('privacyBudgetPercent 正确计算', () => {
      const { privacyStats, privacyBudgetPercent } = useDashboardData()
      privacyStats.epsilonUsed = 5
      privacyStats.epsilonTotal = 10
      expect(privacyBudgetPercent.value).toBe(50)
    })

    it('privacyBudgetPercent 不超过 100', () => {
      const { privacyStats, privacyBudgetPercent } = useDashboardData()
      privacyStats.epsilonUsed = 20
      privacyStats.epsilonTotal = 10
      expect(privacyBudgetPercent.value).toBe(100)
    })

    it('privacyBudgetPercent epsilonTotal 为 0 时返回 0', () => {
      const { privacyStats, privacyBudgetPercent } = useDashboardData()
      privacyStats.epsilonTotal = 0
      expect(privacyBudgetPercent.value).toBe(0)
    })

    it('privacyBudgetColor 低于 50 为绿色', () => {
      const { privacyStats, privacyBudgetColor } = useDashboardData()
      privacyStats.epsilonUsed = 2
      privacyStats.epsilonTotal = 10
      expect(privacyBudgetColor.value).toBe('#2E7D32')
    })

    it('privacyBudgetColor 50-80 为橙色', () => {
      const { privacyStats, privacyBudgetColor } = useDashboardData()
      privacyStats.epsilonUsed = 6
      privacyStats.epsilonTotal = 10
      expect(privacyBudgetColor.value).toBe('#E65100')
    })

    it('privacyBudgetColor 80+ 为红色', () => {
      const { privacyStats, privacyBudgetColor } = useDashboardData()
      privacyStats.epsilonUsed = 9
      privacyStats.epsilonTotal = 10
      expect(privacyBudgetColor.value).toBe('#BA1A1A')
    })
  })

  describe('formatNumber', () => {
    it('小于 10000 使用 toLocaleString', () => {
      const { formatNumber } = useDashboardData()
      expect(formatNumber(1234)).toBe('1,234')
    })

    it('大于等于 10000 使用万为单位', () => {
      const { formatNumber } = useDashboardData()
      expect(formatNumber(10000)).toBe('1.0w')
    })

    it('格式化 50000', () => {
      const { formatNumber } = useDashboardData()
      expect(formatNumber(50000)).toBe('5.0w')
    })

    it('格式化 0', () => {
      const { formatNumber } = useDashboardData()
      expect(formatNumber(0)).toBe('0')
    })
  })

  describe('techBadges', () => {
    it('包含 4 个技术标签', () => {
      const { techBadges } = useDashboardData()
      expect(techBadges).toHaveLength(4)
    })

    it('每个标签有 icon 和 label', () => {
      const { techBadges } = useDashboardData()
      techBadges.forEach((b: any) => {
        expect(b).toHaveProperty('icon')
        expect(b).toHaveProperty('label')
      })
    })
  })

  describe('lakeWeather', () => {
    it('默认温度 50 返回多云', () => {
      const { lakeWeather } = useDashboardData()
      expect(lakeWeather.value.label).toBe('多云')
    })

    it('高温返回晴朗', () => {
      const { emotionPulseOption, lakeWeather } = useDashboardData()
      emotionPulseOption.value.series[0].data[0] = { value: 70, name: '情绪温度' }
      expect(lakeWeather.value.label).toBe('晴朗')
    })

    it('低温返回小雨', () => {
      const { emotionPulseOption, lakeWeather } = useDashboardData()
      emotionPulseOption.value.series[0].data[0] = { value: 20, name: '情绪温度' }
      expect(lakeWeather.value.label).toBe('小雨')
    })

    it('极低温返回暴风雨', () => {
      const { emotionPulseOption, lakeWeather } = useDashboardData()
      emotionPulseOption.value.series[0].data[0] = { value: 5, name: '情绪温度' }
      expect(lakeWeather.value.label).toBe('暴风雨')
    })
  })

  describe('refreshData', () => {
    it('刷新时 loading 变为 true 然后 false', async () => {
      const { refreshData, loading } = useDashboardData()
      const promise = refreshData()
      expect(loading.value).toBe(true)
      await promise
      expect(loading.value).toBe(false)
    })

    it('刷新后更新 lastUpdateTime', async () => {
      const { refreshData, lastUpdateTime } = useDashboardData()
      await new Promise(r => setTimeout(r, 10))
      await refreshData()
      // lastUpdateTime 格式为 HH:mm:ss，至少不为空
      expect(lastUpdateTime.value).toBeTruthy()
    })
  })

  describe('返回值完整性', () => {
    it('包含所有必要属性', () => {
      const result = useDashboardData()
      expect(result).toHaveProperty('loading')
      expect(result).toHaveProperty('stats')
      expect(result).toHaveProperty('statsCards')
      expect(result).toHaveProperty('privacyStats')
      expect(result).toHaveProperty('resonanceStats')
      expect(result).toHaveProperty('refreshData')
      expect(result).toHaveProperty('exportData')
      expect(result).toHaveProperty('lakeWeather')
      expect(result).toHaveProperty('weatherMoodPieOption')
      expect(result).toHaveProperty('greeting')
    })
  })
})