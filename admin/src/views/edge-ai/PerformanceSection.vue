<!--
  性能区重构：
  - 左侧改为四张紧凑指标卡 + 说明
  - 右侧保留双图，但做成统一脉搏面板
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col
      :xs="24"
      :lg="11"
    >
      <el-card
        shadow="hover"
        class="chart-card performance-card"
      >
        <template #header>
          <div class="perf-head">
            <div>
              <span class="perf-eyebrow">Engine Snapshot</span>
              <h3>表现概览</h3>
            </div>
            <span class="perf-note">30 秒更新</span>
          </div>
        </template>

        <div class="metrics-grid">
          <article
            v-for="m in performanceMetrics"
            :key="m.label"
            class="metric-item"
          >
            <span class="metric-label">{{ m.label }}</span>
            <strong
              class="metric-value"
              :style="{ color: m.color }"
            >
              {{ m.value }}
            </strong>
            <el-progress
              :percentage="m.percent"
              :color="m.color"
              :stroke-width="8"
              :show-text="false"
            />
          </article>
        </div>
      </el-card>
    </el-col>

    <el-col
      :xs="24"
      :lg="13"
    >
      <el-card
        shadow="hover"
        class="chart-card pulse-card"
      >
        <template #header>
          <div class="perf-head">
            <div>
              <span class="perf-eyebrow">Pulse Monitor</span>
              <h3>近期情绪</h3>
            </div>
            <span class="perf-note">实时观察</span>
          </div>
        </template>

        <div class="emotion-pulse-container">
          <div class="gauge-wrapper">
            <v-chart
              :option="emotionGaugeOption"
              autoresize
              class="pulse-chart pulse-chart--gauge"
            />
          </div>
          <div class="pulse-line-wrapper">
            <v-chart
              :option="emotionPulseLineOption"
              autoresize
              class="pulse-chart pulse-chart--line"
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

<style scoped lang="scss">
.performance-card,
.pulse-card {
  position: relative;
  overflow: hidden;
}

.perf-head {
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

.perf-eyebrow {
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

.perf-note {
  color: var(--hl-ink-soft);
  font-size: 12px;
  font-weight: 600;
}

.metrics-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.metric-item {
  padding: 18px 16px;
  border-radius: 22px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.66);
}

.metric-label {
  display: block;
  color: var(--hl-ink-soft);
  font-size: 12px;
}

.metric-value {
  display: block;
  margin: 10px 0 14px;
  font-family: var(--hl-font-display);
  font-size: clamp(24px, 3vw, 34px);
  line-height: 1;
}

.emotion-pulse-container {
  display: grid;
  grid-template-columns: minmax(220px, 280px) 1fr;
  gap: 18px;
  align-items: stretch;
}

.gauge-wrapper,
.pulse-line-wrapper {
  padding: 12px;
  border-radius: 24px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.64);
}

.pulse-chart--gauge {
  height: 250px;
}

.pulse-chart--line {
  height: 250px;
}

@media (max-width: 960px) {
  .perf-head {
    flex-direction: column;
  }

  .metrics-grid {
    grid-template-columns: 1fr;
  }

  .emotion-pulse-container {
    grid-template-columns: 1fr;
  }
}
</style>
