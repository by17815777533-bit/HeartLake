<template>
  <div class="ops-dashboard ops-page">
    <section v-if="dashboardWarnings.length" class="warning-strip">
      <strong>数据刷新提示</strong>
      <span>{{ dashboardWarnings.map((item) => item.message).join('；') }}</span>
    </section>

    <section class="dashboard-command">
      <div>
        <span class="eyebrow">HeartLake Operations Console</span>
        <h1>运营工作台</h1>
        <p>
          {{ lakeWeather.desc }} · 最近刷新 {{ lastUpdateTime }} ·
          {{ guideChip }}
        </p>
      </div>
      <div class="header-actions">
        <el-select v-model="chartRange" class="range-select" size="large" @change="loadGrowthData">
          <el-option label="近 7 天" :value="7" />
          <el-option label="近 14 天" :value="14" />
          <el-option label="近 30 天" :value="30" />
        </el-select>
        <button type="button" class="action-btn" @click="exportData">
          <el-icon><Download /></el-icon>
          <span>导出</span>
        </button>
        <button
          type="button"
          class="action-btn is-primary"
          :disabled="loading"
          @click="refreshData"
        >
          <el-icon><RefreshRight /></el-icon>
          <span>刷新</span>
        </button>
      </div>
    </section>

    <section class="metric-grid" aria-label="核心运营指标">
      <article v-for="card in metricCards" :key="card.label" class="metric-tile" :class="card.tone">
        <span>{{ card.label }}</span>
        <strong>{{ card.value }}</strong>
        <small>{{ card.note }}</small>
      </article>
    </section>

    <section class="dashboard-board">
      <div class="board-main">
        <article class="panel panel--trend">
          <div class="panel-head">
            <div>
              <span>增长趋势</span>
              <h2>旅人增长与活跃基线</h2>
            </div>
            <em>{{ trendChipLabel }}</em>
          </div>
          <div class="trend-summary">
            <span>
              峰值窗口
              <strong>{{ trendPeakInfo?.dateLabel || '--' }}</strong>
            </span>
            <span>
              峰值规模
              <strong>{{ trendPeakInfo?.valueLabel || '--' }}</strong>
            </span>
            <span>
              当前稳定度
              <strong>{{ guidePulseValue }}</strong>
            </span>
          </div>
          <v-chart :option="userGrowthOption" autoresize class="chart chart--trend" />
        </article>

        <div class="chart-pair">
          <article class="panel panel--compact-chart">
            <div class="panel-head">
              <div>
                <span>情绪结构</span>
                <h2>情绪分布</h2>
              </div>
              <em>{{ lakeWeather.label }}</em>
            </div>
            <v-chart :option="weatherMoodPieOption" autoresize class="chart chart--donut" />
          </article>

          <article class="panel panel--compact-chart">
            <div class="panel-head">
              <div>
                <span>时段节律</span>
                <h2>24 小时投石</h2>
              </div>
              <em>{{ guideChip }}</em>
            </div>
            <v-chart :option="activeTimeOption" autoresize class="chart chart--bar" />
          </article>
        </div>

        <article class="panel panel--mood">
          <div class="panel-head">
            <div>
              <span>情绪趋势</span>
              <h2>多情绪波动对比</h2>
            </div>
            <div class="segmented">
              <button
                v-for="range in [7, 14, 30]"
                :key="range"
                type="button"
                :class="{ 'is-active': moodTrendRange === range }"
                @click="setMoodTrendRange(range)"
              >
                {{ range }}天
              </button>
            </div>
          </div>
          <v-chart :option="moodTrendOption" autoresize class="chart chart--mood" />
        </article>
      </div>

      <aside class="board-side" aria-label="运营处置侧栏">
        <article class="panel score-card">
          <div class="panel-head">
            <div>
              <span>健康度</span>
              <h2>湖面稳定评分</h2>
            </div>
            <button type="button" class="text-link" @click="jumpToReports">预警</button>
          </div>
          <div class="score-card__body">
            <strong>{{ formatCreditScore(creditScoreValue) }}</strong>
            <span>{{ creditScoreLabel }}</span>
          </div>
          <div class="score-track">
            <i :style="{ width: `${creditScoreValue}%` }" />
          </div>
          <div class="score-meta">
            <span>待处理 {{ formatNumber(Number(stats.pendingReports || 0)) }}</span>
            <span>在线 {{ formatNumber(Number(stats.onlineCount || 0)) }}</span>
          </div>
        </article>

        <article class="panel panel--flow">
          <div class="panel-head">
            <div>
              <span>数据处理</span>
              <h2>链路水位</h2>
            </div>
            <em>{{ currentDateTime }}</em>
          </div>
          <div class="flow-list">
            <div v-for="item in processingFlow" :key="item.label" class="flow-row">
              <div class="flow-row__copy">
                <strong>{{ item.label }}</strong>
                <span>{{ item.value }}</span>
              </div>
              <div class="flow-row__track">
                <i :style="{ width: `${item.percent}%` }" />
              </div>
            </div>
          </div>
        </article>

        <article class="panel panel--insight">
          <div class="panel-head">
            <div>
              <span>运营建议</span>
              <h2>{{ guideHeadline }}</h2>
            </div>
            <em>{{ guidePulseValue }}</em>
          </div>
          <p>{{ guideCopy }}</p>
          <div class="signal-grid">
            <button
              v-for="item in guideSignals"
              :key="item.label"
              type="button"
              class="signal-pill"
              @click="jumpToReports"
            >
              <span>{{ item.label }}</span>
              <strong>{{ item.badge }}</strong>
            </button>
          </div>
        </article>
      </aside>
    </section>

    <section class="lower-grid">
      <article class="panel panel--ranking">
        <div class="panel-head">
          <div>
            <span>内容识别</span>
            <h2>重点内容排行</h2>
          </div>
          <button type="button" class="text-link" @click="jumpToContent">内容库</button>
        </div>
        <div v-if="activityRows.length" class="ranking-list">
          <button
            v-for="row in activityRows"
            :key="row.id"
            type="button"
            class="ranking-row"
            @click="jumpToContent"
          >
            <b>{{ row.badge }}</b>
            <span>
              <strong>{{ row.title }}</strong>
              <small>{{ row.meta }}</small>
            </span>
            <em :class="row.tone">{{ row.amount }}</em>
          </button>
        </div>
        <div v-else class="empty-state">暂无高置信内容排行</div>
      </article>

      <article class="panel panel--queue">
        <div class="panel-head">
          <div>
            <span>值守清单</span>
            <h2>今日优先处理</h2>
          </div>
          <button type="button" class="text-link" @click="jumpToReports">工单</button>
        </div>
        <div class="queue-grid">
          <div v-for="item in dutyItems" :key="item.label" class="queue-item">
            <span>{{ item.label }}</span>
            <strong>{{ item.value }}</strong>
            <small>{{ item.note }}</small>
          </div>
        </div>
      </article>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, onUnmounted } from 'vue'
