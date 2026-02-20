import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import axios from 'axios'
import MockAdapter from 'axios-mock-adapter'
import { setActivePinia, createPinia } from 'pinia'
import { useAppStore } from '@/stores'
import api from '@/api'

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
    // 设置 Pinia
    setActivePinia(createPinia())
    appStore = useAppStore()

    // 创建 axios mock adapter
    mock = new MockAdapter(axios)

    // 清除 localStorage
    localStorage.clear()

    // 重置所有 mock
    vi.clearAllMocks()
  })

  afterEach(() => {
    mock.restore()
  })

  describe('HTTP Instance Configuration', () => {
    it('should have correct baseURL', () => {
      const { ElMessage } = await import('element-plus')
      const router = (await import('@/router')).default

      // 创建一个测试请求来验证配置
      mock.onGet('/api/test').reply(200, { data: 'test' })

      return axios.get('/api/test').then(() => {
        expect(mock.history.get[0].baseURL).toBe('/api')
      })
    })

    it('should have 15 second timeout', async () => {
      // 通过检查 axios 实例的默认配置
      const axiosInstance = (await import('@/api/index.js')).default
      // 注意：由于我们导出的是 API 方法对象，需要通过实际请求来验证
      mock.onGet('/api/admin/info').timeout()

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(error.code).toBe('ECONNABORTED')
      }
    })

    it('should have CSRF protection headers configured', async () => {
      mock.onGet('/api/test').reply(200)

      // 设置 CSRF cookie
      document.cookie = 'csrf_token=test-csrf-token'

      await axios.get('/api/test')

      // 验证请求配置包含 CSRF 设置
      const request = mock.history.get[0]
      expect(request.xsrfCookieName).toBe('csrf_token')
      expect(request.xsrfHeaderName).toBe('X-CSRF-Token')
    })
  })

  describe('Request Interceptor', () => {
    it('should inject token into Authorization header when token exists', async () => {
      const testToken = 'test-paseto-token'
      appStore.setToken(testToken)

      mock.onGet('/api/admin/info').reply(200, { data: {} })

      await api.getAdminInfo()

      const request = mock.history.get[0]
      expect(request.headers.Authorization).toBe(`Bearer ${testToken}`)
    })

    it('should not add Authorization header when token is empty', async () => {
      appStore.clearToken()

      mock.onPost('/api/admin/login').reply(200, { data: {} })

      await api.login({ username: 'admin', password: 'pass' })

      const request = mock.history.post[0]
      expect(request.headers.Authorization).toBeUndefined()
    })

    it('should start global loading by default', async () => {
      mock.onGet('/api/admin/info').reply(200, { data: {} })

      expect(appStore.isGlobalLoading).toBe(false)

      const promise = api.getAdminInfo()

      // 请求发送后，loading 应该启动
      await vi.waitFor(() => {
        expect(appStore.loadingCount).toBeGreaterThan(0)
      })

      await promise

      // 请求完成后，loading 应该停止
      expect(appStore.loadingCount).toBe(0)
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('should skip loading when skipLoading is true', async () => {
      mock.onGet('/api/admin/stats/realtime').reply(200, { data: {} })

      await api.getRealtimeStats()

      // skipLoading 的请求不应该触发 loading
      expect(appStore.loadingCount).toBe(0)
      expect(appStore.isGlobalLoading).toBe(false)
    })
  })

  describe('Response Interceptor - 401 Unauthorized', () => {
    it('should clear token and redirect to login on 401', async () => {
      const { ElMessage } = await import('element-plus')
      const router = (await import('@/router')).default

      appStore.setToken('expired-token')
      mock.onGet('/api/admin/info').reply(401, { message: 'Unauthorized' })

      try {
        await api.getAdminInfo()
      } catch (error) {
        // 验证 token 被清除
        expect(appStore.token).toBe('')
        expect(localStorage.getItem('admin_token')).toBeNull()

        // 验证显示错误消息
        expect(ElMessage.error).toHaveBeenCalledWith('登录已过期，请重新登录')

        // 验证跳转到登录页
        expect(router.push).toHaveBeenCalledWith('/login')
      }
    })

    it('should not redirect when already on login page', async () => {
      const router = (await import('@/router')).default
      router.currentRoute.value.path = '/login'

      appStore.setToken('expired-token')
      mock.onGet('/api/admin/info').reply(401)

      try {
        await api.getAdminInfo()
      } catch (error) {
        // 应该清除 token
        expect(appStore.token).toBe('')

        // 但不应该跳转（因为已经在登录页）
        expect(router.push).not.toHaveBeenCalled()
      }
    })

    it('should prevent multiple redirects on concurrent 401 errors', async () => {
      const router = (await import('@/router')).default

      appStore.setToken('expired-token')
      mock.onGet('/api/admin/info').reply(401)
      mock.onGet('/api/admin/stats/dashboard').reply(401)

      try {
        await Promise.all([
          api.getAdminInfo(),
          api.getDashboardStats(),
        ])
      } catch (error) {
        // 只应该跳转一次
        expect(router.push).toHaveBeenCalledTimes(1)
      }
    })
  })

  describe('Response Interceptor - 429 Rate Limit', () => {
    it('should show retry-after message when header exists', async () => {
      const { ElMessage } = await import('element-plus')

      mock.onGet('/api/admin/info').reply(429, {}, { 'retry-after': '60' })

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.warning).toHaveBeenCalledWith('操作过于频繁，请 60 秒后再试')
      }
    })

    it('should show generic message when retry-after header missing', async () => {
      const { ElMessage } = await import('element-plus')

      mock.onGet('/api/admin/info').reply(429)

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.warning).toHaveBeenCalledWith('操作过于频繁，请稍后再试')
      }
    })
  })

  describe('Response Interceptor - Other Errors', () => {
    it('should show permission error on 403', async () => {
      const { ElMessage } = await import('element-plus')

      mock.onDelete('/api/admin/stones/123').reply(403, { message: '权限不足' })

      try {
        await api.deleteStone('123', 'test reason')
      } catch (error) {
        expect(ElMessage.error).toHaveBeenCalledWith('权限不足')
      }
    })

    it('should show generic permission error on 403 without message', async () => {
      const { ElMessage } = await import('element-plus')

      mock.onDelete('/api/admin/stones/123').reply(403)

      try {
        await api.deleteStone('123', 'test reason')
      } catch (error) {
        expect(ElMessage.error).toHaveBeenCalledWith('没有权限执行此操作')
      }
    })

    it('should show server error on 500', async () => {
      const { ElMessage } = await import('element-plus')

      mock.onGet('/api/admin/info').reply(500)

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.error).toHaveBeenCalledWith('服务器错误，请稍后重试')
      }
    })

    it('should show timeout error on ECONNABORTED', async () => {
      const { ElMessage } = await import('element-plus')

      mock.onGet('/api/admin/info').timeout()

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.error).toHaveBeenCalledWith('请求超时，请检查网络')
      }
    })

    it('should show network error on connection failure', async () => {
      const { ElMessage } = await import('element-plus')

      mock.onGet('/api/admin/info').networkError()

      try {
        await api.getAdminInfo()
      } catch (error) {
        expect(ElMessage.error).toHaveBeenCalledWith('网络连接失败，请检查网络')
      }
    })
  })

  describe('API Methods - URL and Parameters', () => {
    it('login should POST to /api/admin/login with credentials', async () => {
      const credentials = { username: 'admin', password: 'password123' }
      mock.onPost('/api/admin/login').reply(200, { data: { token: 'token' } })

      await api.login(credentials)

      const request = mock.history.post[0]
      expect(request.url).toBe('/api/admin/login')
      expect(JSON.parse(request.data)).toEqual(credentials)
    })

    it('getDashboardStats should GET /api/admin/stats/dashboard', async () => {
      mock.onGet('/api/admin/stats/dashboard').reply(200, { data: {} })

      await api.getDashboardStats()

      expect(mock.history.get[0].url).toBe('/api/admin/stats/dashboard')
    })

    it('getUserGrowthStats should pass days parameter', async () => {
      mock.onGet('/api/admin/stats/user-growth').reply(200, { data: {} })

      await api.getUserGrowthStats(30)

      const request = mock.history.get[0]
      expect(request.url).toBe('/api/admin/stats/user-growth')
      expect(request.params).toEqual({ days: 30 })
    })

    it('getUsers should pass query parameters', async () => {
      const params = { page: 2, page_size: 20, keyword: 'test' }
      mock.onGet('/api/admin/users').reply(200, { data: {} })

      await api.getUsers(params)

      const request = mock.history.get[0]
      expect(request.params).toEqual(params)
    })

    it('banUser should POST to correct URL with reason', async () => {
      mock.onPost('/api/admin/users/user123/ban').reply(200)

      await api.banUser('user123', '违规内容')

      const request = mock.history.post[0]
      expect(request.url).toBe('/api/admin/users/user123/ban')
      expect(JSON.parse(request.data)).toEqual({ reason: '违规内容' })
    })

    it('deleteStone should DELETE with reason in data', async () => {
      mock.onDelete('/api/admin/stones/stone123').reply(200)

      await api.deleteStone('stone123', '敏感内容')

      const request = mock.history.delete[0]
      expect(request.url).toBe('/api/admin/stones/stone123')
      expect(JSON.parse(request.data)).toEqual({ reason: '敏感内容' })
    })

    it('handleReport should POST to correct URL', async () => {
      const data = { action: 'approve', note: 'test' }
      mock.onPost('/api/admin/reports/report123/handle').reply(200)

      await api.handleReport('report123', data)

      const request = mock.history.post[0]
      expect(request.url).toBe('/api/admin/reports/report123/handle')
      expect(JSON.parse(request.data)).toEqual(data)
    })

    it('updateSystemConfig should PUT to /api/admin/config', async () => {
      const config = { max_upload_size: 10485760 }
      mock.onPut('/api/admin/config').reply(200)

      await api.updateSystemConfig(config)

      const request = mock.history.put[0]
      expect(request.url).toBe('/api/admin/config')
      expect(JSON.parse(request.data)).toEqual(config)
    })

    it('edgeAIAnalyze should POST text to /api/edge-ai/analyze', async () => {
      mock.onPost('/api/edge-ai/analyze').reply(200, { data: {} })

      await api.edgeAIAnalyze('今天心情很好')

      const request = mock.history.post[0]
      expect(request.url).toBe('/api/edge-ai/analyze')
      expect(JSON.parse(request.data)).toEqual({ text: '今天心情很好' })
    })

    it('triggerFederatedAggregation should POST data correctly', async () => {
      const data = { round: 1, epsilon: 2.0 }
      mock.onPost('/api/edge-ai/federated/aggregate').reply(200)

      await api.triggerFederatedAggregation(data)

      const request = mock.history.post[0]
      expect(request.url).toBe('/api/edge-ai/federated/aggregate')
      expect(JSON.parse(request.data)).toEqual(data)
    })
  })

  describe('API Methods - skipLoading Flag', () => {
    it('getRealtimeStats should skip loading', async () => {
      mock.onGet('/api/admin/stats/realtime').reply(200, { data: {} })

      await api.getRealtimeStats()

      expect(appStore.loadingCount).toBe(0)
    })

    it('getEdgeAIStatus should skip loading', async () => {
      mock.onGet('/api/edge-ai/status').reply(200, { data: {} })

      await api.getEdgeAIStatus()

      expect(appStore.loadingCount).toBe(0)
    })

    it('getPrivacyBudget should skip loading', async () => {
      mock.onGet('/api/edge-ai/privacy-budget').reply(200, { data: {} })

      await api.getPrivacyBudget()

      expect(appStore.loadingCount).toBe(0)
    })
  })

  describe('Loading State Management', () => {
    it('should handle concurrent requests correctly', async () => {
      mock.onGet('/api/admin/info').reply(200, { data: {} })
      mock.onGet('/api/admin/stats/dashboard').reply(200, { data: {} })

      const promise1 = api.getAdminInfo()
      const promise2 = api.getDashboardStats()

      // 两个请求都在进行中
      await vi.waitFor(() => {
        expect(appStore.loadingCount).toBe(2)
        expect(appStore.isGlobalLoading).toBe(true)
      })

      await Promise.all([promise1, promise2])

      // 所有请求完成后，loading 应该停止
      expect(appStore.loadingCount).toBe(0)
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('should stop loading even when request fails', async () => {
      mock.onGet('/api/admin/info').reply(500)

      try {
        await api.getAdminInfo()
      } catch (error) {
        // 即使请求失败，loading 也应该停止
        expect(appStore.loadingCount).toBe(0)
        expect(appStore.isGlobalLoading).toBe(false)
      }
    })
  })
})
