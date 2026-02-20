<!--
  @file Dashboard.vue
  @brief Dashboard 组件 - 数据大屏 + 欢迎信息
  Created by 林子怡
-->

<template>
  <div class="dashboard">
    <!-- 欢迎信息 + 页面标题 -->
    <div class="page-header">
      <div class="welcome-section">
        <h1>{{ greeting }}，管理员</h1>
        <p class="welcome-sub">{{ currentDateTime }} · 心湖管理后台</p>
      </div>
      <div class="header-actions">
        <el-button type="success" :icon="Download" @click="exportData">导出数据</el-button>
        <el-button type="primary" :icon="Refresh" @click="refreshData" :loading="loading">
          刷新数据
        </el-button>
        <span class="update-time">最后更新: {{ lastUpdateTime }}</span>
      </div>
    </div>

    <!-- 创新技术标签 -->
    <div class="tech-badges">
      <span class="tech-badge" v-for="badge in techBadges" :key="badge.label">
        <span class="badge-icon">{{ badge.icon }}</span>
        <span class="badge-label">{{ badge.label }}</span>
      </span>
    </div>

    <!-- 统计卡片 -->
    <el-row :gutter="20" class="stats-cards">
      <!-- L-2: 响应式断点 -->
      <el-col :xs="24" :sm="12" :md="6" v-for="stat in statsCards" :key="stat.title">
        <el-card shadow="hover" class="stat-card" :style="{ borderTop: `4px solid ${stat.color}` }">
          <div class="stat-content">
            <div class="stat-info">
              <div class="stat-value">{{ formatNumber(stat.value) }}</div>
              <div class="stat-title">{{ stat.title }}</div>
            </div>
            <div class="stat-icon" :style="{ background: `${stat.color}15`, color: stat.color }">
              <el-icon :size="32"><component :is="stat.icon" /></el-icon>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 图表区 -->
    <el-row :gutter="20" class="charts-row">
      <el-col :span="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>用户增长趋势</span>
              <el-radio-group v-model="chartRange" size="small" @change="loadGrowthData">
                <el-radio-button :value="7">7天</el-radio-button>
                <el-radio-button :value="14">14天</el-radio-button>
                <el-radio-button :value="30">30天</el-radio-button>
              </el-radio-group>
            </div>
          </template>
          <v-chart :option="userGrowthOption" autoresize style="height: 300px" />
        </el-card>
      </el-col>

      <el-col :span="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>情绪分布</template>
          <v-chart :option="moodDistributionOption" autoresize style="height: 300px" />
        </el-card>
      </el-col>
    </el-row>

    <!-- 情绪趋势分析 -->
    <el-row :gutter="20" class="charts-row">
      <el-col :span="24">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>情绪趋势分析</span>
              <el-radio-group v-model="moodTrendRange" size="small" @change="loadMoodTrend">
                <el-radio-button :value="7">7天</el-radio-button>
                <el-radio-button :value="14">14天</el-radio-button>
              </el-radio-group>
            </div>
          </template>
          <v-chart :option="moodTrendOption" autoresize style="height: 280px" />
        </el-card>
      </el-col>
    </el-row>

    <!-- 热门话题和活跃时段 -->
    <el-row :gutter="20" class="charts-row">
      <el-col :span="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>热门话题</template>
          <div class="topics-list">
            <div v-for="(topic, i) in trendingTopics" :key="topic.topic" class="topic-item">
              <span class="topic-rank" :class="{ top: i < 3 }">{{ i + 1 }}</span>
              <span class="topic-name">{{ topic.topic }}</span>
              <span class="topic-count">{{ topic.count }} 条</span>
            </div>
            <el-empty v-if="!trendingTopics.length" description="暂无数据" :image-size="60" />
          </div>
        </el-card>
      </el-col>
      <el-col :span="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>活跃时段分布</template>
          <v-chart :option="activeTimeOption" autoresize style="height: 300px" />
        </el-card>
      </el-col>
    </el-row>

    <!-- 隐私保护 + 情绪共鸣 + 情绪脉搏 -->
    <el-row :gutter="20" class="charts-row">
      <el-col :xs="24" :sm="12" :md="8">
        <el-card shadow="hover" class="chart-card innovation-card">
          <template #header>
            <div class="card-header">
              <span>🔒 隐私保护统计</span>
            </div>
          </template>
          <div class="innovation-stats" v-loading="privacyLoading">
            <div class="inno-stat-item">
              <span class="inno-label">差分隐私查询次数</span>
              <span class="inno-value">{{ formatNumber(privacyStats.queryCount) }}</span>
            </div>
            <div class="inno-stat-item">
              <span class="inno-label">隐私预算消耗 (ε)</span>
              <span class="inno-value epsilon">{{ privacyStats.epsilonUsed.toFixed(2) }} / {{ privacyStats.epsilonTotal.toFixed(1) }}</span>
            </div>
            <div class="budget-bar">
              <el-progress
                :percentage="privacyBudgetPercent"
                :color="privacyBudgetColor"
                :stroke-width="10"
                :show-text="true"
                :format="() => privacyBudgetPercent.toFixed(1) + '%'"
              />
            </div>
            <div class="inno-stat-item">
              <span class="inno-label">保护的用户数</span>
              <span class="inno-value">{{ formatNumber(privacyStats.protectedUsers) }}</span>
            </div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="8">
        <el-card shadow="hover" class="chart-card innovation-card">
          <template #header>
            <div class="card-header">
              <span>💫 情绪共鸣统计</span>
            </div>
          </template>
          <div class="innovation-stats" v-loading="resonanceLoading">
            <div class="inno-stat-item">
              <span class="inno-label">今日共鸣匹配数</span>
              <span class="inno-value">{{ formatNumber(resonanceStats.todayMatches) }}</span>
            </div>
            <div class="inno-stat-item">
              <span class="inno-label">平均共鸣分数</span>
              <span class="inno-value highlight">{{ resonanceStats.avgScore.toFixed(1) }}</span>
            </div>
            <div class="inno-stat-item">
              <span class="inno-label">最活跃的情绪类型</span>
              <el-tag size="small" type="primary">{{ resonanceStats.topMood || '-' }}</el-tag>
            </div>
            <div class="inno-stat-item">
              <span class="inno-label">共鸣成功率</span>
              <span class="inno-value">{{ resonanceStats.successRate.toFixed(1) }}%</span>
            </div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="24" :md="8">
        <el-card shadow="hover" class="chart-card innovation-card">
          <template #header>
            <div class="card-header">
              <span>🌊 湖面情绪温度</span>
              <span class="pulse-hint">每30秒更新</span>
            </div>
          </template>
          <v-chart :option="emotionPulseOption" autoresize style="height: 260px" />
        </el-card>
      </el-col>
    </el-row>

  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, onUnmounted } from 'vue'
