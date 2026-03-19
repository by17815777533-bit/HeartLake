<template>
  <div class="dashboard-bank ops-page">
    <section class="dashboard-grid">
      <article class="dashboard-card dashboard-card--balance">
        <div class="card-heading">
          <div>
            <span class="card-eyebrow">{{ lakeWeather.label }}</span>
            <h2>湖面总览</h2>
          </div>
          <span class="card-chip">{{ currentDateTime }}</span>
        </div>

        <div class="balance-total">
          <span>累计旅人</span>
          <div class="balance-total__value">
            {{ formattedTravelerCount }}
            <small>位</small>
          </div>
          <p>{{ balanceDescription }}</p>
        </div>

        <div class="balance-actions">
          <button
            type="button"
            class="balance-action"
            @click="exportData"
          >
            <el-icon><Download /></el-icon>
            <span>导出概览</span>
          </button>
          <button
            type="button"
            class="balance-action"
            :disabled="loading"
            @click="refreshData"
          >
            <el-icon><RefreshRight /></el-icon>
            <span>刷新湖面</span>
          </button>
        </div>

        <div class="balance-stack">
          <article
            v-for="item in highlightCards"
            :key="item.label"
            class="balance-mini-card"
            :class="item.tone"
          >
            <span>{{ item.label }}</span>
            <strong>{{ item.value }}</strong>
            <small>{{ item.note }}</small>
          </article>

          <button
            type="button"
            class="balance-mini-card balance-mini-card--more"
            @click="jumpToAssist"
          >
            <span>+</span>
          </button>
        </div>
      </article>

      <article class="dashboard-card dashboard-card--pulse">
        <div class="card-heading">
          <div>
            <span class="card-eyebrow">Pulse</span>
            <h3>湖面节律</h3>
          </div>
          <el-select
            v-model="chartRange"
            size="small"
            class="range-select"
            @change="loadGrowthData"
          >
            <el-option
              label="7天"
              :value="7"
            />
            <el-option
              label="14天"
              :value="14"
            />
            <el-option
              label="30天"
              :value="30"
            />
          </el-select>
        </div>

        <OpsMiniBars :items="pulseBars" />
      </article>

      <article class="dashboard-card dashboard-card--activity">
        <div class="card-heading">
          <div>
            <span class="card-eyebrow">Activity</span>
            <h3>湖面动态</h3>
          </div>
          <button
            type="button"
            class="text-action"
            @click="jumpToContent"
          >
            查看全部
          </button>
        </div>

        <div class="activity-list">
          <article
            v-for="row in activityRows"
            :key="row.id"
            class="activity-item"
          >
            <div class="activity-item__avatar">
              {{ row.badge }}
            </div>
            <div class="activity-item__copy">
              <strong>{{ row.title }}</strong>
              <span>{{ row.meta }}</span>
            </div>
            <div
              class="activity-item__value"
              :class="row.tone"
            >
              {{ row.value }}
            </div>
          </article>
        </div>
      </article>

      <article class="dashboard-card dashboard-card--guide">
        <div class="card-heading">
          <div>
            <span class="card-eyebrow">Guide</span>
            <h3>如何安放湖面？</h3>
          </div>
        </div>

        <p class="guide-copy">
          {{ guideCopy }}
        </p>

        <button
          type="button"
          class="guide-button"
          @click="jumpToAssist"
        >
          查看建议
        </button>
      </article>

      <article class="dashboard-card dashboard-card--chart">
        <div class="card-heading">
          <div>
            <span class="card-eyebrow">Trend</span>
            <h3>旅人波动曲线</h3>
          </div>
          <span class="card-chip">{{ trendHeadline }}</span>
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
            <span class="card-eyebrow">Score</span>
            <h3>湖面评分</h3>
          </div>
          <button
            type="button"
            class="text-action"
            @click="jumpToReports"
          >
            处理提醒
          </button>
        </div>

        <OpsGaugeMeter
          :value="communityScore"
          :max="100"
          :label="communityScoreLabel"
        />

        <button
          type="button"
          class="score-button"
          @click="jumpToReports"
        >
          查看详情
        </button>
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
import OpsMiniBars from '@/components/OpsMiniBars.vue'
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

