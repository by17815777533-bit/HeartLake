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
      <el-col :xs="24" :sm="24" :md="12">
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

      <el-col :xs="24" :sm="24" :md="12">
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

    <!-- 湖面天气可视化 -->
    <el-row :gutter="20" class="charts-row">
      <el-col :xs="24" :sm="24" :md="10">
        <el-card shadow="hover" class="chart-card lake-weather-card">
          <template #header>
            <div class="card-header">
              <span>🌤️ 湖面天气</span>
              <el-tag size="small" :color="lakeWeather.color" effect="dark" style="border: none; color: #fff;">
                {{ lakeWeather.label }}
              </el-tag>
            </div>
          </template>
          <div class="lake-weather-content">
            <div class="weather-display" :style="{ background: lakeWeather.bg }">
              <div class="weather-icon">{{ lakeWeather.icon }}</div>
              <div class="weather-info">
                <div class="weather-temp">{{ lakeWeatherTemp.toFixed(0) }}°</div>
                <div class="weather-label">{{ lakeWeather.label }}</div>
                <div class="weather-desc">{{ lakeWeather.desc }}</div>
              </div>
            </div>
            <div class="weather-scale">
              <div class="scale-bar">
                <div class="scale-segment storm" style="width: 10%"></div>
                <div class="scale-segment rain" style="width: 20%"></div>
                <div class="scale-segment cloudy" style="width: 30%"></div>
                <div class="scale-segment sunny" style="width: 40%"></div>
                <div class="scale-pointer" :style="{ left: lakeWeatherTemp + '%' }"></div>
              </div>
              <div class="scale-labels">
                <span>暴风雨</span>
                <span>小雨</span>
                <span>多云</span>
                <span>晴朗</span>
              </div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="24" :md="14">
        <el-card shadow="hover" class="chart-card lake-weather-card">
          <template #header>
            <div class="card-header">
              <span>🎭 心情分布</span>
              <span class="pulse-hint">基于情绪脉搏数据</span>
            </div>
          </template>
          <v-chart :option="weatherMoodPieOption" autoresize style="height: 260px" />
        </el-card>
      </el-col>
    </el-row>

    <!-- AI 趋势分析 -->
    <el-row :gutter="20" style="margin-bottom: 20px">
      <el-col :xs="24" :sm="24" :md="14">
        <el-card shadow="hover" class="chart-card ai-trends-card">
          <template #header>
            <div class="card-header">
              <span>📈 AI 情绪趋势</span>
              <el-tag size="small" type="info">近7天</el-tag>
            </div>
          </template>
          <v-chart :option="emotionTrendsOption" autoresize style="height: 280px" />
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="24" :md="10">
        <el-card shadow="hover" class="chart-card ai-trending-card">
          <template #header>
            <div class="card-header">
              <span>🔥 AI 热门内容</span>
              <el-tag size="small" type="warning">推荐引擎</el-tag>
            </div>
          </template>
          <div class="trending-content-list">
            <div v-for="(item, i) in aiTrendingContent" :key="i" class="trending-item">
              <span class="trending-rank" :class="{ top: i < 3 }">{{ i + 1 }}</span>
              <div class="trending-info">
                <span class="trending-title">{{ item.title || item.content || item.text || '未知内容' }}</span>
                <div class="trending-meta">
                  <el-tag v-if="item.emotion || item.mood" size="small" :type="(item.emotion || item.mood) === 'positive' ? 'success' : (item.emotion || item.mood) === 'negative' ? 'danger' : 'info'">
                    {{ item.emotion || item.mood || '中性' }}
                  </el-tag>
                  <span v-if="item.score != null" class="trending-score">匹配度 {{ (item.score * 100).toFixed(0) }}%</span>
                </div>
              </div>
            </div>
            <el-empty v-if="!aiTrendingContent.length" description="暂无推荐数据" :image-size="60" />
          </div>
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

// 活跃时段图表配置 - Material Design 3 风格
const activeTimeOption = ref({
  tooltip: { trigger: 'axis', backgroundColor: '#2B2B2F', borderColor: '#44474E', textStyle: { color: '#E3E2E6' } },
  grid: { left: 50, right: 20, top: 20, bottom: 30 },
  xAxis: { type: 'category', data: Array.from({ length: 24 }, (_, i) => `${i}:00`), axisLine: { lineStyle: { color: '#C4C6CF' } }, axisLabel: { color: '#44474E' } },
  yAxis: { type: 'value', splitLine: { lineStyle: { color: '#C4C6CF', opacity: 0.2 } }, axisLabel: { color: '#44474E' } },
  series: [{ type: 'bar', data: [], itemStyle: { color: '#2E7D32', borderRadius: [2, 2, 0, 0] } }]
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
  if (privacyBudgetPercent.value < 50) return '#2E7D32'
  if (privacyBudgetPercent.value < 80) return '#E65100'
  return '#BA1A1A'
})

