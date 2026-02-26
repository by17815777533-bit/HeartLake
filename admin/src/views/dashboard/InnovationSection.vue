<!--
  @file InnovationSection.vue
  @brief 创新功能区 - 隐私保护统计 + 情绪共鸣统计 + 情绪脉搏
  Created by 林子怡
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
            <span>🔒 隐私保护统计</span>
          </div>
        </template>
        <div
          v-loading="privacyLoading"
          class="innovation-stats"
        >
          <div class="inno-stat-item">
            <span class="inno-label">差分隐私查询次数</span>
            <span class="inno-value">{{ formatNumber(privacyStats.queryCount) }}</span>
          </div>
          <div class="inno-stat-item">
            <span class="inno-label">隐私预算消耗 (ε)</span>
            <span class="inno-value epsilon">{{ privacyStats.epsilonUsed.toFixed(2) }} / {{ privacyStats.epsilonTotal.toFixed(1) }}</span>
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
            <span class="inno-label">保护的用户数</span>
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
            <span>💫 情绪共鸣统计</span>
          </div>
        </template>
        <div
          v-loading="resonanceLoading"
          class="innovation-stats"
        >
          <div class="inno-stat-item">
            <span class="inno-label">今日共鸣匹配数</span>
            <span class="inno-value">{{ formatNumber(resonanceStats.todayMatches) }}</span>
          </div>
          <div class="inno-stat-item">
            <span class="inno-label">平均共鸣分数</span>
            <span class="inno-value highlight">{{ resonanceStats.avgScore.toFixed(1) }}</span>
          </div>
          <div class="inno-stat-item">
            <span class="inno-label">最活跃的情绪类型</span>
            <el-tag
              size="small"
              type="primary"
            >
              {{ resonanceStats.topMood || '暂无' }}
            </el-tag>
          </div>
          <div class="inno-stat-item">
            <span class="inno-label">共鸣成功率</span>
            <span class="inno-value">{{ resonanceStats.successRate.toFixed(1) }}%</span>
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
            <span>🌊 湖面情绪温度</span>
            <span class="pulse-hint">每30秒更新</span>
          </div>
        </template>
        <v-chart
          :option="emotionPulseOption"
          autoresize
          style="height: 260px"
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