const formattedTravelerCount = computed(() => Number(stats.totalUsers || 0).toLocaleString())

const balanceDescription = computed(() => {
  return `${lakeWeather.value.desc}，当前在线 ${formatNumber(Number(stats.onlineCount || 0))} 人，待处理提醒 ${formatNumber(Number(stats.pendingReports || 0))} 条。`
})

const highlightCards = computed(() => [
  {
    label: '今日投石',
    value: formatNumber(Number(stats.todayStones || 0)),
    note: '公开表达',
    tone: 'is-blue',
  },
  {
    label: '在线陪伴',
    value: formatNumber(Number(stats.onlineCount || 0)),
    note: '实时活跃',
    tone: 'is-mint',
  },
])

const pulseBars = computed(() => {
  const source = trendingTopics.value.slice(0, 6).map((item) => Number(item.count || 0))
  const values = source.length ? source : [42, 58, 76, 64, 84, 50]
  const labels = ['周一', '周二', '周三', '周四', '周五', '周六']

  return values.map((value, index) => ({
    label: labels[index] || `序列${index + 1}`,
    value,
    display: `${value}`,
  }))
})

const activityRows = computed(() => {
  if (aiTrendingContent.value.length) {
    return aiTrendingContent.value.slice(0, 5).map((item, index) => ({
      id: item.id,
      badge: String(index + 1),
      title: item.content.length > 16 ? `${item.content.slice(0, 16)}...` : item.content,
      meta: item.likes ? `${item.likes} 次互动` : '智能推荐',
      value: item.score ? `+${Math.round(item.score)}` : `${item.likes || 0}`,
      tone: 'is-up',
    }))
  }

  return trendingTopics.value.slice(0, 5).map((item, index) => ({
    id: `${item.keyword}-${index}`,
    badge: String(index + 1),
    title: item.keyword,
    meta: '热度话题',
    value: `${item.count}条`,
    tone: 'is-neutral',
  }))
})

const guideCopy = computed(() => {
  const topTopic = trendingTopics.value[0]?.keyword
  if (topTopic) {
    return `今天最明显的话题是“${topTopic}”。建议把温和引导文案提前放到高峰期入口，避免在情绪抬头后再被动处理。`
  }

  return '当前没有明显突刺话题，适合继续观察旅人增长曲线和求助提醒的变化。'
})

const communityScore = computed(() => {
  const base = lakeWeatherTemp.value * 0.62
  const activeBonus = Math.min(18, Number(stats.onlineCount || 0) / 5)
  const reportPenalty = Math.min(28, Number(stats.pendingReports || 0) * 4)
  return Math.max(38, Math.min(96, Math.round(base + activeBonus + 18 - reportPenalty)))
})

const communityScoreLabel = computed(() => {
  if (communityScore.value >= 82) return '稳定'
  if (communityScore.value >= 66) return '可控'
  return '需关注'
})

const trendHeadline = computed(() => `最近刷新 ${lastUpdateTime.value} · 窗口 ${chartRange.value} 天`)

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
  padding-bottom: 6px;
}

.dashboard-grid {
  display: grid;
  grid-template-columns: minmax(0, 1.72fr) minmax(220px, 0.92fr) minmax(280px, 1.08fr);
  grid-template-areas:
    "balance pulse activity"
    "balance guide activity"
    "chart chart score";
  gap: 18px;
}

.dashboard-card {
  position: relative;
  overflow: hidden;
  padding: 20px 22px;
  border-radius: 26px;
  border: 1px solid rgba(160, 179, 215, 0.16);
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.94), rgba(242, 247, 255, 0.98));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 18px 34px rgba(107, 130, 173, 0.1);
  transition: transform 180ms ease, box-shadow 180ms ease;
  animation: dashboard-card-in 420ms ease-out both;

  &:hover {
    transform: translateY(-3px);
    box-shadow:
      inset 0 1px 0 rgba(255, 255, 255, 0.92),
      0 22px 38px rgba(107, 130, 173, 0.14);
  }
}

