/**
 * Dashboard 数据拉取函数集合，统一处理接口兼容与异常状态。
 *
 * 每个 loader 负责一个 API 端点，内部做了两层兼容：
 * 1. 响应结构兼容 -- 同时支持 { data: { data: ... } } 和 { data: ... } 两种后端格式
 * 2. 字段名兼容 -- 同时支持 snake_case 和 camelCase（后端迭代过程中两种都出现过）
 *
 * 异常策略：单个 loader 失败不影响其他数据加载，但必须留下显式失败状态，
 * 页面继续展示最近一次成功数据，同时提示哪些模块已经过期。
 */
import { ElMessage } from 'element-plus'
import type { Ref } from 'vue'
import api, { isRequestCanceled } from '@/api'
import type {
  GrowthDataItem,
  MoodDistributionItem,
  MoodTrendItem,
  ActiveTimeItem,
  TrendingTopic,
  TrendingContentItem,
  EmotionTrendItem,
  PrivacyStats,
  ResonanceStats,
} from '@/types'
import { moodNames, moodGradients } from './useChartOptions'
import { createSoftBaseline } from '@/utils/chartSignals'
import { normalizeCollectionResponse, normalizePayloadRecord } from '@/utils/collectionPayload'

const POSITIVE_MOODS = new Set(['开心', 'positive', 'joy', 'happy'])
const NEGATIVE_MOODS = new Set(['难过', '焦虑', 'negative', 'sad', 'anxious'])

/** Dashboard 核心统计数据（前端 camelCase 版本） */
interface DashboardStats {
  totalUsers: number
  todayStones: number
  onlineCount: number
  pendingReports: number
}

/** ECharts 图表最小结构，仅保留 loader 会访问的字段，避免引入完整 ECharts 类型 */
interface ChartOptionWithAxisAndSeries {
  xAxis: { data: string[] }
  series: Array<{
    data: (number | string)[]
    markPoint?: { data: Array<Record<string, unknown>> }
    markLine?: { data: Array<Record<string, unknown>> }
  }>
}

/** 仅含 series 的图表（如活跃时段柱状图，x 轴固定不变） */
interface SeriesOnlyChartOption {
  series: Array<{ data: (number | string)[] }>
}

/** 饼图 option 最小结构 */
interface PieChartOption {
  series: Array<{ data: Array<{ name: string; value: number; itemStyle: unknown }> }>
}

/** 仪表盘 option 最小结构 */
interface GaugeChartOption {
  series: Array<{ data: Array<{ value: number; name: string }> }>
}

/**
 * useDashboardLoaders 的依赖注入接口。
 * 所有字段均由 useDashboardData 创建后传入，loader 直接修改这些响应式引用。
 */
interface LoaderDeps {
  stats: DashboardStats
  chartRange: Ref<number>
  moodTrendRange: Ref<number>
  trendingTopics: Ref<TrendingTopic[]>
  aiTrendingContent: Ref<TrendingContentItem[]>
  loaderIssues: Record<string, string>
  privacyStats: PrivacyStats
  privacyLoading: Ref<boolean>
  resonanceStats: ResonanceStats
  resonanceLoading: Ref<boolean>
  userGrowthOption: Ref<ChartOptionWithAxisAndSeries>
  moodDistributionOption: Ref<PieChartOption>
  moodTrendOption: Ref<ChartOptionWithAxisAndSeries>
  activeTimeOption: Ref<SeriesOnlyChartOption>
  emotionPulseOption: Ref<GaugeChartOption>
  emotionTrendsOption: Ref<ChartOptionWithAxisAndSeries>
}

