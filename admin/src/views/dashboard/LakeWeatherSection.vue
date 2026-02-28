<!--
  湖面天气面板 -- 将情绪温度映射为天气隐喻

  左侧：天气卡片（图标 + 标签 + 描述 + 温度值），背景渐变色随天气变化
    - >60° 晴朗（社区积极）/ >30° 多云（社区平稳）/ >10° 小雨（社区低落）/ <=10° 暴风雨（社区消极）
  右侧：心情分布缩略饼图，复用 moodDistribution 数据
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col
      :xs="24"
      :sm="24"
      :md="10"
    >
      <el-card
        shadow="hover"
        class="chart-card lake-weather-card"
      >
        <template #header>
          <div class="card-header">
            <span>🌤️ 湖面天气</span>
            <el-tag
              size="small"
              :color="lakeWeather.color"
              effect="dark"
              style="border: none; color: #fff;"
            >
              {{ lakeWeather.label }}
            </el-tag>
          </div>
        </template>
        <div class="lake-weather-content">
          <div
            class="weather-display"
            :style="{ background: lakeWeather.bg }"
          >
            <div class="weather-icon">
              {{ lakeWeather.icon }}
            </div>
            <div class="weather-info">
              <div class="weather-temp">
                {{ lakeWeatherTemp.toFixed(0) }}°
              </div>
              <div class="weather-label">
                {{ lakeWeather.label }}
              </div>
              <div class="weather-desc">
                {{ lakeWeather.desc }}
              </div>
            </div>
          </div>
          <div class="weather-scale">
            <div class="scale-bar">
              <div
                class="scale-segment storm"
                style="width: 10%"
              />
              <div
                class="scale-segment rain"
                style="width: 20%"
              />
              <div
                class="scale-segment cloudy"
                style="width: 30%"
              />
              <div
                class="scale-segment sunny"
                style="width: 40%"
              />
              <div
                class="scale-pointer"
                :style="{ left: lakeWeatherTemp + '%' }"
              />
            </div>
            <div class="scale-labels">
              <span>暴风雨</span>
              <span>小雨</span>
              <span>多云</span>
              <span>晴朗</span>
            </div>
          </div>
        </div>
      </el-card>
    </el-col>
    <el-col
      :xs="24"
      :sm="24"
      :md="14"
    >
      <el-card
        shadow="hover"
        class="chart-card lake-weather-card"
      >
        <template #header>
          <div class="card-header">
            <span>🎭 心情分布</span>
            <span class="pulse-hint">基于情绪脉搏数据</span>
          </div>
        </template>
        <v-chart
          :option="weatherMoodPieOption"
          autoresize
          style="height: 280px"
        />
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
import VChart from 'vue-echarts'

interface LakeWeather {
  icon: string
  label: string
  desc: string
  color: string
  bg: string
}

defineProps<{
  lakeWeather: LakeWeather
  lakeWeatherTemp: number
  weatherMoodPieOption: Record<string, unknown>
}>()
</script>