import { Refresh, Download } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import VChart from 'vue-echarts'
import dayjs from 'dayjs'
import api from '@/api'

const loading = ref(false)
const privacyLoading = ref(false)
const resonanceLoading = ref(false)

// 创新技术标签
const techBadges = [
  { icon: '🧠', label: 'DTW情绪共鸣算法' },
  { icon: '🔒', label: 'ε-差分隐私' },
  { icon: '💬', label: '双记忆RAG守护' },
  { icon: '📱', label: '端侧AI推理' },
]

// 欢迎信息：根据时间段显示不同问候语
const greeting = computed(() => {
  const hour = dayjs().hour()
  if (hour < 6) return '夜深了'
  if (hour < 9) return '早上好'
  if (hour < 12) return '上午好'
  if (hour < 14) return '中午好'
  if (hour < 18) return '下午好'
  return '晚上好'
})

// 实时时钟
const currentDateTime = ref(dayjs().format('YYYY年MM月DD日 dddd HH:mm'))
let clockTimer = null

// 数字格式化
const formatNumber = (num) => num >= 10000 ? (num / 10000).toFixed(1) + 'w' : num.toLocaleString()
const lastUpdateTime = ref(dayjs().format('HH:mm:ss'))
const chartRange = ref(7)
const moodTrendRange = ref(7)
const trendingTopics = ref([])

// 活跃时段图表配置 - Google 风格
const activeTimeOption = ref({
  tooltip: { trigger: 'axis', backgroundColor: '#fff', borderColor: '#dadce0', textStyle: { color: '#202124' } },
  grid: { left: 50, right: 20, top: 20, bottom: 30 },
  xAxis: { type: 'category', data: Array.from({ length: 24 }, (_, i) => `${i}:00`), axisLine: { lineStyle: { color: '#dadce0' } }, axisLabel: { color: '#5f6368' } },
  yAxis: { type: 'value', splitLine: { lineStyle: { color: '#f1f3f4', type: 'dashed' } }, axisLabel: { color: '#5f6368' } },
  series: [{ type: 'bar', data: [], itemStyle: { color: '#34A853', borderRadius: [2, 2, 0, 0] } }]
})

