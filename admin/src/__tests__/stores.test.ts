import { describe, it, expect, beforeEach } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useAppStore } from '@/stores'

describe('AppStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    localStorage.clear()
    sessionStorage.clear()
  })

  it('should initialize with empty token', () => {
    const store = useAppStore()
    expect(store.token).toBe('')
  })

  it('should set and get token', () => {
    const store = useAppStore()
    store.setToken('test-token-123')
    expect(store.getToken()).toBe('test-token-123')
    expect(sessionStorage.getItem('admin_token')).toBe('test-token-123')
  })

  it('should clear token and user info', () => {
    const store = useAppStore()
    store.setToken('test-token')
    store.setUserInfo({ username: 'admin' })
    store.clearToken()
    expect(store.token).toBe('')
    expect(store.userInfo).toBeNull()
  })

  it('should validate token correctly', () => {
    const store = useAppStore()
    expect(store.checkTokenValid()).toBe(false)
    store.setToken('valid-token')
    expect(store.checkTokenValid()).toBe(true)
  })

  it('should handle corrupted sessionStorage gracefully', () => {
    sessionStorage.setItem('admin_user_info', '{invalid json}')
    const store = useAppStore()
    expect(store.userInfo).toBeNull()
  })

  it('should toggle dark mode', () => {
    const store = useAppStore()
    expect(store.isDark).toBe(false)
    store.toggleDark()
    expect(store.isDark).toBe(true)
    expect(localStorage.getItem('admin_dark_mode')).toBe('true')
  })

  it('should manage loading state', () => {
    const store = useAppStore()
    expect(store.isGlobalLoading).toBe(false)
    store.startLoading()
    expect(store.isGlobalLoading).toBe(true)
    store.startLoading()
    store.stopLoading()
    expect(store.isGlobalLoading).toBe(true)
    store.stopLoading()
    expect(store.isGlobalLoading).toBe(false)
  })
})
