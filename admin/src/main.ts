import { createApp, type Component, type ComponentPublicInstance } from 'vue'
import { createPinia } from 'pinia'
// 命令式组件样式（ElMessage / ElMessageBox / ElNotification / ElLoading 不会被自动导入插件处理）
import 'element-plus/es/components/message/style/css'
import 'element-plus/es/components/message-box/style/css'
import 'element-plus/es/components/notification/style/css'
import 'element-plus/es/components/loading/style/css'
import 'element-plus/theme-chalk/dark/css-vars.css'
import './styles/m3-theme.scss'
import {
  Bell, ChatDotRound, ChatLineRound, Check, CircleCheck, Cloudy,
  Connection, Cpu, DataAnalysis, Document, Download,
  Expand, Fold, Histogram, Monitor,
  Moon, PieChart, Postcard,
  Refresh, Search, Setting, Star, StarFilled, Stopwatch, Sunny,
  Sunrise, Sunset, Tickets, Timer, TrendCharts, TrophyBase, User,
  View, Warning, WindPower,
} from '@element-plus/icons-vue'
import App from './App.vue'
import router from './router'
import { useAppStore } from './stores'

const app = createApp(App)
const pinia = createPinia()
app.use(pinia)
app.use(router)

// 按需注册实际使用的图标（tree-shaking 优化）
const icons: Record<string, Component> = {
  Bell, ChatDotRound, ChatLineRound, Check, CircleCheck, Cloudy,
  Connection, Cpu, DataAnalysis, Document, Download,
  Expand, Fold, Histogram, Monitor,
  Moon, PieChart, Postcard,
  Refresh, Search, Setting, Star, StarFilled, Stopwatch, Sunny,
  Sunrise, Sunset, Tickets, Timer, TrendCharts, TrophyBase, User,
  View, Warning, WindPower,
}
for (const [key, component] of Object.entries(icons)) {
  app.component(key, component)
}

// 初始化暗色模式（从 localStorage 恢复用户偏好）
const appStore = useAppStore()
appStore.initDarkMode()

// L-6: 全局错误边界 - Vue 组件错误处理
app.config.errorHandler = (err: unknown, instance: ComponentPublicInstance | null, info: string) => {
  console.error('Vue 全局错误:', err, '\n组件信息:', info)
}

// L-6: 未捕获的 Promise 异常处理
window.addEventListener('unhandledrejection', (event) => {
  console.error('未处理的 Promise 异常:', event.reason)
})

app.mount('#app')
