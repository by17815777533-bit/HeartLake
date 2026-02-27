/**
 * useChartOptions composable 测试
 */
import { describe, it, expect } from 'vitest'
import { useChartOptions, moodColors, moodNames, moodGradients } from '@/composables/useChartOptions'

describe('useChartOptions', () => {
  describe('导出常量', () => {
    it('moodColors 包含 5 种颜色', () => {
      expect(moodColors).toHaveLength(5)
    })

    it('moodNames 包含 5 种心情', () => {
      expect(moodNames).toEqual(['开心', '平静', '难过', '焦虑', '其他'])
    })

    it('moodGradients 包含 5 组渐变', () => {
      expect(moodGradients).toHaveLength(5)
    })

    it('每个 gradient 有 start 和 end', () => {
      moodGradients.forEach(g => {
        expect(g).toHaveProperty('start')
        expect(g).toHaveProperty('end')
      })
    })

    it('moodColors 都是有效的十六进制颜色', () => {
      moodColors.forEach(c => {
        expect(c).toMatch(/^#[0-9A-Fa-f]{6}$/)
      })
    })
  })

  describe('userGrowthOption', () => {
    it('返回 ref 对象', () => {
      const { userGrowthOption } = useChartOptions()
      expect(userGrowthOption.value).toBeDefined()
    })

    it('包含 tooltip 配置', () => {
      const { userGrowthOption } = useChartOptions()
      expect(userGrowthOption.value.tooltip).toBeDefined()
      expect(userGrowthOption.value.tooltip.trigger).toBe('axis')
    })
    it('xAxis 初始 data 为空数组', () => {
      const { userGrowthOption } = useChartOptions()
      expect(userGrowthOption.value.xAxis.data).toEqual([])
    })

    it('series 初始 data 为空数组', () => {
      const { userGrowthOption } = useChartOptions()
      expect(userGrowthOption.value.series[0].data).toEqual([])
    })

    it('series 类型为 line', () => {
      const { userGrowthOption } = useChartOptions()
      expect(userGrowthOption.value.series[0].type).toBe('line')
    })

    it('tooltip formatter 转义 HTML', () => {
      const { userGrowthOption } = useChartOptions()
      const formatter = userGrowthOption.value.tooltip.formatter
      const result = formatter([{ name: '<script>', value: 10 }])
      expect(result).not.toContain('<script>')
      expect(result).toContain('&lt;script&gt;')
    })
  })

  describe('moodTrendOption', () => {
    it('包含 5 条 series 对应 5 种心情', () => {
      const { moodTrendOption } = useChartOptions()
      expect(moodTrendOption.value.series).toHaveLength(5)
    })

    it('每条 series name 对应 moodNames', () => {
      const { moodTrendOption } = useChartOptions()
      moodTrendOption.value.series.forEach((s: any, i: number) => {
        expect(s.name).toBe(moodNames[i])
      })
    })

    it('每条 series 颜色对应 moodColors', () => {
      const { moodTrendOption } = useChartOptions()
      moodTrendOption.value.series.forEach((s: any, i: number) => {
        expect(s.itemStyle.color).toBe(moodColors[i])
      })
    })
  })

  describe('moodDistributionOption', () => {
    it('series 类型为 pie', () => {
      const { moodDistributionOption } = useChartOptions()
      expect(moodDistributionOption.value.series[0].type).toBe('pie')
    })

    it('包含 5 个默认数据项', () => {
      const { moodDistributionOption } = useChartOptions()
      expect(moodDistributionOption.value.series[0].data).toHaveLength(5)
    })

    it('tooltip formatter 转义特殊字符', () => {
      const { moodDistributionOption } = useChartOptions()
      const formatter = moodDistributionOption.value.tooltip.formatter
      const result = formatter({ marker: '', name: '&"test"', value: 10, percent: 50 })
      expect(result).toContain('&amp;')
      expect(result).toContain('&quot;')
    })
  })

  describe('activeTimeOption', () => {
    it('xAxis 包含 24 小时', () => {
      const { activeTimeOption } = useChartOptions()
      expect(activeTimeOption.value.xAxis.data).toHaveLength(24)
      expect(activeTimeOption.value.xAxis.data[0]).toBe('0:00')
      expect(activeTimeOption.value.xAxis.data[23]).toBe('23:00')
    })

    it('series 类型为 bar', () => {
      const { activeTimeOption } = useChartOptions()
      expect(activeTimeOption.value.series[0].type).toBe('bar')
    })
  })

  describe('emotionPulseOption', () => {
    it('series 类型为 gauge', () => {
      const { emotionPulseOption } = useChartOptions()
      expect(emotionPulseOption.value.series[0].type).toBe('gauge')
    })

    it('默认值为 50', () => {
      const { emotionPulseOption } = useChartOptions()
      expect(emotionPulseOption.value.series[0].data[0].value).toBe(50)
    })

    it('范围 0-100', () => {
      const { emotionPulseOption } = useChartOptions()
      expect(emotionPulseOption.value.series[0].min).toBe(0)
      expect(emotionPulseOption.value.series[0].max).toBe(100)
    })
  })

  describe('emotionTrendsOption', () => {
    it('包含 3 条 series（积极/中性/消极）', () => {
      const { emotionTrendsOption } = useChartOptions()
      expect(emotionTrendsOption.value.series).toHaveLength(3)
    })

    it('legend 包含正确名称', () => {
      const { emotionTrendsOption } = useChartOptions()
      expect(emotionTrendsOption.value.legend.data).toEqual(['积极', '中性', '消极'])
    })

    it('每条 series 初始 data 为空', () => {
      const { emotionTrendsOption } = useChartOptions()
      emotionTrendsOption.value.series.forEach((s: any) => {
        expect(s.data).toEqual([])
      })
    })
  })

  describe('返回值完整性', () => {
    it('返回所有 6 个图表配置', () => {
      const result = useChartOptions()
      expect(result).toHaveProperty('userGrowthOption')
      expect(result).toHaveProperty('moodTrendOption')
      expect(result).toHaveProperty('moodDistributionOption')
      expect(result).toHaveProperty('activeTimeOption')
      expect(result).toHaveProperty('emotionPulseOption')
      expect(result).toHaveProperty('emotionTrendsOption')
    })
  })
})