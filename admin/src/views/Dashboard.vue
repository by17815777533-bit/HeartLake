<template>
  <div class="dashboard-bank ops-page">
    <section class="dashboard-grid">
      <article class="dashboard-card dashboard-card--overview">
        <div class="card-heading">
          <div>
            <span class="card-caption">{{ lakeWeather.label }}</span>
            <h2>湖面总览</h2>
          </div>
          <span class="card-chip">{{ currentDateTime }}</span>
        </div>

        <div class="overview-total">
          <span>Total Balance</span>
          <div class="overview-total__value">
            <strong>{{ formattedTravelerCount }}</strong>
            <small>.{{ travelerFraction }}</small>
          </div>
          <p>{{ balanceDescription }}</p>
        </div>

        <div class="overview-actions">
          <button type="button" class="overview-action" @click="exportData">
            <el-icon><Download /></el-icon>
            <span>导出概览</span>
          </button>
          <button type="button" class="overview-action" :disabled="loading" @click="refreshData">
            <el-icon><RefreshRight /></el-icon>
            <span>刷新湖面</span>
          </button>
        </div>

        <div class="section-heading">
          <span>My cards</span>
          <small>桌面常用入口</small>
        </div>

        <div class="overview-cards">
          <article
            v-for="item in highlightCards"
            :key="item.label"
            class="overview-mini-card"
            :class="item.tone"
          >
            <div class="overview-mini-card__ornament" />
            <span>{{ item.label }}</span>
            <strong>{{ item.value }}</strong>
            <small>{{ item.note }}</small>
            <em>{{ item.footer }}</em>
          </article>

          <button type="button" class="overview-shortcut" @click="jumpToAssist">
            <span>+</span>
          </button>
        </div>
      </article>

      <article class="dashboard-card dashboard-card--spending">
        <div class="card-heading">
          <div>
            <span class="card-caption">Spending</span>
            <h3>湖面节律</h3>
          </div>
          <el-select
            v-model="chartRange"
            size="small"
            class="range-select"
            @change="loadGrowthData"
          >
            <el-option label="7天" :value="7" />
            <el-option label="14天" :value="14" />
            <el-option label="30天" :value="30" />
          </el-select>
        </div>

        <div class="spending-stage">
          <span class="spending-badge">{{ spendingPeakLabel }}</span>

          <div class="spending-bars">
            <article
              v-for="item in spendingColumns"
              :key="item.label"
              class="spending-bar"
              :class="{ 'is-peak': item.isPeak }"
            >
              <div class="spending-bar__track">
                <span :style="{ height: `${item.height}%` }" />
              </div>
              <small>{{ item.label }}</small>
            </article>
          </div>
        </div>
      </article>

      <article class="dashboard-card dashboard-card--activity">
        <div class="card-heading">
          <div>
            <span class="card-caption">Transactions</span>
            <h3>湖面动态</h3>
          </div>
          <button type="button" class="text-action" @click="jumpToContent">查看全部</button>
        </div>

        <div class="transaction-list">
          <article v-for="row in activityRows" :key="row.id" class="transaction-item">
            <div class="transaction-item__icon" :class="row.iconTone">
              {{ row.badge }}
            </div>
            <div class="transaction-item__copy">
              <strong>{{ row.title }}</strong>
              <span>{{ row.meta }}</span>
            </div>
            <div class="transaction-item__amount" :class="row.tone">
              {{ row.amount }}
            </div>
          </article>
        </div>
      </article>

      <article class="dashboard-card dashboard-card--guide">
        <div class="card-heading">
          <div>
            <span class="card-caption">Guide</span>
            <h3>How To Keep The Lake Stable?</h3>
          </div>
        </div>

        <p class="guide-copy">
          {{ guideCopy }}
        </p>

        <button type="button" class="guide-button" @click="jumpToAssist">Learn More</button>
      </article>

      <article class="dashboard-card dashboard-card--chart">
        <div class="card-heading">
          <div>
            <span class="card-caption">Expenses</span>
            <h3>旅人波动曲线</h3>
          </div>
          <span class="card-chip">{{ trendChipLabel }}</span>
        </div>

        <div class="chart-summary">
          <strong>{{ trendHeadline }}</strong>
          <span>{{ trendContext }}</span>
        </div>

        <v-chart
          :option="userGrowthOption"
          autoresize
          class="dashboard-chart"
          role="img"
          aria-label="旅人增长曲线"
        />
      </article>

      <article class="dashboard-card dashboard-card--score">
        <div class="card-heading">
          <div>
            <span class="card-caption">Credit Score</span>
            <h3>湖面评分</h3>
          </div>
          <button type="button" class="text-action" @click="jumpToReports">查看详情</button>
        </div>

        <OpsGaugeMeter
          :value="creditScoreValue"
          :max="2000"
          :label="creditScoreLabel"
          :formatter="formatCreditScore"
        />

        <button type="button" class="score-button" @click="jumpToReports">Explore Reports</button>
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
import OpsGaugeMeter from '@/components/OpsGaugeMeter.vue'

const router = useRouter()

