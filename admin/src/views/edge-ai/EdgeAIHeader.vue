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
  <div class="page-header">
    <div class="welcome-section">
      <span class="header-kicker">服务辅助</span>
      <div class="title-row">
        <h1>智能辅助概览</h1>
        <span class="update-pill">更新于 {{ lastUpdateTime }}</span>
      </div>
      <p class="welcome-sub">
        {{ currentTime }} · 内容建议、情绪观察与处理支持概况
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
      >
        <div class="stat-accent" :style="{ background: card.color }" />
        <div class="stat-content">
          <div class="stat-info">
            <span class="stat-kicker">当前情况</span>
            <div class="stat-value">
              {{ card.value }}
            </div>
            <div class="stat-title">
              {{ card.title }}
            </div>
          </div>
          <div class="stat-side">
            <div
              class="stat-icon"
              :style="{ background: `${card.color}14`, color: card.color }"
            >
              <el-icon :size="28">
                <component :is="card.icon" />
              </el-icon>
            </div>
            <span class="stat-note">
              {{ card.title === '服务状态' ? '实时更新' : '后台同步' }}
            </span>
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
  background: linear-gradient(135deg, rgba(255, 250, 242, 0.94), rgba(246, 239, 229, 0.98));
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
  background: rgba(154, 106, 53, 0.08);
  color: var(--m3-secondary);
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
  min-width: 42px;
  height: 24px;
  padding: 0 8px;
  border-radius: 999px;
  background: rgba(35, 73, 99, 0.08);
  color: var(--m3-primary);
  font-family: var(--hl-font-mono);
  font-size: 10px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.badge-label {
  font-size: 13px;
  font-weight: 600;
}

.stats-cards {
  margin-bottom: 24px;
}

.stats-cards :deep(.el-col) {
  margin-bottom: 20px;
}

.stat-card {
  position: relative;
  overflow: hidden;
  border: 1px solid rgba(24, 36, 47, 0.08);
  border-radius: 24px;
  background: linear-gradient(180deg, rgba(255, 250, 242, 0.94), rgba(247, 241, 232, 0.98));
  box-shadow: var(--hl-shadow-soft);
}

.stat-accent {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 4px;
}

.stat-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  min-height: 148px;
}

.stat-info {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.stat-kicker {
  display: inline-flex;
  width: fit-content;
  padding: 6px 10px;
  border-radius: 999px;
  background: rgba(24, 36, 47, 0.05);
  color: var(--hl-ink-soft);
  font-family: var(--hl-font-mono);
  font-size: 10px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.stat-value {
  font-family: var(--hl-font-display);
  font-size: clamp(28px, 3vw, 36px);
  line-height: 1;
  color: var(--hl-ink);
}

.stat-title {
  font-size: 15px;
  font-weight: 600;
  color: var(--hl-ink-soft);
}

.stat-side {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  gap: 12px;
}

.stat-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 58px;
  height: 58px;
  border-radius: 18px;
  border: 1px solid rgba(24, 36, 47, 0.06);
}

.stat-note {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  padding: 0 10px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.62);
  color: var(--hl-ink-soft);
  font-size: 12px;
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

  .stat-content {
    align-items: flex-start;
    flex-direction: column;
  }

  .stat-side {
    align-items: flex-start;
  }
}
</style>
