<!--
  图表区域 -- 四张图表卡片

  上排：用户增长折线图（支持 7/14/30 天切换）+ 情绪分布环形饼图
  下排：情绪趋势多线图（支持 7/14/30 天切换）+ 24h 活跃时段柱状图 + 热门话题 Top10

  图表范围切换通过 v-model 双向绑定 chartRange / moodTrendRange，
  切换时 emit load-growth / load-mood-trend 事件触发父组件重新拉取数据。
-->
<template>
  <!-- 用户增长 + 情绪分布 -->
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col
      :xs="24"
      :sm="24"
      :md="12"
    >
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          <div class="card-header">
            <span>用户增长趋势</span>
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
          style="height: 300px"
          role="img"
          aria-label="用户增长趋势图表"
        />
      </el-card>
    </el-col>

    <el-col
      :xs="24"
      :sm="24"
      :md="12"
    >
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          情绪分布
        </template>
        <v-chart
          :option="moodDistributionOption"
          autoresize
          style="height: 300px"
          role="img"
          aria-label="情绪分布图表"
        />
      </el-card>
    </el-col>
  </el-row>

  <!-- 情绪趋势分析 -->
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col :span="24">
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          <div class="card-header">
            <span>情绪趋势分析</span>
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
          style="height: 280px"
          role="img"
          aria-label="情绪趋势分析图表"
        />
      </el-card>
    </el-col>
  </el-row>

  <!-- 热门话题 + 活跃时段 -->
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col :span="12">
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          热门话题
        </template>
        <div class="topics-list">
          <div
            v-for="(topic, i) in trendingTopics"
            :key="topic.topic || topic.keyword || i"
            class="topic-item"
          >
            <span
              class="topic-rank"
              :class="{ top: i < 3 }"
            >{{ i + 1 }}</span>
            <span class="topic-name">{{ topic.topic || topic.keyword || '未命名话题' }}</span>
            <span class="topic-count">{{ topic.count }} 条</span>
          </div>
          <el-empty
            v-if="!trendingTopics.length"
            description="暂无数据"
            :image-size="60"
          />
        </div>
      </el-card>
    </el-col>
    <el-col :span="12">
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          活跃时段分布
        </template>
        <v-chart
          :option="activeTimeOption"
          autoresize
          style="height: 300px"
          role="img"
          aria-label="活跃时段分布图表"
        />
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
import VChart from 'vue-echarts'

interface TrendingTopic {
  topic?: string
  keyword?: string
  count: number
}

defineProps<{
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
</script>
