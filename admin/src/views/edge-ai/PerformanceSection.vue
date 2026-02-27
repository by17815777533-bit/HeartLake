<!--
  性能指标 + 情绪分析（仪表盘 + 脉搏折线）
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col
      :xs="24"
      :md="12"
    >
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          <div class="card-header">
            <span>性能指标</span>
            <el-tag
              type="success"
              size="small"
            >
              30s 刷新
            </el-tag>
          </div>
        </template>
        <div class="metrics-grid">
          <div
            v-for="m in performanceMetrics"
            :key="m.label"
            class="metric-item"
          >
            <div
              class="metric-value"
              :style="{ color: m.color }"
            >
              {{ m.value }}
            </div>
            <div class="metric-label">
              {{ m.label }}
            </div>
            <el-progress
              :percentage="m.percent"
              :color="m.color"
              :stroke-width="6"
              :show-text="false"
            />
          </div>
        </div>
      </el-card>
    </el-col>
    <el-col
      :xs="24"
      :md="12"
    >
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          <div class="card-header">
            <span>情绪分析</span>
            <el-tag
              type="warning"
              size="small"
            >
              30s 刷新
            </el-tag>
          </div>
        </template>
        <div class="emotion-pulse-container">
          <div class="gauge-wrapper">
            <v-chart
              :option="emotionGaugeOption"
              autoresize
              style="height: 220px"
            />
          </div>
          <div class="pulse-line-wrapper">
            <v-chart
              :option="emotionPulseLineOption"
              autoresize
              style="height: 220px"
            />
          </div>
        </div>
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
import VChart from 'vue-echarts'

interface MetricItem {
  label: string
  value: string
  percent: number
  color: string
}

defineProps<{
  performanceMetrics: MetricItem[]
  emotionGaugeOption: Record<string, unknown>
  emotionPulseLineOption: Record<string, unknown>
}>()
</script>
