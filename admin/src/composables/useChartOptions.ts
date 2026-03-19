/**
 * Dashboard 图表配置工厂，提供各类 ECharts option 的响应式引用。
 *
 * 这一版把图表语言统一成“值守台”风格：
 * - 更克制的坐标轴与网格
 * - 更有层次的面积渐变
 * - 更柔和但不发灰的湖面色板
 * - tooltip / legend / 数据点的细节统一
 */
import { ref } from 'vue'
import type { EChartsTooltipParam } from '@/types'

const escapeHtml = (str: string): string => String(str).replace(/[<>&"']/g, c => ({
  '<': '&lt;', '>': '&gt;', '&': '&amp;', '"': '&quot;', "'": '&#39;'
}[c] ?? c))

export const moodColors = ['#2f6b78', '#4d8f6b', '#b67a42', '#a35f5f', '#7a8793']
export const moodNames = ['开心', '平静', '难过', '焦虑', '其他']
export const moodGradients = [
  { start: '#77b6c4', end: '#2f6b78' },
  { start: '#87ba95', end: '#4d8f6b' },
  { start: '#e6bb7f', end: '#b67a42' },
  { start: '#d48c8c', end: '#a35f5f' },
  { start: '#b7c2ca', end: '#7a8793' },
]

const axisLineColor = '#b8c7cd'
const splitLineColor = 'rgba(89, 118, 129, 0.12)'
const axisLabelColor = '#5f7882'
const tooltipBase = {
  trigger: 'axis',
  backgroundColor: 'rgba(15, 28, 34, 0.92)',
  borderColor: 'rgba(208, 221, 226, 0.16)',
  borderWidth: 1,
  textStyle: { color: '#edf5f7', fontSize: 12 },
  padding: [10, 12],
  extraCssText: 'box-shadow: 0 16px 34px rgba(2, 10, 14, 0.22); border-radius: 14px;',
} as const

const softGrid = { left: 48, right: 18, top: 26, bottom: 34 }
const softAxis = {
  axisLine: { lineStyle: { color: axisLineColor } },
  axisLabel: { color: axisLabelColor, fontSize: 11 },
}

const lineArea = (from: string, to: string) => ({
  color: {
    type: 'linear',
    x: 0,
    y: 0,
    x2: 0,
    y2: 1,
    colorStops: [
      { offset: 0, color: from },
      { offset: 1, color: to },
    ],
  },
})