import { useRouter } from 'vue-router'
import dayjs from 'dayjs'
import VChart from 'vue-echarts'
import { Download, RefreshRight } from '@element-plus/icons-vue'
import { useDashboardData } from '@/composables/useDashboardData'
import websocket from '@/services/websocket'

const router = useRouter()

const {
  loading,
  lastUpdateTime,
  chartRange,
  moodTrendRange,
  trendingTopics,
  aiTrendingContent,
  currentDateTime,
  dashboardWarnings,
  formatNumber,
  stats,
  privacyStats,
  privacyBudgetPercent,
  resonanceStats,
  lakeWeather,
  lakeWeatherTemp,
  weatherMoodPieOption,
  userGrowthOption,
  moodTrendOption,
  activeTimeOption,
  loadGrowthData,
  loadMoodTrend,
  loadEmotionPulse,
  exportData,
  refreshData,
} = useDashboardData()

const dashboardTimeFormat = 'YYYY年MM月DD日 HH:mm'

const formatCompactDate = (value: string) => {
  const parsed = dayjs(value)
  return parsed.isValid() ? parsed.format('M月D') : value
}

const formatCompactTitle = (value: string, max = 12) => {
  if (!value) return ''
  return value.length > max ? `${value.slice(0, max)}...` : value
}

