import { createApp, type Component, type ComponentPublicInstance } from 'vue'
import { createPinia } from 'pinia'
// 命令式组件样式（ElMessage / ElMessageBox / ElNotification / ElLoading 不会被自动导入插件处理）
import 'element-plus/es/components/message/style/css'
import 'element-plus/es/components/message-box/style/css'
import 'element-plus/es/components/notification/style/css'
import 'element-plus/es/components/loading/style/css'
import 'element-plus/theme-chalk/dark/css-vars.css'
import './styles/m3-theme.scss'
// 仅全局注册被 <component :is="stringName"> 动态引用的图标（侧边栏菜单 + Dashboard 统计卡片）
// 其余图标在各组件内按需 import，保证 tree-shaking 生效
import {
  Bell, Check, Connection, DataAnalysis, Document, Edit,
  Expand, Fold, Monitor, Moon, Setting, Sunny,
  Tickets, User, Warning,
} from '@element-plus/icons-vue'
import App from './App.vue'
import router from './router'
import { useAppStore } from './stores'

const app = createApp(App)
const pinia = createPinia()
app.use(pinia)
app.use(router)

// 仅注册动态 :is 引用的图标，其余组件内按需 import
const icons: Record<string, Component> = {
  Bell, Check, Connection, DataAnalysis, Document, Edit,
  Expand, Fold, Monitor, Moon, Setting, Sunny,
  Tickets, User, Warning,
}
for (const [key, component] of Object.entries(icons)) {
  app.component(key, component)
}

// 初始化暗色模式（从 localStorage 恢复用户偏好）
const appStore = useAppStore()
appStore.initDarkMode()

// L-6: 全局错误边界 - Vue 组件错误处理
// 生产环境应接入 Sentry / Datadog 等监控服务上报错误，便于线上问题追踪
app.config.errorHandler = (err: unknown, instance: ComponentPublicInstance | null, info: string) => {
  console.error('Vue 全局错误:', err, '\n组件信息:', info)
}

// L-6: 未捕获的 Promise 异常处理
// 同上，生产环境建议通过 Sentry.captureException 上报
window.addEventListener('unhandledrejection', (event) => {
  console.error('未处理的 Promise 异常:', event.reason)
})

app.mount('#app')
