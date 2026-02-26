import { describe, it, expect, beforeEach } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useAppStore } from '@/stores'

describe('AppStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    localStorage.clear()
    sessionStorage.clear()
  })

  describe('Token 管理', () => {
    it('初始 token 为空', () => {
      const store = useAppStore()
      expect(store.token).toBe('')
    })

    it('setToken 设置 token', () => {
      const store = useAppStore()
      store.setToken('test-token-123')
      expect(store.token).toBe('test-token-123')
    })

    it('getToken 获取 token', () => {
      const store = useAppStore()
      store.setToken('test-token-123')
      expect(store.getToken()).toBe('test-token-123')
    })

    it('setToken 持久化到 sessionStorage', () => {
      const store = useAppStore()
      store.setToken('test-token')
      expect(sessionStorage.getItem('admin_token')).toBe('test-token')
    })

    it('setToken 记录时间戳', () => {
      const store = useAppStore()
      store.setToken('test-token')
      const ts = sessionStorage.getItem('admin_token_ts')
      expect(ts).toBeTruthy()
      expect(Number(ts)).toBeGreaterThan(0)
    })

    it('clearToken 清除 token', () => {
      const store = useAppStore()
      store.setToken('test-token')
      store.clearToken()
      expect(store.token).toBe('')
    })

    it('clearToken 清除 sessionStorage', () => {
      const store = useAppStore()
      store.setToken('test-token')
      store.clearToken()
      expect(sessionStorage.getItem('admin_token')).toBeNull()
      expect(sessionStorage.getItem('admin_token_ts')).toBeNull()
    })
    it('clearToken 清除 userInfo', () => {
      const store = useAppStore()
      store.setToken('t')
      store.setUserInfo({ username: 'admin' })
      store.clearToken()
      expect(store.userInfo).toBeNull()
    })

    it('多次 setToken 覆盖', () => {
      const store = useAppStore()
      store.setToken('token-1')
      store.setToken('token-2')
      expect(store.getToken()).toBe('token-2')
    })

    it('setToken 空字符串', () => {
      const store = useAppStore()
      store.setToken('')
      expect(store.token).toBe('')
    })
  })

  describe('Token 验证', () => {
    it('无 token 时 checkTokenValid 返回 false', () => {
      const store = useAppStore()
      expect(store.checkTokenValid()).toBe(false)
    })

    it('有 token 时 checkTokenValid 返回 true', () => {
      const store = useAppStore()
      store.setToken('valid-token')
      expect(store.checkTokenValid()).toBe(true)
    })

    it('token 过期 24 小时后返回 false', () => {
      const store = useAppStore()
      store.setToken('old-token')
      // 模拟 24 小时前的时间戳
      const oldTs = Date.now() - 25 * 60 * 60 * 1000
      sessionStorage.setItem('admin_token_ts', oldTs.toString())
      expect(store.checkTokenValid()).toBe(false)
    })

    it('token 过期后自动清除', () => {
      const store = useAppStore()
      store.setToken('old-token')
      const oldTs = Date.now() - 25 * 60 * 60 * 1000
      sessionStorage.setItem('admin_token_ts', oldTs.toString())
      store.checkTokenValid()
      expect(store.token).toBe('')
    })

    it('token 未过期时返回 true', () => {
      const store = useAppStore()
      store.setToken('fresh-token')
      // 刚设置的 token，时间戳是当前时间
      expect(store.checkTokenValid()).toBe(true)
    })

    it('无时间戳时仍然有效', () => {
      const store = useAppStore()
      store.setToken('token')
      sessionStorage.removeItem('admin_token_ts')
      expect(store.checkTokenValid()).toBe(true)
    })
  })

  describe('用户信息', () => {
    it('初始 userInfo 为 null', () => {
      const store = useAppStore()
      expect(store.userInfo).toBeNull()
    })

    it('setUserInfo 设置用户信息', () => {
      const store = useAppStore()
      store.setUserInfo({ username: 'admin', role: 'super' })
      expect(store.userInfo?.username).toBe('admin')
      expect(store.userInfo?.role).toBe('super')
    })

    it('setUserInfo 持久化到 sessionStorage', () => {
      const store = useAppStore()
      store.setUserInfo({ username: 'admin' })
      const stored = JSON.parse(sessionStorage.getItem('admin_user_info')!)
      expect(stored.username).toBe('admin')
    })

    it('setUserInfo null 清除', () => {
      const store = useAppStore()
      store.setUserInfo({ username: 'admin' })
      store.setUserInfo(null)
      expect(store.userInfo).toBeNull()
    })

    it('损坏的 sessionStorage 数据不崩溃', () => {
      sessionStorage.setItem('admin_user_info', '{invalid json}')
      const store = useAppStore()
      expect(store.userInfo).toBeNull()
    })

    it('损坏数据后 sessionStorage 被清理', () => {
      sessionStorage.setItem('admin_user_info', '{bad}')
      useAppStore()
      expect(sessionStorage.getItem('admin_user_info')).toBeNull()
    })
  })
  describe('暗色模式', () => {
    it('初始 isDark 为 false', () => {
      const store = useAppStore()
      expect(store.isDark).toBe(false)
    })

    it('toggleDark 切换为 true', () => {
      const store = useAppStore()
      store.toggleDark()
      expect(store.isDark).toBe(true)
    })

    it('toggleDark 再次切换回 false', () => {
      const store = useAppStore()
      store.toggleDark()
      store.toggleDark()
      expect(store.isDark).toBe(false)
    })

    it('toggleDark 持久化到 localStorage', () => {
      const store = useAppStore()
      store.toggleDark()
      expect(localStorage.getItem('admin_dark_mode')).toBe('true')
    })

    it('toggleDark false 持久化', () => {
      const store = useAppStore()
      store.toggleDark()
      store.toggleDark()
      expect(localStorage.getItem('admin_dark_mode')).toBe('false')
    })

    it('从 localStorage 恢复暗色模式', () => {
      localStorage.setItem('admin_dark_mode', 'true')
      setActivePinia(createPinia())
      const store = useAppStore()
      expect(store.isDark).toBe(true)
    })

    it('initDarkMode 不改变状态', () => {
      const store = useAppStore()
      store.initDarkMode()
      expect(store.isDark).toBe(false)
    })
  })

  describe('Loading 状态', () => {
    it('初始 isGlobalLoading 为 false', () => {
      const store = useAppStore()
      expect(store.isGlobalLoading).toBe(false)
    })

    it('startLoading 设为 true', () => {
      const store = useAppStore()
      store.startLoading()
      expect(store.isGlobalLoading).toBe(true)
    })

    it('stopLoading 设为 false', () => {
      const store = useAppStore()
      store.startLoading()
      store.stopLoading()
      expect(store.isGlobalLoading).toBe(false)
    })

    it('并发 loading 计数正确', () => {
      const store = useAppStore()
      store.startLoading()
      store.startLoading()
      store.stopLoading()
      expect(store.isGlobalLoading).toBe(true)
      store.stopLoading()
      expect(store.isGlobalLoading).toBe(false)
    })

    it('stopLoading 不会变为负数', () => {
      const store = useAppStore()
      store.stopLoading()
      store.stopLoading()
      expect(store.isGlobalLoading).toBe(false)
    })

    it('三层并发 loading', () => {
      const store = useAppStore()
      store.startLoading()
      store.startLoading()
      store.startLoading()
      store.stopLoading()
      store.stopLoading()
      expect(store.isGlobalLoading).toBe(true)
      store.stopLoading()
      expect(store.isGlobalLoading).toBe(false)
    })
  })
})