const {
  loading,
  lastUpdateTime,
  chartRange,
  trendingTopics,
  aiTrendingContent,
  currentDateTime,
  formatNumber,
  stats,
  lakeWeather,
  lakeWeatherTemp,
  userGrowthOption,
  loadGrowthData,
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

const formattedTravelerCount = computed(() => Number(stats.totalUsers || 0).toLocaleString())
const travelerFraction = computed(() =>
  String(Math.max(0, Math.min(99, Math.round(Number(lakeWeatherTemp.value || 0))))).padStart(
    2,
    '0',
  ),
)

const balanceDescription = computed(() => {
  return `${lakeWeather.value.desc} · 在线 ${formatNumber(Number(stats.onlineCount || 0))} 人 · 待处理 ${formatNumber(Number(stats.pendingReports || 0))}`
})

const growthLabels = computed(() =>
  (userGrowthOption.value.xAxis.data || []).map((item) => String(item)),
)
const growthValues = computed(() =>
  (userGrowthOption.value.series?.[0]?.data || []).map((item) => Number(item) || 0),
)

const communityScore = computed(() => {
  const base = lakeWeatherTemp.value * 0.62
  const activeBonus = Math.min(18, Number(stats.onlineCount || 0) / 5)
  const reportPenalty = Math.min(28, Number(stats.pendingReports || 0) * 4)
  return Math.max(38, Math.min(96, Math.round(base + activeBonus + 18 - reportPenalty)))
})

const highlightCards = computed(() => [
  {
    label: '今日投石',
    value: formatNumber(Number(stats.todayStones || 0)),
    note: '公开表达',
    footer: `待处理 ${formatNumber(Number(stats.pendingReports || 0))} 条`,
    tone: 'is-blue',
  },
  {
    label: '在线陪伴',
    value: formatNumber(Number(stats.onlineCount || 0)),
    note: '实时活跃',
    footer: `${communityScore.value} 分稳定`,
    tone: 'is-mint',
  },
])

const spendingColumns = computed(() => {
  const defaultLabels = ['Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']
  const sourceValues = growthValues.value.length
    ? growthValues.value.slice(-7)
    : [64, 48, 46, 76, 40, 84, 34]
  const sourceLabels = growthLabels.value.length
    ? growthLabels.value.slice(-7).map((item) => formatCompactDate(item))
    : defaultLabels
  const max = Math.max(...sourceValues, 1)
  const peakValue = Math.max(...sourceValues)
  const peakIndex = sourceValues.indexOf(peakValue)

  return sourceValues.map((value, index) => ({
    label: sourceLabels[index] || `序列${index + 1}`,
    value,
    isPeak: index === peakIndex,
    height: Math.max(26, Math.round((value / max) * 100)),
  }))
})

const spendingPeakLabel = computed(() => {
  const peak = spendingColumns.value.find((item) => item.isPeak) || spendingColumns.value[0]
  return peak ? `峰值 ${formatNumber(peak.value)}` : '峰值 --'
})

const activityRows = computed(() => {
  if (aiTrendingContent.value.length) {
    return aiTrendingContent.value.slice(0, 5).map((item, index) => {
      const content = String(item.content || '')
      const normalizedScore = Math.max(18, Math.round((Number(item.score || 0) || 0) * 960))
      const isPositive = normalizedScore >= 520
      return {
        id: item.id || `content-${index}`,
        badge: String(index + 1),
        title: formatCompactTitle(content),
        meta: item.mood ? `情绪 ${item.mood}` : '内容推荐',
        amount: `${isPositive ? '+' : '-'}${normalizedScore}`,
        tone: isPositive ? 'is-positive' : 'is-negative',
        iconTone: index % 2 === 0 ? 'is-blue' : 'is-mint',
      }
    })
  }

  if (trendingTopics.value.length) {
    return trendingTopics.value.slice(0, 5).map((item, index) => ({
      id: `${item.keyword}-${index}`,
      badge: String(index + 1),
      title: formatCompactTitle(item.keyword, 10),
      meta: '热度话题',
      amount: `${index % 2 === 0 ? '-' : '+'}${formatNumber(Number(item.count || 0))}`,
      tone: index % 2 === 0 ? 'is-negative' : 'is-positive',
      iconTone: index % 2 === 0 ? 'is-blue' : 'is-mint',
    }))
  }

  return [
    {
      id: 'traveler-count',
      badge: '旅',
      title: '累计旅人',
      meta: '当前账户池',
      amount: `+${formattedTravelerCount.value}`,
      tone: 'is-positive',
      iconTone: 'is-blue',
    },
    {
      id: 'online-count',
      badge: '活',
      title: '在线陪伴',
      meta: '湖面实时活跃',
      amount: `+${formatNumber(Number(stats.onlineCount || 0))}`,
      tone: 'is-positive',
      iconTone: 'is-mint',
    },
    {
      id: 'report-count',
      badge: '报',
      title: '待处理提醒',
      meta: '守护队列待办',
      amount: `-${formatNumber(Number(stats.pendingReports || 0))}`,
      tone: 'is-negative',
      iconTone: 'is-blue',
    },
    {
      id: 'stone-count',
      badge: '石',
      title: '今日投石',
      meta: '公开表达条数',
      amount: `+${formatNumber(Number(stats.todayStones || 0))}`,
      tone: 'is-positive',
      iconTone: 'is-mint',
    },
    {
      id: 'weather-state',
      badge: lakeWeather.value.label.charAt(0),
      title: `湖面${lakeWeather.value.label}`,
      meta: lakeWeather.value.desc,
      amount: `${travelerFraction.value}°`,
      tone: 'is-neutral',
      iconTone: 'is-blue',
    },
  ]
})

const guideCopy = computed(() => {
  const topTopic = trendingTopics.value[0]?.keyword
  if (topTopic) {
    return `“${topTopic}” 仍在上升，建议前置提醒和巡看。`
  }

  return '波动平稳，保持巡看和提醒节奏即可。'
})

const trendPeakInfo = computed(() => {
  if (!growthLabels.value.length || !growthValues.value.length) {
    return {
      dateLabel: `${chartRange.value}天窗口`,
      valueLabel: `${formatNumber(Number(stats.totalUsers || 0))} 位`,
    }
  }

  const peakValue = Math.max(...growthValues.value)
  const peakIndex = growthValues.value.indexOf(peakValue)
  return {
    dateLabel: formatCompactDate(growthLabels.value[peakIndex] || ''),
    valueLabel: `${formatNumber(peakValue)} 位`,
  }
})

const trendHeadline = computed(() => `峰值 ${trendPeakInfo.value.valueLabel}`)
const trendChipLabel = computed(() => trendPeakInfo.value.dateLabel)
const trendContext = computed(() => `最近刷新 ${lastUpdateTime.value} · 近 ${chartRange.value} 天`)

const creditScoreValue = computed(() =>
  Math.max(1280, Math.min(1880, 1080 + communityScore.value * 7)),
)
const creditScoreLabel = computed(() => {
  if (creditScoreValue.value >= 1680) return 'Excellent'
  if (creditScoreValue.value >= 1540) return 'Healthy'
  if (creditScoreValue.value >= 1440) return 'Stable'
  return 'Watch'
})

const formatCreditScore = (value: number) => Math.round(value).toLocaleString('en-US')

const jumpToAssist = () => router.push('/edge-ai')
const jumpToContent = () => router.push('/content')
const jumpToReports = () => router.push('/reports')

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

<style scoped lang="scss">
.dashboard-bank {
  --dashboard-gap: 18px;
  --dashboard-radius: 28px;
  height: min(756px, calc(100vh - 146px));
  min-height: 0;
  -webkit-font-smoothing: antialiased;
  text-rendering: optimizeLegibility;
  font-family:
    'Avenir Next', 'SF Pro Display', 'PingFang SC', 'Hiragino Sans GB', 'Segoe UI', sans-serif;
  font-variant-numeric: tabular-nums;
}

.dashboard-grid {
  display: grid;
  height: 100%;
  gap: var(--dashboard-gap);
  grid-template-columns: minmax(0, 1.48fr) minmax(230px, 0.82fr) minmax(292px, 0.94fr);
  grid-template-rows: minmax(182px, 0.98fr) minmax(138px, 0.74fr) minmax(228px, 1fr);
  grid-template-areas:
    'overview spending activity'
    'overview guide activity'
    'chart chart score';
}

.dashboard-card {
  position: relative;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  min-height: 0;
  padding: 24px;
  border-radius: var(--dashboard-radius);
  border: 1px solid rgba(163, 183, 224, 0.18);
  background: linear-gradient(180deg, rgba(251, 253, 255, 0.98), rgba(241, 247, 255, 0.98));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 22px 42px rgba(105, 131, 183, 0.1);
}

.dashboard-card--overview {
  grid-area: overview;
}
.dashboard-card--spending {
  grid-area: spending;
  background: linear-gradient(180deg, rgba(215, 227, 255, 0.98), rgba(204, 218, 255, 0.98));
}

.dashboard-card--activity {
  grid-area: activity;
  background: linear-gradient(180deg, rgba(214, 237, 231, 0.98), rgba(204, 233, 226, 0.98));
}

.dashboard-card--guide {
  grid-area: guide;
  background: linear-gradient(180deg, rgba(221, 242, 236, 0.98), rgba(211, 238, 232, 0.98));
}

.dashboard-card--chart {
  grid-area: chart;
  background: linear-gradient(180deg, rgba(241, 247, 255, 0.98), rgba(230, 239, 255, 0.98));
}

.dashboard-card--score {
  grid-area: score;
  background: linear-gradient(180deg, rgba(250, 252, 255, 0.98), rgba(241, 246, 255, 0.98));
}

.card-heading {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;

  > div {
    display: grid;
    gap: 6px;
  }

  h2,
  h3 {
    margin: 0;
    color: #101828;
    font-size: 21px;
    font-weight: 680;
    line-height: 1.12;
  }

  h3 {
    font-size: 19px;
  }
}

.card-caption,
.card-chip {
  display: inline-flex;
  align-items: center;
  min-height: 32px;
  padding: 0 12px;
  border-radius: 999px;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.04em;
  text-transform: uppercase;
}

.card-caption {
  background: rgba(255, 255, 255, 0.66);
  color: var(--hl-ink-soft);
}

.card-chip {
  background: rgba(255, 255, 255, 0.72);
  color: var(--hl-ink);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.86);
}

