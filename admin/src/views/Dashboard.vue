<!--
  湖面总览（Dashboard）页面

  统一到新的值守台视觉语言：
  - 页首改为叙事型 Hero + 态势条
  - 中部增加湖面信号带，强调天气、时间和刷新状态
  - 图表与反馈卡片统一为雾面卡片和编辑感排版
-->

<template>
  <div class="dashboard ops-page">
    <OpsPageHero
      eyebrow="今日湖面"
      title="湖面总览"
      :description="heroDescription"
      status="值守中"
      :chips="heroChips"
    >
      <template #actions>
        <el-button
          plain
          :icon="Download"
          @click="exportData"
        >
          导出概览
        </el-button>
        <el-button
          type="primary"
          :icon="RefreshRight"
          :loading="loading"
          @click="refreshData"
        >
          刷新湖面
        </el-button>
      </template>
    </OpsPageHero>

    <OpsMetricStrip :items="summaryItems" />

    <section class="dashboard-ribbon">
      <article
        v-for="item in lakeSignals"
        :key="item.label"
        class="dashboard-ribbon__panel"
      >
        <span class="dashboard-ribbon__label">{{ item.label }}</span>
        <strong>{{ item.value }}</strong>
        <p>{{ item.note }}</p>
      </article>
    </section>

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
import { computed, onMounted, onUnmounted } from 'vue'
import dayjs from 'dayjs'
import { Download, RefreshRight } from '@element-plus/icons-vue'
import { useDashboardData } from '@/composables/useDashboardData'
import OpsPageHero from '@/components/OpsPageHero.vue'
import OpsMetricStrip from '@/components/OpsMetricStrip.vue'
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

const dashboardTimeFormat = 'YYYY年MM月DD日 HH:mm'

const heroDescription = computed(() => {
  const pendingCount = Number(statsCards.value.find((item) => item.title.includes('待处理'))?.value || 0)
  const watchMessage = pendingCount > 0
    ? `当前仍有 ${formatNumber(pendingCount)} 条提醒需要优先查看。`
    : '当前没有堆积中的提醒，可以把注意力放在趋势和旅人回声上。'

  return `${greeting.value}。${currentDateTime.value} 的湖面呈现 ${lakeWeather.value.label} 状态，${lakeWeather.value.desc}，${watchMessage}`
})

const heroChips = computed(() => [
  ...techBadges.map(({ label }) => label),
  `趋势窗口 ${chartRange.value} 天`,
].slice(0, 5))

const weatherTone = computed(() => {
  if (lakeWeatherTemp.value >= 65) return '情绪偏暖'
  if (lakeWeatherTemp.value >= 40) return '情绪平稳'
  if (lakeWeatherTemp.value >= 20) return '情绪偏低'
  return '需要更细致关怀'
})

const summaryItems = computed(() => [
  {
    label: '旅人总量',
    value: formatNumber(Number(statsCards.value[0]?.value || 0)),
    note: '累计留下足迹的账号规模',
    tone: 'lake' as const,
  },
  {
    label: '今日投石',
    value: formatNumber(Number(statsCards.value[1]?.value || 0)),
    note: '今天新增的公开表达与互动',
    tone: 'amber' as const,
  },
  {
    label: '在线陪伴',
    value: formatNumber(Number(statsCards.value[2]?.value || 0)),
    note: '当前仍在湖面停留的旅人',
    tone: 'sage' as const,
  },
  {
    label: '待处理提醒',
    value: formatNumber(Number(statsCards.value[3]?.value || 0)),
    note: '需要先看一眼的异常或求助',
    tone: 'rose' as const,
  },
])

const lakeSignals = computed(() => [
  {
    label: '此刻湖面',
    value: `${lakeWeather.value.icon} ${lakeWeather.value.label}`,
    note: `情绪温度 ${lakeWeatherTemp.value}° · ${weatherTone.value}`,
  },
  {
    label: '值守时间',
    value: currentDateTime.value.split(' ').pop() || currentDateTime.value,
    note: currentDateTime.value,
  },
  {
    label: '最近刷新',
    value: lastUpdateTime.value,
    note: '趋势、触达与热门内容已同步',
  },
])

let clockTimer: ReturnType<typeof setInterval> | null = null
let pulseTimer: ReturnType<typeof setInterval> | null = null

const startPolling = () => {
  if (!clockTimer) {
    clockTimer = setInterval(() => {
      currentDateTime.value = dayjs().format(dashboardTimeFormat)
    }, 60000)
  }
  if (!pulseTimer) {
    pulseTimer = setInterval(loadEmotionPulse, 30000)
  }
}

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