// 隐私保护统计
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
  if (privacyBudgetPercent.value < 50) return '#34A853'
  if (privacyBudgetPercent.value < 80) return '#FBBC04'
  return '#EA4335'
})

// 情绪共鸣统计
const resonanceStats = reactive({
  todayMatches: 0,
  avgScore: 0,
  topMood: '',
  successRate: 0,
})

// 湖面情绪温度 - 仪表盘
const emotionPulseOption = ref({
  series: [{
    type: 'gauge',
    center: ['50%', '60%'],
    radius: '90%',
    startAngle: 200,
    endAngle: -20,
    min: 0,
    max: 100,
    splitNumber: 10,
    axisLine: {
      lineStyle: {
        width: 16,
        color: [
          [0.3, '#4285F4'],
          [0.5, '#34A853'],
          [0.7, '#FBBC04'],
          [1, '#EA4335'],
        ]
      }
    },
    pointer: { icon: 'path://M12.8,0.7l12,40.1H0.7L12.8,0.7z', length: '55%', width: 8, offsetCenter: [0, '-10%'], itemStyle: { color: 'auto' } },
    axisTick: { length: 6, lineStyle: { color: 'auto', width: 1 } },
    splitLine: { length: 12, lineStyle: { color: 'auto', width: 2 } },
    axisLabel: { color: '#5f6368', fontSize: 10, distance: -40 },
    title: { offsetCenter: [0, '20%'], fontSize: 14, color: '#5f6368' },
    detail: { fontSize: 28, offsetCenter: [0, '45%'], valueAnimation: true, color: 'auto', formatter: '{value}°' },
    data: [{ value: 50, name: '情绪温度' }]
  }]
})

// 统计数据
const stats = reactive({
  totalUsers: 0,
  todayStones: 0,
  onlineCount: 0,
  pendingReports: 0,
})

// 统计卡片 - Google 配色
const statsCards = computed(() => [
  { title: '总用户数', value: stats.totalUsers, color: '#4285F4', icon: 'User' },
  { title: '今日投石', value: stats.todayStones, color: '#34A853', icon: 'Edit' },
  { title: '在线人数', value: stats.onlineCount, color: '#1A73E8', icon: 'Connection' },
  { title: '待处理举报', value: stats.pendingReports, color: '#FBBC04', icon: 'Warning' },
])