.overview-total {
  margin-top: 18px;

  > span {
    display: block;
    color: #202738;
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 0.01em;
  }

  p {
    max-width: 22rem;
    margin: 10px 0 0;
    color: var(--hl-ink-soft);
    font-size: 11px;
    line-height: 1.5;
  }
}

.overview-total__value {
  display: flex;
  align-items: flex-end;
  gap: 2px;
  margin-top: 12px;
  color: #12131a;
  line-height: 0.92;

  strong {
    font-size: clamp(48px, 4.8vw, 64px);
    font-weight: 800;
    letter-spacing: -0.08em;
  }

  small {
    margin-bottom: 6px;
    color: #8b98b3;
    font-size: 20px;
    font-weight: 700;
  }
}

.overview-actions {
  display: flex;
  gap: 10px;
  margin-top: 20px;
}

.overview-action {
  flex: 1 1 0;
  min-width: 0;
  height: 44px;
  display: inline-flex;
  align-items: center;
  justify-content: flex-start;
  gap: 10px;
  padding: 0 18px 0 12px;
  border: none;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.9);
  color: #1b2435;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 12px 18px rgba(113, 138, 184, 0.08);
  transition:
    transform 180ms ease,
    box-shadow 180ms ease;

  &:hover {
    transform: translateY(-1px);
    box-shadow:
      inset 0 1px 0 rgba(255, 255, 255, 0.96),
      0 16px 24px rgba(113, 138, 184, 0.12);
  }

  .el-icon {
    width: 28px;
    height: 28px;
    display: grid;
    place-items: center;
    border-radius: 50%;
    background: rgba(233, 241, 255, 0.92);
    box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.9);
  }
}