// AI 情绪趋势图表 - Material Design 3 风格
const emotionTrendsOption = ref({
  tooltip: { trigger: 'axis', backgroundColor: '#2B2B2F', borderColor: '#44474E', textStyle: { color: '#E3E2E6' } },
  legend: { data: ['积极', '中性', '消极'], bottom: 0, textStyle: { color: '#44474E' } },
  grid: { left: 50, right: 20, top: 20, bottom: 40 },
  xAxis: { type: 'category', data: [], axisLine: { lineStyle: { color: '#C4C6CF' } }, axisLabel: { color: '#44474E' } },
  yAxis: { type: 'value', splitLine: { lineStyle: { color: '#C4C6CF', opacity: 0.2 } }, axisLabel: { color: '#44474E' } },
  series: [
    { name: '积极', type: 'line', smooth: true, data: [], lineStyle: { color: '#2E7D32' }, itemStyle: { color: '#2E7D32' }, areaStyle: { color: { type: 'linear', x: 0, y: 0, x2: 0, y2: 1, colorStops: [{ offset: 0, color: 'rgba(46,125,50,0.25)' }, { offset: 1, color: 'rgba(46,125,50,0.02)' }] } } },
    { name: '中性', type: 'line', smooth: true, data: [], lineStyle: { color: '#E65100' }, itemStyle: { color: '#E65100' }, areaStyle: { color: { type: 'linear', x: 0, y: 0, x2: 0, y2: 1, colorStops: [{ offset: 0, color: 'rgba(230,81,0,0.25)' }, { offset: 1, color: 'rgba(230,81,0,0.02)' }] } } },
    { name: '消极', type: 'line', smooth: true, data: [], lineStyle: { color: '#BA1A1A' }, itemStyle: { color: '#BA1A1A' }, areaStyle: { color: { type: 'linear', x: 0, y: 0, x2: 0, y2: 1, colorStops: [{ offset: 0, color: 'rgba(186,26,26,0.25)' }, { offset: 1, color: 'rgba(186,26,26,0.02)' }] } } },
  ]
})

// AI 热门推荐内容
const aiTrendingContent = ref([])

// 情绪共鸣统计
const resonanceStats = reactive({
  todayMatches: 0,
  avgScore: 0,
  topMood: '',
  successRate: 0,
})

// 湖面天气可视化 - 基于情绪温度映射
const lakeWeather = computed(() => {
  const temp = emotionPulseOption.value.series[0].data[0]?.value ?? 50
  if (temp > 60) return { icon: '\u2600\uFE0F', label: '晴朗', desc: '社区积极', color: '#E65100', bg: 'linear-gradient(135deg, #FFF3E0 0%, #FFB74D 100%)' }
  if (temp > 30) return { icon: '\u26C5', label: '多云', desc: '社区平稳', color: '#44474E', bg: 'linear-gradient(135deg, #ECEFF1 0%, #B0BEC5 100%)' }
  if (temp > 10) return { icon: '\uD83C\uDF27\uFE0F', label: '小雨', desc: '社区低落', color: '#1565C0', bg: 'linear-gradient(135deg, #E3F2FD 0%, #64B5F6 100%)' }
  return { icon: '\u26C8\uFE0F', label: '暴风雨', desc: '社区消极', color: '#BA1A1A', bg: 'linear-gradient(135deg, #FFEBEE 0%, #E57373 100%)' }
})

const lakeWeatherTemp = computed(() => emotionPulseOption.value.series[0].data[0]?.value ?? 50)

// 湖面天气心情分布饼图 - Material Design 3 风格
const weatherMoodPieOption = computed(() => {
  const moodData = moodDistributionOption.value.series[0].data
  return {
    tooltip: {
      trigger: 'item',
      backgroundColor: '#2B2B2F',
      borderColor: '#44474E',
      borderRadius: 4,
      padding: [6, 10],
      textStyle: { color: '#E3E2E6', fontSize: 12 },
      formatter: (p) => {
        const name = String(p.name).replace(/[<>&"']/g, c => ({'<':'&lt;','>':'&gt;','&':'&amp;','"':'&quot;',"'":'&#39;'}[c]))
        return `${p.marker} ${name}: ${Number(p.percent).toFixed(1)}%`
      }
    },
    series: [{
      type: 'pie',
      radius: ['50%', '75%'],
      center: ['50%', '50%'],
      itemStyle: { borderRadius: 3, borderColor: '#fff', borderWidth: 1 },
      label: { show: false },
      emphasis: {
        label: { show: true, fontSize: 11, fontWeight: '500', color: '#1C1B1F' },
        itemStyle: { shadowBlur: 6, shadowColor: 'rgba(0,0,0,0.15)' }
      },
      data: moodData.length ? moodData : moodNames.map((name, i) => ({
        value: [30, 25, 20, 15, 10][i],
        name,
        itemStyle: { color: moodColors[i] }
      }))
    }]
  }
})

// 湖面情绪温度 - 仪表盘 - Material Design 3 风格
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
          [0.3, '#1565C0'],
          [0.5, '#2E7D32'],
          [0.7, '#E65100'],
          [1, '#BA1A1A'],
        ]
      }
    },
    pointer: { icon: 'path://M12.8,0.7l12,40.1H0.7L12.8,0.7z', length: '55%', width: 8, offsetCenter: [0, '-10%'], itemStyle: { color: 'auto' } },
    axisTick: { length: 6, lineStyle: { color: 'auto', width: 1 } },
    splitLine: { length: 12, lineStyle: { color: 'auto', width: 2 } },
    axisLabel: { color: '#44474E', fontSize: 10, distance: -40 },
    title: { offsetCenter: [0, '20%'], fontSize: 14, color: '#44474E' },
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