export function useDashboardLoaders({
  stats,
  chartRange,
  moodTrendRange,
  trendingTopics,
  aiTrendingContent,
  loaderIssues,
  privacyStats,
  privacyLoading,
  resonanceStats,
  resonanceLoading,
  userGrowthOption,
  moodDistributionOption,
  moodTrendOption,
  activeTimeOption,
  emotionPulseOption,
  emotionTrendsOption,
}: LoaderDeps) {
  const normalizeDashboardCollection = <T>(payload: unknown, semanticKeys: readonly string[]) =>
    normalizeCollectionResponse<T>(payload, semanticKeys).items
  const normalizeDashboardRecord = (payload: unknown) => normalizePayloadRecord(payload)
  const moodTrendRequests = new Map<number, Promise<MoodTrendItem[]>>()
  let emotionPulseRequest: Promise<Record<string, unknown>> | null = null
  let emotionPulseSnapshot: { at: number; data: Record<string, unknown> } | null = null

  const clearLoaderIssue = (key: string) => {
    delete loaderIssues[key]
  }

  const resolveErrorMessage = (error: unknown, fallback: string) => {
    if (error instanceof Error && error.message.trim()) {
      return error.message.trim()
    }
    return fallback
  }

  const markLoaderIssue = (key: string, message: string, notify = false) => {
    loaderIssues[key] = message
    if (notify) {
      ElMessage.error(message)
    }
  }

  const fetchMoodTrendList = async (days: number) => {
    const cached = moodTrendRequests.get(days)
    if (cached) return cached

    const request = Promise.resolve(api.getMoodTrend?.(String(days)) ?? { data: null })
      .then((res) => normalizeDashboardCollection<MoodTrendItem>(res.data, ['trends']))
      .finally(() => {
        moodTrendRequests.delete(days)
      })

    moodTrendRequests.set(days, request)
    return request
  }

  const fetchEmotionPulseRecord = async () => {
    const now = Date.now()
    if (emotionPulseSnapshot && now - emotionPulseSnapshot.at < 1000) {
      return emotionPulseSnapshot.data
    }
    if (emotionPulseRequest) {
      return emotionPulseRequest
    }

    emotionPulseRequest = api
      .getEmotionPulse()
      .then((res) => {
        const normalized = normalizeDashboardRecord(res.data)
        emotionPulseSnapshot = { at: Date.now(), data: normalized }
        return normalized
      })
      .finally(() => {
        emotionPulseRequest = null
      })

    return emotionPulseRequest
  }

  /** 读取核心统计信息。 */
  const loadStats = async () => {
    try {
      const [dashResult, realtimeResult] = await Promise.allSettled([
        api.getDashboardStats(),
        api.getRealtimeStats(),
      ])

      const dashboardData =
        dashResult.status === 'fulfilled' ? normalizeDashboardRecord(dashResult.value.data) : null
      const realtimeData =
        realtimeResult.status === 'fulfilled'
          ? normalizeDashboardRecord(realtimeResult.value.data)
          : null

      if (!dashboardData && !realtimeData) {
        throw new Error('统计接口全部请求失败')
      }

      if (realtimeData || dashboardData) {
        stats.totalUsers =
          realtimeData?.total_users ?? dashboardData?.total_users ?? stats.totalUsers
        stats.todayStones =
          dashboardData?.today_stones ?? realtimeData?.today_stones ?? stats.todayStones
        stats.onlineCount = realtimeData?.online_users ?? stats.onlineCount
        stats.pendingReports =
          dashboardData?.pending_reports ?? realtimeData?.pending_reports ?? stats.pendingReports
      }

      if (dashResult.status === 'rejected' || realtimeResult.status === 'rejected') {
        markLoaderIssue('stats', '核心统计部分加载失败，当前展示的是最近一次成功结果', true)
        return
      }
      clearLoaderIssue('stats')
    } catch (e: unknown) {
      if (isRequestCanceled(e)) return
      markLoaderIssue('stats', `核心统计加载失败: ${resolveErrorMessage(e, '请稍后重试')}`, true)
    }
  }

  /** 读取用户增长曲线。 */
  const loadGrowthData = async () => {
    try {
      const res = await api.getUserGrowthStats(String(chartRange.value))
      const list = normalizeDashboardCollection<GrowthDataItem>(res.data, ['stats'])
      if (list.length) {
        const counts = list.map((item: GrowthDataItem) => item.count)
        const dates = list.map((item: GrowthDataItem) => item.date)
        const peakValue = Math.max(...counts)
        const peakIndex = counts.indexOf(peakValue)
        const peakDate = dates[peakIndex]
        userGrowthOption.value.xAxis.data = dates
        userGrowthOption.value.series[0].data = counts
        userGrowthOption.value.series[0].markPoint = {
          data: peakDate ? [{ coord: [peakDate, peakValue], value: peakValue }] : [],
        }
        userGrowthOption.value.series[0].markLine = {
          data: peakDate ? [{ xAxis: peakDate }] : [],
        }
        if (userGrowthOption.value.series[1]) {
          userGrowthOption.value.series[1].data = createSoftBaseline(counts)
        }
      }
      clearLoaderIssue('growth')
    } catch (e: unknown) {
      if (isRequestCanceled(e)) return
      markLoaderIssue('growth', `增长曲线加载失败: ${resolveErrorMessage(e, '请稍后重试')}`, true)
    }
  }

  /** 读取情绪分布。 */
  const loadMoodDistribution = async () => {
    try {
      const res = await api.getMoodDistribution()
      const list = normalizeDashboardCollection<MoodDistributionItem>(res.data, ['moods'])
      if (list.length) {
        moodDistributionOption.value.series[0].data = list.map(
          (item: MoodDistributionItem, i: number) => ({
            name: item.mood_type || item.mood,
            value: item.count,
            itemStyle: {
              color: {
                type: 'linear',
                x: 0,
                y: 0,
                x2: 1,
                y2: 1,
                colorStops: [
                  { offset: 0, color: moodGradients[i % moodGradients.length].start },
                  { offset: 1, color: moodGradients[i % moodGradients.length].end },
                ],
              },
            },
          }),
        )
      }
      clearLoaderIssue('moodDistribution')
    } catch (e: unknown) {
      if (isRequestCanceled(e)) return
      markLoaderIssue(
        'moodDistribution',
        `情绪分布加载失败: ${resolveErrorMessage(e, '请稍后重试')}`,
        true,
      )
    }
  }

  /** 读取情绪趋势。 */
  const loadMoodTrend = async () => {
    try {
      const trendList = await fetchMoodTrendList(moodTrendRange.value)
      if (trendList.length) {
        const dates = [...new Set(trendList.map((item) => item.date))].sort()
        moodTrendOption.value.xAxis.data = dates
        moodNames.forEach((name, i) => {
          moodTrendOption.value.series[i].data = dates.map((date) => {
            const found = trendList.find(
              (item) => item.date === date && (item.mood_type === name || item.mood === name),
            )
            return found?.count ?? 0
          })
        })
      }
      clearLoaderIssue('moodTrend')
    } catch (e: unknown) {
      if (isRequestCanceled(e)) return
      markLoaderIssue('moodTrend', `情绪趋势加载失败: ${resolveErrorMessage(e, '请稍后重试')}`)
    }
  }

  /** 读取热门话题。 */
  const loadTrendingTopics = async () => {
    try {
      const res = (await api.getTrendingTopics?.()) || { data: null }
      const sourceList = normalizeDashboardCollection<
        TrendingTopic & { topic?: string; keyword?: string }
      >(res.data, ['topics'])
      trendingTopics.value = sourceList
        .slice(0, 10)
        .map((item: TrendingTopic & { topic?: string; keyword?: string }) => ({
          ...item,
          keyword: item.keyword || item.topic || '',
        }))
      clearLoaderIssue('trendingTopics')
    } catch (e: unknown) {
      if (isRequestCanceled(e)) return
      markLoaderIssue('trendingTopics', `热门话题加载失败: ${resolveErrorMessage(e, '请稍后重试')}`)
    }
  }

  /** 读取活跃时段。 */
  const loadActiveTimeStats = async () => {
    try {
      const res = (await api.getActiveTimeStats?.()) || { data: null }
      const list = normalizeDashboardCollection<ActiveTimeItem>(res.data, ['hours'])
      if (list.length) {
        activeTimeOption.value.series[0].data = list.map((item: ActiveTimeItem) => item.count)
      }
      clearLoaderIssue('activeTime')
    } catch (e: unknown) {
      if (isRequestCanceled(e)) return
      markLoaderIssue(
        'activeTime',
        `活跃时段加载失败: ${resolveErrorMessage(e, '请稍后重试')}`,
        true,
      )
    }
  }

  /** 读取隐私预算状态。 */
  const loadPrivacyStats = async () => {
    privacyLoading.value = true
    try {
      const res = await api.getPrivacyBudget()
      const d = normalizeDashboardRecord(res.data)
      privacyStats.queryCount = d.query_count ?? 0
      privacyStats.epsilonUsed = d.epsilon_used ?? d.epsilonUsed ?? d.consumed ?? 0
      privacyStats.epsilonTotal = d.epsilon_total ?? d.epsilonTotal ?? d.total_budget ?? 1.0
      privacyStats.protectedUsers = d.protected_users ?? d.protectedUsers ?? 0
      clearLoaderIssue('privacyStats')
    } catch (e: unknown) {
      if (isRequestCanceled(e)) return
      markLoaderIssue('privacyStats', `隐私预算加载失败: ${resolveErrorMessage(e, '请稍后重试')}`)
    } finally {
      privacyLoading.value = false
    }
  }

  /** 读取共鸣统计。 */
  const loadResonanceStats = async () => {
    resonanceLoading.value = true
    try {
      const d = await fetchEmotionPulseRecord()
      resonanceStats.todayMatches = d.today_matches ?? d.sample_count ?? 0
      resonanceStats.avgScore = d.avg_score ?? d.avgScore ?? 0
      resonanceStats.topMood = d.top_mood ?? d.dominant_mood ?? ''
      resonanceStats.successRate = d.top_mood_share_percent ?? d.success_rate ?? 0
      clearLoaderIssue('resonanceStats')
    } catch (e: unknown) {
      if (isRequestCanceled(e)) return
      markLoaderIssue('resonanceStats', `共鸣统计加载失败: ${resolveErrorMessage(e, '请稍后重试')}`)
    } finally {
      resonanceLoading.value = false
    }
  }

  /** 读取情绪温度。 */
  const loadEmotionPulse = async () => {
    try {
      const d = await fetchEmotionPulseRecord()
      const temp =
        d.temperature ?? (d.normalized_score != null ? Number(d.normalized_score) * 100 : 50)
      emotionPulseOption.value.series[0].data = [{ value: temp, name: '情绪温度' }]
      clearLoaderIssue('emotionPulse')
    } catch (e: unknown) {
      if (isRequestCanceled(e)) return
      markLoaderIssue('emotionPulse', `情绪温度加载失败: ${resolveErrorMessage(e, '请稍后重试')}`)
    }
  }

  /** 读取情绪趋势折线。 */
  const loadEmotionTrends = async () => {
    try {
      const list = await fetchMoodTrendList(moodTrendRange.value)
      if (list.length) {
        const dates = [
          ...new Set(list.map((item: EmotionTrendItem) => item.date || item.day)),
        ].sort()
        const bucket = new Map<string, { positive: number; neutral: number; negative: number }>()
        dates.forEach((date) => bucket.set(date, { positive: 0, neutral: 0, negative: 0 }))

        list.forEach(
          (item: EmotionTrendItem & { mood_type?: string; mood?: string; count?: number }) => {
            const date = item.date || item.day
            if (!date) return
            const mood = item.mood_type || item.mood || ''
            const target = bucket.get(date)
            if (!target) return
            const count = Number(item.count ?? 0)
            if (POSITIVE_MOODS.has(mood)) {
              target.positive += count
            } else if (NEGATIVE_MOODS.has(mood)) {
              target.negative += count
            } else {
              target.neutral += count
            }
          },
        )

        emotionTrendsOption.value.xAxis.data = dates
        emotionTrendsOption.value.series[0].data = dates.map(
          (date) => bucket.get(date)?.positive ?? 0,
        )
        emotionTrendsOption.value.series[1].data = dates.map(
          (date) => bucket.get(date)?.neutral ?? 0,
        )
        emotionTrendsOption.value.series[2].data = dates.map(
          (date) => bucket.get(date)?.negative ?? 0,
        )
      }
      clearLoaderIssue('emotionTrends')
    } catch (e: unknown) {
      if (isRequestCanceled(e)) return
      markLoaderIssue('emotionTrends', `情绪折线加载失败: ${resolveErrorMessage(e, '请稍后重试')}`)
    }
  }

  /** 读取热门内容清单。 */
  const loadAITrendingContent = async () => {
    try {
      type TrendingStonePayload = {
        stone_id?: string
        content?: string
        mood_type?: string
        ripple_count?: number
        boat_count?: number
      }

      let list: TrendingStonePayload[] = []
      if (typeof api.getTrendingContent !== 'function') {
        throw new Error('缺少热门内容接口')
      }

      const res = await api.getTrendingContent({ limit: 6 })
      list = normalizeDashboardCollection<TrendingStonePayload>(res?.data, ['trending_stones'])

      const maxScore = Math.max(
        1,
        ...list.map((item: { ripple_count?: number }) => Number(item.ripple_count ?? 0)),
      )

      aiTrendingContent.value = list.slice(0, 6).map((item: TrendingStonePayload) => {
        const score = Number(item.ripple_count ?? 0)
        return {
          id: item.stone_id,
          content: item.content,
          mood: item.mood_type,
          score: maxScore > 0 ? score / maxScore : 0,
        }
      })
      clearLoaderIssue('aiTrendingContent')
    } catch (e: unknown) {
      if (isRequestCanceled(e)) return
      markLoaderIssue(
        'aiTrendingContent',
        `高级热门内容加载失败: ${resolveErrorMessage(e, '请稍后重试')}`,
      )
    }
  }

  return {
    loadStats,
    loadGrowthData,
    loadMoodDistribution,
    loadMoodTrend,
    loadTrendingTopics,
    loadActiveTimeStats,
    loadPrivacyStats,
    loadResonanceStats,
    loadEmotionPulse,
    loadEmotionTrends,
    loadAITrendingContent,
  }
}
