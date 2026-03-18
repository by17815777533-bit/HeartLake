<!--
  湖面总览（Dashboard）页面 -- 数据大屏

  组件结构：
  - DashboardHeader: 问候语、时间、导出/刷新按钮、技术标签
  - StatsCards: 四张核心指标卡片
  - CareFeedbackSection: 热门话题 + 情绪趋势缩略
  - ChartsSection: 用户增长/情绪分布/情绪趋势/活跃时段四张图表
  - InnovationSection: 差分隐私/情绪共鸣/情绪温度
  - LakeWeatherSection: 湖面天气隐喻 + 心情饼图
  - AITrendsSection: AI 情绪趋势 + 热门内容推荐

  数据逻辑全部委托给 useDashboardData composable，
  本组件只负责轮询调度和生命周期管理。
  轮询策略：每分钟刷新时间文案，每 30s 拉取情绪脉搏，
  页面不可见时暂停轮询，恢复可见时立即刷新并重启。
-->

<template>
  <div class="dashboard">
    <DashboardHeader
      :greeting="greeting"
      :current-date-time="currentDateTime"
      :last-update-time="lastUpdateTime"
      :loading="loading"
      :tech-badges="techBadges"
      @export="exportData"
      @refresh="refreshData"
    />

    <StatsCards
      :stats-cards="statsCards"
      :format-number="formatNumber"
    />

    <CareFeedbackSection
      :stats-cards="statsCards"
      :trending-topics="trendingTopics"
      :emotion-trends-option="emotionTrendsOption"
    />

    <ChartsSection
      v-model:chart-range="chartRange"
      v-model:mood-trend-range="moodTrendRange"
      :trending-topics="trendingTopics"
      :user-growth-option="userGrowthOption"
      :mood-distribution-option="moodDistributionOption"
      :mood-trend-option="moodTrendOption"
      :active-time-option="activeTimeOption"
      @load-growth="loadGrowthData"
      @load-mood-trend="loadMoodTrend"
    />

    <InnovationSection
      :privacy-loading="privacyLoading"
      :resonance-loading="resonanceLoading"
      :privacy-stats="privacyStats"
      :privacy-budget-percent="privacyBudgetPercent"
      :privacy-budget-color="privacyBudgetColor"
      :resonance-stats="resonanceStats"
      :emotion-pulse-option="emotionPulseOption"
      :format-number="formatNumber"
    />

    <LakeWeatherSection
      :lake-weather="lakeWeather"
      :lake-weather-temp="lakeWeatherTemp"
      :weather-mood-pie-option="weatherMoodPieOption"
    />

    <AITrendsSection
      :emotion-trends-option="emotionTrendsOption"
      :ai-trending-content="aiTrendingContent"
    />
  </div>
</template>

<script setup lang="ts">
import { onMounted, onUnmounted } from 'vue'
import dayjs from 'dayjs'
import { useDashboardData } from '@/composables/useDashboardData'
import DashboardHeader from './dashboard/DashboardHeader.vue'
import StatsCards from './dashboard/StatsCards.vue'
import ChartsSection from './dashboard/ChartsSection.vue'
import InnovationSection from './dashboard/InnovationSection.vue'
import LakeWeatherSection from './dashboard/LakeWeatherSection.vue'
import AITrendsSection from './dashboard/AITrendsSection.vue'
import CareFeedbackSection from './dashboard/CareFeedbackSection.vue'

const {
  loading,
  privacyLoading,
  resonanceLoading,
  lastUpdateTime,
  chartRange,
  moodTrendRange,
  trendingTopics,
  aiTrendingContent,
  currentDateTime,
  techBadges,
  greeting,
  formatNumber,
  statsCards,
  privacyStats,
  privacyBudgetPercent,
  privacyBudgetColor,
  resonanceStats,
  lakeWeather,
  lakeWeatherTemp,
  weatherMoodPieOption,
  userGrowthOption,
  moodTrendOption,
  moodDistributionOption,
  activeTimeOption,
  emotionPulseOption,
  emotionTrendsOption,
  loadGrowthData,
  loadMoodTrend,
  loadEmotionPulse,
  exportData,
  refreshData,
} = useDashboardData()

let clockTimer: ReturnType<typeof setInterval> | null = null
let pulseTimer: ReturnType<typeof setInterval> | null = null

/**
 * 启动大屏轮询任务。
 * - 每分钟刷新时间文案
 * - 每 30 秒拉取一次情绪脉冲
 */
const startPolling = () => {
  if (!clockTimer) {
    clockTimer = setInterval(() => {
      currentDateTime.value = dayjs().format('YYYY年MM月DD日 dddd HH:mm')
    }, 60000)
  }
  if (!pulseTimer) {
    pulseTimer = setInterval(loadEmotionPulse, 30000)
  }
}

/** 关闭大屏轮询任务。 */
const stopPolling = () => {
  if (clockTimer) {
    clearInterval(clockTimer)
    clockTimer = null
  }
  if (pulseTimer) {
    clearInterval(pulseTimer)
    pulseTimer = null
  }
}

/** 页面可见性变化时控制轮询，避免后台页持续占用资源。 */
const handleVisibilityChange = () => {
  if (document.hidden) {
    stopPolling()
  } else {
    loadEmotionPulse()
    currentDateTime.value = dayjs().format('YYYY年MM月DD日 dddd HH:mm')
    startPolling()
  }
}

onMounted(() => {
  refreshData()
  startPolling()
  document.addEventListener('visibilitychange', handleVisibilityChange)
})

onUnmounted(() => {
  stopPolling()
  document.removeEventListener('visibilitychange', handleVisibilityChange)
})
</script>

<style lang="scss" scoped>
.dashboard {
  min-height: 100%;
  padding: 8px 0 18px;
}

.dashboard :deep(.el-card) {
  border-radius: 24px;
  border: 1px solid rgba(24, 36, 47, 0.08);
  background: linear-gradient(180deg, rgba(255, 250, 242, 0.94), rgba(247, 241, 232, 0.98));
  box-shadow: var(--hl-shadow-soft);
}

.dashboard :deep(.el-card__header) {
  border-bottom: 1px solid rgba(24, 36, 47, 0.08);
  color: var(--hl-ink);
  font-weight: 600;
  background: rgba(255, 255, 255, 0.38);
}

.dashboard :deep(.el-empty__description p) {
  color: var(--hl-ink-soft);
}

.dashboard :deep(.el-carousel__indicator-button) {
  width: 18px;
  height: 4px;
  border-radius: 999px;
  background: rgba(24, 36, 47, 0.18);
}

.dashboard :deep(.el-progress-bar__outer) {
  background: rgba(24, 36, 47, 0.08);
}
</style>
