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
      <span class="header-kicker">Operations Snapshot</span>
      <div class="title-row">
        <h1>{{ greeting }}，管理员</h1>
        <span class="update-pill">更新于 {{ lastUpdateTime }}</span>
      </div>
      <p class="welcome-sub">
        {{ currentDateTime }} · 社区值守、反馈巡检与处置总览
      </p>
    </div>
    <div class="header-actions">
      <el-button
        plain
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

<style scoped lang="scss">
.page-header {
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  gap: 20px;
  padding: 28px 30px;
  margin-bottom: 18px;
  border: 1px solid rgba(24, 36, 47, 0.08);
  border-radius: 28px;
  background:
    linear-gradient(135deg, rgba(255, 250, 242, 0.94), rgba(246, 239, 229, 0.98));
  box-shadow: var(--hl-shadow-soft);
}

.welcome-section {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.header-kicker {
  display: inline-flex;
  width: fit-content;
  padding: 7px 12px;
  border-radius: 999px;
  background: rgba(35, 73, 99, 0.08);
  color: var(--m3-primary);
  font-family: var(--hl-font-mono);
  font-size: 11px;
  letter-spacing: 0.16em;
  text-transform: uppercase;
}

.title-row {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 14px;

  h1 {
    margin: 0;
    font-family: var(--hl-font-display);
    font-size: clamp(28px, 4vw, 40px);
    line-height: 1.05;
    color: var(--hl-ink);
  }
}

.update-pill {
  display: inline-flex;
  align-items: center;
  min-height: 34px;
  padding: 0 14px;
  border-radius: 999px;
  border: 1px solid rgba(24, 36, 47, 0.09);
  background: rgba(255, 255, 255, 0.58);
  color: var(--hl-ink-soft);
  font-family: var(--hl-font-mono);
  font-size: 11px;
  letter-spacing: 0.08em;
}

.welcome-sub {
  margin: 0;
  max-width: 42rem;
  font-size: 14px;
  line-height: 1.7;
  color: var(--hl-ink-soft);
}

.header-actions {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: flex-end;
  gap: 12px;

  :deep(.el-button) {
    min-height: 42px;
    padding: 0 18px;
    border-radius: 14px;
    font-weight: 600;
    letter-spacing: 0.04em;
  }

  :deep(.el-button--default) {
    border-color: rgba(24, 36, 47, 0.12);
    background: rgba(255, 255, 255, 0.62);
    color: var(--hl-ink);
  }
}

.tech-badges {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-bottom: 24px;
}

.tech-badge {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  min-height: 40px;
  padding: 0 14px;
  border-radius: 999px;
  border: 1px solid rgba(24, 36, 47, 0.08);
  background: rgba(255, 250, 242, 0.82);
  color: var(--hl-ink-soft);
  transition: var(--m3-transition);

  &:hover {
    transform: translateY(-1px);
    border-color: rgba(35, 73, 99, 0.18);
    background: rgba(255, 255, 255, 0.94);
  }
}

.badge-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 38px;
  height: 24px;
  padding: 0 8px;
  border-radius: 999px;
  background: rgba(154, 106, 53, 0.08);
  color: var(--m3-secondary);
  font-family: var(--hl-font-mono);
  font-size: 10px;
  letter-spacing: 0.16em;
  text-transform: uppercase;
}

.badge-label {
  font-size: 13px;
  font-weight: 600;
}

@media (max-width: 900px) {
  .page-header {
    align-items: flex-start;
    flex-direction: column;
    padding: 24px;
  }

  .header-actions {
    justify-content: flex-start;
  }
}

@media (max-width: 560px) {
  .page-header {
    border-radius: 22px;
  }

  .title-row h1 {
    font-size: 30px;
  }
}
</style>
