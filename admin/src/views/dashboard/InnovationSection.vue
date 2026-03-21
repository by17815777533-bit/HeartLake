<!--
  创新功能区重构：
  - 三张卡统一成“安全 / 回应 / 氛围”三段式值守面板
  - 既保留原数据，又强化说明和层次
-->
<template>
  <el-row
    :gutter="20"
    class="charts-row innovation-row"
  >
    <el-col
      :xs="24"
      :md="8"
    >
      <el-card
        shadow="hover"
        class="chart-card innovation-card"
      >
        <template #header>
          <div class="innovation-head">
            <div>
              <span class="innovation-eyebrow">Safety Budget</span>
              <h3>安全保护概览</h3>
            </div>
            <span class="innovation-note">今日额度</span>
          </div>
        </template>
        <div
          v-loading="privacyLoading"
          class="innovation-body"
        >
          <div class="innovation-stat">
            <span>今日查询次数</span>
            <strong>{{ formatNumber(privacyStats.queryCount) }}</strong>
          </div>
          <div class="innovation-stat innovation-stat--stack">
            <span>保护额度使用</span>
            <strong>{{ (privacyStats.epsilonUsed ?? 0).toFixed(2) }} / {{ (privacyStats.epsilonTotal ?? 0).toFixed(1) }}</strong>
          </div>
          <div class="budget-bar">
            <el-progress
              :percentage="privacyBudgetPercent"
              :color="privacyBudgetColor"
              :stroke-width="12"
              :show-text="true"
              :format="() => privacyBudgetPercent.toFixed(1) + '%'"
            />
          </div>
          <div class="innovation-stat">
            <span>剩余预算 ε</span>
            <strong>{{ ((privacyStats.epsilonTotal ?? 0) - (privacyStats.epsilonUsed ?? 0)).toFixed(2) }}</strong>
          </div>
        </div>
      </el-card>
    </el-col>

    <el-col
      :xs="24"
      :md="8"
    >
      <el-card
        shadow="hover"
        class="chart-card innovation-card"
      >
        <template #header>
          <div class="innovation-head">
            <div>
              <span class="innovation-eyebrow">Resonance Match</span>
              <h3>关怀回应概览</h3>
            </div>
            <span class="innovation-note">今日匹配</span>
          </div>
        </template>
        <div
          v-loading="resonanceLoading"
          class="innovation-body"
        >
          <div class="innovation-stat">
            <span>今日有效回应</span>
            <strong>{{ formatNumber(resonanceStats.todayMatches) }}</strong>
          </div>
          <div class="innovation-grid">
            <article class="innovation-mini">
              <span>平均评分</span>
              <strong>{{ (resonanceStats.avgScore ?? 0).toFixed(1) }}</strong>
            </article>
            <article class="innovation-mini">
              <span>主导情绪</span>
              <strong>{{ resonanceStats.topMood || '暂无' }}</strong>
            </article>
          </div>
          <div class="innovation-stat">
            <span>主导情绪占比</span>
            <strong>{{ (resonanceStats.successRate ?? 0).toFixed(1) }}%</strong>
          </div>
        </div>
      </el-card>
    </el-col>

    <el-col
      :xs="24"
      :md="8"
    >
      <el-card
        shadow="hover"
        class="chart-card innovation-card innovation-card--pulse"
      >
        <template #header>
          <div class="innovation-head">
            <div>
              <span class="innovation-eyebrow">Pulse Gauge</span>
              <h3>社区氛围</h3>
            </div>
            <span class="innovation-note">30 秒更新</span>
          </div>
        </template>
        <v-chart
          :option="emotionPulseOption"
          autoresize
          class="pulse-chart"
          role="img"
          aria-label="湖面情绪温度仪表盘"
        />
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
import VChart from 'vue-echarts'

interface PrivacyStats {
  queryCount: number
  epsilonUsed: number
  epsilonTotal: number
  protectedUsers: number
}

interface ResonanceStats {
  todayMatches: number
  avgScore: number
  topMood: string
  successRate: number
}

defineProps<{
  privacyLoading: boolean
  resonanceLoading: boolean
  privacyStats: PrivacyStats
  privacyBudgetPercent: number
  privacyBudgetColor: string
  resonanceStats: ResonanceStats
  emotionPulseOption: Record<string, unknown>
  formatNumber: (num: number) => string
}>()
</script>

<style scoped lang="scss">
.innovation-row {
  margin-bottom: 20px;
}

.innovation-card {
  position: relative;
  overflow: hidden;
}

.innovation-card::before {
  content: '';
  position: absolute;
  inset: 0;
  pointer-events: none;
  background:
    radial-gradient(circle at right top, rgba(182, 122, 66, 0.08), transparent 24%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.08), transparent 48%);
}

.innovation-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 14px;

  h3 {
    margin: 8px 0 0;
    color: var(--hl-ink);
    font-family: var(--hl-font-display);
    font-size: clamp(22px, 3vw, 28px);
    line-height: 1.08;
  }
}

.innovation-eyebrow {
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

.innovation-note {
  color: var(--hl-ink-soft);
  font-size: 12px;
  font-weight: 600;
}

.innovation-body {
  display: grid;
  gap: 16px;
}

.innovation-stat,
.innovation-mini {
  padding: 16px;
  border-radius: 20px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.64);

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
    font-size: clamp(24px, 3vw, 30px);
    line-height: 1.04;
  }
}

.innovation-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.budget-bar {
  padding: 4px 2px 0;
}

.pulse-chart {
  height: 290px;
}

@media (max-width: 900px) {
  .innovation-head {
    flex-direction: column;
  }
}

@media (max-width: 640px) {
  .innovation-grid {
    grid-template-columns: 1fr;
  }
}
</style>
