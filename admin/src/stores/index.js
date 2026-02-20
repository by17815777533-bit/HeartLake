import { defineStore } from 'pinia'
import { ref } from 'vue'

export const useAppStore = defineStore('app', () => {
  const token = ref(localStorage.getItem('admin_token') || '')

  // 登录时存储的用户信息（PASETO token 不可客户端解码，直接存登录响应中的用户信息）
  const userInfo = ref((() => {
    try {
      return JSON.parse(localStorage.getItem('admin_user_info') || 'null')
    } catch {
      localStorage.removeItem('admin_user_info')
      return null
    }
  })())

  // 暗色模式状态，从 localStorage 恢复
  const isDark = ref(localStorage.getItem('admin_dark_mode') === 'true')

  // 全局加载计数器（支持并发请求）
  const loadingCount = ref(0)
  const isGlobalLoading = ref(false)

  // 设置 token 并持久化到 localStorage
  const setToken = (t) => {
    token.value = t
    localStorage.setItem('admin_token', t)
  }

  // 设置用户信息并持久化
  const setUserInfo = (info) => {
    userInfo.value = info
    localStorage.setItem('admin_user_info', JSON.stringify(info))
  }

  // 清除 token 和用户信息
  const clearToken = () => {
    token.value = ''
    userInfo.value = null
    localStorage.removeItem('admin_token')
    localStorage.removeItem('admin_user_info')
  }

  // 获取 token（统一入口）
  const getToken = () => token.value

  // 校验当前 token 是否存在（PASETO token 无法客户端解码，仅检查非空）
  // 过期由服务端 401 响应处理
  const checkTokenValid = () => !!token.value && typeof token.value === 'string' && token.value.length > 0

  // 切换暗色模式
  const toggleDark = () => {
    isDark.value = !isDark.value
    applyDarkMode(isDark.value)
    localStorage.setItem('admin_dark_mode', isDark.value)
  }

  // 应用暗色模式到 HTML 元素
  const applyDarkMode = (dark) => {
    document.documentElement.classList.toggle('dark', dark)
  }

  // 初始化暗色模式（应用启动时调用）
  const initDarkMode = () => {
    applyDarkMode(isDark.value)
  }

  // 全局 loading 控制
  const startLoading = () => {
    loadingCount.value++
    isGlobalLoading.value = true
  }

  const stopLoading = () => {
    loadingCount.value = Math.max(0, loadingCount.value - 1)
    if (loadingCount.value === 0) {
      isGlobalLoading.value = false
    }
  }

  return {
    token, setToken, clearToken, getToken, checkTokenValid,
    userInfo, setUserInfo,
    isDark, toggleDark, initDarkMode,
    isGlobalLoading, startLoading, stopLoading,
  }
})
