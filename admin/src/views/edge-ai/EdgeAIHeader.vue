<!--
  EdgeAI 页面头部 -- 标题栏、技术能力标签、四张引擎状态卡片

  props:
  - currentTime: 当前时间（YYYY-MM-DD HH:mm）
  - lastUpdateTime: 上次刷新时间（HH:mm:ss）
  - loading: 刷新按钮 loading 状态
  - techBadges: 技术标签数组（Edge Inference / Federated Learning / DP / ...）
  - statusCards: 引擎状态/模块加载/运行时间/活跃节点四张卡片数据
-->

<template>
  <!-- 页面标题 -->
  <div class="page-header">
    <div class="welcome-section">
      <h1>边缘AI管理</h1>
      <p class="welcome-sub">
        Edge AI 引擎状态监控与管理 · {{ currentTime }}
      </p>
    </div>
    <div class="header-actions">
      <el-button
        type="primary"
        :icon="Refresh"
        :loading="loading"
        @click="$emit('refresh')"
      >
        刷新数据
      </el-button>
      <span class="update-time">最后更新: {{ lastUpdateTime }}</span>
    </div>
  </div>

  <!-- 技术标签 -->
  <div class="tech-badges">
    <span
      v-for="badge in techBadges"
      :key="badge.label"
      class="tech-badge"
    >
      <span class="badge-icon">{{ badge.icon }}</span>
      <span class="badge-label">{{ badge.label }}</span>
    </span>
  </div>

  <!-- 状态卡片 -->
  <el-row
    :gutter="20"
    class="stats-cards"
  >
    <el-col
      v-for="card in statusCards"
      :key="card.title"
      :xs="24"
      :sm="12"
      :md="6"
    >
      <el-card
        shadow="hover"
        class="stat-card"
        :style="{ borderTop: `4px solid ${card.color}` }"
      >
        <div class="stat-content">
          <div class="stat-info">
            <div class="stat-value">
              {{ card.value }}
            </div>
            <div class="stat-title">
              {{ card.title }}
            </div>
          </div>
          <div
            class="stat-icon"
            :style="{ background: `${card.color}15`, color: card.color }"
          >
            <el-icon :size="32">
              <component :is="card.icon" />
            </el-icon>
          </div>
        </div>
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
import { Refresh } from '@element-plus/icons-vue'
import type { Component } from 'vue'

interface TechBadge {
  icon: string
  label: string
}

interface StatusCard {
  title: string
  value: string
  color: string
  icon: Component
}

defineProps<{
  currentTime: string
  lastUpdateTime: string
  loading: boolean
  techBadges: TechBadge[]
  statusCards: StatusCard[]
}>()

defineEmits<{
  (e: 'refresh'): void
}>()
</script>