const handleVisibilityChange = () => {
  if (document.hidden) {
    stopPolling()
  } else {
    loadEmotionPulse()
    currentDateTime.value = dayjs().format(dashboardTimeFormat)
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
  padding: 8px 0 22px;
}

.dashboard-ribbon {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  gap: 14px;
  margin-bottom: 22px;
}

.dashboard-ribbon__panel {
  position: relative;
  padding: 18px 18px 16px;
  border-radius: 24px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background:
    linear-gradient(180deg, rgba(255, 255, 255, 0.78), rgba(244, 248, 249, 0.92)),
    radial-gradient(circle at right top, rgba(123, 160, 173, 0.12), transparent 32%);
  box-shadow: 0 18px 32px rgba(10, 23, 31, 0.05);

  strong {
    display: block;
    margin-top: 16px;
    font-family: var(--hl-font-display);
    font-size: clamp(24px, 3vw, 32px);
    line-height: 1;
    color: var(--hl-ink);
  }

  p {
    margin: 10px 0 0;
    color: var(--hl-ink-soft);
    font-size: 13px;
    line-height: 1.7;
  }
}

.dashboard-ribbon__label {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  padding: 0 10px;
  border-radius: 999px;
  background: rgba(17, 62, 74, 0.08);
  color: var(--m3-primary);
  font-family: var(--hl-font-mono);
  font-size: 10px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.dashboard :deep(.care-feedback-row),
.dashboard :deep(.charts-row) {
  margin-bottom: 22px;
}

.dashboard :deep(.chart-card),
.dashboard :deep(.care-card) {
  border-radius: 28px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background:
    linear-gradient(180deg, rgba(254, 253, 251, 0.94), rgba(242, 247, 248, 0.96)),
    radial-gradient(circle at right top, rgba(123, 160, 173, 0.12), transparent 28%);
  box-shadow: 0 22px 46px rgba(10, 23, 31, 0.06);
  overflow: hidden;
}

.dashboard :deep(.chart-card .el-card__header),
.dashboard :deep(.care-card .el-card__header) {
  padding: 18px 22px 14px;
  border-bottom: 1px solid rgba(24, 36, 47, 0.07);
  background: rgba(255, 255, 255, 0.36);
}

.dashboard :deep(.chart-card .el-card__body),
.dashboard :deep(.care-card .el-card__body) {
  padding: 22px;
}

.dashboard :deep(.card-header),
.dashboard :deep(.card-title) {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  color: var(--hl-ink);
  font-size: 16px;
  font-weight: 600;
}

.dashboard :deep(.chart-card:hover),
.dashboard :deep(.care-card:hover) {
  border-color: rgba(17, 62, 74, 0.16);
}

.dashboard :deep(.feedback-slide) {
  padding: 22px 20px;
  border-radius: 22px;
  border: 1px solid rgba(130, 153, 159, 0.16);
  background:
    linear-gradient(150deg, rgba(255, 255, 255, 0.92), rgba(239, 245, 247, 0.96)),
    radial-gradient(circle at right top, rgba(182, 122, 66, 0.12), transparent 30%);
}

.dashboard :deep(.slide-chip) {
  background: rgba(17, 62, 74, 0.08);
  color: var(--m3-primary);
}

.dashboard :deep(.slide-title) {
  color: var(--hl-ink);
}

.dashboard :deep(.slide-desc),
.dashboard :deep(.touch-head),
.dashboard :deep(.trending-meta),
.dashboard :deep(.topic-count) {
  color: var(--hl-ink-soft);
}

.dashboard :deep(.touch-list) {
  gap: 14px;
}

.dashboard :deep(.touch-item) {
  padding: 14px 14px 12px;
  border-radius: 18px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.64);
}

.dashboard :deep(.topic-item),
.dashboard :deep(.trending-item) {
  display: grid;
  grid-template-columns: auto 1fr auto;
  gap: 14px;
  align-items: start;
  padding: 12px 0;
  border-bottom: 1px solid rgba(24, 36, 47, 0.06);
}

.dashboard :deep(.topic-item:last-child),
.dashboard :deep(.trending-item:last-child) {
  border-bottom: none;
}

.dashboard :deep(.topic-rank),
.dashboard :deep(.trending-rank) {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 30px;
  height: 30px;
  border-radius: 999px;
  background: rgba(17, 62, 74, 0.08);
  color: var(--hl-ink-soft);
  font-family: var(--hl-font-mono);
  font-size: 11px;
  font-weight: 700;
}

.dashboard :deep(.topic-rank.top),
.dashboard :deep(.trending-rank.top) {
  background: rgba(182, 122, 66, 0.14);
  color: #9c6330;
}

.dashboard :deep(.topic-name),
.dashboard :deep(.trending-title) {
  color: var(--hl-ink);
  font-weight: 600;
  line-height: 1.6;
}

.dashboard :deep(.trending-content-list) {
  display: grid;
  gap: 2px;
}

.dashboard :deep(.trending-info) {
  display: grid;
  gap: 8px;
}

.dashboard :deep(.el-radio-button__inner) {
  min-height: 34px;
  border-radius: 999px;
  border-color: rgba(123, 149, 160, 0.2);
  background: rgba(255, 255, 255, 0.74);
  color: var(--hl-ink-soft);
  box-shadow: none;
}

.dashboard :deep(.el-radio-button__original-radio:checked + .el-radio-button__inner) {
  background: var(--m3-primary);
  border-color: var(--m3-primary);
  color: #fff;
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

@media (max-width: 900px) {
  .dashboard-ribbon {
    grid-template-columns: 1fr;
  }
}
</style>
