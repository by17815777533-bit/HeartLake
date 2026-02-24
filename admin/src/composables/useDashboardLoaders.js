// @file useDashboardLoaders.js
// @brief Dashboard 数据加载函数 composable
import { ElMessage } from 'element-plus'
import api from '@/api'
import { moodNames, moodGradients } from './useChartOptions'

export function useDashboardLoaders({
  stats, chartRange, moodTrendRange, trendingTopics, aiTrendingContent,
  privacyStats, privacyLoading, resonanceStats, resonanceLoading,
  userGrowthOption, moodDistributionOption, moodTrendOption,
  activeTimeOption, emotionPulseOption, emotionTrendsOption,
}) {
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
    } catch (e) {
      console.warn('加载统计数据失败:', e.message)
      ElMessage.error('加载统计数据失败，请稍后重试')
    }
  }

  const loadGrowthData = async () => {
    try {
      const res = await api.getUserGrowthStats(chartRange.value)
      const raw = res.data?.data || res.data
      const list = Array.isArray(raw) ? raw : (raw?.list || [])
      if (list.length) {
        userGrowthOption.value.xAxis.data = list.map(item => item.date)
        userGrowthOption.value.series[0].data = list.map(item => item.count)
      }
    } catch (e) {
      console.warn('加载增长数据失败:', e.message)
      ElMessage.error('加载增长数据失败，请稍后重试')
    }
  }

  const loadMoodDistribution = async () => {
    try {
      const res = await api.getMoodDistribution()
      const raw = res.data?.data || res.data
      const list = Array.isArray(raw) ? raw : (raw?.list || [])
      if (list.length) {
        moodDistributionOption.value.series[0].data = list.map((item, i) => ({
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
    } catch (e) {
      console.warn('加载情绪分布失败:', e.message)
      ElMessage.error('加载情绪分布失败，请稍后重试')
    }
  }

  const loadMoodTrend = async () => {
    try {
      const res = await api.getMoodTrend?.(moodTrendRange.value) || { data: null }
      const trendData = res.data?.data || res.data
      const trendList = trendData?.list || (Array.isArray(trendData) ? trendData : [])
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
    } catch (e) {
      console.warn('加载情绪趋势失败:', e.message)
    }
  }

  const loadTrendingTopics = async () => {
    try {
      const res = await api.getTrendingTopics?.() || { data: null }
      const raw = res.data?.data || res.data
      trendingTopics.value = Array.isArray(raw) ? raw.slice(0, 10) : (raw?.list || []).slice(0, 10)
    } catch (e) {
      console.warn('加载热门话题失败:', e.message)
    }
  }

  const loadActiveTimeStats = async () => {
    try {
      const res = await api.getActiveTimeStats?.() || { data: null }
      const timeData = res.data?.data || res.data
      if (timeData) {
        activeTimeOption.value.series[0].data = (Array.isArray(timeData)
          ? timeData : timeData.list || []).map(item => item.count)
      }
    } catch (e) {
      console.warn('加载活跃时段失败:', e.message)
      ElMessage.error('加载活跃时段失败，请稍后重试')
    }
  }

  const loadPrivacyStats = async () => {
    privacyLoading.value = true
    try {
      const res = await api.getPrivacyStats()
      const d = res.data?.data || res.data || {}
      privacyStats.queryCount = d.query_count ?? 0
      privacyStats.epsilonUsed = d.epsilon_used ?? 0
      privacyStats.epsilonTotal = d.epsilon_total ?? 1.0
      privacyStats.protectedUsers = d.protected_users ?? 0
    } catch (e) {
      console.warn('加载隐私统计失败:', e.message)
    } finally {
      privacyLoading.value = false
    }
  }

  const loadResonanceStats = async () => {
    resonanceLoading.value = true
    try {
      const res = await api.getResonanceStats()
      const d = res.data?.data || res.data || {}
      resonanceStats.todayMatches = d.today_matches ?? 0
      resonanceStats.avgScore = d.avg_score ?? 0
      resonanceStats.topMood = d.top_mood ?? ''
      resonanceStats.successRate = d.success_rate ?? 0
    } catch (e) {
      console.warn('加载共鸣统计失败:', e.message)
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
    } catch (e) {
      console.warn('加载情绪温度失败:', e.message)
    }
  }

  const loadEmotionTrends = async () => {
    try {
      const res = await api.getEmotionTrends()
      const d = res.data?.data || res.data
      const list = Array.isArray(d) ? d : (d?.list || [])
      if (list.length) {
        const dates = list.map(item => item.date || item.day)
        emotionTrendsOption.value.xAxis.data = dates
        emotionTrendsOption.value.series[0].data = list.map(item => item.positive ?? item.pos ?? 0)
        emotionTrendsOption.value.series[1].data = list.map(item => item.neutral ?? item.neu ?? 0)
        emotionTrendsOption.value.series[2].data = list.map(item => item.negative ?? item.neg ?? 0)
      }
    } catch (e) {
      console.warn('加载情绪趋势失败:', e.message)
    }
  }

  const loadAITrendingContent = async () => {
    try {
      const res = await api.getTrendingContent()
      const d = res.data?.data || res.data
      aiTrendingContent.value = Array.isArray(d) ? d.slice(0, 10) : (d?.list || []).slice(0, 10)
    } catch (e) {
      console.warn('加载AI热门内容失败:', e.message)
    }
  }

  return {
    loadStats, loadGrowthData, loadMoodDistribution, loadMoodTrend,
    loadTrendingTopics, loadActiveTimeStats, loadPrivacyStats,
    loadResonanceStats, loadEmotionPulse, loadEmotionTrends,
    loadAITrendingContent,
  }
}