const growthLabels = computed(() =>
  (userGrowthOption.value.xAxis.data || []).map((item) => String(item)),
)
const growthValues = computed(() =>
  (userGrowthOption.value.series?.[0]?.data || []).map((item) => Number(item) || 0),
)

const trendPeakInfo = computed(() => {
  if (!growthLabels.value.length || !growthValues.value.length) return null
  const peakValue = Math.max(...growthValues.value)
  const peakIndex = growthValues.value.indexOf(peakValue)
  return {
    dateLabel: formatCompactDate(growthLabels.value[peakIndex] || ''),
    valueLabel: `${formatNumber(peakValue)} 位`,
  }
})

const communityScore = computed(() => {
  const base = lakeWeatherTemp.value * 0.62
  const activeBonus = Math.min(18, Number(stats.onlineCount || 0) / 5)
  const reportPenalty = Math.min(28, Number(stats.pendingReports || 0) * 4)
  return Math.max(38, Math.min(96, Math.round(base + activeBonus + 18 - reportPenalty)))
})

const metricCards = computed(() => [
  {
    label: '累计旅人',
    value: formatNumber(Number(stats.totalUsers || 0)),
    note: '匿名身份总量',
    tone: 'is-blue',
  },
  {
    label: '今日投石',
    value: formatNumber(Number(stats.todayStones || 0)),
    note: '公开表达新增',
    tone: 'is-green',
  },
  {
    label: '在线陪伴',
    value: formatNumber(Number(stats.onlineCount || 0)),
    note: '实时活跃旅人',
    tone: 'is-violet',
  },
  {
    label: '待处理',
    value: formatNumber(Number(stats.pendingReports || 0)),
    note: '举报与复核队列',
    tone: 'is-amber',
  },
])

const trendChipLabel = computed(() => `近 ${chartRange.value} 天`)
const guidePulseValue = computed(() => `${communityScore.value} 分`)
const creditScoreValue = computed(() => communityScore.value)
const creditScoreLabel = computed(() => {
  if (creditScoreValue.value >= 82) return '稳健'
  if (creditScoreValue.value >= 66) return '平稳'
  return '关注中'
})
const formatCreditScore = (value: number) => `${Math.round(value)}`

const activityRows = computed(() => {
  if (aiTrendingContent.value.length) {
    return aiTrendingContent.value.slice(0, 5).map((item, index) => {
      const content = String(item.content || '')
      const normalizedScore = Math.round(Math.max(0, Math.min(1, Number(item.score || 0))) * 100)
      return {
        id: item.id || `content-${index}`,
        badge: String(index + 1),
        title: formatCompactTitle(content, 14),
        meta: item.mood ? `推荐内容 · ${item.mood}` : '推荐内容',
        amount: `${normalizedScore} 分`,
        tone:
          normalizedScore >= 70
            ? 'is-positive'
            : normalizedScore >= 45
              ? 'is-neutral'
              : 'is-negative',
      }
    })
  }

  return trendingTopics.value.slice(0, 5).map((item, index) => ({
    id: `${item.keyword}-${index}`,
    badge: String(index + 1),
    title: formatCompactTitle(item.keyword, 14),
    meta: '热度话题',
    amount: `${formatNumber(Number(item.count || 0))} 次`,
    tone: index === 0 ? 'is-positive' : 'is-neutral',
  }))
})

