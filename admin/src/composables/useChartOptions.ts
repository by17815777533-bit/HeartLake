/**
 * Dashboard 图表配置工厂，提供各类 ECharts option 的响应式引用。
 *
 * 这一版把图表语言统一成“软质金融面板”风格：
 * - 更浅的坐标轴和网格，避免运维大屏感
 * - 主波形 + 陪伴基线的双层曲线
 * - 更柔和的蓝、薄荷、蜜桃色板
 * - tooltip / legend / 数据点的细节统一
 */
import { ref } from 'vue'
import type { EChartsTooltipParam } from '@/types'

const escapeHtml = (str: string): string => String(str).replace(/[<>&"']/g, c => ({
  '<': '&lt;', '>': '&gt;', '&': '&amp;', '"': '&quot;', "'": '&#39;'
}[c] ?? c))

export const moodColors = ['#8eaefd', '#84d0bb', '#efc37b', '#ef9a95', '#b7c3df']
export const moodNames = ['开心', '平静', '难过', '焦虑', '其他']
export const moodGradients = [
  { start: '#b9ccff', end: '#8eaefd' },
  { start: '#b7eadf', end: '#84d0bb' },
  { start: '#f6d8a2', end: '#efc37b' },
  { start: '#f4bcb8', end: '#ef9a95' },
  { start: '#d2dbef', end: '#b7c3df' },
]

const axisLineColor = '#d4deef'
const splitLineColor = 'rgba(141, 161, 206, 0.16)'
const axisLabelColor = '#7c8baa'
const tooltipBase = {
  trigger: 'axis',
  backgroundColor: 'rgba(36, 49, 75, 0.92)',
  borderColor: 'rgba(232, 239, 255, 0.14)',
  borderWidth: 1,
  textStyle: { color: '#f4f7ff', fontSize: 12 },
  padding: [10, 12],
  extraCssText: 'box-shadow: 0 16px 34px rgba(43, 58, 94, 0.24); border-radius: 14px;',
} as const