.section-heading {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-top: 18px;

  span {
    color: #111827;
    font-size: 15px;
    font-weight: 680;
  }

  small {
    color: var(--hl-ink-soft);
    font-size: 10px;
    line-height: 1.4;
  }
}

.overview-cards {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(0, 1fr) 68px;
  gap: 10px;
  margin-top: 14px;
  align-items: stretch;
}

.overview-mini-card,
.overview-shortcut {
  position: relative;
  border: none;
  border-radius: 24px;
  overflow: hidden;
}

.overview-mini-card {
  min-height: 132px;
  padding: 16px;
  text-align: left;
  box-shadow: 0 20px 34px rgba(115, 142, 195, 0.12);

  span,
  small,
  em {
    display: block;
    position: relative;
    z-index: 1;
  }

  span {
    color: rgba(28, 33, 48, 0.72);
    font-size: 12px;
    font-weight: 650;
  }

  strong {
    position: relative;
    z-index: 1;
    display: block;
    margin-top: 26px;
    color: #121827;
    font-size: 24px;
    font-weight: 800;
    letter-spacing: -0.04em;
  }

  small {
    margin-top: 6px;
    color: rgba(27, 33, 48, 0.62);
    font-size: 10px;
    line-height: 1.45;
  }

  em {
    position: absolute;
    left: 18px;
    bottom: 16px;
    font-style: normal;
    color: rgba(27, 33, 48, 0.78);
    font-size: 10px;
    font-weight: 600;
  }
}

.overview-mini-card__ornament {
  position: absolute;
  top: 14px;
  right: 14px;
  width: 88px;
  height: 88px;
  border-radius: 50%;
  border: 1.5px solid rgba(42, 52, 74, 0.18);

  &::before,
  &::after {
    content: '';
    position: absolute;
    border-radius: 50%;
    border: 1.5px solid rgba(42, 52, 74, 0.18);
  }

  &::before {
    inset: 10px;
  }

  &::after {
    inset: -18px 26px 26px -18px;
  }
}

.overview-mini-card.is-blue {
  background: linear-gradient(180deg, rgba(177, 204, 255, 0.96), rgba(163, 192, 255, 0.98));
}

.overview-mini-card.is-mint {
  background: linear-gradient(180deg, rgba(210, 239, 235, 0.96), rgba(196, 234, 227, 0.98));
}

.overview-shortcut {
  display: grid;
  place-items: center;
  min-height: 132px;
  background: #232326;
  color: #ffffff;
  cursor: pointer;
  box-shadow: 0 20px 30px rgba(35, 35, 38, 0.2);

  span {
    font-size: 38px;
    font-weight: 300;
    line-height: 1;
  }
}

.range-select {
  width: 92px;
}

.dashboard-card--spending :deep(.el-select__wrapper) {
  min-height: 34px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.78);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.94);
}

.dashboard-card--spending :deep(.el-select__selected-item) {
  font-size: 12px;
  font-weight: 700;
}

.spending-stage {
  flex: 1 1 auto;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: 14px;
  margin-top: 12px;
}

.spending-badge {
  align-self: flex-start;
  padding: 6px 12px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.7);
  color: #1a2540;
  font-size: 11px;
  font-weight: 700;
}

.spending-bars {
  display: grid;
  grid-template-columns: repeat(7, minmax(0, 1fr));
  gap: 8px;
  align-items: end;
  min-height: 120px;
}

.spending-bar {
  display: grid;
  justify-items: center;
  gap: 10px;
}

.spending-bar__track {
  width: 26px;
  height: 98px;
  display: flex;
  align-items: flex-end;
  justify-content: center;
  padding: 4px 0;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.32);

  span {
    width: 18px;
    border-radius: 999px;
    background: linear-gradient(180deg, rgba(255, 255, 255, 0.96), rgba(241, 246, 255, 0.96));
    box-shadow:
      inset 0 -1px 0 rgba(144, 163, 214, 0.18),
      0 10px 18px rgba(255, 255, 255, 0.28);
  }
}