// 用户增长图表配置 - Google 风格
const userGrowthOption = ref({
  tooltip: {
    trigger: 'axis',
    backgroundColor: '#fff',
    borderColor: '#dadce0',
    textStyle: { color: '#202124' },
    formatter: (params) => {
      const p = params[0]
      const name = String(p.name).replace(/[<>&"']/g, c => ({'<':'&lt;','>':'&gt;','&':'&amp;','"':'&quot;',"'":'&#39;'}[c]))
      return `<div style="font-weight:500">${name}</div><div style="color:#1A73E8">新增 ${Number(p.value)} 人</div>`
    }
  },
  grid: { left: 50, right: 20, top: 20, bottom: 30 },
  xAxis: { type: 'category', data: [], axisLine: { lineStyle: { color: '#dadce0' } }, axisLabel: { color: '#5f6368' } },
  yAxis: { type: 'value', splitLine: { lineStyle: { color: '#f1f3f4', type: 'dashed' } }, axisLabel: { color: '#5f6368' } },
  series: [{
    name: '新增用户',
    type: 'line',
    smooth: true,
    symbol: 'circle',
    symbolSize: 6,
    data: [],
    areaStyle: { opacity: 0.1, color: '#4285F4' },
    itemStyle: { color: '#4285F4', borderWidth: 2, borderColor: '#fff' },
    lineStyle: { color: '#4285F4', width: 2 },
  }],
})

// Google 配色
const moodColors = ['#4285F4', '#34A853', '#EA4335', '#FBBC04', '#9AA0A6']
const moodNames = ['开心', '平静', '难过', '焦虑', '其他']
const moodGradients = [
  { start: '#4285F4', end: '#1A73E8' },
  { start: '#34A853', end: '#1E8E3E' },
  { start: '#EA4335', end: '#C5221F' },
  { start: '#FBBC04', end: '#F9AB00' },
  { start: '#9AA0A6', end: '#80868B' },
]

// 情绪趋势图表配置 - Google 风格
const moodTrendOption = ref({
  tooltip: {
    trigger: 'axis',
    backgroundColor: '#fff',
    borderColor: '#dadce0',
    borderRadius: 4,
    padding: [8, 12],
    textStyle: { color: '#202124' }
  },
  legend: { bottom: 0, itemGap: 16, textStyle: { color: '#5f6368', fontSize: 12 } },
  grid: { left: 50, right: 20, top: 20, bottom: 50 },
  xAxis: { type: 'category', data: [], axisLine: { lineStyle: { color: '#dadce0' } }, axisLabel: { color: '#5f6368' } },
  yAxis: { type: 'value', splitLine: { lineStyle: { color: '#f1f3f4', type: 'dashed' } }, axisLabel: { color: '#5f6368' } },
  series: moodNames.map((name, i) => ({
    name, type: 'line', smooth: 0.3, symbol: 'circle', symbolSize: 5, data: [],
    itemStyle: { color: moodColors[i], borderWidth: 2, borderColor: '#fff' },
    lineStyle: { width: 2 },
    areaStyle: { opacity: 0.05, color: moodColors[i] }
  }))
})

// 情绪分布图表配置 - Google 风格
const moodDistributionOption = ref({
  tooltip: {
    trigger: 'item',
    backgroundColor: '#fff',
    borderColor: '#dadce0',
    borderRadius: 4,
    padding: [8, 12],
    textStyle: { color: '#202124' },
    formatter: (p) => {
      const name = String(p.name).replace(/[<>&"']/g, c => ({'<':'&lt;','>':'&gt;','&':'&amp;','"':'&quot;',"'":'&#39;'}[c]))
      return `<div style="font-weight:500">${p.marker} ${name}</div><div style="color:#5f6368">${Number(p.value)} 条 · ${Number(p.percent)}%</div>`
    }
  },
  legend: { bottom: 0, itemGap: 16, textStyle: { color: '#5f6368', fontSize: 12 } },
  series: [{
    type: 'pie',
    radius: ['45%', '70%'],
    center: ['50%', '42%'],
    itemStyle: { borderRadius: 4, borderColor: '#fff', borderWidth: 2 },
    label: { show: false },
    emphasis: {
      label: { show: true, fontSize: 14, fontWeight: '500', color: '#202124' },
      itemStyle: { shadowBlur: 10, shadowColor: 'rgba(0,0,0,0.1)' }
    },
    data: moodNames.map((name, i) => ({
      value: [30, 25, 20, 15, 10][i],
      name,
      itemStyle: { color: moodColors[i] }
    }))
  }],
})

// 加载统计数据（需要同时调用 getDashboardStats 和 getRealtimeStats）
const loadStats = async () => {
  try {
    const [dashRes, realtimeRes] = await Promise.all([
      api.getDashboardStats().catch(() => ({ data: null })),
      api.getRealtimeStats().catch(() => ({ data: null }))
    ])
    const d = dashRes.data || {}
    const r = realtimeRes.data || {}
    stats.totalUsers = r.total_users ?? d.total_users ?? 0
    stats.todayStones = d.today_stones ?? r.today_stones ?? 0
    stats.onlineCount = r.online_users ?? 0
    stats.pendingReports = d.pending_reports ?? r.pending_reports ?? 0
  } catch (e) {
    console.warn('加载统计数据失败:', e.message)
    ElMessage.error('加载统计数据失败，请稍后重试')
  }
}

// 加载增长数据（后端返回原始数组，非 {list: [...]}）
const loadGrowthData = async () => {
  try {
    const res = await api.getUserGrowthStats(chartRange.value)
    const list = Array.isArray(res.data) ? res.data : (res.data?.list || [])
    if (list.length) {
      const dates = list.map(item => item.date)
      const values = list.map(item => item.count)

      userGrowthOption.value.xAxis.data = dates
      userGrowthOption.value.series[0].data = values
    }
  } catch (e) {
    console.warn('加载增长数据失败:', e.message)
    ElMessage.error('加载增长数据失败，请稍后重试')
  }
}

// 加载情绪分布（后端返回原始数组，字段为 mood_type）
const loadMoodDistribution = async () => {
  try {
    const res = await api.getMoodDistribution()
    const list = Array.isArray(res.data) ? res.data : (res.data?.list || [])
    if (list.length) {
      moodDistributionOption.value.series[0].data = list.map((item, i) => ({
        name: item.mood_type || item.mood,
        value: item.count,
        itemStyle: { color: { type: 'linear', x: 0, y: 0, x2: 1, y2: 1, colorStops: [{ offset: 0, color: moodGradients[i % moodGradients.length].start }, { offset: 1, color: moodGradients[i % moodGradients.length].end }] } }
      }))
    }
  } catch (e) {
    console.warn('加载情绪分布失败:', e.message)
    ElMessage.error('加载情绪分布失败，请稍后重试')
  }
}

// 加载情绪趋势
const loadMoodTrend = async () => {
  try {
    const res = await api.getMoodTrend?.(moodTrendRange.value) || { data: null }
    if (res.data?.list) {
      const dates = [...new Set(res.data.list.map(item => item.date))].sort()
      moodTrendOption.value.xAxis.data = dates
      moodNames.forEach((mood, i) => {
        moodTrendOption.value.series[i].data = dates.map(date => {
          const item = res.data.list.find(d => d.date === date && d.mood === mood)
          return item?.count || 0
        })
      })
    }
  } catch (e) {
    console.warn('加载情绪趋势失败:', e.message)
    ElMessage.error('加载情绪趋势失败，请稍后重试')
  }
}

// 加载热门话题
const loadTrendingTopics = async () => {
  try {
    const res = await api.getTrendingTopics?.() || { data: null }
    if (res.data) trendingTopics.value = Array.isArray(res.data) ? res.data : (res.data.list || [])
  } catch (e) {
    console.warn('加载热门话题失败:', e.message)
    ElMessage.error('加载热门话题失败，请稍后重试')
  }
}

// 加载活跃时段
const loadActiveTimeStats = async () => {
  try {
    const res = await api.getActiveTimeStats?.() || { data: null }
    if (res.data) activeTimeOption.value.series[0].data = (Array.isArray(res.data) ? res.data : res.data.list || []).map(item => item.count)
  } catch (e) {
    console.warn('加载活跃时段失败:', e.message)
    ElMessage.error('加载活跃时段失败，请稍后重试')
  }
}

// 加载隐私保护统计
const loadPrivacyStats = async () => {
  privacyLoading.value = true
  try {
    const res = await api.getPrivacyStats()
    const d = res.data || {}
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

// 加载情绪共鸣统计
const loadResonanceStats = async () => {
  resonanceLoading.value = true
  try {
    const res = await api.getResonanceStats()
    const d = res.data || {}
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

// 加载湖面情绪温度
const loadEmotionPulse = async () => {
  try {
    const res = await api.getEmotionPulse()
    const d = res.data || {}
    const temp = d.temperature ?? 50
    emotionPulseOption.value.series[0].data = [{ value: temp, name: '情绪温度' }]
  } catch (e) {
    console.warn('加载情绪温度失败:', e.message)
  }
}

let pulseTimer = null
const exportData = () => {
  const data = {
    exportTime: dayjs().format('YYYY-MM-DD HH:mm:ss'),
    stats: { ...stats },
    trendingTopics: trendingTopics.value,
    moodDistribution: moodDistributionOption.value.series[0].data
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

// 刷新所有数据
const refreshData = async () => {
  loading.value = true
  await Promise.allSettled([loadStats(), loadGrowthData(), loadMoodDistribution(), loadMoodTrend(), loadTrendingTopics(), loadActiveTimeStats(), loadPrivacyStats(), loadResonanceStats(), loadEmotionPulse()])
  lastUpdateTime.value = dayjs().format('HH:mm:ss')
  loading.value = false
}

onMounted(() => {
  refreshData()
  // 每分钟更新时钟显示
  clockTimer = setInterval(() => {
    currentDateTime.value = dayjs().format('YYYY年MM月DD日 dddd HH:mm')
  }, 60000)
  // 每30秒更新情绪温度
  pulseTimer = setInterval(loadEmotionPulse, 30000)
})

onUnmounted(() => {
  if (clockTimer) {
    clearInterval(clockTimer)
    clockTimer = null
  }
  if (pulseTimer) {
    clearInterval(pulseTimer)
    pulseTimer = null
  }
})
</script>

<style lang="scss" scoped>
.dashboard {
  .page-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 24px;
    padding: 20px 24px;
    background: #fff;
    border-radius: 8px;
    box-shadow: 0 1px 2px rgba(60, 64, 67, 0.1);

    .welcome-section {
      h1 {
        font-size: 22px;
        font-weight: 400;
        color: #202124;
        margin: 0 0 4px 0;
      }

      .welcome-sub {
        font-size: 13px;
        color: #5f6368;
        margin: 0;
      }
    }

    .header-actions {
      display: flex;
      align-items: center;
      gap: 12px;

      :deep(.el-button--primary) {
        background: #1A73E8;
        border-color: #1A73E8;
        border-radius: 4px;
        &:hover { background: #1557b0; }
      }
      :deep(.el-button--success) {
        background: #34A853;
        border-color: #34A853;
        border-radius: 4px;
        &:hover { background: #2d8f47; }
      }

      .update-time {
        font-size: 12px;
        color: #5f6368;
      }
    }
  }

  .stats-cards {
    margin-bottom: 24px;

    .stat-card {
      border-radius: 8px;
      border: none;
      box-shadow: 0 1px 2px rgba(60, 64, 67, 0.1);
      transition: box-shadow 0.2s;

      &:hover {
        box-shadow: 0 4px 12px rgba(60, 64, 67, 0.15);
      }

      .stat-content {
        display: flex;
        justify-content: space-between;
        align-items: center;

        .stat-info {
          .stat-value {
            font-size: 28px;
            font-weight: 400;
            color: #202124;
            line-height: 1.2;
          }

          .stat-title {
            font-size: 14px;
            color: #5f6368;
            margin-top: 8px;
          }
        }

        .stat-icon {
          width: 48px;
          height: 48px;
          border-radius: 50%;
          display: flex;
          align-items: center;
          justify-content: center;
        }
      }
    }
  }

  .charts-row {
    margin-bottom: 24px;

    .chart-card {
      border-radius: 8px;
      border: none;
      box-shadow: 0 1px 2px rgba(60, 64, 67, 0.1);

      .card-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
      }
    }
  }

  .topics-list {
    max-height: 300px;
    overflow-y: auto;

    .topic-item {
      display: flex;
      align-items: center;
      padding: 10px 12px;
      border-radius: 4px;
      margin-bottom: 8px;
      background: #f8f9fa;
      transition: background 0.2s;

      &:hover { background: #e8f0fe; }

      .topic-rank {
        width: 24px;
        height: 24px;
        border-radius: 50%;
        background: #e0e0e0;
        color: #5f6368;
        font-size: 12px;
        font-weight: 500;
        display: flex;
        align-items: center;
        justify-content: center;
        margin-right: 12px;

        &.top { background: #4285F4; color: #fff; }
      }

      .topic-name { flex: 1; color: #202124; font-size: 14px; }
      .topic-count { color: #5f6368; font-size: 13px; }
    }
  }

  .tech-badges {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    margin-bottom: 20px;

    .tech-badge {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 6px 14px;
      background: #fff;
      border: 1px solid #dadce0;
      border-radius: 20px;
      font-size: 13px;
      color: #202124;
      box-shadow: 0 1px 2px rgba(60, 64, 67, 0.08);
      transition: all 0.2s;

      &:hover {
        background: #e8f0fe;
        border-color: #4285F4;
        box-shadow: 0 2px 6px rgba(66, 133, 244, 0.15);
      }

      .badge-icon { font-size: 16px; }
      .badge-label { font-weight: 500; }
    }
  }

  .innovation-card {
    .innovation-stats {
      display: flex;
      flex-direction: column;
      gap: 16px;

      .inno-stat-item {
        display: flex;
        justify-content: space-between;
        align-items: center;

        .inno-label {
          font-size: 13px;
          color: #5f6368;
        }

        .inno-value {
          font-size: 18px;
          font-weight: 500;
          color: #202124;

          &.epsilon { font-family: 'Roboto Mono', monospace; font-size: 15px; }
          &.highlight { color: #1A73E8; }
        }
      }

      .budget-bar {
        :deep(.el-progress__text) {
          font-size: 12px !important;
          color: #5f6368;
        }
      }
    }

    .pulse-hint {
      font-size: 12px;
      color: #9AA0A6;
    }
  }
}
</style>
