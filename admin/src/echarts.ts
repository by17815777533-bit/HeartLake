/**
 * ECharts 按需注册入口
 *
 * 只引入 Dashboard 实际用到的 renderer、图表类型和交互组件，
 * 配合 echarts/core 的 tree-shaking 将打包体积控制在 ~120KB（gzip 后约 40KB）。
 * 新增图表类型时在此追加即可，无需改动业务组件。
 */
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
