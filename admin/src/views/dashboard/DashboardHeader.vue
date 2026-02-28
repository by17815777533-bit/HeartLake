<!--
  Dashboard 页面头部 -- 时段问候语、当前时间、最后更新时间、导出/刷新按钮、技术标签栏

  props:
  - greeting: 根据当前小时计算的问候语（早上好/下午好等）
  - currentDateTime: 格式化的当前日期时间
  - lastUpdateTime: 上次数据刷新的 HH:mm:ss
  - loading: 刷新按钮的 loading 状态
  - techBadges: 技术能力标签数组（DTW 共鸣 / DP 隐私 / RAG 守护 / 端侧推理）
-->

<template>
  <div class="page-header">
    <div class="welcome-section">
      <h1>{{ greeting }}，管理员</h1>
      <p class="welcome-sub">
        {{ currentDateTime }} · 心湖管理后台
      </p>
    </div>
    <div class="header-actions">
      <el-button
        type="success"
        :icon="Download"
        @click="$emit('export')"
      >
        导出数据
      </el-button>
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
</template>

<script setup lang="ts">
import { Refresh, Download } from '@element-plus/icons-vue'

interface TechBadge {
  icon: string
  label: string
}

defineProps<{
  greeting: string
  currentDateTime: string
  lastUpdateTime: string
  loading: boolean
  techBadges: TechBadge[]
}>()

defineEmits<{
  (e: 'export'): void
  (e: 'refresh'): void
}>()
</script>
