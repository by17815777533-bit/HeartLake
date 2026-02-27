// Dashboard 数据逻辑主 composable - 整合图表配置和数据加载
import { ref, reactive, computed } from 'vue'
import { ElMessage } from 'element-plus'
import dayjs from 'dayjs'
import type { TrendingTopic, TrendingContentItem } from '@/types'
import { useChartOptions, moodColors, moodNames } from './useChartOptions'
import { useDashboardLoaders } from './useDashboardLoaders'

export function useDashboardData() {
  // ── 基础状态 ──
  const loading = ref(false)
  const privacyLoading = ref(false)
  const resonanceLoading = ref(false)
  const lastUpdateTime = ref(dayjs().format('HH:mm:ss'))
  const chartRange = ref(7)
  const moodTrendRange = ref(7)
  const trendingTopics = ref<TrendingTopic[]>([])
  const aiTrendingContent = ref<TrendingContentItem[]>([])
  const currentDateTime = ref(dayjs().format('YYYY年MM月DD日 dddd HH:mm'))

  const techBadges = [
    { icon: '🧠', label: 'DTW情绪共鸣算法' },
    { icon: '🔒', label: 'ε-差分隐私' },
    { icon: '💬', label: '双记忆RAG守护' },
    { icon: '📱', label: '端侧AI推理' },
  ]

  const greeting = computed(() => {
    const hour = dayjs().hour()
    if (hour < 6) return '夜深了'
    if (hour < 9) return '早上好'
    if (hour < 12) return '上午好'
    if (hour < 14) return '中午好'
    if (hour < 18) return '下午好'
    return '晚上好'
  })

  const formatNumber = (num: number) =>
    num >= 10000 ? (num / 10000).toFixed(1) + 'w' : num.toLocaleString()

  // ── 统计数据 ──
  const stats = reactive({
    totalUsers: 0, todayStones: 0, onlineCount: 0, pendingReports: 0,
  })
  const statsCards = computed(() => [
    { title: '总用户数', value: stats.totalUsers, color: '#1565C0', icon: 'User' },
    { title: '今日投石', value: stats.todayStones, color: '#2E7D32', icon: 'Edit' },
    { title: '在线人数', value: stats.onlineCount, color: '#6E5676', icon: 'Connection' },
    { title: '待处理举报', value: stats.pendingReports, color: '#E65100', icon: 'Warning' },
  ])

  // ── 隐私保护统计 ──
  const privacyStats = reactive({
    queryCount: 0, epsilonUsed: 0, epsilonTotal: 1.0, protectedUsers: 0,
  })
  const privacyBudgetPercent = computed(() => {
    if (privacyStats.epsilonTotal <= 0) return 0
    return Math.min((privacyStats.epsilonUsed / privacyStats.epsilonTotal) * 100, 100)
  })
  const privacyBudgetColor = computed(() => {
    if (privacyBudgetPercent.value < 50) return '#2E7D32'
    if (privacyBudgetPercent.value < 80) return '#E65100'
    return '#BA1A1A'
  })

  // ── 情绪共鸣统计 ──
  const resonanceStats = reactive({
    todayMatches: 0, avgScore: 0, topMood: '', successRate: 0,
  })

  // ── 图表配置 ──
  const charts = useChartOptions()

  // ── 湖面天气 ──
  const lakeWeather = computed(() => {
    const temp = charts.emotionPulseOption.value.series[0].data[0]?.value ?? 50
    if (temp > 60) return { icon: '☀️', label: '晴朗', desc: '社区积极', color: '#E65100', bg: 'linear-gradient(135deg, #FFF3E0 0%, #FFB74D 100%)' }
    if (temp > 30) return { icon: '⛅', label: '多云', desc: '社区平稳', color: '#44474E', bg: 'linear-gradient(135deg, #ECEFF1 0%, #B0BEC5 100%)' }
    if (temp > 10) return { icon: '🌧️', label: '小雨', desc: '社区低落', color: '#1565C0', bg: 'linear-gradient(135deg, #E3F2FD 0%, #64B5F6 100%)' }
    return { icon: '⛈️', label: '暴风雨', desc: '社区消极', color: '#BA1A1A', bg: 'linear-gradient(135deg, #FFEBEE 0%, #E57373 100%)' }
  })
  const lakeWeatherTemp = computed(() =>
    charts.emotionPulseOption.value.series[0].data[0]?.value ?? 50
  )

  // ── 天气心情饼图 ──
  const escapeHtml = (str: string | number) => String(str).replace(/[<>&"']/g, (c: string) => ({
    '<': '&lt;', '>': '&gt;', '&': '&amp;', '"': '&quot;', "'": '&#39;'
  }[c] ?? c))

  const weatherMoodPieOption = computed(() => {
    const moodData = charts.moodDistributionOption.value.series[0].data
    return {
      tooltip: {
        trigger: 'item', backgroundColor: '#2B2B2F', borderColor: '#44474E',
        borderRadius: 4, padding: [6, 10], textStyle: { color: '#E3E2E6', fontSize: 12 },
        formatter: (p: { marker: string; name: string; percent: number }) => {
          const name = escapeHtml(p.name)
          return `${p.marker} ${name}: ${Number(p.percent).toFixed(1)}%`
        }
      },
      series: [{
        type: 'pie', radius: ['50%', '75%'], center: ['50%', '50%'],
        itemStyle: { borderRadius: 3, borderColor: '#fff', borderWidth: 1 },
        label: { show: false },
        emphasis: {
          label: { show: true, fontSize: 11, fontWeight: '500', color: '#1C1B1F' },
          itemStyle: { shadowBlur: 6, shadowColor: 'rgba(0,0,0,0.15)' }
        },
        data: moodData.length ? moodData : moodNames.map((name, i) => ({
          value: [30, 25, 20, 15, 10][i], name,
          itemStyle: { color: moodColors[i] }
        }))
      }]
    }
  })

  // ── 数据加载 ──
  const loaders = useDashboardLoaders({
    stats, chartRange, moodTrendRange, trendingTopics, aiTrendingContent,
    privacyStats, privacyLoading, resonanceStats, resonanceLoading,
    ...charts,
  })

  // ── 导出数据（仅包含业务需要的字段，过滤原始响应中的冗余/敏感信息） ──
  const exportData = () => {
    // 清洗统计数据，只保留展示用的业务字段
    const cleanedStats = {
      totalUsers: stats.totalUsers,
      todayStones: stats.todayStones,
      onlineCount: stats.onlineCount,
      pendingReports: stats.pendingReports,
    }

    // 清洗话题数据，只保留关键词和热度
    const cleanedTopics = (trendingTopics.value || []).map(
      (t: { keyword?: string; count?: number }) => ({
        keyword: t.keyword ?? '',
        count: t.count ?? 0,
      })
    )

    // 清洗情绪分布数据，只保留名称和数值
    const rawMoodData = charts.moodDistributionOption.value.series[0].data || []
    const cleanedMoodDistribution = rawMoodData.map(
      (d: { name?: string; value?: number }) => ({
        name: d.name ?? '',
        value: d.value ?? 0,
      })
    )

    const data = {
      exportTime: dayjs().format('YYYY-MM-DD HH:mm:ss'),
      stats: cleanedStats,
      trendingTopics: cleanedTopics,
      moodDistribution: cleanedMoodDistribution,
    }
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `dashboard_${dayjs().format('YYYYMMDD_HHmmss')}.json`
    a.click()
    URL.revokeObjectURL(url)
    ElMessage.success('数据导出成功')
  }

  // ── 刷新所有数据 ──
  const refreshData = async () => {
    loading.value = true
    await Promise.allSettled([
      loaders.loadStats(), loaders.loadGrowthData(),
      loaders.loadMoodDistribution(), loaders.loadMoodTrend(),
      loaders.loadTrendingTopics(), loaders.loadActiveTimeStats(),
      loaders.loadPrivacyStats(), loaders.loadResonanceStats(),
      loaders.loadEmotionPulse(), loaders.loadEmotionTrends(),
      loaders.loadAITrendingContent(),
    ])
    lastUpdateTime.value = dayjs().format('HH:mm:ss')
    loading.value = false
  }

  return {
    // 状态
    loading, privacyLoading, resonanceLoading, lastUpdateTime,
    chartRange, moodTrendRange, trendingTopics, aiTrendingContent,
    currentDateTime, techBadges, greeting, formatNumber,
    stats, statsCards,
    privacyStats, privacyBudgetPercent, privacyBudgetColor,
    resonanceStats,
    // 天气
    lakeWeather, lakeWeatherTemp, weatherMoodPieOption,
    // 图表
    ...charts,
    // 加载
    ...loaders, exportData, refreshData,
  }
}
