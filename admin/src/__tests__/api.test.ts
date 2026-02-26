import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import MockAdapter from 'axios-mock-adapter'
import { setActivePinia, createPinia } from 'pinia'
import { useAppStore } from '@/stores'
import api, { http, cancelAllRequests } from '@/api'

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
      try { await api.getAdminInfo() } catch { expect(appStore.isGlobalLoading).toBe(false) }
    })
  })

  describe('Users API', () => {
    it('getUsers 成功', async () => {
      mock.onGet('/admin/users').reply(200, { data: { list: [{ user_id: 'u1' }], total: 1 } })
      const res = await api.getUsers({ page: 1, page_size: 10 })
      expect(res.data.data.list).toHaveLength(1)
    })

    it('getUserDetail 成功', async () => {
      mock.onGet('/admin/users/u1').reply(200, { data: { user_id: 'u1' } })
      const res = await api.getUserDetail('u1')
      expect(res.data.data.user_id).toBe('u1')
    })

    it('banUser 成功', async () => {
      mock.onPost('/admin/users/u1/ban').reply(200, { code: 0 })
      await api.banUser('u1', { reason: '违规' })
      expect(mock.history.post[0].url).toBe('/admin/users/u1/ban')
    })

    it('unbanUser 成功', async () => {
      mock.onPost('/admin/users/u1/unban').reply(200, { code: 0 })
      await api.unbanUser('u1')
      expect(mock.history.post[0].url).toBe('/admin/users/u1/unban')
    })

    it('getUsers 500', async () => {
      mock.onGet('/admin/users').reply(500)
      try { await api.getUsers({}); expect.unreachable() } catch (e: any) { expect(e.response.status).toBe(500) }
    })

    it('getUsers 带 keyword', async () => {
      mock.onGet('/admin/users').reply(200, { data: { list: [] } })
      await api.getUsers({ page: 1, keyword: '测试' })
      expect(mock.history.get[0].params.keyword).toBe('测试')
    })
  })

  describe('Content API', () => {
    it('deleteContent 成功', async () => {
      mock.onDelete('/admin/contents/c1').reply(200, { code: 0 })
      await api.deleteContent('c1')
      expect(mock.history.delete[0].url).toBe('/admin/contents/c1')
    })

    it('getContents 带 keyword', async () => {
      mock.onGet('/admin/contents').reply(200, { data: { list: [] } })
      await api.getContents({ page: 1, keyword: '心湖' })
      expect(mock.history.get[0].params.keyword).toBe('心湖')
    })

    it('getContents 500', async () => {
      mock.onGet('/admin/contents').reply(500)
      try { await api.getContents({}); expect.unreachable() } catch (e: any) { expect(e.response.status).toBe(500) }
    })
  })

  describe('Reports API', () => {
    it('getReports 成功', async () => {
      mock.onGet('/admin/reports').reply(200, { data: { list: [], total: 0 } })
      const res = await api.getReports({ page: 1 })
      expect(res.status).toBe(200)
    })

    it('getReports 带状态筛选', async () => {
      mock.onGet('/admin/reports').reply(200, { data: { list: [] } })
      await api.getReports({ page: 1, status: 'pending' })
      expect(mock.history.get[0].params.status).toBe('pending')
    })

    it('handleReport resolve', async () => {
      mock.onPost('/admin/reports/r1/handle').reply(200, { code: 0 })
      await api.handleReport('r1', { action: 'resolve' })
      expect(JSON.parse(mock.history.post[0].data).action).toBe('resolve')
    })

    it('handleReport ignore', async () => {
      mock.onPost('/admin/reports/r2/handle').reply(200, { code: 0 })
      await api.handleReport('r2', { action: 'ignore' })
      expect(mock.history.post[0].url).toBe('/admin/reports/r2/handle')
    })

    it('getReports 500', async () => {
      mock.onGet('/admin/reports').reply(500)
      try { await api.getReports({}); expect.unreachable() } catch (e: any) { expect(e.response.status).toBe(500) }
    })
  })

  describe('Moderation API', () => {
    it('getPendingModeration 成功', async () => {
      mock.onGet('/admin/moderation/pending').reply(200, { data: { list: [] } })
      const res = await api.getPendingModeration({ page: 1 })
      expect(res.status).toBe(200)
    })

    it('approveContent 成功', async () => {
      mock.onPost('/admin/moderation/m1/approve').reply(200, { code: 0 })
      await api.approveContent('m1')
      expect(mock.history.post[0].url).toBe('/admin/moderation/m1/approve')
    })

    it('rejectContent 成功', async () => {
      mock.onPost('/admin/moderation/m1/reject').reply(200, { code: 0 })
      await api.rejectContent('m1', '内容违规')
      expect(JSON.parse(mock.history.post[0].data).reason).toBe('内容违规')
    })

    it('getPendingModeration 500', async () => {
      mock.onGet('/admin/moderation/pending').reply(500)
      try { await api.getPendingModeration({}); expect.unreachable() } catch (e: any) { expect(e.response.status).toBe(500) }
    })
  })

  describe('Sensitive Words API', () => {
    it('getSensitiveWords 成功', async () => {
      mock.onGet('/admin/sensitive-words').reply(200, { data: { words: [], total: 0 } })
      const res = await api.getSensitiveWords({ page: 1 })
      expect(res.status).toBe(200)
    })

    it('addSensitiveWord 成功', async () => {
      mock.onPost('/admin/sensitive-words').reply(200, { code: 0 })
      await api.addSensitiveWord({ word: '测试', level: 'high' })
      expect(mock.history.post[0].url).toBe('/admin/sensitive-words')
    })

    it('updateSensitiveWord 成功', async () => {
      mock.onPut('/admin/sensitive-words/1').reply(200, { code: 0 })
      await api.updateSensitiveWord(1, { word: '更新', level: 'medium' })
      expect(mock.history.put[0].url).toBe('/admin/sensitive-words/1')
    })

    it('deleteSensitiveWord 成功', async () => {
      mock.onDelete('/admin/sensitive-words/1').reply(200, { code: 0 })
      await api.deleteSensitiveWord(1)
      expect(mock.history.delete[0].url).toBe('/admin/sensitive-words/1')
    })

    it('getSensitiveWords 500', async () => {
      mock.onGet('/admin/sensitive-words').reply(500)
      try { await api.getSensitiveWords({}); expect.unreachable() } catch (e: any) { expect(e.response.status).toBe(500) }
    })
  })

  describe('Settings API', () => {
    it('getSystemConfig 成功', async () => {
      mock.onGet('/admin/config').reply(200, { data: {} })
      const res = await api.getSystemConfig()
      expect(res.status).toBe(200)
    })

    it('updateSystemConfig 成功', async () => {
      mock.onPut('/admin/config').reply(200, { code: 0 })
      await api.updateSystemConfig({ system: { name: '心湖' } })
      expect(mock.history.put[0].url).toBe('/admin/config')
    })

    it('broadcastMessage 成功', async () => {
      mock.onPost('/admin/broadcast').reply(200, { code: 0 })
      await api.broadcastMessage({ message: '测试', level: 'info' })
      expect(mock.history.post[0].url).toBe('/admin/broadcast')
    })

    it('getSystemConfig 500', async () => {
      mock.onGet('/admin/config').reply(500)
      try { await api.getSystemConfig(); expect.unreachable() } catch (e: any) { expect(e.response.status).toBe(500) }
    })
  })

  describe('Logs API', () => {
    it('getOperationLogs 成功', async () => {
      mock.onGet('/admin/logs').reply(200, { data: { list: [] } })
      const res = await api.getOperationLogs({ page: 1 })
      expect(res.status).toBe(200)
    })

    it('getOperationLogs 带筛选', async () => {
      mock.onGet('/admin/logs').reply(200, { data: { list: [] } })
      await api.getOperationLogs({ page: 1, action: 'login' })
      expect(mock.history.get[0].params.action).toBe('login')
    })

    it('getOperationLogs 500', async () => {
      mock.onGet('/admin/logs').reply(500)
      try { await api.getOperationLogs({}); expect.unreachable() } catch (e: any) { expect(e.response.status).toBe(500) }
    })
  })

  describe('Edge AI API', () => {
    it('getEdgeAIStatus 成功', async () => {
      mock.onGet('/admin/edge-ai/status').reply(200, { data: { status: 'ok' } })
      const res = await api.getEdgeAIStatus()
      expect(res.status).toBe(200)
    })

    it('getEdgeAIMetrics 成功', async () => {
      mock.onGet('/admin/edge-ai/metrics').reply(200, { data: {} })
      const res = await api.getEdgeAIMetrics()
      expect(res.status).toBe(200)
    })

    it('getEmotionPulse 成功', async () => {
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(200, { data: {} })
      const res = await api.getEmotionPulse()
      expect(res.status).toBe(200)
    })

    it('getPrivacyBudget 成功', async () => {
      mock.onGet('/admin/edge-ai/privacy-budget').reply(200, { data: {} })
      const res = await api.getPrivacyBudget()
      expect(res.status).toBe(200)
    })

    it('triggerFederatedAggregation 成功', async () => {
      mock.onPost('/admin/edge-ai/federated/aggregate').reply(200, { code: 0 })
      await api.triggerFederatedAggregation({ rounds: 5 })
      expect(mock.history.post[0].url).toBe('/admin/edge-ai/federated/aggregate')
    })

    it('edgeAIVectorSearch 成功', async () => {
      mock.onPost('/admin/edge-ai/vector-search').reply(200, { data: [] })
      await api.edgeAIVectorSearch({ query: 'test' })
      expect(mock.history.post[0].url).toBe('/admin/edge-ai/vector-search')
    })

    it('getEdgeAIConfig 成功', async () => {
      mock.onGet('/admin/edge-ai/config').reply(200, { data: {} })
      const res = await api.getEdgeAIConfig()
      expect(res.status).toBe(200)
    })

    it('updateEdgeAIConfig 成功', async () => {
      mock.onPut('/admin/edge-ai/config').reply(200, { code: 0 })
      await api.updateEdgeAIConfig({ key: 'value' })
      expect(mock.history.put[0].url).toBe('/admin/edge-ai/config')
    })

    it('analyzeText 成功', async () => {
      mock.onPost('/admin/edge-ai/analyze').reply(200, { data: { sentiment: 'positive' } })
      const res = await api.analyzeText('开心')
      expect(res.status).toBe(200)
    })

    it('moderateText 成功', async () => {
      mock.onPost('/admin/edge-ai/moderate').reply(200, { data: { safe: true } })
      const res = await api.moderateText('正常内容')
      expect(res.status).toBe(200)
    })

    it('getPrivacyBudget 成功', async () => {
      mock.onGet('/admin/edge-ai/privacy-budget').reply(200, { data: {} })
      const res = await api.getPrivacyBudget()
      expect(res.status).toBe(200)
    })

    it('getEmotionPulse (resonance) 成功', async () => {
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(200, { data: {} })
      const res = await api.getEmotionPulse()
      expect(res.status).toBe(200)
    })
  })

  describe('Recommendations API', () => {
    it('getTrendingContent 成功', async () => {
      mock.onGet('/recommendations/trending').reply(200, { data: [] })
      const res = await api.getTrendingContent()
      expect(res.status).toBe(200)
    })

    it('getEmotionTrends 成功', async () => {
      mock.onGet('/recommendations/emotion-trends').reply(200, { data: [] })
      const res = await api.getEmotionTrends()
      expect(res.status).toBe(200)
    })

    it('getRecommendationStats 成功', async () => {
      mock.onGet('/recommendations/stones').reply(200, { data: [] })
      const res = await api.getRecommendationStats()
      expect(res.status).toBe(200)
    })
  })

  describe('Request Interceptor - skipLoading', () => {
    it('skipLoading 不触发 loading', async () => {
      const startSpy = vi.spyOn(appStore, 'startLoading')
      mock.onGet('/admin/edge-ai/status').reply(200, { data: {} })
      await api.getEdgeAIStatus()
      expect(startSpy).not.toHaveBeenCalled()
    })

    it('testAIConnection 复用 edge-ai/status', async () => {
      mock.onGet('/admin/edge-ai/status').reply(200, { code: 0 })
      await api.testAIConnection()
      expect(mock.history.get[0].url).toBe('/admin/edge-ai/status')
    })
  })

  describe('Response Interceptor - Business Error', () => {
    it('业务码非0非200触发错误', async () => {
      mock.onGet('/admin/info').reply(200, { code: 400001, message: '参数错误' })
      try { await api.getAdminInfo(); expect.unreachable() } catch (e: any) { expect(e.message).toContain('参数错误') }
    })

    it('业务码0表示成功', async () => {
      mock.onGet('/admin/info').reply(200, { code: 0, data: {} })
      const res = await api.getAdminInfo()
      expect(res.data.code).toBe(0)
    })

    it('业务码200表示成功', async () => {
      mock.onGet('/admin/info').reply(200, { code: 200, data: {} })
      const res = await api.getAdminInfo()
      expect(res.data.code).toBe(200)
    })

    it('认证错误码清除token', async () => {
      appStore.setToken('some-token')
      mock.onGet('/admin/info').reply(200, { code: 200001, message: 'token过期' })
      try { await api.getAdminInfo() } catch { expect(appStore.token).toBe('') }
    })
  })

  describe('cancelAllRequests', () => {
    it('函数存在且可调用', () => {
      expect(typeof cancelAllRequests).toBe('function')
      cancelAllRequests()
    })
  })
})