const softGrid = { left: 44, right: 20, top: 28, bottom: 30 }
const softAxis = {
  axisLine: { show: false, lineStyle: { color: axisLineColor } },
  axisTick: { show: false },
  axisLabel: { color: axisLabelColor, fontSize: 11, margin: 14 },
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
        const rows = Array.isArray(params) ? params : [params]
        const mainPoint = rows.find(item => item.seriesName === '旅人波峰') ?? rows[0]
        const basePoint = rows.find(item => item.seriesName === '陪伴基线')
        const name = escapeHtml(mainPoint?.name ?? '')
        return `<div style="font-weight:600; letter-spacing:0.04em; margin-bottom:4px;">${name}</div><div style="color:#d9e4ff;">主波形 ${Number(mainPoint?.value ?? 0)} 位</div>${basePoint ? `<div style="color:#b9c6ea; margin-top:2px;">基线 ${Number(basePoint.value ?? 0)} 位</div>` : ''}`
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
      splitLine: { lineStyle: { color: splitLineColor, type: 'dashed' } },
      axisLine: { show: false },
      axisTick: { show: false },
      axisLabel: { color: axisLabelColor, fontSize: 11 },
    },
    series: [
      {
        name: '旅人波峰',
        type: 'line',
        smooth: 0.42,
        symbol: 'circle',
        symbolSize: 7,
        showSymbol: false,
        data: [],
        z: 2,
        itemStyle: {
          color: '#8eaefd',
          borderColor: '#f8fbff',
          borderWidth: 2,
        },
        lineStyle: { color: '#8eaefd', width: 3.4 },
        areaStyle: lineArea('rgba(142, 174, 253, 0.38)', 'rgba(142, 174, 253, 0.08)'),
      },
      {
        name: '陪伴基线',
        type: 'line',
        smooth: 0.5,
        symbol: 'none',
        data: [],
        z: 3,
        lineStyle: { color: '#283245', width: 2.6 },
        itemStyle: { color: '#283245' },
      }
    ],
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
      splitLine: { lineStyle: { color: splitLineColor, type: 'dashed' } },
      axisLine: { show: false },
      axisTick: { show: false },
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
      itemStyle: { color: moodColors[i], borderColor: '#f7fbff', borderWidth: 2 },
      lineStyle: { color: moodColors[i], width: 2.6 },
      areaStyle: lineArea(`${moodColors[i]}35`, `${moodColors[i]}06`),
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
      splitLine: { lineStyle: { color: splitLineColor, type: 'dashed' } },
      axisLine: { show: false },
      axisTick: { show: false },
      axisLabel: { color: axisLabelColor, fontSize: 11 },
    },
    series: [{
      type: 'bar',
      barWidth: '48%',
      data: [],
      itemStyle: {
        borderRadius: [18, 18, 6, 6],
        color: (params: { dataIndex: number }) => params.dataIndex % 2 === 0
          ? {
              type: 'linear',
              x: 0,
              y: 0,
              x2: 0,
              y2: 1,
              colorStops: [
                { offset: 0, color: '#9cb8ff' },
                { offset: 1, color: '#7d9ff4' },
              ],
            }
          : {
              type: 'linear',
              x: 0,
              y: 0,
              x2: 0,
              y2: 1,
              colorStops: [
                { offset: 0, color: 'rgba(255, 255, 255, 0.98)' },
                { offset: 1, color: 'rgba(233, 240, 255, 0.96)' },
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
            [0.22, '#f08686'],
            [0.5, '#f0b761'],
            [0.76, '#9cd99c'],
            [1, '#78d4c4'],
          ],
        },
      },
      pointer: {
        icon: 'path://M10 0 L20 42 L0 42 Z',
        length: '52%',
        width: 8,
        offsetCenter: [0, '-10%'],
        itemStyle: { color: '#2e3445' },
      },
      anchor: {
        show: true,
        size: 12,
        itemStyle: { color: '#ffffff', borderColor: '#2e3445', borderWidth: 3 },
      },
      axisTick: { length: 6, lineStyle: { color: 'auto', width: 1.2 } },
      splitLine: { length: 12, lineStyle: { color: 'auto', width: 2.1 } },
      axisLabel: { color: axisLabelColor, fontSize: 10, distance: -42 },
      title: { offsetCenter: [0, '22%'], fontSize: 13, color: axisLabelColor },
      detail: {
        fontSize: 30,
        offsetCenter: [0, '48%'],
        valueAnimation: true,
        color: '#2d364a',
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
      splitLine: { lineStyle: { color: splitLineColor, type: 'dashed' } },
      axisLine: { show: false },
      axisTick: { show: false },
      axisLabel: { color: axisLabelColor, fontSize: 11 },
    },
    series: [
      {
        name: '积极',
        type: 'line',
        smooth: true,
        showSymbol: false,
        data: [],
        lineStyle: { color: '#84d0bb', width: 2.8 },
        itemStyle: { color: '#84d0bb' },
        areaStyle: lineArea('rgba(132, 208, 187, 0.26)', 'rgba(132, 208, 187, 0.04)'),
      },
      {
        name: '中性',
        type: 'line',
        smooth: true,
        showSymbol: false,
        data: [],
        lineStyle: { color: '#efc37b', width: 2.8 },
        itemStyle: { color: '#efc37b' },
        areaStyle: lineArea('rgba(239, 195, 123, 0.24)', 'rgba(239, 195, 123, 0.04)'),
      },
      {
        name: '消极',
        type: 'line',
        smooth: true,
        showSymbol: false,
        data: [],
        lineStyle: { color: '#ef9a95', width: 2.8 },
        itemStyle: { color: '#ef9a95' },
        areaStyle: lineArea('rgba(239, 154, 149, 0.24)', 'rgba(239, 154, 149, 0.04)'),
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
