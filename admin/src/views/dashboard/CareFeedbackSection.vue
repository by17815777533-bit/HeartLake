<!--
  关怀反馈面板 -- 热门话题词云 + 情绪趋势缩略图

  左侧展示热门话题 Top10 的关键词和热度，右侧展示情绪三维趋势折线的缩略版。
  作为 Dashboard 中间区域的快速概览，详细图表在 ChartsSection 和 AITrendsSection 中。
-->
<template>
  <el-row
    :gutter="20"
    class="care-feedback-row"
  >
    <el-col
      :xs="24"
      :sm="24"
      :md="10"
    >
      <el-card
        shadow="hover"
        class="care-card feedback-carousel-card"
      >
        <template #header>
          <div class="card-title">
            旅人回声
          </div>
        </template>
        <el-carousel
          indicator-position="outside"
          height="240px"
          :interval="5000"
          trigger="click"
          autoplay
        >
          <el-carousel-item
            v-for="slide in feedbackSlides"
            :key="slide.title"
          >
            <div class="feedback-slide">
              <div class="slide-chip">
                {{ slide.chip }}
              </div>
              <div class="slide-title">
                {{ slide.title }}
              </div>
              <div class="slide-desc">
                {{ slide.desc }}
              </div>
              <div class="slide-metric">
                {{ slide.metric }}
              </div>
            </div>
          </el-carousel-item>
        </el-carousel>
      </el-card>
    </el-col>

    <el-col
      :xs="24"
      :sm="24"
      :md="14"
    >
      <el-card
        shadow="hover"
        class="care-card trend-card"
      >
        <template #header>
          <div class="card-title">
            情绪水位
          </div>
        </template>
        <v-chart
          :option="emotionTrendsOption"
          autoresize
          style="height: 240px"
          role="img"
          aria-label="情绪反馈趋势图"
        />
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
          <div class="card-title">
            关怀触达
          </div>
        </template>
        <div class="touch-list">
          <div
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
              color="#f7b44d"
              :stroke-width="12"
            />
          </div>
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
      title: '社区整体状态稳定',
      desc: '活跃用户和互动数量保持在安全区间，系统持续巡检中。',
      metric: `在线人数 ${props.statsCards.find((s) => s.title.includes('在线'))?.value ?? 0}`,
    },
    {
      chip: '关注重点',
      title: hotTopicName ? `热点话题：${hotTopicName}` : '热点话题持续更新中',
      desc: hotTopic
        ? '建议在高频话题中增加温和提示与主动关怀文案。'
        : '当前暂无明显热点，建议继续观察社区波动。',
      metric: hotTopic ? `讨论量 ${hotTopic.count}` : '讨论量 0',
    },
    {
      chip: '服务建议',
      title: '引导语义可读性提升',
      desc: '在高情绪波动时段，优先展示简短、可执行的引导语。',
      metric: `今日投石 ${props.statsCards.find((s) => s.title.includes('投石'))?.value ?? 0}`,
    },
  ]
})

const touchRateRows = computed(() => {
  return props.statsCards.map((item) => {
    const value = Number(item.value || 0)
    const percent = Math.round((value / maxStatValue.value) * 100)
    return {
      title: item.title,
      display: value.toLocaleString(),
      percent: Number.isFinite(percent) ? percent : 0,
    }
  })
})
</script>

<style scoped lang="scss">
.care-feedback-row {
  margin-bottom: 20px;
}

.care-card {
  border: 1px solid rgba(123, 149, 160, 0.14);
  border-radius: 24px;
  background:
    linear-gradient(180deg, rgba(254, 253, 251, 0.94), rgba(242, 247, 248, 0.96)),
    radial-gradient(circle at right top, rgba(123, 160, 173, 0.12), transparent 28%);
}

.card-title {
  font-size: 16px;
  font-weight: 600;
  color: var(--hl-ink);
}

.feedback-slide {
  height: 100%;
  padding: 22px 20px;
  border-radius: 22px;
  background:
    linear-gradient(150deg, rgba(255, 255, 255, 0.92), rgba(239, 245, 247, 0.96)),
    radial-gradient(circle at right top, rgba(182, 122, 66, 0.12), transparent 30%);
  border: 1px solid rgba(123, 149, 160, 0.14);
}

.slide-chip {
  display: inline-flex;
  padding: 4px 10px;
  border-radius: 999px;
  background: rgba(17, 62, 74, 0.08);
  color: var(--m3-primary);
  font-size: 12px;
  font-weight: 600;
}

.slide-title {
  margin-top: 14px;
  font-size: 20px;
  font-weight: 700;
  color: var(--hl-ink);
}

.slide-desc {
  margin-top: 12px;
  line-height: 1.7;
  color: var(--hl-ink-soft);
  font-size: 14px;
}

.slide-metric {
  margin-top: 18px;
  font-size: 14px;
  color: var(--hl-ink);
  font-weight: 600;
}

.touch-list {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.touch-item {
  padding: 14px;
  border-radius: 18px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.64);
}

.touch-head {
  display: flex;
  justify-content: space-between;
  margin-bottom: 10px;
}

.touch-title {
  color: var(--hl-ink-soft);
  font-size: 13px;
}

.touch-value {
  color: var(--hl-ink);
  font-size: 13px;
  font-weight: 600;
}

@media (max-width: 960px) {
  .touch-list {
    grid-template-columns: 1fr;
  }
}
</style>