.dashboard-card:nth-child(1) { animation-delay: 0ms; }
.dashboard-card:nth-child(2) { animation-delay: 50ms; }
.dashboard-card:nth-child(3) { animation-delay: 100ms; }
.dashboard-card:nth-child(4) { animation-delay: 150ms; }
.dashboard-card:nth-child(5) { animation-delay: 200ms; }
.dashboard-card:nth-child(6) { animation-delay: 250ms; }

.dashboard-card--balance { grid-area: balance; min-height: 386px; }
.dashboard-card--pulse { grid-area: pulse; min-height: 182px; }
.dashboard-card--activity { grid-area: activity; min-height: 386px; background: linear-gradient(180deg, rgba(224, 246, 240, 0.94), rgba(214, 239, 232, 0.98)); }
.dashboard-card--guide { grid-area: guide; min-height: 186px; background: linear-gradient(180deg, rgba(228, 245, 239, 0.94), rgba(219, 240, 234, 0.98)); }
.dashboard-card--chart { grid-area: chart; min-height: 304px; }
.dashboard-card--score { grid-area: score; min-height: 304px; }

.card-heading {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 14px;

  h2,
  h3 {
    margin: 8px 0 0;
    color: var(--hl-ink);
    font-size: 26px;
    font-weight: 700;
    letter-spacing: -0.03em;
  }

  h3 {
    font-size: 22px;
  }
}

.card-eyebrow,
.card-chip {
  display: inline-flex;
  align-items: center;
  min-height: 30px;
  padding: 0 12px;
  border-radius: 999px;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.12em;
}

.card-eyebrow {
  background: rgba(239, 246, 255, 0.96);
  color: var(--hl-ink-soft);
  text-transform: uppercase;
}

.card-chip {
  background: rgba(255, 255, 255, 0.78);
  color: var(--hl-ink);
}

.text-action {
  border: none;
  background: transparent;
  color: var(--hl-ink-soft);
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
}

.balance-total {
  margin-top: 18px;

  > span {
    display: block;
    color: var(--hl-ink-soft);
    font-size: 13px;
  }

  p {
    max-width: 34rem;
    margin-top: 10px;
    color: var(--hl-ink-soft);
    font-size: 13px;
    line-height: 1.7;
  }
}

.balance-total__value {
  margin-top: 8px;
  color: var(--hl-ink);
  font-size: clamp(42px, 5vw, 56px);
  font-weight: 800;
  line-height: 1;
  letter-spacing: -0.05em;

  small {
    margin-left: 6px;
    color: #8da5db;
    font-size: 22px;
    font-weight: 700;
  }
}

.balance-actions {
  display: flex;
  gap: 12px;
  margin-top: 18px;
}

.balance-action {
  min-width: 124px;
  height: 40px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  border: none;
  border-radius: 999px;
  background: rgba(244, 249, 255, 0.9);
  color: var(--hl-ink);
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.92);
  transition: var(--m3-transition);

  &:hover {
    transform: translateY(-1px);
    background: rgba(255, 255, 255, 0.96);
  }
}

.balance-stack {
  display: grid;
  grid-template-columns: 1fr 1fr 56px;
  gap: 12px;
  align-items: end;
  margin-top: 28px;
}

.balance-mini-card {
  position: relative;
  min-height: 132px;
  padding: 16px;
  border: none;
  border-radius: 22px;
  text-align: left;
  box-shadow: 0 16px 28px rgba(120, 146, 194, 0.12);
  overflow: hidden;

  &::before,
  &::after {
    content: '';
    position: absolute;
    right: 14px;
    border-radius: 999px;
    border: 1.5px solid rgba(41, 53, 76, 0.28);
    opacity: 0.4;
  }

  &::before {
    top: 18px;
    width: 52px;
    height: 52px;
  }

  &::after {
    top: 30px;
    width: 72px;
    height: 72px;
  }

  span,
  small {
    display: block;
  }

  span {
    color: rgba(35, 45, 67, 0.74);
    font-size: 12px;
  }

  strong {
    display: block;
    margin-top: 28px;
    color: #1d2740;
    font-size: 20px;
    font-weight: 700;
  }

  small {
    margin-top: 8px;
    color: rgba(35, 45, 67, 0.62);
    font-size: 12px;
  }
}

