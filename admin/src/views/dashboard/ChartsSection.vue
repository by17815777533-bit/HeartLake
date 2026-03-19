<!--
  图表区重构：
  - 强化“趋势主图 + 分布侧图 + 热点榜单 + 活跃节律”的叙事顺序
  - 热门话题从普通列表改成带热度轨迹的榜单卡
  - 图表标题统一加入辅助文案，弱化模板化观感
-->
<template>
  <el-row
    :gutter="20"
    class="charts-row charts-row--feature"
  >
    <el-col
      :xs="24"
      :lg="15"
    >
      <el-card
        shadow="hover"
        class="chart-card feature-card"
      >
        <template #header>
          <div class="chart-head">
            <div>
              <span class="chart-eyebrow">Growth Arc</span>
              <div class="chart-title-row">
                <span class="chart-title">旅人增长轨迹</span>
                <p class="chart-copy">看新增旅人的变化，判断湖面是在扩散、回稳，还是进入短暂静水期。</p>
              </div>
            </div>
            <el-radio-group
              :model-value="chartRange"
              size="small"
              @update:model-value="$emit('update:chartRange', $event)"
              @change="$emit('loadGrowth', $event)"
            >
              <el-radio-button :value="7">
                7天
              </el-radio-button>
              <el-radio-button :value="14">
                14天
              </el-radio-button>
              <el-radio-button :value="30">
                30天
              </el-radio-button>
            </el-radio-group>
          </div>
        </template>
        <v-chart
          :option="userGrowthOption"
          autoresize
          class="feature-chart"
          role="img"
          aria-label="用户增长趋势图表"
        />
      </el-card>
    </el-col>

    <el-col
      :xs="24"
      :lg="9"
    >
      <el-card
        shadow="hover"
        class="chart-card mood-card"
      >
        <template #header>
          <div class="chart-head chart-head--stack">
            <span class="chart-eyebrow">Mood Ring</span>
            <span class="chart-title">情绪环流</span>
            <p class="chart-copy">看哪类表达正在占据今天的主水位。</p>
          </div>
        </template>
        <v-chart
          :option="moodDistributionOption"
          autoresize
          class="feature-chart feature-chart--mood"
          role="img"
          aria-label="情绪分布图表"
        />
      </el-card>
    </el-col>
  </el-row>

  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col :span="24">
      <el-card
        shadow="hover"
        class="chart-card trend-card"
      >
        <template #header>
          <div class="chart-head">
            <div>
              <span class="chart-eyebrow">Emotional Drift</span>
              <div class="chart-title-row">
                <span class="chart-title">情绪潮汐</span>
                <p class="chart-copy">把积极、中性、消极三条曲线放在同一水位线上，判断情绪是否在拐头。</p>
              </div>
            </div>
            <el-radio-group
              :model-value="moodTrendRange"
              size="small"
              @update:model-value="$emit('update:moodTrendRange', $event)"
              @change="$emit('loadMoodTrend', $event)"
            >
              <el-radio-button :value="7">
                7天
              </el-radio-button>
              <el-radio-button :value="14">
                14天
              </el-radio-button>
            </el-radio-group>
          </div>
        </template>
        <v-chart
          :option="moodTrendOption"
          autoresize
          class="feature-chart feature-chart--wide"
          role="img"
          aria-label="情绪趋势分析图表"
        />
      </el-card>
    </el-col>
  </el-row>

  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col
      :xs="24"
      :lg="9"
    >
      <el-card
        shadow="hover"
        class="chart-card topic-card"
      >
        <template #header>
          <div class="chart-head chart-head--stack">
            <span class="chart-eyebrow">Topic Heat</span>
            <span class="chart-title">热门话题榜</span>
            <p class="chart-copy">高频关键词会先在这里浮起来，再蔓延到其他图表区域。</p>
          </div>
        </template>

        <div
          v-if="topicRows.length"
          class="topic-list"
        >
          <article
            v-for="topic in topicRows"
            :key="topic.name"
            class="topic-item"
          >
            <div class="topic-item__top">
              <div class="topic-item__identity">
                <span
                  class="topic-rank"
                  :class="{ 'is-top': topic.rank <= 3 }"
                >
                  {{ topic.rank }}
                </span>
                <strong>{{ topic.name }}</strong>
              </div>
              <span class="topic-count">{{ topic.count }} 条</span>
            </div>
            <div class="topic-bar">
              <span :style="{ width: `${topic.percent}%` }" />
            </div>
          </article>
        </div>

        <el-empty
          v-else
          description="暂无话题热度"
          :image-size="72"
        />
      </el-card>
    </el-col>

    <el-col
      :xs="24"
      :lg="15"
    >
      <el-card
        shadow="hover"
        class="chart-card cadence-card"
      >
        <template #header>
          <div class="chart-head chart-head--stack">
            <span class="chart-eyebrow">Daily Rhythm</span>
            <span class="chart-title">活跃节律</span>
            <p class="chart-copy">值守时段应该追着活跃高峰走，而不是平均用力。</p>
          </div>
        </template>
        <v-chart
          :option="activeTimeOption"
          autoresize
          class="feature-chart"
          role="img"
          aria-label="活跃时段分布图表"
        />
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import VChart from 'vue-echarts'

