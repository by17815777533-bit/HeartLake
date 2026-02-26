// @file useChartOptions.js
// @brief Dashboard 图表配置 composable - Material Design 3 风格
import { ref } from 'vue'
import type { EChartsTooltipParam } from '@/types'

// XSS 安全转义
const escapeHtml = (str: string): string => String(str).replace(/[<>&"']/g, c => ({
  '<': '&lt;', '>': '&gt;', '&': '&amp;', '"': '&quot;', "'": '&#39;'
}[c] ?? c))

// Material Design 3 配色
export const moodColors = ['#1565C0', '#2E7D32', '#BA1A1A', '#E65100', '#44474E']
export const moodNames = ['开心', '平静', '难过', '焦虑', '其他']
export const moodGradients = [
  { start: '#1565C0', end: '#0D47A1' },
  { start: '#2E7D32', end: '#1B5E20' },
  { start: '#BA1A1A', end: '#8B0000' },
  { start: '#E65100', end: '#BF360C' },
  { start: '#44474E', end: '#2B2B2F' },
]

export function useChartOptions() {
  const userGrowthOption = ref({
    tooltip: {
      trigger: 'axis',
      backgroundColor: '#2B2B2F',
      borderColor: '#44474E',
      textStyle: { color: '#E3E2E6' },
      formatter: (params: EChartsTooltipParam[]) => {
        const p = params[0]
        const name = escapeHtml(p.name)
        return `<div style="font-weight:500">${name}</div><div style="color:#A8C8FF">新增 ${Number(p.value)} 人</div>`
      }
    },
    grid: { left: 50, right: 20, top: 20, bottom: 30 },
    xAxis: { type: 'category', data: [], axisLine: { lineStyle: { color: '#C4C6CF' } }, axisLabel: { color: '#44474E' } },
    yAxis: { type: 'value', splitLine: { lineStyle: { color: '#C4C6CF', opacity: 0.2 } }, axisLabel: { color: '#44474E' } },
    series: [{
      name: '新增用户', type: 'line', smooth: true, symbol: 'circle', symbolSize: 6, data: [],
      areaStyle: { opacity: 0.2, color: '#1565C0' },
      itemStyle: { color: '#1565C0', borderWidth: 2, borderColor: '#fff' },
      lineStyle: { color: '#1565C0', width: 2 },
    }],
  })

  const moodTrendOption = ref({
    tooltip: {
      trigger: 'axis', backgroundColor: '#2B2B2F', borderColor: '#44474E',
      borderRadius: 4, padding: [8, 12], textStyle: { color: '#E3E2E6' }
    },
    legend: { bottom: 0, itemGap: 16, textStyle: { color: '#44474E', fontSize: 12 } },
    grid: { left: 50, right: 20, top: 20, bottom: 50 },
    xAxis: { type: 'category', data: [], axisLine: { lineStyle: { color: '#C4C6CF' } }, axisLabel: { color: '#44474E' } },
    yAxis: { type: 'value', splitLine: { lineStyle: { color: '#C4C6CF', opacity: 0.2 } }, axisLabel: { color: '#44474E' } },
    series: moodNames.map((name, i) => ({
      name, type: 'line', smooth: 0.3, symbol: 'circle', symbolSize: 5, data: [],
      itemStyle: { color: moodColors[i], borderWidth: 2, borderColor: '#fff' },
      lineStyle: { width: 2 }, areaStyle: { opacity: 0.1, color: moodColors[i] }
    }))
  })

  const moodDistributionOption = ref({
    tooltip: {
      trigger: 'item', backgroundColor: '#2B2B2F', borderColor: '#44474E',
      borderRadius: 4, padding: [8, 12], textStyle: { color: '#E3E2E6' },
      formatter: (p: EChartsTooltipParam) => {
        const name = escapeHtml(p.name)
        return `<div style="font-weight:500">${p.marker} ${name}</div><div style="color:#BCC7DC">${Number(p.value)} 条 · ${Number(p.percent)}%</div>`
      }
    },
    legend: { bottom: 0, itemGap: 16, textStyle: { color: '#44474E', fontSize: 12 } },
    series: [{
      type: 'pie', radius: ['45%', '70%'], center: ['50%', '42%'],
      itemStyle: { borderRadius: 4, borderColor: '#fff', borderWidth: 2 },
      label: { show: false },
      emphasis: {
        label: { show: true, fontSize: 14, fontWeight: '500', color: '#1C1B1F' },
        itemStyle: { shadowBlur: 10, shadowColor: 'rgba(0,0,0,0.2)' }
      },
      data: moodNames.map((name, i) => ({
        value: [30, 25, 20, 15, 10][i], name,
        itemStyle: { color: moodColors[i] }
      }))
    }],
  })

  const activeTimeOption = ref({
    tooltip: { trigger: 'axis', backgroundColor: '#2B2B2F', borderColor: '#44474E', textStyle: { color: '#E3E2E6' } },
    grid: { left: 50, right: 20, top: 20, bottom: 30 },
    xAxis: { type: 'category', data: Array.from({ length: 24 }, (_, i) => `${i}:00`), axisLine: { lineStyle: { color: '#C4C6CF' } }, axisLabel: { color: '#44474E' } },
    yAxis: { type: 'value', splitLine: { lineStyle: { color: '#C4C6CF', opacity: 0.2 } }, axisLabel: { color: '#44474E' } },
    series: [{ type: 'bar', data: [], itemStyle: { color: '#2E7D32', borderRadius: [2, 2, 0, 0] } }]
  })

  const emotionPulseOption = ref({
    series: [{
      type: 'gauge', center: ['50%', '60%'], radius: '90%',
      startAngle: 200, endAngle: -20, min: 0, max: 100, splitNumber: 10,
      axisLine: { lineStyle: { width: 16, color: [[0.3, '#1565C0'], [0.5, '#2E7D32'], [0.7, '#E65100'], [1, '#BA1A1A']] } },
      pointer: { icon: 'path://M12.8,0.7l12,40.1H0.7L12.8,0.7z', length: '55%', width: 8, offsetCenter: [0, '-10%'], itemStyle: { color: 'auto' } },
      axisTick: { length: 6, lineStyle: { color: 'auto', width: 1 } },
      splitLine: { length: 12, lineStyle: { color: 'auto', width: 2 } },
      axisLabel: { color: '#44474E', fontSize: 10, distance: -40 },
      title: { offsetCenter: [0, '20%'], fontSize: 14, color: '#44474E' },
      detail: { fontSize: 28, offsetCenter: [0, '45%'], valueAnimation: true, color: 'auto', formatter: '{value}°' },
      data: [{ value: 50, name: '情绪温度' }]
    }]
  })

  const emotionTrendsOption = ref({
    tooltip: { trigger: 'axis', backgroundColor: '#2B2B2F', borderColor: '#44474E', textStyle: { color: '#E3E2E6' } },
    legend: { data: ['积极', '中性', '消极'], bottom: 0, textStyle: { color: '#44474E' } },
    grid: { left: 50, right: 20, top: 20, bottom: 40 },
    xAxis: { type: 'category', data: [], axisLine: { lineStyle: { color: '#C4C6CF' } }, axisLabel: { color: '#44474E' } },
    yAxis: { type: 'value', splitLine: { lineStyle: { color: '#C4C6CF', opacity: 0.2 } }, axisLabel: { color: '#44474E' } },
    series: [
      { name: '积极', type: 'line', smooth: true, data: [], lineStyle: { color: '#2E7D32' }, itemStyle: { color: '#2E7D32' }, areaStyle: { color: { type: 'linear', x: 0, y: 0, x2: 0, y2: 1, colorStops: [{ offset: 0, color: 'rgba(46,125,50,0.25)' }, { offset: 1, color: 'rgba(46,125,50,0.02)' }] } } },
      { name: '中性', type: 'line', smooth: true, data: [], lineStyle: { color: '#E65100' }, itemStyle: { color: '#E65100' }, areaStyle: { color: { type: 'linear', x: 0, y: 0, x2: 0, y2: 1, colorStops: [{ offset: 0, color: 'rgba(230,81,0,0.25)' }, { offset: 1, color: 'rgba(230,81,0,0.02)' }] } } },
      { name: '消极', type: 'line', smooth: true, data: [], lineStyle: { color: '#BA1A1A' }, itemStyle: { color: '#BA1A1A' }, areaStyle: { color: { type: 'linear', x: 0, y: 0, x2: 0, y2: 1, colorStops: [{ offset: 0, color: 'rgba(186,26,26,0.25)' }, { offset: 1, color: 'rgba(186,26,26,0.02)' }] } } },
    ]
  })

  return {
    userGrowthOption, moodTrendOption, moodDistributionOption,
    activeTimeOption, emotionPulseOption, emotionTrendsOption,
  }
}