const guideCopy = computed(() => {
  const topTopic = trendingTopics.value[0]?.keyword
  if (Number(stats.pendingReports || 0) > 0) {
    return '先处理举报和复核队列，再继续观察情绪趋势是否同步升温。'
  }
  if (topTopic) return `“${formatCompactTitle(topTopic, 10)}”正在升温，建议提高对应内容的巡看密度。`
  return '当前没有明显异常主题，保持巡看频次并关注夜间活跃时段即可。'
})

const guideChip = computed(() => {
  const pendingCount = Number(stats.pendingReports || 0)
  if (pendingCount > 0) return `待处理 ${formatNumber(pendingCount)} 条`
  return trendingTopics.value[0]?.keyword ? '话题升温' : '当前平稳'
})

const guideHeadline = computed(() => {
  const topTopic = trendingTopics.value[0]?.keyword
  if (Number(stats.pendingReports || 0) > 0) return '优先处理异常反馈'
  if (topTopic) return `关注“${formatCompactTitle(topTopic, 8)}”波动`
  return '湖面状态稳定'
})

const guideSignals = computed(() => [
  {
    label: '峰值窗口',
    badge: trendPeakInfo.value?.dateLabel || '未加载',
  },
  {
    label: '巡看主题',
    badge: trendingTopics.value[0]?.keyword
      ? formatCompactTitle(trendingTopics.value[0].keyword, 6)
      : '平稳',
  },
  {
    label: '隐私余量',
    badge: `${Math.max(0, 100 - Math.round(privacyBudgetPercent.value))}%`,
  },
  {
    label: '共鸣情绪',
    badge: resonanceStats.topMood ? formatCompactTitle(resonanceStats.topMood, 6) : '未识别',
  },
])

const dutyItems = computed(() => [
  {
    label: '举报复核',
    value: `${formatNumber(Number(stats.pendingReports || 0))} 条`,
    note: Number(stats.pendingReports || 0) > 0 ? '优先完成回执闭环' : '当前无积压',
  },
  {
    label: '内容巡看',
    value: `${formatNumber(Number(stats.todayStones || 0))} 条`,
    note: '按最新入湖顺序抽检',
  },
  {
    label: '在线陪伴',
    value: `${formatNumber(Number(stats.onlineCount || 0))} 人`,
    note: '关注夜间高峰窗口',
  },
  {
    label: '隐私预算',
    value: `${Math.max(0, 100 - Math.round(privacyBudgetPercent.value))}%`,
    note: '保持推荐与分析余量',
  },
])

const processingFlow = computed(() => {
  const totalUsers = Number(stats.totalUsers || 0)
  const todayStones = Number(stats.todayStones || 0)
  const onlineCount = Number(stats.onlineCount || 0)
  const pendingReports = Number(stats.pendingReports || 0)
  const protectedUsers = Number(privacyStats.protectedUsers || 0)
  const resonanceSamples = Number(resonanceStats.todayMatches || 0)
  const privacyRemaining = Math.max(0, 100 - Math.round(privacyBudgetPercent.value))
  const peak = Math.max(
    totalUsers,
    todayStones,
    onlineCount,
    pendingReports,
    protectedUsers,
    resonanceSamples,
    1,
  )
  const toPercent = (value: number, min = 10) =>
    Math.max(min, Math.min(100, Math.round((value / peak) * 100)))

  return [
    {
      label: '身份接入',
      value: `${formatNumber(totalUsers)} 位`,
      percent: toPercent(totalUsers, 24),
    },
    {
      label: '内容入湖',
      value: `${formatNumber(todayStones)} 条`,
      percent: toPercent(todayStones, 16),
    },
    {
      label: '实时在线',
      value: `${formatNumber(onlineCount)} 人`,
      percent: toPercent(onlineCount, 14),
    },
    {
      label: '复核队列',
      value: `${formatNumber(pendingReports)} 条`,
      percent: pendingReports ? toPercent(pendingReports, 18) : 8,
    },
    {
      label: '隐私预算',
      value: `${privacyRemaining}% 余量`,
      percent: Math.max(8, privacyRemaining),
    },
    {
      label: '共鸣样本',
      value: `${formatNumber(resonanceSamples)} 次`,
      percent: toPercent(resonanceSamples, 12),
    },
  ]
})

