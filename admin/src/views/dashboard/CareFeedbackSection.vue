<!--
  关怀反馈区重构：
  - 左侧改成更强叙事感的“旅人回声”轮播
  - 右侧情绪水位图增加说明与实时提示
  - 下方触达数据改成更像运营面板的响应块
-->
<template>
  <el-row
    :gutter="20"
    class="care-feedback-row"
  >
    <el-col
      :xs="24"
      :lg="10"
    >
      <el-card
        shadow="hover"
        class="care-card feedback-carousel-card"
      >
        <template #header>
          <div class="section-head">
            <div>
              <span class="section-eyebrow">Traveler Echo</span>
              <h3>旅人回声</h3>
            </div>
            <span class="section-note">自动轮播</span>
          </div>
        </template>
        <el-carousel
          indicator-position="outside"
          height="290px"
          :interval="5000"
          trigger="click"
          autoplay
        >
          <el-carousel-item
            v-for="slide in feedbackSlides"
            :key="slide.title"
          >
            <div class="feedback-slide">
              <span class="slide-chip">{{ slide.chip }}</span>
              <h4 class="slide-title">{{ slide.title }}</h4>
              <p class="slide-desc">{{ slide.desc }}</p>
              <div class="slide-metric">{{ slide.metric }}</div>
            </div>
          </el-carousel-item>
        </el-carousel>
      </el-card>
    </el-col>

    <el-col
      :xs="24"
      :lg="14"
    >
      <el-card
        shadow="hover"
        class="care-card trend-card"
      >
        <template #header>
          <div class="section-head">
            <div>
              <span class="section-eyebrow">Emotional Level</span>
              <h3>情绪水位</h3>
            </div>
            <span class="section-note">实时趋势</span>
          </div>
        </template>

        <div class="trend-card__body">
          <div class="trend-card__meta">
            <div
              v-for="entry in quickSignals"
              :key="entry.label"
              class="trend-chip"
            >
              <span>{{ entry.label }}</span>
              <strong>{{ entry.value }}</strong>
            </div>
          </div>
          <v-chart
            :option="emotionTrendsOption"
            autoresize
            class="trend-chart"
            role="img"
            aria-label="情绪反馈趋势图"
          />
        </div>
      </el-card>
    </el-col>
  </el-row>

  <el-row
    :gutter="20"
    class="care-feedback-row"
  >
    <el-col :span="24">
      <el-card
        shadow="hover"
        class="care-card touch-card"
      >
        <template #header>
          <div class="section-head">
            <div>
              <span class="section-eyebrow">Care Reach</span>
              <h3>关怀触达</h3>
            </div>
            <span class="section-note">响应概览</span>
          </div>
        </template>
        <div class="touch-list">
          <article
            v-for="entry in touchRateRows"
            :key="entry.title"
            class="touch-item"
          >
            <div class="touch-head">
              <span class="touch-title">{{ entry.title }}</span>
              <span class="touch-value">{{ entry.display }}</span>
            </div>
            <el-progress
              :percentage="entry.percent"
              :show-text="false"
              color="#b67a42"
              :stroke-width="12"
            />
            <p class="touch-desc">按当前板块最高值折算，用来看这一类提醒是否明显抬头。</p>
          </article>
        </div>
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import VChart from 'vue-echarts'

interface StatCard {
  title: string
  value: number
}

interface TrendingTopic {
  topic?: string
  keyword?: string
  count: number
}

interface FeedbackSlide {
  chip: string
  title: string
  desc: string
  metric: string
}

const props = defineProps<{
  statsCards: StatCard[]
  trendingTopics: TrendingTopic[]
  emotionTrendsOption: Record<string, unknown>
}>()

const maxStatValue = computed(() => {
  const values = props.statsCards.map((item) => Number(item.value || 0))
  return Math.max(1, ...values)
})

const feedbackSlides = computed<FeedbackSlide[]>(() => {
  const hotTopic = props.trendingTopics[0]
  const hotTopicName = hotTopic?.topic || hotTopic?.keyword || ''
  return [
    {
      chip: '实时观察',
      title: '湖面当前仍在可控区间',
      desc: '大部分指标没有出现突刺，适合把注意力放在趋势变化和个别异常反馈上。',
      metric: `在线人数 ${props.statsCards.find((s) => s.title.includes('在线'))?.value ?? 0}`,
    },
    {
      chip: '高频话题',
      title: hotTopicName ? `热度焦点：${hotTopicName}` : '热度焦点持续更新中',
      desc: hotTopic
        ? '建议围绕高频话题补充更短、更温和的引导文案。'
        : '当前没有明显集中话题，可以继续观察水位波动。',
      metric: hotTopic ? `讨论量 ${hotTopic.count}` : '讨论量 0',
    },
    {
      chip: '服务建议',
      title: '把关怀提示前置到高峰前',
      desc: '当表达密度开始抬升时，提前一格给出支持提示，比事后补救更有效。',
      metric: `今日投石 ${props.statsCards.find((s) => s.title.includes('投石'))?.value ?? 0}`,
    },
  ]
})

