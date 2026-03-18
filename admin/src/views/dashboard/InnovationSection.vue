<!--
  创新功能区 -- 三列布局展示心湖核心技术指标

  左列：差分隐私预算（epsilon 消耗进度条 + 查询次数 + 受保护用户数）
  中列：DTW 情绪共鸣统计（今日匹配数 + 平均分 + 主导情绪 + 成功率）
  右列：情绪温度仪表盘（0-100 度 gauge 图表）

  隐私预算进度条颜色随消耗比例变化：<50% 绿色 / 50-80% 橙色 / >80% 红色
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <!-- 隐私保护统计 -->
    <el-col
      :xs="24"
      :sm="12"
      :md="8"
    >
      <el-card
        shadow="hover"
        class="chart-card innovation-card"
      >
        <template #header>
          <div class="card-header">
            <span>安全保护概览</span>
          </div>
        </template>
        <div
          v-loading="privacyLoading"
          class="innovation-stats"
        >
          <div class="inno-stat-item">
            <span class="inno-label">今日查询次数</span>
            <span class="inno-value">{{ formatNumber(privacyStats.queryCount) }}</span>
          </div>
          <div class="inno-stat-item">
            <span class="inno-label">保护额度使用</span>
            <span class="inno-value epsilon">{{ (privacyStats.epsilonUsed ?? 0).toFixed(2) }} / {{ (privacyStats.epsilonTotal ?? 0).toFixed(1) }}</span>
          </div>
          <div class="budget-bar">
            <el-progress
              :percentage="privacyBudgetPercent"
              :color="privacyBudgetColor"
              :stroke-width="10"
              :show-text="true"
              :format="() => privacyBudgetPercent.toFixed(1) + '%'"
            />
          </div>
          <div class="inno-stat-item">
            <span class="inno-label">受保护用户</span>
            <span class="inno-value">{{ formatNumber(privacyStats.protectedUsers) }}</span>
          </div>
        </div>
      </el-card>
    </el-col>

    <!-- 情绪共鸣统计 -->
    <el-col
      :xs="24"
      :sm="12"
      :md="8"
    >
      <el-card
        shadow="hover"
        class="chart-card innovation-card"
      >
        <template #header>
          <div class="card-header">
            <span>关怀回应概览</span>
          </div>
        </template>
        <div
          v-loading="resonanceLoading"
          class="innovation-stats"
        >
          <div class="inno-stat-item">
            <span class="inno-label">今日有效回应</span>
            <span class="inno-value">{{ formatNumber(resonanceStats.todayMatches) }}</span>
          </div>
          <div class="inno-stat-item">
            <span class="inno-label">平均回应评分</span>
            <span class="inno-value highlight">{{ (resonanceStats.avgScore ?? 0).toFixed(1) }}</span>
          </div>
          <div class="inno-stat-item">
            <span class="inno-label">最常见情绪</span>
            <el-tag
              size="small"
              type="primary"
            >
              {{ resonanceStats.topMood || '暂无' }}
            </el-tag>
          </div>
          <div class="inno-stat-item">
            <span class="inno-label">回应完成率</span>
            <span class="inno-value">{{ (resonanceStats.successRate ?? 0).toFixed(1) }}%</span>
          </div>
        </div>
      </el-card>
    </el-col>

    <!-- 情绪脉搏 -->
    <el-col
      :xs="24"
      :sm="24"
      :md="8"
    >
      <el-card
        shadow="hover"
        class="chart-card innovation-card"
      >
        <template #header>
          <div class="card-header">
            <span>社区氛围</span>
            <span class="pulse-hint">每30秒更新</span>
          </div>
        </template>
        <v-chart
          :option="emotionPulseOption"
          autoresize
          style="height: 260px"
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