const setMoodTrendRange = async (range: number) => {
  moodTrendRange.value = range
  await loadMoodTrend()
}

const jumpToContent = () => router.push('/content')
const jumpToReports = () => router.push('/reports')

let clockTimer: ReturnType<typeof setInterval> | null = null
let pulseTimer: ReturnType<typeof setInterval> | null = null
let realtimeRefreshTimer: ReturnType<typeof setTimeout> | null = null

const scheduleRealtimeRefresh = () => {
  if (document.hidden) return
  if (realtimeRefreshTimer) clearTimeout(realtimeRefreshTimer)
  realtimeRefreshTimer = setTimeout(() => {
    void refreshData()
    realtimeRefreshTimer = null
  }, 280)
}

const startPolling = () => {
  if (!clockTimer) {
    clockTimer = setInterval(() => {
      currentDateTime.value = dayjs().format(dashboardTimeFormat)
    }, 60000)
  }
  if (!pulseTimer) pulseTimer = setInterval(loadEmotionPulse, 30000)
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
  websocket.on('stats_update', scheduleRealtimeRefresh)
  websocket.on('new_stone', scheduleRealtimeRefresh)
  websocket.on('stone_deleted', scheduleRealtimeRefresh)
  websocket.on('ripple_update', scheduleRealtimeRefresh)
  websocket.on('ripple_deleted', scheduleRealtimeRefresh)
  websocket.on('boat_update', scheduleRealtimeRefresh)
  websocket.on('boat_deleted', scheduleRealtimeRefresh)
  websocket.on('new_report', scheduleRealtimeRefresh)
  websocket.on('new_moderation', scheduleRealtimeRefresh)
  document.addEventListener('visibilitychange', handleVisibilityChange)
})

onUnmounted(() => {
  stopPolling()
  if (realtimeRefreshTimer) {
    clearTimeout(realtimeRefreshTimer)
    realtimeRefreshTimer = null
  }
  websocket.off('stats_update', scheduleRealtimeRefresh)
  websocket.off('new_stone', scheduleRealtimeRefresh)
  websocket.off('stone_deleted', scheduleRealtimeRefresh)
  websocket.off('ripple_update', scheduleRealtimeRefresh)
  websocket.off('ripple_deleted', scheduleRealtimeRefresh)
  websocket.off('boat_update', scheduleRealtimeRefresh)
  websocket.off('boat_deleted', scheduleRealtimeRefresh)
  websocket.off('new_report', scheduleRealtimeRefresh)
  websocket.off('new_moderation', scheduleRealtimeRefresh)
  document.removeEventListener('visibilitychange', handleVisibilityChange)
})
</script>

<style scoped lang="scss">
.ops-dashboard {
  min-height: calc(100vh - 90px);
  display: grid;
  gap: 10px;
  padding: 0 0 12px;
  color: #1f2937;
  font-variant-numeric: tabular-nums;
}

.warning-strip {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 9px 12px;
  border: 1px solid #f1c27d;
  border-radius: 6px;
  background: #fff8ea;
  color: #7c4d12;
  font-size: 13px;
}

.dashboard-command {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 14px;
  padding: 12px 14px;
  border: 1px solid #d8dee8;
  border-radius: 6px;
  background: #ffffff;

  h1 {
    margin: 2px 0 4px;
    font-size: 22px;
    font-weight: 760;
    letter-spacing: 0;
    color: #111827;
  }

  p {
    margin: 0;
    color: #64748b;
    font-size: 13px;
  }
}

.eyebrow {
  color: #0f766e;
  font-size: 12px;
  font-weight: 760;
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}

.range-select {
  width: 118px;
}