const quickSignals = computed(() => props.statsCards.slice(0, 3).map((item) => ({
  label: item.title,
  value: Number(item.value || 0).toLocaleString(),
})))

const touchRateRows = computed(() => props.statsCards.map((item) => {
  const value = Number(item.value || 0)
  const percent = Math.round((value / maxStatValue.value) * 100)
  return {
    title: item.title,
    display: value.toLocaleString(),
    percent: Number.isFinite(percent) ? percent : 0,
  }
}))
</script>

<style scoped lang="scss">
.care-feedback-row {
  margin-bottom: 20px;
}

.care-card {
  position: relative;
  overflow: hidden;
  border: 1px solid rgba(123, 149, 160, 0.14);
  border-radius: 28px;
  background:
    linear-gradient(180deg, rgba(254, 253, 251, 0.94), rgba(242, 247, 248, 0.96)),
    radial-gradient(circle at right top, rgba(123, 160, 173, 0.12), transparent 28%);
}

.section-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;

  h3 {
    margin: 8px 0 0;
    color: var(--hl-ink);
    font-family: var(--hl-font-display);
    font-size: clamp(22px, 3vw, 28px);
    line-height: 1.08;
  }
}

.section-eyebrow {
  display: inline-flex;
  min-height: 26px;
  align-items: center;
  padding: 0 10px;
  border-radius: 999px;
  background: rgba(17, 62, 74, 0.08);
  color: var(--m3-primary);
  font-family: var(--hl-font-mono);
  font-size: 10px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.section-note {
  color: var(--hl-ink-soft);
  font-size: 12px;
  font-weight: 600;
}

.feedback-slide {
  display: grid;
  align-content: start;
  height: 100%;
  padding: 24px;
  border-radius: 24px;
  background:
    linear-gradient(150deg, rgba(255, 255, 255, 0.94), rgba(239, 245, 247, 0.96)),
    radial-gradient(circle at right top, rgba(182, 122, 66, 0.14), transparent 32%);
  border: 1px solid rgba(123, 149, 160, 0.14);
}

.slide-chip {
  display: inline-flex;
  width: fit-content;
  min-height: 30px;
  align-items: center;
  padding: 0 12px;
  border-radius: 999px;
  background: rgba(17, 62, 74, 0.08);
  color: var(--m3-primary);
  font-family: var(--hl-font-mono);
  font-size: 11px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.slide-title {
  margin: 18px 0 0;
  color: var(--hl-ink);
  font-family: var(--hl-font-display);
  font-size: clamp(24px, 3vw, 34px);
  line-height: 1.08;
}

.slide-desc {
  margin: 14px 0 0;
  color: var(--hl-ink-soft);
  font-size: 14px;
  line-height: 1.8;
}

.slide-metric {
  margin-top: 22px;
  color: var(--hl-ink);
  font-size: 14px;
  font-weight: 700;
}

.trend-card__body {
  display: grid;
  gap: 16px;
}

.trend-card__meta {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
}

.trend-chip {
  padding: 14px 16px;
  border-radius: 18px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.62);

  span {
    display: block;
    color: var(--hl-ink-soft);
    font-size: 12px;
  }

  strong {
    display: block;
    margin-top: 8px;
    color: var(--hl-ink);
    font-family: var(--hl-font-display);
    font-size: clamp(22px, 2.8vw, 28px);
    line-height: 1;
  }
}

.trend-chart {
  height: 250px;
}

.touch-list {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 16px;
}

.touch-item {
  padding: 16px;
  border-radius: 20px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.64);
}

.touch-head {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 12px;
}

.touch-title {
  color: var(--hl-ink-soft);
  font-size: 13px;
}

.touch-value {
  color: var(--hl-ink);
  font-size: 13px;
  font-weight: 700;
}

.touch-desc {
  margin: 12px 0 0;
  color: var(--hl-ink-soft);
  font-size: 12px;
  line-height: 1.6;
}

@media (max-width: 1100px) {
  .touch-list {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 900px) {
  .section-head {
    flex-direction: column;
  }

  .trend-card__meta {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 640px) {
  .touch-list {
    grid-template-columns: 1fr;
  }
}
</style>