// 统计卡片 - Material Design 3 配色
const statsCards = computed(() => [
  { title: '总用户数', value: stats.totalUsers, color: '#1565C0', icon: 'User' },
  { title: '今日投石', value: stats.todayStones, color: '#2E7D32', icon: 'Edit' },
  { title: '在线人数', value: stats.onlineCount, color: '#6E5676', icon: 'Connection' },
  { title: '待处理举报', value: stats.pendingReports, color: '#E65100', icon: 'Warning' },
])

// 用户增长图表配置 - Material Design 3 风格
const userGrowthOption = ref({
  tooltip: {
    trigger: 'axis',
    backgroundColor: '#2B2B2F',
    borderColor: '#44474E',
    textStyle: { color: '#E3E2E6' },
    formatter: (params) => {
      const p = params[0]
      const name = String(p.name).replace(/[<>&"']/g, c => ({'<':'&lt;','>':'&gt;','&':'&amp;','"':'&quot;',"'":'&#39;'}[c]))
      return `<div style="font-weight:500">${name}</div><div style="color:#A8C8FF">新增 ${Number(p.value)} 人</div>`
    }
  },
  grid: { left: 50, right: 20, top: 20, bottom: 30 },
  xAxis: { type: 'category', data: [], axisLine: { lineStyle: { color: '#C4C6CF' } }, axisLabel: { color: '#44474E' } },
  yAxis: { type: 'value', splitLine: { lineStyle: { color: '#C4C6CF', opacity: 0.2 } }, axisLabel: { color: '#44474E' } },
  series: [{
    name: '新增用户',
    type: 'line',
    smooth: true,
    symbol: 'circle',
    symbolSize: 6,
    data: [],
    areaStyle: { opacity: 0.2, color: '#1565C0' },
    itemStyle: { color: '#1565C0', borderWidth: 2, borderColor: '#fff' },
    lineStyle: { color: '#1565C0', width: 2 },
  }],
})

// Material Design 3 配色
const moodColors = ['#1565C0', '#2E7D32', '#BA1A1A', '#E65100', '#44474E']
const moodNames = ['开心', '平静', '难过', '焦虑', '其他']
const moodGradients = [
  { start: '#1565C0', end: '#0D47A1' },
  { start: '#2E7D32', end: '#1B5E20' },
  { start: '#BA1A1A', end: '#8B0000' },
  { start: '#E65100', end: '#BF360C' },
  { start: '#44474E', end: '#2B2B2F' },
]

// 情绪趋势图表配置 - Material Design 3 风格
const moodTrendOption = ref({
  tooltip: {
    trigger: 'axis',
    backgroundColor: '#2B2B2F',
    borderColor: '#44474E',
    borderRadius: 4,
    padding: [8, 12],
    textStyle: { color: '#E3E2E6' }
  },
  legend: { bottom: 0, itemGap: 16, textStyle: { color: '#44474E', fontSize: 12 } },
  grid: { left: 50, right: 20, top: 20, bottom: 50 },
  xAxis: { type: 'category', data: [], axisLine: { lineStyle: { color: '#C4C6CF' } }, axisLabel: { color: '#44474E' } },
  yAxis: { type: 'value', splitLine: { lineStyle: { color: '#C4C6CF', opacity: 0.2 } }, axisLabel: { color: '#44474E' } },
  series: moodNames.map((name, i) => ({
    name, type: 'line', smooth: 0.3, symbol: 'circle', symbolSize: 5, data: [],
    itemStyle: { color: moodColors[i], borderWidth: 2, borderColor: '#fff' },
    lineStyle: { width: 2 },
    areaStyle: { opacity: 0.1, color: moodColors[i] }
  }))
})

// 情绪分布图表配置 - Material Design 3 风格
const moodDistributionOption = ref({
  tooltip: {
    trigger: 'item',
    backgroundColor: '#2B2B2F',
    borderColor: '#44474E',
    borderRadius: 4,
    padding: [8, 12],
    textStyle: { color: '#E3E2E6' },
    formatter: (p) => {
      const name = String(p.name).replace(/[<>&"']/g, c => ({'<':'&lt;','>':'&gt;','&':'&amp;','"':'&quot;',"'":'&#39;'}[c]))
      return `<div style="font-weight:500">${p.marker} ${name}</div><div style="color:#BCC7DC">${Number(p.value)} 条 · ${Number(p.percent)}%</div>`
    }
  },
  legend: { bottom: 0, itemGap: 16, textStyle: { color: '#44474E', fontSize: 12 } },
  series: [{
    type: 'pie',
    radius: ['45%', '70%'],
    center: ['50%', '42%'],
    itemStyle: { borderRadius: 4, borderColor: '#fff', borderWidth: 2 },
    label: { show: false },
    emphasis: {
      label: { show: true, fontSize: 14, fontWeight: '500', color: '#1C1B1F' },
      itemStyle: { shadowBlur: 10, shadowColor: 'rgba(0,0,0,0.2)' }
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
    // 兼容后端响应格式: axios res.data 是 {code, data: {...}}，实际数据在 .data.data
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

// 加载增长数据（后端返回原始数组，非 {list: [...]}）
const loadGrowthData = async () => {
  try {
    const res = await api.getUserGrowthStats(chartRange.value)
    const raw = res.data?.data || res.data
    const list = Array.isArray(raw) ? raw : (raw?.list || [])
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
    const raw = res.data?.data || res.data
    const list = Array.isArray(raw) ? raw : (raw?.list || [])
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
    const trendData = res.data?.data || res.data
    const trendList = trendData?.list || (Array.isArray(trendData) ? trendData : [])
    if (trendList.length) {
      const dates = [...new Set(trendList.map(item => item.date))].sort()
      moodTrendOption.value.xAxis.data = dates
      moodNames.forEach((mood, i) => {
        moodTrendOption.value.series[i].data = dates.map(date => {
          const item = trendList.find(d => d.date === date && d.mood === mood)
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
    const topicData = res.data?.data || res.data
    if (topicData) trendingTopics.value = Array.isArray(topicData) ? topicData : (topicData.list || [])
  } catch (e) {
    console.warn('加载热门话题失败:', e.message)
    ElMessage.error('加载热门话题失败，请稍后重试')
  }
}

// 加载活跃时段
const loadActiveTimeStats = async () => {
  try {
    const res = await api.getActiveTimeStats?.() || { data: null }
    const timeData = res.data?.data || res.data
    if (timeData) activeTimeOption.value.series[0].data = (Array.isArray(timeData) ? timeData : timeData.list || []).map(item => item.count)
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

// 加载情绪共鸣统计
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

// 加载湖面情绪温度
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

// 加载 AI 情绪趋势
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

// 加载 AI 热门推荐内容
const loadAITrendingContent = async () => {
  try {
    const res = await api.getTrendingContent()
    const d = res.data?.data || res.data
    aiTrendingContent.value = Array.isArray(d) ? d.slice(0, 10) : (d?.list || []).slice(0, 10)
  } catch (e) {
    console.warn('加载AI热门内容失败:', e.message)
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
  await Promise.allSettled([loadStats(), loadGrowthData(), loadMoodDistribution(), loadMoodTrend(), loadTrendingTopics(), loadActiveTimeStats(), loadPrivacyStats(), loadResonanceStats(), loadEmotionPulse(), loadEmotionTrends(), loadAITrendingContent()])
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
/* ═══════════════════════════════════════════════════
   Dashboard 样式 - Material Design 3 主题
   ═══════════════════════════════════════════════════ */

.dashboard {
  // ── 页面头部 ──
  .page-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 24px;
    padding: 20px 24px;
    background: var(--m3-surface-container);
    border: 1px solid var(--m3-outline-variant);
    border-radius: var(--m3-shape-lg);
    box-shadow: var(--m3-elevation-1);

    .welcome-section {
      h1 {
        font-size: 24px;
        font-weight: 600;
        margin: 0 0 4px 0;
        color: var(--m3-on-surface);
      }

      .welcome-sub {
        font-size: 13px;
        color: var(--m3-on-surface-variant);
        margin: 0;
      }
    }

    .header-actions {
      display: flex;
      align-items: center;
      gap: 12px;

      .update-time {
        font-size: 12px;
        color: var(--m3-on-surface-variant);
      }
    }
  }

  // ── 技术标签 ──
  .tech-badges {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    margin-bottom: 24px;

    .tech-badge {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 6px 14px;
      background: var(--m3-surface-container-high);
      border: 1px solid var(--m3-outline-variant);
      border-radius: var(--m3-shape-md);
      font-size: 12px;
      color: var(--m3-on-surface-variant);
      transition: all var(--m3-transition-duration);

      &:hover {
        background: var(--m3-surface-container-highest);
        border-color: var(--m3-outline);
        box-shadow: var(--m3-elevation-1);
      }

      .badge-icon {
        font-size: 14px;
      }
    }
  }

  // ── 统计卡片 ──
  .stats-cards {
    margin-bottom: 20px;

    .stat-card {
      background: var(--m3-surface-container);
      border: 1px solid var(--m3-outline-variant);
      border-radius: var(--m3-shape-lg);
      box-shadow: var(--m3-elevation-1);
      transition: all var(--m3-transition-duration);

      &:hover {
        box-shadow: var(--m3-elevation-2);
        transform: translateY(-2px);
      }

      :deep(.el-card__body) {
        padding: 20px;
      }

      .stat-content {
        display: flex;
        justify-content: space-between;
        align-items: center;

        .stat-info {
          .stat-value {
            font-size: 28px;
            font-weight: 700;
            color: var(--m3-on-surface);
            margin-bottom: 4px;
          }

          .stat-title {
            font-size: 13px;
            color: var(--m3-on-surface-variant);
          }
        }

        .stat-icon {
          width: 56px;
          height: 56px;
          display: flex;
          align-items: center;
          justify-content: center;
          border-radius: var(--m3-shape-lg);
        }
      }
    }
  }

  // ── 图表卡片 ──
  .charts-row {
    margin-bottom: 20px;
  }

  .chart-card {
    background: var(--m3-surface-container);
    border: 1px solid var(--m3-outline-variant);
    border-radius: var(--m3-shape-lg);
    box-shadow: var(--m3-elevation-1);
    transition: all var(--m3-transition-duration);

    &:hover {
      box-shadow: var(--m3-elevation-2);
    }

    :deep(.el-card__header) {
      padding: 16px 20px;
      border-bottom: 1px solid var(--m3-outline-variant);
      font-weight: 600;
      font-size: 15px;
      color: var(--m3-on-surface);

      .card-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
      }
    }

    :deep(.el-card__body) {
      padding: 20px;
    }
  }

  // ── 热门话题列表 ──
  .trending-topics-list {
    .topic-item {
      display: flex;
      align-items: center;
      gap: 12px;
      padding: 12px 0;
      border-bottom: 1px solid var(--m3-outline-variant);
      transition: all var(--m3-transition-duration);

      &:last-child {
        border-bottom: none;
      }

      &:hover {
        background: var(--m3-surface-container-high);
        margin: 0 -12px;
        padding: 12px;
        border-radius: var(--m3-shape-sm);
      }

      .topic-rank {
        width: 24px;
        height: 24px;
        display: flex;
        align-items: center;
        justify-content: center;
        border-radius: var(--m3-shape-sm);
        background: var(--m3-surface-container-highest);
        color: var(--m3-on-surface-variant);
        font-size: 12px;
        font-weight: 600;
        flex-shrink: 0;

        &.top {
          background: var(--m3-primary);
          color: var(--m3-on-primary);
        }
      }

      .topic-info {
        flex: 1;
        min-width: 0;

        .topic-title {
          font-size: 14px;
          color: var(--m3-on-surface);
          font-weight: 500;
          white-space: nowrap;
          overflow: hidden;
          text-overflow: ellipsis;
        }

        .topic-meta {
          display: flex;
          align-items: center;
          gap: 8px;
          margin-top: 4px;
          font-size: 12px;
          color: var(--m3-on-surface-variant);
        }
      }
    }
  }

  // ── 隐私保护卡片 ──
  .privacy-card {
    .privacy-stats {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 16px;
      margin-bottom: 16px;

      .privacy-stat-item {
        text-align: center;
        padding: 12px;
        background: var(--m3-surface-container-high);
        border-radius: var(--m3-shape-md);

        .stat-value {
          font-size: 20px;
          font-weight: 700;
          color: var(--m3-on-surface);
          margin-bottom: 4px;
        }

        .stat-label {
          font-size: 12px;
          color: var(--m3-on-surface-variant);
        }
      }
    }

    .budget-bar {
      margin-top: 16px;

      .budget-label {
        display: flex;
        justify-content: space-between;
        margin-bottom: 8px;
        font-size: 13px;
        color: var(--m3-on-surface-variant);
      }
    }
  }

  // ── 情绪共鸣卡片 ──
  .resonance-card {
    .resonance-stats {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 16px;

      .resonance-stat-item {
        text-align: center;
        padding: 12px;
        background: var(--m3-surface-container-high);
        border-radius: var(--m3-shape-md);

        .stat-value {
          font-size: 20px;
          font-weight: 700;
          color: var(--m3-on-surface);
          margin-bottom: 4px;
        }

        .stat-label {
          font-size: 12px;
          color: var(--m3-on-surface-variant);
        }
      }
    }
  }

  // ── 湖面天气卡片 ──
  .lake-weather-card {
    .lake-weather-content {
      .weather-display {
        padding: 24px;
        border-radius: var(--m3-shape-lg);
        margin-bottom: 16px;
        display: flex;
        align-items: center;
        gap: 20px;

        .weather-icon {
          font-size: 48px;
        }

        .weather-info {
          flex: 1;

          .weather-temp {
            font-size: 36px;
            font-weight: 700;
            color: var(--m3-on-surface);
          }

          .weather-label {
            font-size: 16px;
            font-weight: 600;
            color: var(--m3-on-surface);
            margin-top: 4px;
          }

          .weather-desc {
            font-size: 13px;
            color: var(--m3-on-surface-variant);
            margin-top: 2px;
          }
        }
      }

      .weather-scale {
        .scale-bar {
          position: relative;
          height: 8px;
          border-radius: 4px;
          overflow: hidden;
          display: flex;
          margin-bottom: 8px;

          .scale-segment {
            height: 100%;

            &.storm { background: #BA1A1A; }
            &.rain { background: #1565C0; }
            &.cloudy { background: #44474E; }
            &.sunny { background: #E65100; }
          }

          .scale-pointer {
            position: absolute;
            top: -4px;
            width: 16px;
            height: 16px;
            background: var(--m3-on-surface);
            border: 2px solid var(--m3-surface);
            border-radius: 50%;
            transform: translateX(-50%);
            box-shadow: var(--m3-elevation-2);
          }
        }

        .scale-labels {
          display: flex;
          justify-content: space-between;
          font-size: 11px;
          color: var(--m3-on-surface-variant);
        }
      }
    }
  }

  // ── AI 趋势卡片 ──
  .ai-trends-card, .ai-trending-card {
    .trending-content-list {
      .trending-item {
        display: flex;
        align-items: center;
        gap: 12px;
        padding: 12px 0;
        border-bottom: 1px solid var(--m3-outline-variant);
        transition: all var(--m3-transition-duration);

        &:last-child {
          border-bottom: none;
        }

        &:hover {
          background: var(--m3-surface-container-high);
          margin: 0 -12px;
          padding: 12px;
          border-radius: var(--m3-shape-sm);
        }

        .trending-rank {
          width: 24px;
          height: 24px;
          display: flex;
          align-items: center;
          justify-content: center;
          border-radius: var(--m3-shape-sm);
          background: var(--m3-surface-container-highest);
          color: var(--m3-on-surface-variant);
          font-size: 12px;
          font-weight: 600;
          flex-shrink: 0;

          &.top {
            background: var(--m3-primary);
            color: var(--m3-on-primary);
          }
        }

        .trending-info {
          flex: 1;
          min-width: 0;

          .trending-title {
            font-size: 14px;
            color: var(--m3-on-surface);
            font-weight: 500;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
          }

          .trending-meta {
            display: flex;
            align-items: center;
            gap: 8px;
            margin-top: 4px;

            .trending-score {
              font-size: 12px;
              color: var(--m3-on-surface-variant);
            }
          }
        }
      }
    }
  }

  .pulse-hint {
    font-size: 12px;
    color: var(--m3-on-surface-variant);
  }
}
</style>

// 毛玻璃混入
@mixin glass-card($bg: rgba(26, 26, 62, 0.7), $blur: 16px, $border-alpha: 0.1) {
  background: $bg;
  backdrop-filter: blur($blur);
  -webkit-backdrop-filter: blur($blur);
  border: 1px solid rgba(242, 204, 143, $border-alpha);
  border-radius: 16px;
}

// 金色渐变文字
@mixin gold-text {
  background: linear-gradient(135deg, $sky-gold, $sky-amber);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.dashboard {
  // ── 页面头部 ──
  .page-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 24px;
    padding: 20px 24px;
    background: transparent;
    border-radius: 16px;

    .welcome-section {
      h1 {
        font-size: 24px;
        font-weight: 600;
        margin: 0 0 4px 0;
        @include gold-text;
      }

      .welcome-sub {
        font-size: 13px;
        color: $sky-text-secondary;
        margin: 0;
      }
    }

    .header-actions {
      display: flex;
      align-items: center;
      gap: 12px;

      :deep(.el-button--primary) {
        background: linear-gradient(135deg, $sky-gold, $sky-amber);
        border-color: transparent;
        border-radius: 20px;
        color: $sky-bg-base;
        font-weight: 600;
        &:hover {
          background: linear-gradient(135deg, $sky-amber, $sky-sunset);
          box-shadow: 0 0 16px rgba(242, 204, 143, 0.3);
        }
      }
      :deep(.el-button--success) {
        background: linear-gradient(135deg, rgba(242, 204, 143, 0.15), rgba(232, 168, 124, 0.15));
        border: 1px solid rgba(242, 204, 143, 0.25);
        border-radius: 20px;
        color: $sky-gold;
        &:hover {
          background: linear-gradient(135deg, rgba(242, 204, 143, 0.25), rgba(232, 168, 124, 0.25));
          box-shadow: 0 0 16px rgba(242, 204, 143, 0.2);
        }
      }

      .update-time {
        font-size: 12px;
        color: $sky-text-secondary;
      }
    }
  }

  // ── 技术标签 ──
  .tech-badges {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    margin-bottom: 24px;

    .tech-badge {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 6px 16px;
      background: rgba(26, 26, 62, 0.6);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border: 1px solid rgba(242, 204, 143, 0.12);
      border-radius: 20px;
      font-size: 12px;
      color: $sky-gold;
      transition: all 0.3s ease;

      &:hover {
        border-color: rgba(242, 204, 143, 0.3);
        box-shadow: 0 0 12px rgba(242, 204, 143, 0.1);
      }

      .badge-icon {
        font-size: 14px;
      }

      .badge-label {
        font-weight: 500;
      }
    }
  }

  // ── 统计卡片 ──
  .stats-cards {
    margin-bottom: 24px;

    .stat-card {
      @include glass-card;
      border-top: none !important;
      position: relative;
      overflow: hidden;
      transition: all 0.35s ease;

      // 左侧金色渐变条
      &::before {
        content: '';
        position: absolute;
        left: 0;
        top: 0;
        bottom: 0;
        width: 3px;
        background: linear-gradient(180deg, $sky-gold, $sky-amber);
        border-radius: 16px 0 0 16px;
      }

      &:hover {
        transform: translateY(-4px);
        box-shadow: 0 0 20px rgba(242, 204, 143, 0.15);
        border-color: rgba(242, 204, 143, 0.2);
      }

      :deep(.el-card__body) {
        padding: 20px;
      }

      .stat-content {
        display: flex;
        justify-content: space-between;
        align-items: center;

        .stat-info {
          .stat-value {
            font-size: 28px;
            font-weight: 600;
            line-height: 1.2;
            @include gold-text;
          }

          .stat-title {
            font-size: 14px;
            color: $sky-text-secondary;
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
          background: rgba(242, 204, 143, 0.1) !important;
          color: $sky-gold !important;
        }
      }
    }
  }

  // ── 图表卡片 ──
  .charts-row {
    margin-bottom: 24px;

    .chart-card {
      @include glass-card(rgba(26, 26, 62, 0.6), 16px, 0.08);

      :deep(.el-card__header) {
        border-bottom: 1px solid rgba(242, 204, 143, 0.08);
        color: $sky-gold;
        font-weight: 500;
      }

      :deep(.el-card__body) {
        background: transparent;
      }

      .card-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        color: $sky-gold;

        :deep(.el-radio-group) {
          .el-radio-button__inner {
            background: rgba(242, 204, 143, 0.08);
            border-color: rgba(242, 204, 143, 0.15);
            color: $sky-text-secondary;
          }
          .el-radio-button__original-radio:checked + .el-radio-button__inner {
            background: linear-gradient(135deg, $sky-gold, $sky-amber);
            border-color: transparent;
            color: $sky-bg-base;
            box-shadow: none;
          }
        }

        :deep(.el-tag) {
          background: rgba(242, 204, 143, 0.1);
          border-color: rgba(242, 204, 143, 0.2);
          color: $sky-gold;
        }
      }
    }
  }

  // ── 热门话题列表 ──
  .topics-list {
    max-height: 300px;
    overflow-y: auto;

    .topic-item {
      display: flex;
      align-items: center;
      padding: 10px 12px;
      border-radius: 10px;
      margin-bottom: 8px;
      background: rgba(26, 26, 62, 0.4);
      transition: all 0.25s ease;

      &:hover {
        background: rgba(242, 204, 143, 0.06);
      }

      .topic-rank {
        width: 24px;
        height: 24px;
        border-radius: 50%;
        background: rgba(242, 204, 143, 0.1);
        color: $sky-text-secondary;
        font-size: 12px;
        font-weight: 500;
        display: flex;
        align-items: center;
        justify-content: center;
        margin-right: 12px;

        &.top {
          background: linear-gradient(135deg, $sky-gold, $sky-amber);
          color: $sky-bg-base;
        }
      }

      .topic-name { flex: 1; color: $sky-text-primary; font-size: 14px; }
      .topic-count { color: $sky-text-secondary; font-size: 13px; }
    }
  }

  // ── 创新卡片 (隐私保护 / 情绪共鸣 / 情绪脉搏) ──
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
          color: $sky-text-secondary;
        }

        .inno-value {
          font-size: 18px;
          font-weight: 500;
          @include gold-text;

          &.epsilon {
            font-family: 'Roboto Mono', monospace;
            font-size: 15px;
          }
          &.highlight {
            background: linear-gradient(135deg, $sky-amber, $sky-sunset);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
          }
        }
      }

      .budget-bar {
        :deep(.el-progress-bar__outer) {
          background: rgba(242, 204, 143, 0.1);
        }
        :deep(.el-progress__text) {
          font-size: 12px !important;
          color: $sky-text-secondary;
        }
      }
    }

    .pulse-hint {
      font-size: 12px;
      color: $sky-text-secondary;
    }
  }

  // ── 心湖天气卡片 ──
  .lake-weather-card {
    .lake-weather-content {
      display: flex;
      flex-direction: column;
      gap: 20px;

      .weather-display {
        display: flex;
        align-items: center;
        gap: 20px;
        padding: 24px;
        border-radius: 12px;
        background: rgba(242, 204, 143, 0.04);

        .weather-icon {
          font-size: 64px;
          line-height: 1;
        }

        .weather-info {
          .weather-temp {
            font-size: 42px;
            font-weight: 300;
            line-height: 1;
            @include gold-text;
          }

          .weather-label {
            font-size: 18px;
            font-weight: 500;
            color: $sky-text-primary;
            margin-top: 4px;
          }

          .weather-desc {
            font-size: 13px;
            color: $sky-text-secondary;
            margin-top: 2px;
          }
        }
      }

      .weather-scale {
        padding: 0 4px;

        .scale-bar {
          position: relative;
          display: flex;
          height: 8px;
          border-radius: 4px;
          overflow: hidden;

          .scale-segment {
            height: 100%;
            &.storm { background: $sky-sunset; }
            &.rain { background: $sky-purple; }
            &.cloudy { background: $sky-text-secondary; }
            &.sunny { background: $sky-gold; }
          }

          .scale-pointer {
            position: absolute;
            top: -4px;
            width: 4px;
            height: 16px;
            background: $sky-text-primary;
            border-radius: 2px;
            transform: translateX(-50%);
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3);
          }
        }

        .scale-labels {
          display: flex;
          justify-content: space-between;
          margin-top: 6px;
          font-size: 11px;
          color: $sky-text-secondary;
        }
      }
    }
  }

  // ── AI 趋势卡片 ──
  .ai-trends-card, .ai-trending-card {
    .trending-content-list {
      max-height: 320px;
      overflow-y: auto;

      .trending-item {
        display: flex;
        align-items: flex-start;
        padding: 10px 0;
        border-bottom: 1px solid rgba(242, 204, 143, 0.06);
        transition: background 0.25s ease;

        &:last-child { border-bottom: none; }

        &:hover {
          background: rgba(242, 204, 143, 0.04);
        }

        .trending-rank {
          flex-shrink: 0;
          width: 24px;
          height: 24px;
          border-radius: 6px;
          background: rgba(242, 204, 143, 0.1);
          color: $sky-text-secondary;
          font-size: 12px;
          font-weight: 600;
          display: flex;
          align-items: center;
          justify-content: center;
          margin-right: 12px;
          margin-top: 2px;

          &.top {
            background: linear-gradient(135deg, $sky-gold, $sky-amber);
            color: $sky-bg-base;
          }
        }

        .trending-info {
          flex: 1;
          min-width: 0;

          .trending-title {
            display: block;
            font-size: 13px;
            color: $sky-text-primary;
            line-height: 1.5;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
          }

          .trending-meta {
            display: flex;
            align-items: center;
            gap: 8px;
            margin-top: 4px;

            :deep(.el-tag) {
              background: rgba(242, 204, 143, 0.1);
              border-color: rgba(242, 204, 143, 0.2);
              color: $sky-gold;
            }

            .trending-score {
              font-size: 11px;
              color: $sky-text-secondary;
            }
          }
        }
      }
    }
  }
}

// ── Element Plus 全局覆盖 (scoped deep) ──
.dashboard {
  :deep(.el-card) {
    background: rgba(26, 26, 62, 0.6);
    border: 1px solid rgba(242, 204, 143, 0.08);
    border-radius: 16px;
    color: $sky-text-primary;
    --el-card-bg-color: transparent;
  }

  :deep(.el-card__header) {
    border-bottom: 1px solid rgba(242, 204, 143, 0.08);
    color: $sky-gold;
  }

  :deep(.el-empty__description p) {
    color: $sky-text-secondary;
  }

  :deep(.el-loading-mask) {
    background: rgba(20, 20, 50, 0.7);
  }
}

// ── 亮色模式覆盖 ──
// 保持金色主题色，卡片白色半透明，文字深棕色
html:not(.dark) .dashboard {
  $light-text: #3D2B1F;
  $light-text-secondary: #7A6B5D;
  $light-card-bg: rgba(255, 255, 255, 0.75);
  $light-card-border: rgba(242, 204, 143, 0.2);

  .page-header {
    .welcome-section .welcome-sub { color: $light-text-secondary; }
    .header-actions .update-time { color: $light-text-secondary; }
  }

  .tech-badges .tech-badge {
    background: rgba(255, 255, 255, 0.6);
    border-color: $light-card-border;
    backdrop-filter: blur(12px);
  }

  .stats-cards .stat-card {
    background: $light-card-bg;
    border-color: $light-card-border;
    backdrop-filter: blur(16px);

    .stat-content {
      .stat-title { color: $light-text-secondary; }
      .stat-icon {
        background: rgba(242, 204, 143, 0.15) !important;
      }
    }
  }

  .charts-row .chart-card {
    background: $light-card-bg;
    border-color: $light-card-border;
    backdrop-filter: blur(16px);

    .card-header {
      :deep(.el-radio-group) {
        .el-radio-button__inner {
          background: rgba(242, 204, 143, 0.1);
          border-color: rgba(242, 204, 143, 0.2);
          color: $light-text-secondary;
        }
      }
    }
  }

  .topics-list .topic-item {
    background: rgba(255, 255, 255, 0.5);
    .topic-name { color: $light-text; }
    .topic-count { color: $light-text-secondary; }
    .topic-rank { background: rgba(242, 204, 143, 0.15); color: $light-text-secondary; }
  }

  .innovation-card .innovation-stats {
    .inno-label { color: $light-text-secondary; }
    .budget-bar :deep(.el-progress__text) { color: $light-text-secondary; }
  }

  .lake-weather-card .lake-weather-content {
    .weather-display { background: rgba(242, 204, 143, 0.06); }
    .weather-info {
      .weather-label { color: $light-text; }
      .weather-desc { color: $light-text-secondary; }
    }
    .weather-scale .scale-labels { color: $light-text-secondary; }
  }

  .ai-trends-card, .ai-trending-card {
    .trending-item {
      border-bottom-color: rgba(242, 204, 143, 0.1);
      .trending-info {
        .trending-title { color: $light-text; }
        .trending-meta .trending-score { color: $light-text-secondary; }
      }
    }
  }

  :deep(.el-card) {
    background: $light-card-bg;
    border-color: $light-card-border;
    color: $light-text;
  }

  :deep(.el-card__header) {
    border-bottom-color: rgba(242, 204, 143, 0.12);
  }

  :deep(.el-empty__description p) { color: $light-text-secondary; }
  :deep(.el-loading-mask) { background: rgba(255, 255, 255, 0.7); }

  .pulse-hint { color: $light-text-secondary; }
}
</style>