.action-btn {
  height: 32px;
  display: inline-flex;
  align-items: center;
  gap: 8px;
  padding: 0 10px;
  border: 1px solid #d7e1ea;
  border-radius: 4px;
  background: #ffffff;
  color: #1f2937;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;

  &.is-primary {
    border-color: #0f766e;
    background: #0f766e;
    color: #ffffff;
  }

  &:disabled {
    opacity: 0.56;
    cursor: progress;
  }
}

.metric-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 10px;
}

.metric-tile {
  min-height: 82px;
  padding: 12px;
  border: 1px solid #dce5ec;
  border-radius: 6px;
  background: #ffffff;
  box-shadow: none;

  span,
  small {
    display: block;
    color: #64748b;
    font-size: 12px;
  }

  strong {
    display: block;
    margin: 8px 0 5px;
    color: #111827;
    font-size: 26px;
    font-weight: 800;
    line-height: 1;
  }

  &.is-blue {
    border-top: 4px solid #3b82f6;
  }
  &.is-green {
    border-top: 4px solid #0f766e;
  }
  &.is-violet {
    border-top: 4px solid #7c3aed;
  }
  &.is-amber {
    border-top: 4px solid #d97706;
  }
}

.dashboard-board {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 360px;
  gap: 10px;
  align-items: stretch;
}

.board-main,
.board-side {
  min-width: 0;
  display: grid;
  gap: 10px;
}

.board-side {
  align-content: start;
}

.chart-pair {
  display: grid;
  grid-template-columns: minmax(0, 0.9fr) minmax(0, 1.1fr);
  gap: 10px;
}

.lower-grid {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 360px;
  gap: 10px;
}

.panel {
  min-width: 0;
  min-height: 0;
  display: flex;
  flex-direction: column;
  padding: 12px;
  border: 1px solid #dce5ec;
  border-radius: 6px;
  background: #ffffff;
  box-shadow: none;
}

.panel--trend {
  min-height: 276px;
}

.panel--compact-chart {
  min-height: 220px;
}

.panel--mood {
  min-height: 230px;
}

.panel--flow,
.panel--insight,
.panel--ranking,
.panel--queue {
  min-height: 0;
}

.panel-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 10px;
  margin-bottom: 8px;

  span,
  em {
    color: #64748b;
    font-size: 12px;
    font-style: normal;
    font-weight: 700;
  }

  h2 {
    margin: 3px 0 0;
    color: #111827;
    font-size: 15px;
    font-weight: 760;
    line-height: 1.25;
    white-space: nowrap;
  }
}

.chart {
  flex: 1 1 auto;
  min-height: 0;

  &--trend {
    height: 184px;
  }

  &--donut,
  &--bar {
    height: 158px;
  }

  &--mood {
    height: 166px;
  }
}

.trend-summary {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 8px;
  margin-bottom: 8px;

  span {
    min-height: 48px;
    display: grid;
    align-content: center;
    gap: 4px;
    padding: 8px 10px;
    border: 1px solid #e2e8f0;
    border-radius: 6px;
    background: #f8fafc;
    color: #64748b;
    font-size: 11px;
  }

  strong {
    color: #111827;
    font-size: 16px;
  }
}

.segmented {
  display: inline-flex;
  padding: 3px;
  border: 1px solid #d7e1ea;
  border-radius: 6px;
  background: #f8fafc;

  button {
    min-width: 48px;
    height: 28px;
    border: 0;
    border-radius: 6px;
    background: transparent;
    color: #64748b;
    font-size: 12px;
    font-weight: 700;
    cursor: pointer;

    &.is-active {
      background: #0f766e;
      color: #ffffff;
    }
  }
}

.text-link {
  border: 0;
  background: transparent;
  color: #0f766e;
  font-size: 12px;
  font-weight: 760;
  cursor: pointer;
}

.score-card {
  min-height: 156px;
}

.score-card__body {
  display: flex;
  align-items: flex-end;
  gap: 10px;
  margin-top: 4px;

  strong {
    color: #111827;
    font-size: 44px;
    font-weight: 820;
    line-height: 1;
  }

  span {
    margin-bottom: 6px;
    color: #0f766e;
    font-size: 14px;
    font-weight: 760;
  }
}

