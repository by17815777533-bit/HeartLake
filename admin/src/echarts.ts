// ECharts 按需注册：renderer + 图表类型 + 组件
import { use } from 'echarts/core'
import { CanvasRenderer } from 'echarts/renderers'
import { LineChart, BarChart, PieChart, GaugeChart } from 'echarts/charts'
import {
  TitleComponent, TooltipComponent, LegendComponent,
  GridComponent, DataZoomComponent,
} from 'echarts/components'

use([
  CanvasRenderer,
  LineChart, BarChart, PieChart, GaugeChart,
  TitleComponent, TooltipComponent, LegendComponent,
  GridComponent, DataZoomComponent,
])
