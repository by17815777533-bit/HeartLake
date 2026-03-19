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

        <div class="pulse-bars">
          <div
            v-for="bar in pulseBars"
            :key="bar.label"
            class="pulse-bar"
          >
            <span :style="{ height: `${bar.height}%` }" />
            <small>{{ bar.label }}</small>
          </div>
        </div>
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

        <div class="score-meter">
          <div class="score-meter__arc" />
          <div class="score-meter__mask" />
          <div class="score-meter__content">
            <strong>{{ communityScore }}</strong>
            <span>{{ communityScoreLabel }}</span>
          </div>
        </div>

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
  const max = Math.max(...values, 1)
  const labels = ['周一', '周二', '周三', '周四', '周五', '周六']

  return values.map((value, index) => ({
    label: labels[index] || `序列${index + 1}`,
    height: Math.max(24, Math.round((value / max) * 100)),
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
  grid-template-columns: 1.55fr 0.92fr 1.18fr;
  grid-template-areas:
    "balance pulse activity"
    "balance guide activity"
    "chart chart score";
  gap: 20px;
}

.dashboard-card {
  position: relative;
  overflow: hidden;
  padding: 22px 24px;
  border-radius: 28px;
  border: 1px solid rgba(133, 156, 201, 0.12);
  background:
    radial-gradient(circle at 100% 0%, rgba(151, 221, 209, 0.2), transparent 22%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.9), rgba(236, 245, 255, 0.96));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 22px 46px rgba(104, 128, 173, 0.12);
}

.dashboard-card--balance { grid-area: balance; min-height: 420px; }
.dashboard-card--pulse { grid-area: pulse; min-height: 198px; }
.dashboard-card--activity { grid-area: activity; min-height: 420px; background: linear-gradient(180deg, rgba(218, 244, 239, 0.86), rgba(212, 239, 233, 0.94)); }
.dashboard-card--guide { grid-area: guide; min-height: 198px; }
.dashboard-card--chart { grid-area: chart; min-height: 340px; }
.dashboard-card--score { grid-area: score; min-height: 340px; }

.card-heading {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 14px;

  h2,
  h3 {
    margin: 8px 0 0;
    color: var(--hl-ink);
    font-size: 28px;
    font-weight: 700;
    letter-spacing: -0.03em;
  }

  h3 {
    font-size: 24px;
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
  margin-top: 22px;

  > span {
    display: block;
    color: var(--hl-ink-soft);
    font-size: 13px;
  }

  p {
    max-width: 34rem;
    margin-top: 12px;
    color: var(--hl-ink-soft);
    font-size: 14px;
    line-height: 1.8;
  }
}

.balance-total__value {
  margin-top: 10px;
  color: var(--hl-ink);
  font-size: clamp(48px, 6vw, 62px);
  font-weight: 800;
  line-height: 1;
  letter-spacing: -0.05em;

  small {
    margin-left: 8px;
    color: #8da5db;
    font-size: 26px;
    font-weight: 700;
  }
}

.balance-actions {
  display: flex;
  gap: 12px;
  margin-top: 22px;
}

.balance-action {
  min-width: 132px;
  height: 44px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  border: none;
  border-radius: 999px;
  background: rgba(244, 249, 255, 0.9);
  color: var(--hl-ink);
  font-size: 13px;
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
  grid-template-columns: 1fr 1fr 64px;
  gap: 14px;
  align-items: end;
  margin-top: 34px;
}

.balance-mini-card {
  min-height: 148px;
  padding: 18px;
  border: none;
  border-radius: 24px;
  text-align: left;
  box-shadow: 0 18px 32px rgba(120, 146, 194, 0.16);

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
    margin-top: 34px;
    color: #1d2740;
    font-size: 22px;
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
  min-height: 152px;
  display: grid;
  place-items: center;
  background: #212121;
  box-shadow: 0 20px 34px rgba(33, 33, 33, 0.22);

  span {
    color: #ffffff;
    font-size: 34px;
    font-weight: 300;
  }
}

.range-select {
  width: 96px;
}

.pulse-bars {
  display: flex;
  align-items: end;
  justify-content: space-between;
  gap: 10px;
  height: 124px;
  margin-top: 26px;
}

.pulse-bar {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  min-width: 0;

  span {
    width: 100%;
    border-radius: 999px;
    background: linear-gradient(180deg, rgba(144, 175, 255, 0.96), rgba(125, 157, 255, 0.96));
    box-shadow: 0 8px 18px rgba(125, 157, 255, 0.18);
  }

  small {
    color: var(--hl-ink-soft);
    font-size: 11px;
  }
}

.activity-list {
  display: grid;
  gap: 10px;
  margin-top: 20px;
}

.activity-item {
  display: grid;
  grid-template-columns: 40px minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
  padding: 12px 0;
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
  margin: 18px 0 0;
  color: var(--hl-ink);
  font-size: 15px;
  line-height: 1.7;
}

.guide-button,
.score-button {
  margin-top: 22px;
  height: 42px;
  padding: 0 18px;
  border: none;
  border-radius: 999px;
  background: linear-gradient(180deg, #8cb2ff, #789cf2);
  color: #ffffff;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 14px 26px rgba(120, 156, 242, 0.22);
}

.dashboard-chart {
  height: 250px;
  margin-top: 18px;
}

.score-meter {
  position: relative;
  width: 220px;
  height: 120px;
  margin: 34px auto 0;
  overflow: hidden;
}

.score-meter__arc,
.score-meter__mask {
  position: absolute;
  left: 50%;
  width: 220px;
  border-radius: 220px 220px 0 0;
  transform: translateX(-50%);
}

.score-meter__arc {
  bottom: 0;
  height: 220px;
  background: conic-gradient(
    from 180deg,
    #f07272 0 18%,
    #f1c25e 18% 52%,
    #80dacb 52% 82%,
    #7ca6ff 82% 100%
  );
}

.score-meter__mask {
  bottom: 16px;
  height: 188px;
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.98), rgba(239, 246, 255, 0.98));
}

.score-meter__content {
  position: absolute;
  inset: auto 0 6px;
  z-index: 1;
  text-align: center;

  strong {
    display: block;
    color: var(--hl-ink);
    font-size: 50px;
    font-weight: 800;
    letter-spacing: -0.05em;
  }

  span {
    display: block;
    margin-top: 2px;
    color: #62b9ac;
    font-size: 18px;
    font-weight: 700;
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