interface TrendingTopic {
  topic?: string
  keyword?: string
  count: number
}

const props = defineProps<{
  chartRange: number
  moodTrendRange: number
  trendingTopics: TrendingTopic[]
  userGrowthOption: Record<string, unknown>
  moodDistributionOption: Record<string, unknown>
  moodTrendOption: Record<string, unknown>
  activeTimeOption: Record<string, unknown>
}>()

defineEmits<{
  (e: 'update:chartRange', val: number): void
  (e: 'update:moodTrendRange', val: number): void
  (e: 'loadGrowth', val: number): void
  (e: 'loadMoodTrend', val: number): void
}>()

const topicRows = computed(() => {
  const maxCount = Math.max(1, ...props.trendingTopics.map((item) => Number(item.count || 0)))
  return props.trendingTopics.slice(0, 6).map((item, index) => ({
    rank: index + 1,
    name: item.topic || item.keyword || `未命名话题 ${index + 1}`,
    count: Number(item.count || 0),
    percent: Math.max(8, Math.round((Number(item.count || 0) / maxCount) * 100)),
  }))
})
</script>

<style scoped lang="scss">
.charts-row {
  margin-bottom: 20px;
}

.feature-card,
.mood-card,
.trend-card,
.topic-card,
.cadence-card {
  position: relative;
  overflow: hidden;
}

.feature-card::before,
.mood-card::before,
.trend-card::before,
.topic-card::before,
.cadence-card::before {
  content: '';
  position: absolute;
  inset: 0;
  pointer-events: none;
  background:
    radial-gradient(circle at right top, rgba(123, 160, 173, 0.12), transparent 26%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.12), transparent 48%);
}

.chart-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.chart-head--stack {
  display: grid;
  gap: 8px;
}

.chart-title-row {
  display: grid;
  gap: 8px;
  margin-top: 8px;
}

.chart-eyebrow {
  display: inline-flex;
  width: fit-content;
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

.chart-title {
  color: var(--hl-ink);
  font-family: var(--hl-font-display);
  font-size: clamp(22px, 3vw, 30px);
  line-height: 1.08;
}

.chart-copy {
  margin: 0;
  max-width: 34rem;
  color: var(--hl-ink-soft);
  font-size: 13px;
  line-height: 1.7;
}

.feature-chart {
  height: 320px;
}

.feature-chart--mood {
  height: 320px;
}

.feature-chart--wide {
  height: 300px;
}

.topic-list {
  display: grid;
  gap: 14px;
}

.topic-item {
  padding: 16px;
  border-radius: 22px;
  border: 1px solid rgba(118, 145, 155, 0.14);
  background:
    linear-gradient(160deg, rgba(255, 255, 255, 0.76), rgba(239, 244, 246, 0.92));
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.44);
}

.topic-item__top {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.topic-item__identity {
  display: flex;
  align-items: center;
  gap: 10px;

  strong {
    color: var(--hl-ink);
    font-size: 14px;
    font-weight: 700;
  }
}

.topic-rank {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  border-radius: 999px;
  background: rgba(17, 62, 74, 0.08);
  color: var(--m3-primary);
  font-family: var(--hl-font-mono);
  font-size: 11px;
  font-weight: 700;
}

.topic-rank.is-top {
  background: rgba(182, 122, 66, 0.14);
  color: #a96e37;
}

.topic-count {
  color: var(--hl-ink-soft);
  font-size: 12px;
  font-weight: 600;
}

.topic-bar {
  margin-top: 12px;
  height: 8px;
  border-radius: 999px;
  background: rgba(118, 145, 155, 0.12);
  overflow: hidden;

  span {
    display: block;
    height: 100%;
    border-radius: inherit;
    background: linear-gradient(90deg, #b67a42, #2f6b78);
  }
}

@media (max-width: 900px) {
  .chart-head {
    flex-direction: column;
  }
}
</style>
