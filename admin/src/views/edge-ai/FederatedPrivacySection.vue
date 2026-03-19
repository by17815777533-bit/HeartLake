<!--
  联邦学习 + 差分隐私区重构：
  - 左侧做成处理进度舱
  - 右侧做成预算舱，统一视觉语义
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col
      :xs="24"
      :lg="12"
    >
      <el-card
        shadow="hover"
        class="chart-card fed-card"
      >
        <template #header>
          <div class="fed-head">
            <div>
              <span class="fed-eyebrow">Federated Flow</span>
              <h3>处理进度</h3>
            </div>
            <el-tag
              :type="federated.status === 'aggregating' ? 'warning' : 'info'"
              size="small"
            >
              {{ federated.status === 'aggregating' ? '进行中' : '就绪' }}
            </el-tag>
          </div>
        </template>
        <div class="federated-panel">
          <div class="fed-stats">
            <article class="fed-stat-item">
              <span>当前批次</span>
              <strong>{{ federated.currentRound }}</strong>
            </article>
            <article class="fed-stat-item">
              <span>参与服务</span>
              <strong>{{ federated.participatingNodes }}</strong>
            </article>
            <article class="fed-stat-item">
              <span>处理准确度</span>
              <strong>{{ federated.modelAccuracy }}</strong>
            </article>
            <article class="fed-stat-item">
              <span>完成速度</span>
              <strong>{{ federated.convergenceRate }}</strong>
            </article>
          </div>
          <div class="fed-progress">
            <span class="fed-progress-label">当前进度</span>
            <el-progress
              :percentage="federated.aggregationProgress"
              :stroke-width="12"
              color="#2f6b78"
              striped
              striped-flow
            />
          </div>
          <div class="fed-actions">
            <el-button
              type="primary"
              :loading="federated.aggregating"
              :disabled="federated.status === 'aggregating'"
              @click="$emit('triggerAggregation')"
            >
              开始处理
            </el-button>
            <el-button @click="$emit('refreshFederated')">
              重新获取
            </el-button>
          </div>
        </div>
      </el-card>
    </el-col>

    <el-col
      :xs="24"
      :lg="12"
    >
      <el-card
        shadow="hover"
        class="chart-card fed-card"
      >
        <template #header>
          <div class="fed-head">
            <div>
              <span class="fed-eyebrow">Privacy Budget</span>
              <h3>使用额度</h3>
            </div>
            <el-tag size="small">
              用量
            </el-tag>
          </div>
        </template>
        <div class="privacy-panel">
          <div class="budget-main">
            <div class="epsilon">
              {{ privacy.epsilonUsed }} / {{ privacy.epsilonTotal }}
            </div>
            <el-progress
              :percentage="privacy.epsilonPercent"
              :stroke-width="16"
              :color="privacyBudgetColor"
            />
            <span class="budget-hint">剩余额度: {{ privacy.epsilonRemaining }}</span>
          </div>
          <div class="budget-chart">
            <v-chart
              :option="privacyPieOption"
              autoresize
              class="privacy-chart"
            />
          </div>
        </div>
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
import VChart from 'vue-echarts'

interface FederatedState {
  status: string
  currentRound: number
  participatingNodes: number
  modelAccuracy: string
  convergenceRate: string
  aggregationProgress: number
  aggregating: boolean
}

interface PrivacyState {
  epsilonUsed: string | number
  epsilonTotal: string | number
  epsilonPercent: number
  epsilonRemaining: string | number
}

defineProps<{
  federated: FederatedState
  privacy: PrivacyState
  privacyBudgetColor: string
  privacyPieOption: Record<string, unknown>
}>()

defineEmits<{
  (e: 'triggerAggregation'): void
  (e: 'refreshFederated'): void
}>()
</script>

<style scoped lang="scss">
.fed-card {
  position: relative;
  overflow: hidden;
}

.fed-head {
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

.fed-eyebrow {
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

.federated-panel,
.privacy-panel {
  display: grid;
  gap: 18px;
}

.fed-stats {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.fed-stat-item {
  padding: 16px;
  border-radius: 20px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.66);

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
    font-size: clamp(22px, 2.8vw, 30px);
    line-height: 1.04;
  }
}

.fed-progress-label,
.budget-hint {
  color: var(--hl-ink-soft);
  font-size: 12px;
  font-weight: 600;
}

.fed-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
}

.budget-main {
  padding: 18px;
  border-radius: 24px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.64);
}

.epsilon {
  margin-bottom: 14px;
  color: var(--hl-ink);
  font-family: var(--hl-font-display);
  font-size: clamp(28px, 3vw, 36px);
  line-height: 1;
}

.budget-hint {
  display: inline-block;
  margin-top: 12px;
}

.budget-chart {
  padding: 16px;
  border-radius: 24px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.64);
}

.privacy-chart {
  height: 240px;
}

@media (max-width: 900px) {
  .fed-head {
    flex-direction: column;
  }
}

@media (max-width: 640px) {
  .fed-stats {
    grid-template-columns: 1fr;
  }
}
</style>
