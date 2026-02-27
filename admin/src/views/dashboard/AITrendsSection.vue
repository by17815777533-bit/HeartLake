<!--
  AI 情绪趋势 + AI 热门内容推荐
-->

<template>
  <el-row
    :gutter="20"
    style="margin-bottom: 20px"
  >
    <el-col
      :xs="24"
      :sm="24"
      :md="14"
    >
      <el-card
        shadow="hover"
        class="chart-card ai-trends-card"
      >
        <template #header>
          <div class="card-header">
            <span>📈 AI 情绪趋势</span>
            <el-tag
              size="small"
              type="info"
            >
              近7天
            </el-tag>
          </div>
        </template>
        <v-chart
          :option="emotionTrendsOption"
          autoresize
          style="height: 280px"
          role="img"
          aria-label="AI情绪趋势图表"
        />
      </el-card>
    </el-col>
    <el-col
      :xs="24"
      :sm="24"
      :md="10"
    >
      <el-card
        shadow="hover"
        class="chart-card ai-trending-card"
      >
        <template #header>
          <div class="card-header">
            <span>🔥 AI 热门内容</span>
            <el-tag
              size="small"
              type="warning"
            >
              推荐引擎
            </el-tag>
          </div>
        </template>
        <div class="trending-content-list">
          <div
            v-for="(item, i) in aiTrendingContent"
            :key="i"
            class="trending-item"
          >
            <span
              class="trending-rank"
              :class="{ top: i < 3 }"
            >{{ i + 1 }}</span>
            <div class="trending-info">
              <span class="trending-title">{{ item.title || item.content || item.text || '未知内容' }}</span>
              <div class="trending-meta">
                <el-tag
                  v-if="item.emotion || item.mood"
                  size="small"
                  :type="(item.emotion || item.mood) === 'positive' ? 'success' : (item.emotion || item.mood) === 'negative' ? 'danger' : 'info'"
                >
                  {{ item.emotion || item.mood || '中性' }}
                </el-tag>
                <span
                  v-if="item.score != null"
                  class="trending-score"
                >匹配度 {{ (item.score * 100).toFixed(0) }}%</span>
              </div>
            </div>
          </div>
          <el-empty
            v-if="!aiTrendingContent.length"
            description="暂无推荐数据"
            :image-size="60"
          />
        </div>
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
import VChart from 'vue-echarts'

interface TrendingItem {
  title?: string
  content?: string
  text?: string
  emotion?: string
  mood?: string
  score?: number
}

defineProps<{
  emotionTrendsOption: Record<string, unknown>
  aiTrendingContent: TrendingItem[]
}>()
</script>