.spending-bar.is-peak .spending-bar__track span {
  width: 20px;
  background: linear-gradient(180deg, #8fb3ff, #7d9ff3);
  box-shadow: 0 16px 26px rgba(126, 156, 241, 0.24);
}

.spending-bar small {
  color: rgba(27, 39, 65, 0.72);
  font-size: 10px;
  font-weight: 600;
}

.text-action {
  border: none;
  background: transparent;
  color: rgba(22, 31, 51, 0.72);
  font-size: 11px;
  font-weight: 700;
  cursor: pointer;
}

.transaction-list {
  display: grid;
  gap: 0;
  margin-top: 8px;
  flex: 1 1 auto;
}

.transaction-item {
  display: grid;
  grid-template-columns: 46px minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
  padding: 10px 0;
  border-bottom: 1px solid rgba(80, 115, 113, 0.12);
}

.transaction-item:last-child {
  border-bottom: none;
}

.transaction-item__icon {
  width: 46px;
  height: 46px;
  display: grid;
  place-items: center;
  border-radius: 50%;
  border: 1px solid rgba(28, 34, 45, 0.28);
  color: #1a2435;
  font-size: 13px;
  font-weight: 700;

  &.is-blue {
    background: rgba(235, 243, 255, 0.72);
  }

  &.is-mint {
    background: rgba(236, 248, 244, 0.78);
  }
}

.transaction-item__copy {
  min-width: 0;

  strong {
    display: block;
    color: #132033;
    font-size: 14px;
    font-weight: 680;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  span {
    display: block;
    margin-top: 3px;
    color: rgba(19, 32, 51, 0.62);
    font-size: 10px;
  }
}

.transaction-item__amount {
  font-size: 14px;
  font-weight: 720;
  letter-spacing: -0.02em;
}

.transaction-item__amount.is-positive {
  color: #0f6e55;
}
.transaction-item__amount.is-negative {
  color: #1c2433;
}
.transaction-item__amount.is-neutral {
  color: #51627e;
}

.dashboard-card--guide::before,
.dashboard-card--guide::after {
  content: '';
  position: absolute;
  border-radius: 50%;
  border: 1.5px solid rgba(84, 121, 188, 0.18);
}

.dashboard-card--guide::before {
  right: 32px;
  bottom: 28px;
  width: 108px;
  height: 108px;
}

.dashboard-card--guide::after {
  right: 80px;
  bottom: 8px;
  width: 68px;
  height: 68px;
}

.guide-copy {
  max-width: 18ch;
  margin: 12px 0 0;
  color: #172033;
  font-size: 12px;
  line-height: 1.5;
  display: -webkit-box;
  overflow: hidden;
  -webkit-line-clamp: 3;
  -webkit-box-orient: vertical;
}

.guide-button,
.score-button {
  height: 40px;
  margin-top: auto;
  align-self: flex-start;
  padding: 0 18px;
  border: none;
  border-radius: 999px;
  background: linear-gradient(180deg, #94b6ff, #7ea3f4);
  color: #ffffff;
  font-size: 11px;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 18px 28px rgba(126, 156, 241, 0.22);
  transition:
    transform 180ms ease,
    box-shadow 180ms ease;

  &:hover {
    transform: translateY(-1px);
    box-shadow: 0 22px 32px rgba(126, 156, 241, 0.26);
  }
}

.chart-summary {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-top: 10px;

  strong {
    color: #131c2f;
    font-size: 13px;
    font-weight: 700;
  }

  span {
    color: rgba(19, 32, 51, 0.62);
    font-size: 10px;
  }
}

.dashboard-chart {
  flex: 1 1 auto;
  min-height: 236px;
  height: 100%;
  margin-top: 4px;
}

.dashboard-card--chart :deep(.echarts) {
  height: 100%;
}

.dashboard-card--score {
  padding-bottom: 18px;
  align-items: stretch;
}

.dashboard-card--score :deep(.ops-gauge-meter) {
  flex: 1 1 auto;
  display: grid;
  place-items: center;
  padding-top: 4px;
}

.dashboard-card--score :deep(.ops-gauge-meter__dial) {
  width: min(198px, 100%);
}

.dashboard-card--score :deep(.ops-gauge-meter__marker) {
  display: none;
}

.dashboard-card--score :deep(.ops-gauge-meter__mask) {
  bottom: 10px;
}

.dashboard-card--score :deep(.ops-gauge-meter__content) {
  inset: auto 0 14px;
  gap: 2px;
}

.dashboard-card--score :deep(.ops-gauge-meter__content strong) {
  font-size: clamp(32px, 3vw, 44px);
}

.dashboard-card--score :deep(.ops-gauge-meter__content span) {
  font-size: 13px;
}

.score-button {
  align-self: center;
  min-width: 146px;
}

@media (max-width: 1180px) and (min-width: 961px) {
  .dashboard-bank {
    --dashboard-gap: 14px;
    --dashboard-radius: 24px;
    height: min(632px, calc(100vh - 112px));
  }

  .dashboard-grid {
    grid-template-columns: minmax(0, 1.4fr) minmax(176px, 0.76fr) minmax(232px, 0.88fr);
    grid-template-rows: minmax(148px, 0.96fr) minmax(116px, 0.74fr) minmax(198px, 1fr);
  }

  .dashboard-card {
    padding: 18px;
  }

  .card-heading {
    h2,
    h3 {
      font-size: 18px;
    }

    h3 {
      font-size: 16px;
    }
  }

  .card-caption,
  .card-chip {
    min-height: 28px;
    padding: 0 10px;
    font-size: 9px;
  }

  .overview-total {
    margin-top: 12px;

    > span,
    p {
      font-size: 10px;
    }
  }

  .overview-total__value {
    margin-top: 8px;

    strong {
      font-size: clamp(40px, 5vw, 52px);
    }

    small {
      margin-bottom: 5px;
      font-size: 16px;
    }
  }

  .overview-actions {
    margin-top: 14px;
  }

  .overview-action {
    height: 38px;
    padding: 0 12px 0 10px;
    font-size: 10px;

    .el-icon {
      width: 24px;
      height: 24px;
    }
  }

  .section-heading {
    margin-top: 14px;

    span {
      font-size: 14px;
    }

    small {
      display: none;
    }
  }

  .overview-cards {
    grid-template-columns: minmax(0, 1fr) minmax(0, 0.94fr) 58px;
    margin-top: 10px;
  }

  .overview-mini-card {
    min-height: 104px;
    padding: 12px;

    span,
    small,
    em {
      font-size: 9px;
    }

    strong {
      margin-top: 16px;
      font-size: 19px;
    }

    em {
      left: 14px;
      bottom: 12px;
    }
  }

  .overview-mini-card__ornament {
    top: 10px;
    right: 10px;
    width: 64px;
    height: 64px;

    &::before {
      inset: 8px;
    }

    &::after {
      inset: -14px 18px 18px -14px;
    }
  }

  .overview-shortcut {
    min-height: 104px;

    span {
      font-size: 34px;
    }
  }

  .range-select {
    width: 76px;
  }

  .dashboard-card--spending :deep(.el-select__wrapper) {
    min-height: 30px;
  }

  .spending-stage {
    gap: 10px;
    margin-top: 10px;
  }

  .spending-badge,
  .text-action {
    font-size: 9px;
  }

  .spending-bars {
    min-height: 82px;
    gap: 6px;
  }

  .spending-bar {
    gap: 6px;
  }

  .spending-bar__track {
    width: 20px;
    height: 66px;

    span {
      width: 12px;
    }
  }

  .spending-bar.is-peak .spending-bar__track span {
    width: 14px;
  }

  .spending-bar small {
    font-size: 8px;
  }

  .transaction-item {
    grid-template-columns: 34px minmax(0, 1fr) auto;
    gap: 8px;
    padding: 6px 0;
  }

  .transaction-item__icon {
    width: 34px;
    height: 34px;
    font-size: 10px;
  }

  .transaction-item__copy {
    strong {
      font-size: 12px;
    }

    span {
      margin-top: 2px;
      font-size: 9px;
    }
  }

  .transaction-item__amount {
    font-size: 12px;
  }

  .dashboard-card--guide::before {
    right: 22px;
    bottom: 18px;
    width: 74px;
    height: 74px;
  }

  .dashboard-card--guide::after {
    right: 54px;
    bottom: 4px;
    width: 46px;
    height: 46px;
  }

  .guide-copy {
    max-width: 17ch;
    margin-top: 10px;
    font-size: 10px;
  }

  .guide-button,
  .score-button {
    height: 34px;
    padding: 0 14px;
    font-size: 10px;
  }

  .chart-summary {
    margin-top: 8px;

    strong {
      font-size: 11px;
    }

    span {
      font-size: 9px;
    }
  }

  .dashboard-chart {
    min-height: 176px;
  }

  .dashboard-card--score {
    padding-bottom: 14px;
  }

  .dashboard-card--score :deep(.ops-gauge-meter__dial) {
    width: min(148px, 100%);
  }

  .dashboard-card--score :deep(.ops-gauge-meter__content strong) {
    font-size: clamp(24px, 3vw, 34px);
  }

  .dashboard-card--score :deep(.ops-gauge-meter__content span) {
    font-size: 11px;
  }
}

@media (max-width: 1440px) and (min-width: 1181px) {
  .dashboard-grid {
    gap: 16px;
    grid-template-columns: minmax(0, 1.56fr) minmax(228px, 0.86fr) minmax(286px, 0.94fr);
  }

  .dashboard-card {
    padding: 20px;
    border-radius: 26px;
  }

  .overview-total__value strong {
    font-size: clamp(46px, 4.3vw, 60px);
  }

  .overview-mini-card {
    min-height: 134px;
    padding: 16px;
  }

  .overview-mini-card strong {
    margin-top: 28px;
    font-size: 25px;
  }

  .overview-shortcut {
    min-height: 134px;
  }

  .spending-bars {
    min-height: 120px;
  }

  .spending-bar__track {
    width: 26px;
    height: 96px;
  }

  .dashboard-chart {
    min-height: 210px;
  }

  .dashboard-card--score :deep(.ops-gauge-meter__dial) {
    width: min(178px, 100%);
  }
}

@media (max-height: 820px) and (min-width: 1181px) {
  .dashboard-bank {
    height: min(596px, calc(100vh - 136px));
  }

  .dashboard-grid {
    gap: 14px;
    grid-template-columns: minmax(0, 1.6fr) minmax(214px, 0.88fr) minmax(270px, 0.98fr);
    grid-template-rows: minmax(0, 1.01fr) minmax(0, 0.6fr) minmax(0, 0.92fr);
  }

  .dashboard-card {
    padding: 18px;
    border-radius: 24px;
  }

  .card-heading {
    gap: 12px;

    h2,
    h3 {
      margin-top: 6px;
      font-size: 19px;
    }

    h3 {
      font-size: 17px;
    }
  }

  .card-caption,
  .card-chip {
    min-height: 31px;
    padding: 0 11px;
    font-size: 10px;
  }

  .overview-total {
    margin-top: 12px;

    p {
      margin-top: 8px;
      font-size: 11px;
      line-height: 1.48;
    }
  }

  .overview-total__value {
    margin-top: 6px;

    strong {
      font-size: clamp(40px, 4vw, 54px);
    }

    small {
      font-size: 18px;
      margin-bottom: 5px;
    }
  }

  .overview-actions,
  .section-heading {
    margin-top: 12px;
  }

  .overview-action,
  .guide-button,
  .score-button {
    height: 38px;
    font-size: 11px;
  }

  .overview-cards {
    gap: 12px;
    margin-top: 12px;
  }

  .overview-mini-card {
    min-height: 118px;
    padding: 14px;

    span {
      font-size: 11px;
    }

    strong {
      margin-top: 20px;
      font-size: 21px;
    }

    small,
    em {
      font-size: 10px;
    }

    em {
      left: 16px;
      bottom: 14px;
    }
  }

  .overview-shortcut {
    min-height: 118px;

    span {
      font-size: 36px;
    }
  }

  .overview-mini-card__ornament {
    top: 12px;
    right: 12px;
    width: 76px;
    height: 76px;

    &::before {
      inset: 8px;
    }

    &::after {
      inset: -16px 22px 22px -16px;
    }
  }

  .range-select {
    width: 86px;
  }

  .dashboard-card--spending :deep(.el-select__wrapper) {
    min-height: 33px;
  }

  .spending-stage {
    gap: 14px;
    margin-top: 12px;
  }

  .spending-badge {
    padding: 5px 10px;
    font-size: 10px;
  }

  .spending-bars {
    min-height: 102px;
    gap: 10px;
  }

  .spending-bar {
    gap: 8px;
  }

  .spending-bar__track {
    width: 24px;
    height: 82px;

    span {
      width: 16px;
    }
  }

  .spending-bar.is-peak .spending-bar__track span {
    width: 18px;
  }

  .spending-bar small {
    font-size: 9px;
  }

  .transaction-list {
    margin-top: 10px;
  }

  .transaction-item {
    grid-template-columns: 40px minmax(0, 1fr) auto;
    gap: 10px;
    padding: 8px 0;
  }

  .transaction-item__icon {
    width: 40px;
    height: 40px;
    font-size: 12px;
  }

  .transaction-item__copy {
    strong {
      font-size: 13px;
    }

    span {
      margin-top: 3px;
      font-size: 10px;
    }
  }

  .transaction-item__amount {
    font-size: 13px;
  }

  .dashboard-card--guide::before {
    right: 28px;
    bottom: 22px;
    width: 92px;
    height: 92px;
  }

  .dashboard-card--guide::after {
    right: 68px;
    bottom: 4px;
    width: 58px;
    height: 58px;
  }

  .guide-copy {
    margin-top: 10px;
    font-size: 11px;
    line-height: 1.5;
  }

  .chart-summary {
    margin-top: 6px;

    strong {
      font-size: 13px;
    }

    span {
      font-size: 10px;
    }
  }

  .dashboard-chart {
    min-height: 164px;
    margin-top: 2px;
  }

  .dashboard-card--score {
    padding-bottom: 10px;
  }

  .dashboard-card--score :deep(.ops-gauge-meter) {
    padding-top: 0;
  }

  .dashboard-card--score :deep(.ops-gauge-meter__dial) {
    width: min(144px, 100%);
  }

  .dashboard-card--score :deep(.ops-gauge-meter__mask) {
    width: 114px;
    height: 114px;
    bottom: 6px;
  }

  .dashboard-card--score :deep(.ops-gauge-meter__content) {
    inset: auto 0 8px;
  }

  .dashboard-card--score :deep(.ops-gauge-meter__content strong) {
    font-size: clamp(24px, 3vw, 32px);
  }

  .dashboard-card--score :deep(.ops-gauge-meter__content span) {
    font-size: 11px;
  }

  .score-button {
    min-width: 126px;
    height: 32px;
    margin-top: 6px;
  }
}

@media (max-height: 740px) and (min-width: 1181px) {
  .dashboard-bank {
    height: min(584px, calc(100vh - 118px));
  }

  .dashboard-grid {
    gap: 12px;
    grid-template-columns: minmax(0, 1.6fr) minmax(208px, 0.86fr) minmax(258px, 0.94fr);
    grid-template-rows: minmax(0, 1fr) minmax(0, 0.58fr) minmax(0, 0.88fr);
  }

  .dashboard-card {
    padding: 16px;
    border-radius: 22px;
  }

  .card-heading {
    h2,
    h3 {
      margin-top: 4px;
      font-size: 17px;
    }

    h3 {
      font-size: 16px;
    }
  }

  .card-caption,
  .card-chip {
    min-height: 28px;
    padding: 0 10px;
    font-size: 9px;
  }

  .overview-total {
    margin-top: 10px;

    > span,
    p {
      font-size: 10px;
    }

    p {
      margin-top: 6px;
    }
  }

  .overview-total__value {
    margin-top: 6px;

    strong {
      font-size: clamp(36px, 3.8vw, 48px);
    }

    small {
      margin-bottom: 4px;
      font-size: 16px;
    }
  }

  .overview-actions,
  .section-heading {
    margin-top: 10px;
  }

  .overview-action {
    height: 34px;
    min-width: 124px;
    padding: 0 14px 0 10px;
    font-size: 10px;

    .el-icon {
      width: 22px;
      height: 22px;
    }
  }

  .section-heading {
    span {
      font-size: 14px;
    }

    small {
      font-size: 9px;
    }
  }

  .overview-cards {
    grid-template-columns: minmax(0, 1.06fr) minmax(0, 0.88fr) 62px;
    gap: 10px;
    margin-top: 10px;
  }

  .overview-mini-card {
    min-height: 102px;
    padding: 12px;

    span,
    small,
    em {
      font-size: 9px;
    }

    strong {
      margin-top: 16px;
      font-size: 18px;
    }

    em {
      left: 14px;
      bottom: 12px;
    }
  }

  .overview-mini-card__ornament {
    top: 10px;
    right: 10px;
    width: 64px;
    height: 64px;

    &::before {
      inset: 8px;
    }

    &::after {
      inset: -14px 18px 18px -14px;
    }
  }

  .overview-shortcut {
    min-height: 102px;

    span {
      font-size: 32px;
    }
  }

  .range-select {
    width: 76px;
  }

  .dashboard-card--spending :deep(.el-select__wrapper) {
    min-height: 30px;
  }

  .spending-stage {
    gap: 10px;
    margin-top: 10px;
  }

  .spending-badge,
  .text-action,
  .guide-copy,
  .chart-summary span {
    font-size: 9px;
  }

  .spending-bars {
    min-height: 84px;
    gap: 8px;
  }

  .spending-bar {
    gap: 6px;
  }

  .spending-bar__track {
    width: 22px;
    height: 68px;

    span {
      width: 14px;
    }
  }

  .spending-bar.is-peak .spending-bar__track span {
    width: 16px;
  }

  .spending-bar small {
    font-size: 8px;
  }

  .transaction-list {
    margin-top: 8px;
  }

  .transaction-item {
    grid-template-columns: 34px minmax(0, 1fr) auto;
    gap: 8px;
    padding: 6px 0;
  }

  .transaction-item__icon {
    width: 34px;
    height: 34px;
    font-size: 10px;
  }

  .transaction-item__copy {
    strong {
      font-size: 12px;
    }

    span {
      margin-top: 2px;
      font-size: 9px;
    }
  }

  .transaction-item__amount {
    font-size: 12px;
  }

  .dashboard-card--guide::before {
    right: 24px;
    bottom: 18px;
    width: 76px;
    height: 76px;
  }

  .dashboard-card--guide::after {
    right: 56px;
    bottom: 4px;
    width: 46px;
    height: 46px;
  }

  .guide-copy {
    max-width: 22ch;
    margin-top: 8px;
    line-height: 1.45;
  }

  .guide-button,
  .score-button {
    height: 32px;
    padding: 0 14px;
    font-size: 10px;
  }

  .chart-summary {
    margin-top: 6px;

    strong {
      font-size: 11px;
    }
  }

  .dashboard-chart {
    min-height: 152px;
    margin-top: 0;
  }

  .dashboard-card--score {
    padding-bottom: 12px;
  }

  .dashboard-card--score :deep(.ops-gauge-meter__dial) {
    width: min(146px, 100%);
  }

  .dashboard-card--score :deep(.ops-gauge-meter__mask) {
    width: 116px;
    height: 116px;
    bottom: 8px;
  }

  .dashboard-card--score :deep(.ops-gauge-meter__content) {
    inset: auto 0 10px;
  }

  .dashboard-card--score :deep(.ops-gauge-meter__content strong) {
    font-size: clamp(24px, 2.5vw, 32px);
  }

  .dashboard-card--score :deep(.ops-gauge-meter__content span) {
    font-size: 11px;
  }

  .score-button {
    min-width: 122px;
  }
}

@media (max-width: 960px) {
  .dashboard-bank {
    height: auto;
  }

  .dashboard-grid {
    height: auto;
    grid-template-columns: 1fr 1fr;
    grid-template-rows: none;
    grid-template-areas:
      'overview overview'
      'spending activity'
      'guide activity'
      'chart chart'
      'score score';
  }
}

@media (max-width: 860px) {
  .dashboard-grid {
    grid-template-columns: 1fr;
    grid-template-areas:
      'overview'
      'spending'
      'guide'
      'activity'
      'chart'
      'score';
  }

  .overview-actions,
  .overview-cards {
    grid-template-columns: 1fr;
  }

  .overview-action {
    width: 100%;
  }

  .overview-shortcut {
    min-height: 90px;
  }

  .chart-summary {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
