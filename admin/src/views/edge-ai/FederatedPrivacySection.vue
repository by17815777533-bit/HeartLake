<!--
  联邦学习 + 差分隐私面板

  左列：联邦学习控制面板
  - 当前轮次、参与节点数、模型准确率、收敛率
  - 聚合进度条 + 手动触发聚合按钮
  - 刷新联邦状态按钮

  右列：差分隐私预算监控
  - epsilon 消耗/总量/剩余、delta 值、噪声等级
  - 预算消耗进度条（颜色随比例变化）
  - 各子系统 epsilon 分配饼图
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
            <span>处理进度</span>
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
            <div class="fed-stat-item">
              <span class="fed-stat-value">{{ federated.currentRound }}</span>
              <span class="fed-stat-label">当前批次</span>
            </div>
            <div class="fed-stat-item">
              <span class="fed-stat-value">{{ federated.participatingNodes }}</span>
              <span class="fed-stat-label">参与服务</span>
            </div>
            <div class="fed-stat-item">
              <span class="fed-stat-value">{{ federated.modelAccuracy }}</span>
              <span class="fed-stat-label">处理准确度</span>
            </div>
            <div class="fed-stat-item">
              <span class="fed-stat-value">{{ federated.convergenceRate }}</span>
              <span class="fed-stat-label">完成速度</span>
            </div>
          </div>
          <div class="fed-progress">
            <span class="fed-progress-label">当前进度</span>
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
              开始处理
            </el-button>
            <el-button @click="$emit('refreshFederated')">
              重新获取
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
            <span>使用额度</span>
            <el-tag size="small">
              用量
            </el-tag>
          </div>
        </template>
        <div class="privacy-panel">
          <div class="budget-overview">
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
