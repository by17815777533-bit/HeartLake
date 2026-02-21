import { createApp } from 'vue'
import { createPinia } from 'pinia'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import 'element-plus/theme-chalk/dark/css-vars.css'
import './styles/sky-theme.scss'
import * as ElementPlusIconsVue from '@element-plus/icons-vue'
import App from './App.vue'
import router from './router'
import { useAppStore } from './stores'

const app = createApp(App)
const pinia = createPinia()
app.use(pinia)
app.use(router)
app.use(ElementPlus)
for (const [key, component] of Object.entries(ElementPlusIconsVue)) {
  app.component(key, component)
}

// 初始化暗色模式（从 localStorage 恢复用户偏好）
const appStore = useAppStore()
appStore.initDarkMode()

// L-6: 全局错误边界 - Vue 组件错误处理
app.config.errorHandler = (err, instance, info) => {
  console.error('Vue 全局错误:', err, '\n组件信息:', info)
}

// L-6: 未捕获的 Promise 异常处理
window.addEventListener('unhandledrejection', (event) => {
  console.error('未处理的 Promise 异常:', event.reason)
})

app.mount('#app')
