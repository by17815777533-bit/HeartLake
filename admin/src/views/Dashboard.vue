<!--
  @file Dashboard.vue
  @brief Dashboard 组件 - 数据大屏 + 欢迎信息（组合子组件）
  Created by 林子怡

  组件结构说明：
  本页面已拆分为 6 个子组件（位于 views/dashboard/ 目录下），各自负责独立的展示区域：
    - DashboardHeader: 欢迎信息、技术标签、导出/刷新操作
    - StatsCards: 统计卡片（用户数、投石数、在线人数、待处理举报）
    - ChartsSection: 图表区（用户增长、情绪分布、趋势、话题、活跃时段）
    - InnovationSection: 隐私保护统计、情绪共鸣指标、情绪脉搏仪表盘
    - LakeWeatherSection: 湖面天气可视化、心情分布饼图
    - AITrendsSection: AI情绪趋势折线图、热门内容列表
  当前文件保留定时器管理和 composable 调用，作为子组件的协调层，结构合理无需进一步拆分。
-->

<template>
  <div class="dashboard">
    <!-- 欢迎信息 + 技术标签 -->
    <DashboardHeader
      :greeting="greeting"
      :current-date-time="currentDateTime"
      :last-update-time="lastUpdateTime"
      :loading="loading"
      :tech-badges="techBadges"
      @export="exportData"
      @refresh="refreshData"
    />

    <!-- 统计卡片 -->
    <StatsCards
      :stats-cards="statsCards"
      :format-number="formatNumber"
    />

    <!-- 图表区：用户增长 + 情绪分布 + 趋势 + 话题 + 活跃时段 -->
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

    <!-- 隐私保护 + 情绪共鸣 + 情绪脉搏 -->
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

    <!-- 湖面天气 + 心情分布 -->
    <LakeWeatherSection
      :lake-weather="lakeWeather"
      :lake-weather-temp="lakeWeatherTemp"
      :weather-mood-pie-option="weatherMoodPieOption"
    />

    <!-- AI 趋势 + 热门内容 -->
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

const {
  loading, privacyLoading, resonanceLoading, lastUpdateTime,
  chartRange, moodTrendRange, trendingTopics, aiTrendingContent,
  currentDateTime, techBadges, greeting, formatNumber,
  statsCards,
  privacyStats, privacyBudgetPercent, privacyBudgetColor,
  resonanceStats,
  lakeWeather, lakeWeatherTemp, weatherMoodPieOption,
  userGrowthOption, moodTrendOption, moodDistributionOption,
  activeTimeOption, emotionPulseOption, emotionTrendsOption,
  loadGrowthData, loadMoodTrend, loadEmotionPulse,
  exportData, refreshData,
} = useDashboardData()

let clockTimer: ReturnType<typeof setInterval> | null = null
let pulseTimer: ReturnType<typeof setInterval> | null = null

// L-16: 页面不可见时暂停轮询，节省资源和带宽
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
    // 恢复可见时立即刷新一次，再重启轮询
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

<style lang="scss" scoped>
// ── 星空主题色板 ──
$sky-gold: #F2CC8F;
$sky-amber: #E8A87C;
$sky-sunset: #E07A5F;
$sky-purple: #B8A9C9;
$sky-bg-base: #1A1A3E;
$sky-text-primary: #F0EDE6;
$sky-text-secondary: rgba(240, 237, 230, 0.6);

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
