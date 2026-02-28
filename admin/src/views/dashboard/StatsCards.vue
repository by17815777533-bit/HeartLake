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
        :style="{ borderTop: `4px solid ${stat.color}` }"
      >
        <div class="stat-content">
          <div class="stat-info">
            <div class="stat-value">
              {{ formatNumber(stat.value) }}
            </div>
            <div class="stat-title">
              {{ stat.title }}
            </div>
          </div>
          <div
            class="stat-icon"
            :style="{ background: `${stat.color}15`, color: stat.color }"
          >
            <el-icon :size="32">
              <component :is="stat.icon" />
            </el-icon>
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