.score-track {
  height: 9px;
  margin-top: 16px;
  overflow: hidden;
  border-radius: 999px;
  background: #e7edf5;

  i {
    display: block;
    height: 100%;
    border-radius: inherit;
    background: #0f766e;
  }
}

.score-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-top: 12px;

  span {
    min-height: 24px;
    display: inline-flex;
    align-items: center;
    padding: 0 8px;
    border: 1px solid #d8dee8;
    border-radius: 4px;
    background: #f8fafc;
    color: #475569;
    font-size: 11px;
    font-weight: 700;
  }
}

.flow-list {
  display: grid;
  gap: 9px;
}

.flow-row {
  display: grid;
  gap: 8px;
}

.flow-row__copy {
  display: flex;
  justify-content: space-between;
  gap: 12px;

  strong {
    color: #1f2937;
    font-size: 13px;
  }

  span {
    color: #64748b;
    font-size: 12px;
    font-weight: 700;
  }
}

.flow-row__track {
  height: 8px;
  overflow: hidden;
  border-radius: 999px;
  background: #edf2f7;

  i {
    display: block;
    height: 100%;
    border-radius: inherit;
    background: #0f766e;
  }
}

.ranking-list {
  display: grid;
  gap: 0;
}

.ranking-row {
  min-height: 40px;
  display: grid;
  grid-template-columns: 28px minmax(0, 1fr) auto;
  align-items: center;
  gap: 10px;
  padding: 6px 0;
  border: 0;
  border-bottom: 1px solid #eef2f7;
  background: transparent;
  text-align: left;
  cursor: pointer;

  b {
    width: 24px;
    height: 24px;
    display: grid;
    place-items: center;
    border-radius: 6px;
    background: #e8f5f3;
    color: #0f766e;
    font-size: 12px;
  }

  strong,
  small {
    display: block;
  }

  strong {
    color: #111827;
    font-size: 13px;
  }

  small {
    margin-top: 3px;
    color: #64748b;
    font-size: 11px;
  }

  em {
    font-style: normal;
    font-size: 12px;
    font-weight: 800;

    &.is-positive {
      color: #0f766e;
    }
    &.is-neutral {
      color: #2563eb;
    }
    &.is-negative {
      color: #dc2626;
    }
  }
}

.empty-state {
  min-height: 120px;
  display: grid;
  place-items: center;
  border: 1px dashed #cbd5e1;
  border-radius: 8px;
  color: #64748b;
  font-size: 13px;
}

.panel--insight p,
.panel--queue p {
  margin: 0 0 10px;
  color: #475569;
  font-size: 13px;
  line-height: 1.7;
}

.signal-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 8px;
}

.signal-pill {
  min-height: 56px;
  display: grid;
  align-content: center;
  gap: 8px;
  padding: 10px;
  border: 1px solid #dce5ec;
  border-radius: 6px;
  background: #f8fafc;
  text-align: left;
  cursor: pointer;

  span {
    color: #64748b;
    font-size: 12px;
  }

  strong {
    color: #111827;
    font-size: 15px;
  }
}

.queue-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 8px;
}

.queue-item {
  min-height: 82px;
  display: grid;
  align-content: center;
  gap: 6px;
  padding: 10px;
  border: 1px solid #dce5ec;
  border-radius: 6px;
  background: #f8fafc;

  span,
  small {
    color: #64748b;
    font-size: 12px;
  }

  strong {
    color: #111827;
    font-size: 20px;
  }
}

@media (max-width: 1180px) {
  .metric-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .dashboard-board,
  .lower-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 760px) {
  .dashboard-command,
  .header-actions {
    align-items: stretch;
    flex-direction: column;
  }

  .metric-grid,
  .chart-pair,
  .trend-summary,
  .queue-grid {
    grid-template-columns: 1fr;
  }
}
</style>
