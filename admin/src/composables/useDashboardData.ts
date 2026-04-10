/**
 * 大屏数据组合逻辑，聚合图表配置、统计加载与数据导出。
 *
 * 这是 Dashboard 页面的核心 composable，职责包括：
 * 1. 聚合 useChartOptions（图表配置）和 useDashboardLoaders（数据拉取）
 * 2. 维护统计卡片、隐私预算、情绪共鸣等业务状态
 * 3. 提供湖面天气模型（情绪温度 → 天气隐喻的映射）
 * 4. 数据导出（JSON 格式，自动脱敏只保留展示字段）
 * 5. 全量刷新（Promise.allSettled 并行拉取，单个失败不阻塞其他）
 *
 * 返回值直接解构给 Dashboard.vue 使用，子组件通过 props 接收。
 */
import { ref, reactive, computed } from 'vue'
import { ElMessage } from 'element-plus'
import dayjs from 'dayjs'
import type { TrendingTopic, TrendingContentItem } from '@/types'
import { useChartOptions } from './useChartOptions'
import { useDashboardLoaders } from './useDashboardLoaders'

export function useDashboardData() {
  /** 基础状态。 */
  const loading = ref(false)
  const privacyLoading = ref(false)
  const resonanceLoading = ref(false)
  const lastUpdateTime = ref(dayjs().format('HH:mm:ss'))
  const chartRange = ref(7)
  const moodTrendRange = ref(7)
  const trendingTopics = ref<TrendingTopic[]>([])
  const aiTrendingContent = ref<TrendingContentItem[]>([])
  const loaderIssues = reactive<Record<string, string>>({})
  const currentDateTime = ref(dayjs().format('YYYY年MM月DD日 HH:mm'))

  const techBadges = [
    { icon: '巡看', label: '今日湖面' },
    { icon: '反馈', label: '旅人反馈' },
    { icon: '提醒', label: '异常提醒' },
    { icon: '待办', label: '处置进度' },
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

  /** 统计卡片基础数据。 */
  const stats = reactive({
    totalUsers: 0,
    todayStones: 0,
    onlineCount: 0,
    pendingReports: 0,
  })
  const statsCards = computed(() => [
    { title: '总用户数', value: stats.totalUsers, color: '#144552', icon: 'User' },
    { title: '今日投石', value: stats.todayStones, color: '#2f6b78', icon: 'Edit' },
    { title: '在线人数', value: stats.onlineCount, color: '#7b97a3', icon: 'Connection' },
    { title: '待处理举报', value: stats.pendingReports, color: '#b67a42', icon: 'Warning' },
  ])

  /** 隐私预算与查询统计。 */
  const privacyStats = reactive({
    queryCount: 0,
    epsilonUsed: 0,
    epsilonTotal: 1.0,
    protectedUsers: 0,
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

  /** 情绪共鸣统计。 */
  const resonanceStats = reactive({
    todayMatches: 0,
    avgScore: 0,
    topMood: '',
    successRate: 0,
  })

  /** 图表配置。 */
  const charts = useChartOptions()

  const dashboardWarnings = computed(() =>
    Object.entries(loaderIssues).map(([key, message]) => ({ key, message })),
  )

  /** 情绪温度对应的湖面天气模型。 */
  const lakeWeather = computed(() => {
    const temp = charts.emotionPulseOption.value.series[0].data[0]?.value ?? 50
    if (temp > 60)
      return {
        icon: '☀️',
        label: '晴朗',
        desc: '社区积极',
        color: '#b67a42',
        bg: 'linear-gradient(135deg, #f8efe1 0%, #e9b97a 100%)',
      }
    if (temp > 30)
      return {
        icon: '⛅',
        label: '多云',
        desc: '社区平稳',
        color: '#56717d',
        bg: 'linear-gradient(135deg, #eef3f5 0%, #acc3cb 100%)',
      }
    if (temp > 10)
      return {
        icon: '🌧️',
        label: '小雨',
        desc: '社区低落',
        color: '#2f6b78',
        bg: 'linear-gradient(135deg, #dfecef 0%, #7ba4b0 100%)',
      }
    return {
      icon: '⛈️',
      label: '暴风雨',
      desc: '社区消极',
      color: '#8a4f46',
      bg: 'linear-gradient(135deg, #efe2df 0%, #bb7d75 100%)',
    }
  })
  const lakeWeatherTemp = computed(
    () => charts.emotionPulseOption.value.series[0].data[0]?.value ?? 50,
  )

  /** 天气心情饼图配置。 */
  const escapeHtml = (str: string | number) =>
    String(str).replace(
      /[<>&"']/g,
      (c: string) =>
        ({
          '<': '&lt;',
          '>': '&gt;',
          '&': '&amp;',
          '"': '&quot;',
          "'": '&#39;',
        })[c] ?? c,
    )

  const weatherMoodPieOption = computed(() => {
    const moodData = charts.moodDistributionOption.value.series[0].data
    return {
      tooltip: {
        trigger: 'item',
        backgroundColor: 'rgba(15, 28, 34, 0.92)',
        borderColor: 'rgba(208, 221, 226, 0.16)',
        borderRadius: 14,
        padding: [8, 12],
        textStyle: { color: '#edf5f7', fontSize: 12 },
        formatter: (p: { marker: string; name: string; percent: number }) => {
          const name = escapeHtml(p.name)
          return `${p.marker} ${name}: ${Number(p.percent).toFixed(1)}%`
        },
      },
      legend: {
        bottom: 0,
        icon: 'circle',
        itemGap: 18,
        textStyle: { color: '#5f7882', fontSize: 12 },
      },
      series: [
        {
          type: 'pie',
          radius: ['58%', '80%'],
          center: ['50%', '42%'],
          padAngle: 2,
          itemStyle: { borderRadius: 10, borderColor: '#f8fbfc', borderWidth: 3 },
          label: { show: false },
          emphasis: {
            label: { show: true, fontSize: 12, fontWeight: '600', color: '#1C1B1F' },
            itemStyle: { shadowBlur: 10, shadowColor: 'rgba(0,0,0,0.16)' },
          },
          data: moodData,
        },
      ],
    }
  })

  /** 远程加载器集合。 */
  const loaders = useDashboardLoaders({
    stats,
    chartRange,
    moodTrendRange,
    trendingTopics,
    aiTrendingContent,
    privacyStats,
    privacyLoading,
    resonanceStats,
    resonanceLoading,
    loaderIssues,
    ...charts,
  })

  const loadPrimaryDashboardData = () =>
    Promise.allSettled([
      loaders.loadStats(),
      loaders.loadGrowthData(),
      loaders.loadTrendingTopics(),
      loaders.loadEmotionPulse(),
      loaders.loadAITrendingContent(),
    ])

  const loadSecondaryDashboardData = () =>
    Promise.allSettled([
      loaders.loadMoodDistribution(),
      loaders.loadMoodTrend(),
      loaders.loadActiveTimeStats(),
      loaders.loadPrivacyStats(),
      loaders.loadResonanceStats(),
      loaders.loadEmotionTrends(),
    ])

  const ensureMoodDistributionLoaded = async () => {
    const moodItems = charts.moodDistributionOption.value.series[0].data || []
    if (moodItems.length) return
    await loaders.loadMoodDistribution()
  }

  let secondaryLoadTimer: ReturnType<typeof setTimeout> | null = null
  let refreshInFlight: Promise<void> | null = null
  let refreshQueued = false

  /**
   * 导出大屏数据为 JSON 文件，只保留展示用的业务字段。
   */
  const exportData = async () => {
    await ensureMoodDistributionLoaded()
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
      }),
    )

    // 清洗情绪分布数据，只保留名称和数值
    const rawMoodData = charts.moodDistributionOption.value.series[0].data || []
    const cleanedMoodDistribution = rawMoodData.map((d: { name?: string; value?: number }) => ({
      name: d.name ?? '',
      value: d.value ?? 0,
    }))

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

  /** 并行刷新大屏全部数据。 */
  const runDashboardRefresh = async () => {
    loading.value = true
    await loadPrimaryDashboardData()
    lastUpdateTime.value = dayjs().format('HH:mm:ss')
    if (secondaryLoadTimer) {
      clearTimeout(secondaryLoadTimer)
    }
    secondaryLoadTimer = setTimeout(() => {
      void loadSecondaryDashboardData()
      secondaryLoadTimer = null
    }, 180)
    loading.value = false
  }

  const refreshData = async () => {
    if (refreshInFlight) {
      refreshQueued = true
      return refreshInFlight
    }

    refreshInFlight = (async () => {
      do {
        refreshQueued = false
        await runDashboardRefresh()
      } while (refreshQueued)
    })().finally(() => {
      refreshInFlight = null
    })

    return refreshInFlight
  }

  return {
    // 状态
    loading,
    privacyLoading,
    resonanceLoading,
    lastUpdateTime,
    chartRange,
    moodTrendRange,
    trendingTopics,
    aiTrendingContent,
    loaderIssues,
    dashboardWarnings,
    currentDateTime,
    techBadges,
    greeting,
    formatNumber,
    stats,
    statsCards,
    privacyStats,
    privacyBudgetPercent,
    privacyBudgetColor,
    resonanceStats,
    // 天气
    lakeWeather,
    lakeWeatherTemp,
    weatherMoodPieOption,
    // 图表
    ...charts,
    // 加载
    ...loaders,
    exportData,
    refreshData,
  }
}
