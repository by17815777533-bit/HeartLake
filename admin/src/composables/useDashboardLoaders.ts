/**
 * Dashboard 数据拉取函数集合，统一处理接口兼容与异常回退。
 *
 * 每个 loader 负责一个 API 端点，内部做了两层兼容：
 * 1. 响应结构兼容 -- 同时支持 { data: { data: ... } } 和 { data: ... } 两种后端格式
 * 2. 字段名兼容 -- 同时支持 snake_case 和 camelCase（后端迭代过程中两种都出现过）
 *
 * 异常策略：单个 loader 失败不影响其他数据加载，console.warn 记录后静默降级。
 * 关键接口（stats / growth / mood）额外弹 ElMessage 提示用户。
 */
import { ElMessage } from 'element-plus'
import type { Ref } from 'vue'
import api from '@/api'
import type {
  GrowthDataItem, MoodDistributionItem, MoodTrendItem,
  ActiveTimeItem, TrendingTopic, TrendingContentItem, EmotionTrendItem,
  PrivacyStats, ResonanceStats,
} from '@/types'
import { moodNames, moodGradients } from './useChartOptions'

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
  series: Array<{ data: (number | string)[] }>
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
  stats, chartRange, moodTrendRange, trendingTopics, aiTrendingContent,
  privacyStats, privacyLoading, resonanceStats, resonanceLoading,
  userGrowthOption, moodDistributionOption, moodTrendOption,
  activeTimeOption, emotionPulseOption, emotionTrendsOption,
}: LoaderDeps) {
  /** 读取核心统计信息。 */
  const loadStats = async () => {
    try {
      const [dashRes, realtimeRes] = await Promise.all([
        api.getDashboardStats().catch(() => ({ data: null })),
        api.getRealtimeStats().catch(() => ({ data: null }))
      ])
      const d = dashRes.data?.data || dashRes.data || {}
      const r = realtimeRes.data?.data || realtimeRes.data || {}
      stats.totalUsers = r.total_users ?? d.total_users ?? 0
      stats.todayStones = d.today_stones ?? r.today_stones ?? 0
      stats.onlineCount = r.online_users ?? 0
      stats.pendingReports = d.pending_reports ?? r.pending_reports ?? 0
    } catch (e: unknown) {
      console.warn('加载统计数据失败:', (e as Error).message)
      ElMessage.error('加载统计数据失败，请稍后重试')
    }
  }

  /** 读取用户增长曲线。 */
  const loadGrowthData = async () => {
    try {
      const res = await api.getUserGrowthStats(String(chartRange.value))
      const raw = res.data?.data || res.data
      const list = Array.isArray(raw) ? raw : (raw?.list || [])
      if (list.length) {
        userGrowthOption.value.xAxis.data = list.map((item: GrowthDataItem) => item.date)
        userGrowthOption.value.series[0].data = list.map((item: GrowthDataItem) => item.count)
      }
    } catch (e: unknown) {
      console.warn('加载增长数据失败:', (e as Error).message)
      ElMessage.error('加载增长数据失败，请稍后重试')
    }
  }

  /** 读取情绪分布。 */
  const loadMoodDistribution = async () => {
    try {
      const res = await api.getMoodDistribution()
      const raw = res.data?.data || res.data
      const list = Array.isArray(raw) ? raw : (raw?.list || [])
      if (list.length) {
        moodDistributionOption.value.series[0].data = list.map((item: MoodDistributionItem, i: number) => ({
          name: item.mood_type || item.mood,
          value: item.count,
          itemStyle: {
            color: {
              type: 'linear', x: 0, y: 0, x2: 1, y2: 1,
              colorStops: [
                { offset: 0, color: moodGradients[i % moodGradients.length].start },
                { offset: 1, color: moodGradients[i % moodGradients.length].end }
              ]
            }
          }
        }))
      }
    } catch (e: unknown) {
      console.warn('加载情绪分布失败:', (e as Error).message)
      ElMessage.error('加载情绪分布失败，请稍后重试')
    }
  }

  /** 读取情绪趋势。 */
  const loadMoodTrend = async () => {
    try {
      const res = await api.getMoodTrend?.(String(moodTrendRange.value)) || { data: null }
      const trendData = res.data?.data || res.data
      const trendList: MoodTrendItem[] = trendData?.list || (Array.isArray(trendData) ? trendData : [])
      if (trendList.length) {
        const dates = [...new Set(trendList.map(item => item.date))].sort()
        moodTrendOption.value.xAxis.data = dates
        moodNames.forEach((name, i) => {
          moodTrendOption.value.series[i].data = dates.map(date => {
            const found = trendList.find(item =>
              item.date === date && (item.mood_type === name || item.mood === name)
            )
            return found?.count ?? 0
          })
        })
      }
    } catch (e: unknown) {
      console.warn('加载情绪趋势失败:', (e as Error).message)
    }
  }

  /** 读取热门话题。 */
  const loadTrendingTopics = async () => {
    try {
      const res = await api.getTrendingTopics?.() || { data: null }
      const raw = res.data?.data || res.data
      trendingTopics.value = Array.isArray(raw) ? raw.slice(0, 10) : (raw?.list || []).slice(0, 10)
    } catch (e: unknown) {
      console.warn('加载热门话题失败:', (e as Error).message)
    }
  }

  /** 读取活跃时段。 */
  const loadActiveTimeStats = async () => {
    try {
      const res = await api.getActiveTimeStats?.() || { data: null }
      const timeData = res.data?.data || res.data
      if (timeData) {
        activeTimeOption.value.series[0].data = (Array.isArray(timeData)
          ? timeData : timeData.list || []).map((item: ActiveTimeItem) => item.count)
      }
    } catch (e: unknown) {
      console.warn('加载活跃时段失败:', (e as Error).message)
      ElMessage.error('加载活跃时段失败，请稍后重试')
    }
  }

  /** 读取隐私预算状态。 */
  const loadPrivacyStats = async () => {
    privacyLoading.value = true
    try {
      const res = await api.getPrivacyBudget()
      const d = res.data?.data || res.data || {}
      privacyStats.queryCount = d.query_count ?? 0
      privacyStats.epsilonUsed = d.epsilon_used ?? 0
      privacyStats.epsilonTotal = d.epsilon_total ?? 1.0
      privacyStats.protectedUsers = d.protected_users ?? 0
    } catch (e: unknown) {
      console.warn('加载隐私统计失败:', (e as Error).message)
    } finally {
      privacyLoading.value = false
    }
  }

  /** 读取共鸣统计。 */
  const loadResonanceStats = async () => {
    resonanceLoading.value = true
    try {
      const res = await api.getEmotionPulse()
      const d = res.data?.data || res.data || {}
      resonanceStats.todayMatches = d.today_matches ?? 0
      resonanceStats.avgScore = d.avg_score ?? 0
      resonanceStats.topMood = d.top_mood ?? ''
      resonanceStats.successRate = d.success_rate ?? 0
    } catch (e: unknown) {
      console.warn('加载共鸣统计失败:', (e as Error).message)
    } finally {
      resonanceLoading.value = false
    }
  }

  /** 读取情绪温度。 */
  const loadEmotionPulse = async () => {
    try {
      const res = await api.getEmotionPulse()
      const d = res.data?.data || res.data || {}
      const temp = d.temperature ?? 50
      emotionPulseOption.value.series[0].data = [{ value: temp, name: '情绪温度' }]
    } catch (e: unknown) {
      console.warn('加载情绪温度失败:', (e as Error).message)
    }
  }

  /** 读取情绪趋势折线。 */
  const loadEmotionTrends = async () => {
    try {
      const res = await api.getMoodTrend(String(moodTrendRange.value))
      const raw = res.data?.data || res.data
      const list = Array.isArray(raw) ? raw : (raw?.list || [])
      if (list.length) {
        const dates = [...new Set(list.map((item: EmotionTrendItem) => item.date || item.day))].sort()
        const bucket = new Map<string, { positive: number; neutral: number; negative: number }>()
        dates.forEach((date) => bucket.set(date, { positive: 0, neutral: 0, negative: 0 }))

        list.forEach((item: EmotionTrendItem & { mood_type?: string; mood?: string; count?: number }) => {
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
        })

        emotionTrendsOption.value.xAxis.data = dates
        emotionTrendsOption.value.series[0].data = dates.map((date) => bucket.get(date)?.positive ?? 0)
        emotionTrendsOption.value.series[1].data = dates.map((date) => bucket.get(date)?.neutral ?? 0)
        emotionTrendsOption.value.series[2].data = dates.map((date) => bucket.get(date)?.negative ?? 0)
      }
    } catch (e: unknown) {
      console.warn('加载情绪趋势失败:', (e as Error).message)
    }
  }

  /** 读取热门内容清单。 */
  const loadAITrendingContent = async () => {
    try {
      const res = await api.getStones({ page: 1, page_size: 10 })
      const d = res.data?.data || res.data || {}
      const list = Array.isArray(d) ? d : (d?.list || [])
      const maxScore = Math.max(1, ...list.map((item: { ripple_count?: number; boat_count?: number }) =>
        Number(item.ripple_count ?? 0) + Number(item.boat_count ?? 0)
      ))

      aiTrendingContent.value = list.slice(0, 10).map((item: {
        stone_id?: string
        content?: string
        mood_type?: string
        ripple_count?: number
        boat_count?: number
      }) => {
        const score = Number(item.ripple_count ?? 0) + Number(item.boat_count ?? 0)
        return {
          id: item.stone_id,
          content: item.content,
          mood: item.mood_type,
          score: maxScore > 0 ? score / maxScore : 0,
        }
      })
    } catch (e: unknown) {
      console.warn('加载AI热门内容失败:', (e as Error).message)
    }
  }

  return {
    loadStats, loadGrowthData, loadMoodDistribution, loadMoodTrend,
    loadTrendingTopics, loadActiveTimeStats, loadPrivacyStats,
    loadResonanceStats, loadEmotionPulse, loadEmotionTrends,
    loadAITrendingContent,
  }
}
