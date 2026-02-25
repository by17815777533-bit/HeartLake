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
  let mock: MockAdapter
  let appStore: ReturnType<typeof useAppStore>

  beforeEach(() => {
    localStorage.clear()
    sessionStorage.clear()
    vi.clearAllMocks()

    setActivePinia(createPinia())
    appStore = useAppStore()

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
      expect(mock.history.get[0].headers!.Authorization).toBe('Bearer test-token-123')
    })

    it('should not add Authorization header when token is empty', async () => {
      mock.onGet('/admin/info').reply(200, { data: {} })

      await api.getAdminInfo()
      expect(mock.history.get[0].headers!.Authorization).toBeUndefined()
    })

    it('should start global loading by default', async () => {
      const startSpy = vi.spyOn(appStore, 'startLoading')
      mock.onGet('/admin/info').reply(200, { data: {} })

      await api.getAdminInfo()
      expect(startSpy).toHaveBeenCalled()
    })
  })

  describe('Response Interceptor - Error Handling', () => {
    it('should handle 401 by clearing token', async () => {
      appStore.setToken('expired-token')
      mock.onGet('/admin/info').reply(401)

      try {
        await api.getAdminInfo()
      } catch {
        expect(appStore.token).toBe('')
      }
    })

    it('should handle 500 server error', async () => {
      mock.onGet('/admin/info').reply(500)
      await expect(api.getAdminInfo()).rejects.toThrow()
    })

    it('should handle network error', async () => {
      mock.onGet('/admin/info').networkError()
      await expect(api.getAdminInfo()).rejects.toThrow()
    })
  })

  describe('Auth API', () => {
    it('should call login endpoint', async () => {
      mock.onPost('/admin/login').reply(200, { data: { token: 'new-token' } })

      const res = await api.login({ username: 'admin', password: '123456' })
      expect(res.data.data.token).toBe('new-token')
      expect(mock.history.post[0].url).toBe('/admin/login')
    })

    it('should call logout endpoint', async () => {
      mock.onPost('/admin/logout').reply(200, { code: 0 })
      await api.logout()
      expect(mock.history.post[0].url).toBe('/admin/logout')
    })
  })

  describe('Business API Endpoints', () => {
    it('should call dashboard stats endpoint', async () => {
      mock.onGet('/admin/dashboard/stats').reply(200, { data: {} })
      const res = await api.getDashboardStats()
      expect(res.status).toBe(200)
    })

    it('should call users endpoint with params', async () => {
      mock.onGet('/admin/users').reply(200, { data: { list: [], total: 0 } })
      await api.getUsers({ page: 1, page_size: 20 })
      expect(mock.history.get[0].params.page).toBe(1)
    })

    it('should call contents endpoint', async () => {
      mock.onGet('/admin/contents').reply(200, { data: { list: [] } })
      await api.getContents({ page: 1 })
      expect(mock.history.get[0].url).toBe('/admin/contents')
    })

    it('should call reports handle endpoint', async () => {
      mock.onPost('/admin/reports/789/handle').reply(200, { code: 0 })
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
      mock.onGet('/admin/dashboard/stats').reply(200, { data: {} })

      const promise1 = api.getAdminInfo()
      const promise2 = api.getDashboardStats()

      await Promise.all([promise1, promise2])
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('should stop loading even when request fails', async () => {
      mock.onGet('/admin/info').reply(500)

      try {
        await api.getAdminInfo()
      } catch {
        expect(appStore.isGlobalLoading).toBe(false)
      }
    })
  })
})
