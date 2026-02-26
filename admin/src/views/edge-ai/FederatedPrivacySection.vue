<!--
  @file FederatedPrivacySection.vue
  @brief 联邦学习控制 + 差分隐私预算监控
  Created by 林子怡
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <!-- 联邦学习控制 -->
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
            <span>联邦学习控制</span>
            <el-tag
              :type="federated.status === 'aggregating' ? 'warning' : 'info'"
              size="small"
            >
              {{ federated.status === 'aggregating' ? '聚合中' : '就绪' }}
            </el-tag>
          </div>
        </template>
        <div class="federated-panel">
          <div class="fed-stats">
            <div class="fed-stat-item">
              <span class="fed-stat-value">{{ federated.currentRound }}</span>
              <span class="fed-stat-label">当前轮次</span>
            </div>
            <div class="fed-stat-item">
              <span class="fed-stat-value">{{ federated.participatingNodes }}</span>
              <span class="fed-stat-label">参与节点</span>
            </div>
            <div class="fed-stat-item">
              <span class="fed-stat-value">{{ federated.modelAccuracy }}</span>
              <span class="fed-stat-label">模型精度</span>
            </div>
            <div class="fed-stat-item">
              <span class="fed-stat-value">{{ federated.convergenceRate }}</span>
              <span class="fed-stat-label">收敛速率</span>
            </div>
          </div>
          <div class="fed-progress">
            <span class="fed-progress-label">聚合进度</span>
            <el-progress
              :percentage="federated.aggregationProgress"
              :stroke-width="12"
              :color="'#1565C0'"
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
              触发聚合
            </el-button>
            <el-button @click="$emit('refreshFederated')">
              刷新状态
            </el-button>
          </div>
        </div>
      </el-card>
    </el-col>

    <!-- 差分隐私预算 -->
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
            <span>差分隐私预算</span>
            <el-tag size="small">
              ε 监控
            </el-tag>
          </div>
        </template>
        <div class="privacy-panel">
          <div class="budget-overview">
            <div class="budget-main">
              <div class="epsilon">
                ε = {{ privacy.epsilonUsed }} / {{ privacy.epsilonTotal }}
              </div>
              <el-progress
                :percentage="privacy.epsilonPercent"
                :stroke-width="16"
                :color="privacyBudgetColor"
              />
              <span class="budget-hint">剩余预算: {{ privacy.epsilonRemaining }}</span>
            </div>
          </div>
          <div class="budget-chart">
            <v-chart
              :option="privacyPieOption"
              autoresize
              style="height: 200px"
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
