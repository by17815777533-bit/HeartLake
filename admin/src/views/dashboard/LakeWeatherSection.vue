<!--
  湖面天气区重构：
  - 左侧做成更有氛围感的天气观测卡
  - 右侧饼图容器和标题说明统一提升
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col
      :xs="24"
      :lg="10"
    >
      <el-card
        shadow="hover"
        class="chart-card lake-weather-card"
      >
        <template #header>
          <div class="weather-head">
            <div>
              <span class="weather-eyebrow">Lake Forecast</span>
              <h3>湖面天气</h3>
            </div>
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
                :style="{ left: `${lakeWeatherTemp}%` }"
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
      :lg="14"
    >
      <el-card
        shadow="hover"
        class="chart-card lake-weather-card"
      >
        <template #header>
          <div class="weather-head">
            <div>
              <span class="weather-eyebrow">Mood Atmosphere</span>
              <h3>心情分布</h3>
            </div>
            <span class="weather-note">基于情绪脉搏数据</span>
          </div>
        </template>
        <v-chart
          :option="weatherMoodPieOption"
          autoresize
          class="weather-chart"
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

<style scoped lang="scss">
.lake-weather-card {
  position: relative;
  overflow: hidden;
}

.weather-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;

  h3 {
    margin: 8px 0 0;
    color: var(--hl-ink);
    font-family: var(--hl-font-display);
    font-size: clamp(22px, 3vw, 28px);
    line-height: 1.08;
  }
}

.weather-eyebrow {
  display: inline-flex;
  min-height: 26px;
  align-items: center;
  padding: 0 10px;
  border-radius: 999px;
  background: rgba(17, 62, 74, 0.08);
  color: var(--m3-primary);
  font-family: var(--hl-font-mono);
  font-size: 10px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.weather-note {
  color: var(--hl-ink-soft);
  font-size: 12px;
  font-weight: 600;
}

.lake-weather-content {
  display: grid;
  gap: 18px;
}

.weather-display {
  position: relative;
  display: grid;
  grid-template-columns: 120px 1fr;
  gap: 18px;
  align-items: center;
  min-height: 220px;
  padding: 26px;
  border-radius: 28px;
  overflow: hidden;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.42);
}

.weather-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 104px;
  height: 104px;
  border-radius: 28px;
  background: rgba(255, 255, 255, 0.28);
  font-size: 52px;
  backdrop-filter: blur(10px);
}

.weather-temp {
  color: #fff;
  font-family: var(--hl-font-display);
  font-size: clamp(44px, 5vw, 62px);
  line-height: 0.95;
}

.weather-label {
  margin-top: 10px;
  color: rgba(255, 255, 255, 0.96);
  font-size: 20px;
  font-weight: 700;
}

.weather-desc {
  margin-top: 8px;
  color: rgba(255, 255, 255, 0.82);
  font-size: 14px;
  line-height: 1.8;
}

.weather-scale {
  padding: 8px 2px 0;
}

.scale-bar {
  position: relative;
  display: flex;
  height: 12px;
  overflow: hidden;
  border-radius: 999px;
  background: rgba(126, 149, 158, 0.12);
}

.scale-segment.storm { background: #8a4f46; }
.scale-segment.rain { background: #5b8f9d; }
.scale-segment.cloudy { background: #8fb0ba; }
.scale-segment.sunny { background: #d9a75f; }

.scale-pointer {
  position: absolute;
  top: 50%;
  width: 14px;
  height: 14px;
  border-radius: 999px;
  background: #fff;
  border: 3px solid #173740;
  transform: translate(-50%, -50%);
}

.scale-labels {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 8px;
  margin-top: 10px;
  color: var(--hl-ink-soft);
  font-size: 11px;
}

.weather-chart {
  height: 300px;
}

@media (max-width: 900px) {
  .weather-head {
    flex-direction: column;
  }

  .weather-display {
    grid-template-columns: 1fr;
  }
}
</style>
