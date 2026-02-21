import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import MockAdapter from 'axios-mock-adapter'
import { setActivePinia, createPinia } from 'pinia'
import { useAppStore } from '@/stores'
import api, { http } from '@/api'

// 模拟 Element Plus 和 Vue Router
vi.mock('element-plus', () => ({
  ElMessage: {
    error: vi.fn(),
    warning: vi.fn(),
    success: vi.fn(),
  },
}))

vi.mock('@/router', () => ({
  default: {
    push: vi.fn().mockResolvedValue(undefined),
    currentRoute: {
      value: { path: '/dashboard' },
    },
  },
}))

describe('API Module', () => {
  let mock
  let appStore

  beforeEach(() => {
    // 先清 localStorage，再创建 pinia，避免 store 初始化时读到上一个测试的残留 token
    localStorage.clear()
    vi.clearAllMocks()

    setActivePinia(createPinia())
    appStore = useAppStore()

    // mock 绑定到实际的 http 实例
    mock = new MockAdapter(http)
  })

  afterEach(() => {
    mock.restore()
  })

  describe('HTTP Instance Configuration', () => {
    it('should have correct baseURL', () => {
      expect(http.defaults.baseURL).toBe('/api')
    })

    it('should have 15 second timeout', () => {
      expect(http.defaults.timeout).toBe(15000)
    })

    it('should have CSRF protection headers configured', () => {
      expect(http.defaults.xsrfCookieName).toBe('csrf_token')
      expect(http.defaults.xsrfHeaderName).toBe('X-CSRF-Token')
    })
  })

  describe('Request Interceptor', () => {
    it('should inject token into Authorization header when token exists', async () => {
      appStore.setToken('test-token-123')
      mock.onGet('/admin/info').reply(200, { data: {} })

      await api.getAdminInfo()
      expect(mock.history.get[0].headers.Authorization).toBe('Bearer test-token-123')
    })

    it('should not add Authorization header when token is empty', async () => {
      mock.onGet('/admin/info').reply(200, { data: {} })

      await api.getAdminInfo()
      expect(mock.history.get[0].headers.Authorization).toBeUndefined()
    })

    it('should start global loading by default', async () => {
      const startSpy = vi.spyOn(appStore, 'startLoading')
      mock.onGet('/admin/info').reply(200, { data: {} })

      await api.getAdminInfo()
      // 验证 startLoading 被调用过（MockAdapter 同步响应，loading 可能已结束）
      expect(startSpy).toHaveBeenCalled()
    })

    it('should skip loading when skipLoading is true', async () => {
      mock.onGet('/admin/stats/realtime').reply(200, { data: {} })

      await api.getRealtimeStats()
      // skipLoading 请求不应触发 loading
      expect(appStore.isGlobalLoading).toBe(false)
    })
  })

  describe('Response Interceptor - 401 Unauthorized', () => {
    it('should clear token and redirect to login on 401', async () => {
      const { ElMessage } = await import('element-plus')
      const router = (await import('@/router')).default

      appStore.setToken('expired-token')
      mock.onGet('/admin/info').reply(401, { message: 'Unauthorized' })

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(appStore.token).toBe('')
        expect(ElMessage.error).toHaveBeenCalledWith('登录已过期，请重新登录')
        expect(router.push).toHaveBeenCalledWith('/login')
      }
    })

    it('should not redirect if already on login page', async () => {
      const router = (await import('@/router')).default
      router.currentRoute.value.path = '/login'

      appStore.setToken('expired-token')
      mock.onGet('/admin/info').reply(401)

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(router.push).not.toHaveBeenCalled()
      }

      // 恢复
      router.currentRoute.value.path = '/dashboard'
    })
  })

  describe('Response Interceptor - 403 Forbidden', () => {
    it('should show permission error message', async () => {
      const { ElMessage } = await import('element-plus')
      mock.onGet('/admin/info').reply(403, { message: '权限不足' })

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.error).toHaveBeenCalledWith('权限不足')
      }
    })

    it('should show default message when no message in response', async () => {
      const { ElMessage } = await import('element-plus')
      mock.onGet('/admin/info').reply(403)

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.error).toHaveBeenCalledWith('没有权限执行此操作')
      }
    })
  })

  describe('Response Interceptor - 429 Rate Limit', () => {
    it('should show rate limit warning with retry-after', async () => {
      const { ElMessage } = await import('element-plus')
      mock.onGet('/admin/info').reply(429, {}, { 'retry-after': '30' })

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.warning).toHaveBeenCalledWith('操作过于频繁，请 30 秒后再试')
      }
    })

    it('should show generic rate limit warning without retry-after', async () => {
      const { ElMessage } = await import('element-plus')
      mock.onGet('/admin/info').reply(429)

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.warning).toHaveBeenCalledWith('操作过于频繁，请稍后再试')
      }
    })
  })

  describe('Response Interceptor - 5xx Server Error', () => {
    it('should show server error message on 500', async () => {
      const { ElMessage } = await import('element-plus')
      mock.onGet('/admin/info').reply(500)

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.error).toHaveBeenCalledWith('服务器错误，请稍后重试')
      }
    })

    it('should show server error message on 502', async () => {
      const { ElMessage } = await import('element-plus')
      mock.onGet('/admin/info').reply(502)

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.error).toHaveBeenCalledWith('服务器错误，请稍后重试')
      }
    })
  })

  describe('Response Interceptor - Network Error', () => {
    it('should show network error message', async () => {
      const { ElMessage } = await import('element-plus')
      mock.onGet('/admin/info').networkError()

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.error).toHaveBeenCalledWith('网络连接失败，请检查网络')
      }
    })

    it('should show timeout error message', async () => {
      const { ElMessage } = await import('element-plus')
      mock.onGet('/admin/info').timeout()

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.error).toHaveBeenCalledWith('请求超时，请检查网络')
      }
    })
  })

  describe('API Methods', () => {
    it('should call login endpoint', async () => {
      mock.onPost('/admin/login').reply(200, { token: 'new-token' })

      const res = await api.login({ username: 'admin', password: '123' })
      expect(res.data.token).toBe('new-token')
      expect(mock.history.post[0].url).toBe('/admin/login')
    })

    it('should call getDashboardStats endpoint', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { data: {} })

      await api.getDashboardStats()
      expect(mock.history.get[0].url).toBe('/admin/stats/dashboard')
    })

    it('should call getUsers with params', async () => {
      mock.onGet('/admin/users').reply(200, { data: [] })

      await api.getUsers({ page: 1, limit: 10 })
      expect(mock.history.get[0].params).toEqual({ page: 1, limit: 10 })
    })

    it('should call banUser endpoint', async () => {
      mock.onPost('/admin/users/123/ban').reply(200)

      await api.banUser('123', '违规')
      expect(JSON.parse(mock.history.post[0].data)).toEqual({ reason: '违规' })
    })

    it('should call unbanUser endpoint', async () => {
      mock.onPost('/admin/users/123/unban').reply(200)

      await api.unbanUser('123')
      expect(mock.history.post[0].url).toBe('/admin/users/123/unban')
    })

    it('should call deleteStone endpoint', async () => {
      mock.onDelete('/admin/stones/456').reply(200)

      await api.deleteStone('456', '内容违规')
      expect(mock.history.delete[0].url).toBe('/admin/stones/456')
    })

    it('should call getReports with params', async () => {
      mock.onGet('/admin/reports').reply(200, { data: [] })

      await api.getReports({ status: 'pending' })
      expect(mock.history.get[0].params).toEqual({ status: 'pending' })
    })

    it('should call handleReport endpoint', async () => {
      mock.onPost('/admin/reports/789/handle').reply(200)

      await api.handleReport('789', { action: 'resolve' })
      expect(mock.history.post[0].url).toBe('/admin/reports/789/handle')
    })

    it('should call edge AI endpoints', async () => {
      mock.onGet('/admin/edge-ai/status').reply(200, { status: 'ok' })

      const res = await api.getEdgeAIStatus()
      expect(res.data.status).toBe('ok')
    })
  })

  describe('Loading State Management', () => {
    it('should handle concurrent requests correctly', async () => {
      mock.onGet('/admin/info').reply(200, { data: {} })
      mock.onGet('/admin/stats/dashboard').reply(200, { data: {} })

      const promise1 = api.getAdminInfo()
      const promise2 = api.getDashboardStats()

      await Promise.all([promise1, promise2])

      // 所有请求完成后，loading 应该停止
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('should stop loading even when request fails', async () => {
      mock.onGet('/admin/info').reply(500)

      try {
        await api.getAdminInfo()
      } catch (error) {
        // 即使请求失败，loading 也应该停止
        expect(appStore.isGlobalLoading).toBe(false)
      }
    })
  })
})
