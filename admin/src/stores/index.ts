import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { UserInfo } from '@/types'

export const useAppStore = defineStore('app', () => {
  // S-1: 敏感凭据使用 sessionStorage（关闭标签页即清除）
  // A-5: try-catch 保护 sessionStorage 读取，防止隐私模式等异常
  const token = ref((() => {
    try {
      return sessionStorage.getItem('admin_token') || ''
    } catch {
      return ''
    }
  })())

  // 登录时存储的用户信息（PASETO token 不可客户端解码，直接存登录响应中的用户信息）
  const userInfo = ref<UserInfo | null>((() => {
    try {
      return JSON.parse(sessionStorage.getItem('admin_user_info') || 'null')
    } catch {
      sessionStorage.removeItem('admin_user_info')
      return null
    }
  })())

  // 暗色模式状态，从 localStorage 恢复
  const isDark = ref(localStorage.getItem('admin_dark_mode') === 'true')

  // 全局加载计数器（支持并发请求）
  const loadingCount = ref(0)
  const isGlobalLoading = ref(false)

  // 设置 token 并持久化到 sessionStorage，同时记录时间戳用于过期检查
  const setToken = (t: string) => {
    token.value = t
    sessionStorage.setItem('admin_token', t)
    sessionStorage.setItem('admin_token_ts', Date.now().toString())
  }

  // 设置用户信息并持久化
  const setUserInfo = (info: UserInfo | null) => {
    userInfo.value = info
    sessionStorage.setItem('admin_user_info', JSON.stringify(info))
  }

  // 清除 token、用户信息及时间戳
  const clearToken = () => {
    token.value = ''
    userInfo.value = null
    sessionStorage.removeItem('admin_token')
    sessionStorage.removeItem('admin_user_info')
    sessionStorage.removeItem('admin_token_ts')
  }

  // 获取 token（统一入口）
  const getToken = () => token.value

  // 校验当前 token 是否存在且未过期（S-4: 24小时客户端过期检查）
  // PASETO token 无法客户端解码，服务端 401 仍为最终防线
  const checkTokenValid = () => {
    if (!token.value || typeof token.value !== 'string' || token.value.length === 0) {
      return false
    }
    const ts = sessionStorage.getItem('admin_token_ts')
    if (ts && Date.now() - Number(ts) > 24 * 60 * 60 * 1000) {
      clearToken()
      return false
    }
    return true
  }

  // 切换暗色模式
  const toggleDark = () => {
    isDark.value = !isDark.value
    applyDarkMode(isDark.value)
    localStorage.setItem('admin_dark_mode', String(isDark.value))
  }

  // 应用暗色模式到 HTML 元素
  const applyDarkMode = (dark: boolean) => {
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
