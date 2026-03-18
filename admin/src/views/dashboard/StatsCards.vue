<!--
  统计卡片区域 -- 四张核心指标卡片（总用户/今日投石/在线人数/待处理举报）

  每张卡片包含图标、数值和标题，数值通过 formatNumber 格式化（>=1w 显示为 x.xw）。
  卡片颜色由 statsCards 数组中的 color 字段控制。
-->

<template>
  <el-row
    :gutter="20"
    class="stats-cards"
  >
    <el-col
      v-for="stat in statsCards"
      :key="stat.title"
      :xs="24"
      :sm="12"
      :md="6"
    >
      <el-card
        shadow="hover"
        class="stat-card"
      >
        <div class="stat-accent" :style="{ background: stat.color }" />
        <div class="stat-content">
          <div class="stat-info">
            <span class="stat-kicker">核心指标</span>
            <div class="stat-value">
              {{ formatNumber(stat.value) }}
            </div>
            <div class="stat-title">
              {{ stat.title }}
            </div>
          </div>
          <div class="stat-side">
            <div
              class="stat-icon"
              :style="{ background: `${stat.color}14`, color: stat.color }"
            >
              <el-icon :size="28">
                <component :is="stat.icon" />
              </el-icon>
            </div>
            <span class="stat-note">
              {{ stat.title === '待处理举报' ? '需优先处理' : '实时同步' }}
            </span>
          </div>
        </div>
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
interface StatCard {
  title: string
  value: number
  color: string
  icon: string
}

defineProps<{
  statsCards: StatCard[]
  formatNumber: (num: number) => string
}>()
</script>

<style scoped lang="scss">
.stats-cards {
  margin-bottom: 20px;
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
  min-height: 150px;
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
  font-size: clamp(34px, 4vw, 42px);
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
  .stat-content {
    min-height: 132px;
  }
}

@media (max-width: 560px) {
  .stat-content {
    align-items: flex-start;
    flex-direction: column;
  }

  .stat-side {
    align-items: flex-start;
  }
}
</style>
