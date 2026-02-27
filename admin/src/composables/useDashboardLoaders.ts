// Dashboard 数据加载函数 composable
import { ElMessage } from 'element-plus'
import type { Ref } from 'vue'
import api from '@/api'
import type {
  GrowthDataItem, MoodDistributionItem, MoodTrendItem,
  ActiveTimeItem, TrendingTopic, TrendingContentItem, EmotionTrendItem,
  PrivacyStats, ResonanceStats,
} from '@/types'
import { moodNames, moodGradients } from './useChartOptions'

interface DashboardStats {
  totalUsers: number
  todayStones: number
  onlineCount: number
  pendingReports: number
}

// ECharts 图表配置的最小结构定义，仅描述 loader 实际访问的字段
interface ChartOptionWithAxisAndSeries {
  xAxis: { data: string[] }
  series: Array<{ data: (number | string)[] }>
}

interface SeriesOnlyChartOption {
  series: Array<{ data: (number | string)[] }>
}

interface PieChartOption {
  series: Array<{ data: Array<{ name: string; value: number; itemStyle: unknown }> }>
}

interface GaugeChartOption {
  series: Array<{ data: Array<{ value: number; name: string }> }>
}

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

  const loadTrendingTopics = async () => {
    try {
      const res = await api.getTrendingTopics?.() || { data: null }
      const raw = res.data?.data || res.data
      trendingTopics.value = Array.isArray(raw) ? raw.slice(0, 10) : (raw?.list || []).slice(0, 10)
    } catch (e: unknown) {
      console.warn('加载热门话题失败:', (e as Error).message)
    }
  }

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

  const loadEmotionTrends = async () => {
    try {
      const res = await api.getEmotionTrends()
      const d = res.data?.data || res.data
      const list = Array.isArray(d) ? d : (d?.list || [])
      if (list.length) {
        const dates = list.map((item: EmotionTrendItem) => item.date || item.day)
        emotionTrendsOption.value.xAxis.data = dates
        emotionTrendsOption.value.series[0].data = list.map((item: EmotionTrendItem) => item.positive ?? item.pos ?? 0)
        emotionTrendsOption.value.series[1].data = list.map((item: EmotionTrendItem) => item.neutral ?? item.neu ?? 0)
        emotionTrendsOption.value.series[2].data = list.map((item: EmotionTrendItem) => item.negative ?? item.neg ?? 0)
      }
    } catch (e: unknown) {
      console.warn('加载情绪趋势失败:', (e as Error).message)
    }
  }

  const loadAITrendingContent = async () => {
    try {
      const res = await api.getTrendingContent()
      const d = res.data?.data || res.data
      aiTrendingContent.value = Array.isArray(d) ? d.slice(0, 10) : (d?.list || []).slice(0, 10)
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