export function useChartOptions() {
  const userGrowthOption = ref({
    tooltip: {
      ...tooltipBase,
      formatter: (params: EChartsTooltipParam[]) => {
        const point = params[0]
        const name = escapeHtml(point?.name ?? '')
        return `<div style="font-weight:600; letter-spacing:0.04em; margin-bottom:4px;">${name}</div><div style="color:#9ed0db;">新增 ${Number(point?.value ?? 0)} 位旅人</div>`
      },
    },
    grid: softGrid,
    xAxis: {
      type: 'category',
      data: [],
      boundaryGap: false,
      ...softAxis,
    },
    yAxis: {
      type: 'value',
      splitNumber: 4,
      splitLine: { lineStyle: { color: splitLineColor } },
      axisLabel: { color: axisLabelColor, fontSize: 11 },
    },
    series: [{
      name: '新增用户',
      type: 'line',
      smooth: 0.35,
      symbol: 'circle',
      symbolSize: 8,
      showSymbol: false,
      data: [],
      itemStyle: {
        color: '#2f6b78',
        borderColor: '#f8fcfd',
        borderWidth: 2,
      },
      lineStyle: { color: '#2f6b78', width: 3 },
      areaStyle: lineArea('rgba(47, 107, 120, 0.22)', 'rgba(47, 107, 120, 0.02)'),
    }],
  })

  const moodTrendOption = ref({
    tooltip: { ...tooltipBase },
    legend: {
      top: 0,
      itemGap: 18,
      icon: 'circle',
      textStyle: { color: axisLabelColor, fontSize: 12 },
    },
    grid: { left: 48, right: 18, top: 46, bottom: 36 },
    xAxis: {
      type: 'category',
      data: [],
      boundaryGap: false,
      ...softAxis,
    },
    yAxis: {
      type: 'value',
      splitNumber: 4,
      splitLine: { lineStyle: { color: splitLineColor } },
      axisLabel: { color: axisLabelColor, fontSize: 11 },
    },
    series: moodNames.map((name, i) => ({
      name,
      type: 'line',
      smooth: 0.3,
      symbol: 'circle',
      symbolSize: 6,
      showSymbol: false,
      data: [],
      itemStyle: { color: moodColors[i], borderColor: '#f7fbfc', borderWidth: 2 },
      lineStyle: { color: moodColors[i], width: 2.4 },
      areaStyle: lineArea(`${moodColors[i]}22`, `${moodColors[i]}02`),
    })),
  })

  const moodDistributionOption = ref({
    tooltip: {
      trigger: 'item',
      backgroundColor: 'rgba(15, 28, 34, 0.92)',
      borderColor: 'rgba(208, 221, 226, 0.16)',
      borderWidth: 1,
      borderRadius: 14,
      padding: [10, 12],
      textStyle: { color: '#edf5f7', fontSize: 12 },
      formatter: (p: EChartsTooltipParam) => {
        const name = escapeHtml(p.name)
        return `<div style="font-weight:600; margin-bottom:4px;">${p.marker} ${name}</div><div style="color:#c7d8de;">${Number(p.value)} 条 · ${Number(p.percent)}%</div>`
      },
    },
    legend: {
      bottom: 2,
      itemGap: 18,
      icon: 'circle',
      textStyle: { color: axisLabelColor, fontSize: 12 },
    },
    series: [{
      type: 'pie',
      radius: ['55%', '78%'],
      center: ['50%', '42%'],
      padAngle: 2,
      itemStyle: {
        borderRadius: 10,
        borderColor: '#f8fbfc',
        borderWidth: 3,
      },
      label: { show: false },
      emphasis: {
        scale: true,
        scaleSize: 6,
        label: {
          show: true,
          fontSize: 13,
          fontWeight: '600',
          color: '#1b2c33',
        },
      },
      data: moodNames.map((name, i) => ({
        value: [30, 25, 20, 15, 10][i],
        name,
        itemStyle: { color: moodColors[i] },
      })),
    }],
  })

  const activeTimeOption = ref({
    tooltip: { ...tooltipBase },
    grid: softGrid,
    xAxis: {
      type: 'category',
      data: Array.from({ length: 24 }, (_, i) => `${i}:00`),
      ...softAxis,
    },
    yAxis: {
      type: 'value',
      splitNumber: 4,
      splitLine: { lineStyle: { color: splitLineColor } },
      axisLabel: { color: axisLabelColor, fontSize: 11 },
    },
    series: [{
      type: 'bar',
      barWidth: '52%',
      data: [],
      itemStyle: {
        borderRadius: [10, 10, 2, 2],
        color: {
          type: 'linear',
          x: 0,
          y: 0,
          x2: 0,
          y2: 1,
          colorStops: [
            { offset: 0, color: '#7bb0bb' },
            { offset: 1, color: '#2f6b78' },
          ],
        },
      },
    }],
  })

  const emotionPulseOption = ref({
    series: [{
      type: 'gauge',
      center: ['50%', '58%'],
      radius: '92%',
      startAngle: 210,
      endAngle: -30,
      min: 0,
      max: 100,
      splitNumber: 8,
      axisLine: {
        lineStyle: {
          width: 18,
          color: [
            [0.25, '#6f9dab'],
            [0.5, '#4d8f6b'],
            [0.75, '#d09a54'],
            [1, '#a35f5f'],
          ],
        },
      },
      pointer: {
        icon: 'path://M10 0 L20 42 L0 42 Z',
        length: '54%',
        width: 10,
        offsetCenter: [0, '-10%'],
        itemStyle: { color: '#1f3942' },
      },
      anchor: {
        show: true,
        size: 14,
        itemStyle: { color: '#f6fafb', borderColor: '#1f3942', borderWidth: 3 },
      },
      axisTick: { length: 7, lineStyle: { color: 'auto', width: 1.4 } },
      splitLine: { length: 14, lineStyle: { color: 'auto', width: 2.4 } },
      axisLabel: { color: axisLabelColor, fontSize: 10, distance: -42 },
      title: { offsetCenter: [0, '22%'], fontSize: 13, color: axisLabelColor },
      detail: {
        fontSize: 30,
        offsetCenter: [0, '48%'],
        valueAnimation: true,
        color: '#213840',
        formatter: '{value}°',
      },
      data: [{ value: 50, name: '情绪温度' }],
    }],
  })

  const emotionTrendsOption = ref({
    tooltip: { ...tooltipBase },
    legend: {
      data: ['积极', '中性', '消极'],
      top: 0,
      itemGap: 18,
      icon: 'circle',
      textStyle: { color: axisLabelColor, fontSize: 12 },
    },
    grid: { left: 48, right: 18, top: 44, bottom: 36 },
    xAxis: {
      type: 'category',
      data: [],
      boundaryGap: false,
      ...softAxis,
    },
    yAxis: {
      type: 'value',
      splitNumber: 4,
      splitLine: { lineStyle: { color: splitLineColor } },
      axisLabel: { color: axisLabelColor, fontSize: 11 },
    },
    series: [
      {
        name: '积极',
        type: 'line',
        smooth: true,
        showSymbol: false,
        data: [],
        lineStyle: { color: '#4d8f6b', width: 2.4 },
        itemStyle: { color: '#4d8f6b' },
        areaStyle: lineArea('rgba(77, 143, 107, 0.22)', 'rgba(77, 143, 107, 0.02)'),
      },
      {
        name: '中性',
        type: 'line',
        smooth: true,
        showSymbol: false,
        data: [],
        lineStyle: { color: '#b67a42', width: 2.4 },
        itemStyle: { color: '#b67a42' },
        areaStyle: lineArea('rgba(182, 122, 66, 0.2)', 'rgba(182, 122, 66, 0.02)'),
      },
      {
        name: '消极',
        type: 'line',
        smooth: true,
        showSymbol: false,
        data: [],
        lineStyle: { color: '#a35f5f', width: 2.4 },
        itemStyle: { color: '#a35f5f' },
        areaStyle: lineArea('rgba(163, 95, 95, 0.2)', 'rgba(163, 95, 95, 0.02)'),
      },
    ],
  })

  return {
    userGrowthOption,
    moodTrendOption,
    moodDistributionOption,
    activeTimeOption,
    emotionPulseOption,
    emotionTrendsOption,
  }
}
