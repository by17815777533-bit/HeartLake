/**
 * 应用入口 -- 初始化 Vue 实例、Pinia 状态管理、路由、全局图标注册
 *
 * 启动流程：
 * 1. 手动引入命令式组件样式（ElMessage 等不走自动导入插件）
 * 2. 按需注册 ECharts 模块（见 ./echarts.ts）
 * 3. 从 localStorage 恢复暗色模式偏好
 * 4. 挂载全局错误边界（Vue 组件错误 + 未捕获 Promise 异常）
 */
import { createApp, type ComponentPublicInstance } from 'vue'
import { createPinia } from 'pinia'
// 命令式组件样式需手动引入（ElMessage / ElMessageBox 等不会被自动导入插件处理）
import 'element-plus/es/components/message/style/css'
import 'element-plus/es/components/message-box/style/css'
import 'element-plus/es/components/notification/style/css'
import 'element-plus/es/components/loading/style/css'
import 'element-plus/theme-chalk/dark/css-vars.css'
import './styles/m3-theme.scss'
import './echarts'
import App from './App.vue'
import router from './router'
import { useAppStore } from './stores'

const app = createApp(App)
const pinia = createPinia()
app.use(pinia)
app.use(router)

// 初始化暗色模式（从 localStorage 恢复用户偏好）
const appStore = useAppStore()
appStore.initDarkMode()

// 全局错误边界 -- Vue 组件错误处理
// 生产环境应接入 Sentry 等监控服务上报
app.config.errorHandler = (
  err: unknown,
  instance: ComponentPublicInstance | null,
  info: string,
) => {
  console.error('Vue 全局错误:', err, '\n组件信息:', info)
}

// 未捕获的 Promise 异常兜底
window.addEventListener('unhandledrejection', (event) => {
  console.error('未处理的 Promise 异常:', event.reason)
})

app.mount('#app')