.balance-mini-card.is-blue {
  background: linear-gradient(180deg, rgba(177, 204, 255, 0.92), rgba(166, 194, 255, 0.96));
}

.balance-mini-card.is-mint {
  background: linear-gradient(180deg, rgba(209, 241, 236, 0.92), rgba(196, 236, 229, 0.96));
}

.balance-mini-card--more {
  min-height: 136px;
  display: grid;
  place-items: center;
  background: #212121;
  box-shadow: 0 20px 34px rgba(33, 33, 33, 0.22);

  &::before,
  &::after {
    display: none;
  }

  span {
    color: #ffffff;
    font-size: 34px;
    font-weight: 300;
  }
}

.range-select {
  width: 92px;
}

.activity-list {
  display: grid;
  gap: 8px;
  margin-top: 16px;
}

.activity-item {
  display: grid;
  grid-template-columns: 40px minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
  padding: 10px 0;
  border-bottom: 1px solid rgba(122, 163, 157, 0.16);
}

.activity-item:last-child {
  border-bottom: none;
}

.activity-item__avatar {
  width: 40px;
  height: 40px;
  display: grid;
  place-items: center;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.78);
  color: var(--hl-ink);
  font-weight: 700;
}

.activity-item__copy {
  min-width: 0;

  strong {
    display: block;
    color: var(--hl-ink);
    font-size: 14px;
    font-weight: 700;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  span {
    display: block;
    margin-top: 4px;
    color: var(--hl-ink-soft);
    font-size: 12px;
  }
}

.activity-item__value {
  font-size: 14px;
  font-weight: 700;
}

.activity-item__value.is-up { color: #5aaf9d; }
.activity-item__value.is-neutral { color: #6f7c96; }

.dashboard-card--guide::before,
.dashboard-card--guide::after {
  content: '';
  position: absolute;
  border-radius: 999px;
  border: 1px solid rgba(129, 157, 230, 0.26);
}

.dashboard-card--guide::before {
  width: 112px;
  height: 112px;
  right: 22px;
  bottom: 18px;
}

.dashboard-card--guide::after {
  width: 74px;
  height: 74px;
  right: 88px;
  bottom: 44px;
}

.guide-copy {
  max-width: 28ch;
  margin: 14px 0 0;
  color: var(--hl-ink);
  font-size: 14px;
  line-height: 1.65;
}

.guide-button,
.score-button {
  margin-top: 18px;
  height: 40px;
  padding: 0 18px;
  border: none;
  border-radius: 999px;
  background: linear-gradient(180deg, #8cb2ff, #789cf2);
  color: #ffffff;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 14px 26px rgba(120, 156, 242, 0.22);
  transition: transform 180ms ease, box-shadow 180ms ease;

  &:hover {
    transform: translateY(-2px);
    box-shadow: 0 18px 28px rgba(120, 156, 242, 0.24);
  }
}

.dashboard-chart {
  height: 220px;
  margin-top: 16px;
}

.dashboard-card--pulse :deep(.ops-mini-bars) {
  min-height: 138px;
}

.dashboard-card--score :deep(.ops-gauge-meter__dial) {
  width: min(220px, 100%);
}

@keyframes dashboard-card-in {
  from {
    opacity: 0;
    transform: translateY(12px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@media (max-width: 1180px) {
  .dashboard-grid {
    grid-template-columns: 1fr 1fr;
    grid-template-areas:
      "balance balance"
      "pulse activity"
      "guide activity"
      "chart chart"
      "score score";
  }
}

@media (max-width: 760px) {
  .dashboard-grid {
    grid-template-columns: 1fr;
    grid-template-areas:
      "balance"
      "pulse"
      "guide"
      "activity"
      "chart"
      "score";
  }

  .balance-stack {
    grid-template-columns: 1fr;
  }

  .balance-mini-card--more {
    min-height: 76px;
  }
}
</style>
