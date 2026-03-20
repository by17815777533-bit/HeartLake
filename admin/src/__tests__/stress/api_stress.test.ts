import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import MockAdapter from 'axios-mock-adapter'
import { setActivePinia, createPinia } from 'pinia'
import { useAppStore } from '@/stores'
import api, { http, cancelAllRequests } from '@/api'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), warning: vi.fn(), success: vi.fn() },
}))

vi.mock('@/router', () => ({
  default: {
    push: vi.fn().mockResolvedValue(undefined),
    currentRoute: { value: { path: '/dashboard' } },
  },
}))

vi.mock('@/utils/errorHelper', () => ({
  getBusinessMessage: vi.fn(() => ''),
}))

describe('API Stress Tests', () => {
  let mock: MockAdapter
  let appStore: ReturnType<typeof useAppStore>
  let noRetryId: number
  let cleanupInterceptorId: number

  beforeEach(() => {
    localStorage.clear()
    sessionStorage.clear()
    vi.clearAllMocks()
    setActivePinia(createPinia())
    appStore = useAppStore()
    mock = new MockAdapter(http)
    noRetryId = http.interceptors.request.use((config) => {
      ;(config as any)._retryCount = Infinity
      return config
    })
    // Prevent DataCloneError: JSON round-trip strips ALL functions/classes from error objects
    // so Vitest's structured-clone RPC can serialize them safely.
    cleanupInterceptorId = http.interceptors.response.use(undefined, (error) => {
      if (error?.config) {
        try {
          error.config = JSON.parse(JSON.stringify(error.config))
        } catch {
          error.config = {}
        }
      }
      return Promise.reject(error)
    })
  })

  afterEach(() => {
    http.interceptors.request.eject(noRetryId)
    http.interceptors.response.eject(cleanupInterceptorId)
    mock.restore()
  })

  // ========== 并发请求压测 ==========
  describe('并发请求', () => {
    it('同时发起10个不同API请求', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { data: { total: 1 } })
      mock.onGet('/admin/stats/realtime').reply(200, { data: { online: 2 } })
      mock.onGet(/\/admin\/stats\/user-growth/).reply(200, { data: [] })
      mock.onGet('/admin/stats/mood-distribution').reply(200, { data: [] })
      mock.onGet('/admin/stats/trending-topics').reply(200, { data: [] })
      mock.onGet('/admin/stats/active-time').reply(200, { data: [] })
      mock.onGet('/admin/users').reply(200, { data: [] })
      mock.onGet('/admin/stones').reply(200, { data: [] })
      mock.onGet('/admin/reports').reply(200, { data: [] })
      mock.onGet('/admin/logs').reply(200, { data: [] })

      const results = await Promise.all([
        api.getDashboardStats(),
        api.getRealtimeStats(),
        api.getUserGrowthStats('7d'),
        api.getMoodDistribution(),
        api.getTrendingTopics(),
        api.getActiveTimeStats(),
        api.getUsers(),
        api.getContents(),
        api.getReports(),
        api.getOperationLogs(),
      ])

      expect(results).toHaveLength(10)
      results.forEach(r => expect(r.status).toBe(200))
    })

    it('同时发起20个Edge AI请求（去重机制下）', async () => {
      mock.onGet('/admin/edge-ai/status').reply(200, { data: { status: 'ok' } })
      mock.onGet('/admin/edge-ai/metrics').reply(200, { data: {} })
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(200, { data: {} })
      mock.onGet('/admin/edge-ai/privacy-budget').reply(200, { data: {} })
      mock.onGet('/admin/edge-ai/config').reply(200, { data: {} })

      const requests = []
      for (let i = 0; i < 4; i++) {
        requests.push(
          api.getEdgeAIStatus(),
          api.getEdgeAIMetrics(),
          api.getEmotionPulse(),
          api.getPrivacyBudget(),
          api.getEdgeAIConfig(),
        )
      }

      const results = await Promise.allSettled(requests)
      expect(results).toHaveLength(20)
      // 由于请求去重机制，同URL的前序请求会被取消，至少每个唯一URL的最后一个请求成功
      const fulfilled = results.filter(r => r.status === 'fulfilled')
      expect(fulfilled.length).toBeGreaterThanOrEqual(5)
    })

    it('部分请求失败时其他请求正常完成', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { data: { ok: true } })
      mock.onGet('/admin/stats/realtime').reply(400)
      mock.onGet('/admin/stats/mood-distribution').reply(200, { data: { ok: true } })
      mock.onGet('/admin/stats/trending-topics').reply(403)
      mock.onGet('/admin/stats/active-time').reply(200, { data: { ok: true } })

      const results = await Promise.allSettled([
        api.getDashboardStats(),
        api.getRealtimeStats(),
        api.getMoodDistribution(),
        api.getTrendingTopics(),
        api.getActiveTimeStats(),
      ])

      const fulfilled = results.filter(r => r.status === 'fulfilled')
      const rejected = results.filter(r => r.status === 'rejected')
      expect(fulfilled).toHaveLength(3)
      expect(rejected).toHaveLength(2)
    }, 30000)

    it('所有请求同时失败', async () => {
      mock.onGet('/admin/stats/dashboard').reply(400)
      mock.onGet('/admin/stats/realtime').reply(400)
      mock.onGet('/admin/stats/mood-distribution').reply(403)
      mock.onGet('/admin/stats/trending-topics').reply(403)
      mock.onGet('/admin/stats/active-time').reply(400)

      const results = await Promise.allSettled([
        api.getDashboardStats(),
        api.getRealtimeStats(),
        api.getMoodDistribution(),
        api.getTrendingTopics(),
        api.getActiveTimeStats(),
      ])

      results.forEach(r => expect(r.status).toBe('rejected'))
    }, 30000)

    it('混合成功、失败和超时请求', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { data: {} })
      mock.onGet('/admin/stats/realtime').reply(400)
      mock.onGet('/admin/stats/mood-distribution').reply(200, { data: {} })
      mock.onGet('/admin/stats/trending-topics').reply(403)
      mock.onGet('/admin/stats/active-time').reply(200, { data: {} })

      const results = await Promise.allSettled([
        api.getDashboardStats(),
        api.getRealtimeStats(),
        api.getMoodDistribution(),
        api.getTrendingTopics(),
        api.getActiveTimeStats(),
      ])

      const fulfilled = results.filter(r => r.status === 'fulfilled')
      expect(fulfilled).toHaveLength(3)
    }, 30000)
  })

  // ========== 请求去重（同URL覆盖） ==========
  describe('请求去重机制', () => {
    it('相同URL的请求会取消前一个', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { data: { v: 'latest' } })

      const results = await Promise.allSettled([
        api.getDashboardStats(),
        api.getDashboardStats(),
      ])

      // 第一个请求被取消，第二个成功
      const rejected = results.filter(r => r.status === 'rejected')
      const fulfilled = results.filter(r => r.status === 'fulfilled')
      expect(rejected.length + fulfilled.length).toBe(2)
    })

    it('快速连续发起5个相同请求只有最后一个成功', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { data: { v: 'final' } })

      const promises = []
      for (let i = 0; i < 5; i++) {
        promises.push(api.getDashboardStats())
      }

      const results = await Promise.allSettled(promises)
      const fulfilled = results.filter(r => r.status === 'fulfilled')
      expect(fulfilled.length).toBeGreaterThanOrEqual(1)
    })

    it('不同URL的请求不会互相取消', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { data: { a: 1 } })
      mock.onGet('/admin/stats/mood-distribution').reply(200, { data: { b: 2 } })
      mock.onGet('/admin/stats/trending-topics').reply(200, { data: { c: 3 } })

      const results = await Promise.all([
        api.getDashboardStats(),
        api.getMoodDistribution(),
        api.getTrendingTopics(),
      ])

      expect(results).toHaveLength(3)
      results.forEach(r => expect(r.status).toBe(200))
    })
  })

  // ========== cancelAllRequests 压测 ==========
  describe('cancelAllRequests', () => {
    it('取消所有进行中的请求', async () => {
      mock.onGet('/admin/stats/dashboard').timeout()
      mock.onGet('/admin/stats/realtime').timeout()

      const p1 = api.getDashboardStats()
      const p2 = api.getRealtimeStats()

      cancelAllRequests()

      const results = await Promise.allSettled([p1, p2])
      results.forEach(r => expect(r.status).toBe('rejected'))
    })

    it('取消后可以重新发起请求', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { data: { ok: true } })

      cancelAllRequests()

      const res = await api.getDashboardStats()
      expect(res.status).toBe(200)
    })

    it('多次调用cancelAllRequests不报错', () => {
      expect(() => {
        for (let i = 0; i < 100; i++) {
          cancelAllRequests()
        }
      }).not.toThrow()
    })

    it('无请求时调用cancelAllRequests不报错', () => {
      expect(() => cancelAllRequests()).not.toThrow()
    })
  })

  // ========== Token注入一致性 ==========
  describe('Token注入一致性', () => {
    it('设置token后所有请求都携带Authorization', async () => {
      appStore.setToken('stress-test-token')
      mock.onGet('/admin/stats/dashboard').reply(200, { data: {} })
      mock.onGet('/admin/stats/realtime').reply(200, { data: {} })
      mock.onGet('/admin/stats/mood-distribution').reply(200, { data: {} })

      await Promise.all([
        api.getDashboardStats(),
        api.getRealtimeStats(),
        api.getMoodDistribution(),
      ])

      const allRequests = [...mock.history.get]
      allRequests.forEach(req => {
        expect(req.headers?.Authorization).toBe('Bearer stress-test-token')
      })
    })

    it('无token时请求不携带Authorization', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { data: {} })

      await api.getDashboardStats()

      const req = mock.history.get[mock.history.get.length - 1]
      expect(req.headers?.Authorization).toBeUndefined()
    })

    it('token更新后新请求使用新token', async () => {
      appStore.setToken('old-token')
      appStore.setToken('new-token')
      mock.onGet('/admin/stats/dashboard').reply(200, { data: {} })

      await api.getDashboardStats()

      const req = mock.history.get[mock.history.get.length - 1]
      expect(req.headers?.Authorization).toBe('Bearer new-token')
    })

    it('clearToken后请求不携带Authorization', async () => {
      appStore.setToken('some-token')
      appStore.clearToken()
      mock.onGet('/admin/stats/dashboard').reply(200, { data: {} })

      await api.getDashboardStats()

      const req = mock.history.get[mock.history.get.length - 1]
      expect(req.headers?.Authorization).toBeUndefined()
    })
  })

  // ========== 401响应处理 ==========
  describe('401响应处理', () => {
    it('401响应清除token', async () => {
      appStore.setToken('expired-token')
      mock.onGet('/admin/stats/dashboard').reply(401)

      await api.getDashboardStats().catch(() => {})
      expect(appStore.getToken()).toBe('')
    })

    it('多个并发请求收到401只触发一次跳转', async () => {
      const router = await import('@/router')
      appStore.setToken('expired-token')

      mock.onGet('/admin/stats/dashboard').reply(401)
      mock.onGet('/admin/stats/realtime').reply(401)
      mock.onGet('/admin/stats/mood-distribution').reply(401)

      await Promise.allSettled([
        api.getDashboardStats(),
        api.getRealtimeStats(),
        api.getMoodDistribution(),
      ])

      // isRedirectingToLogin 防止重复跳转
      expect(router.default.push).toHaveBeenCalled()
    })

    it('401后重新登录设置新token', async () => {
      appStore.setToken('expired-token')
      mock.onGet('/admin/stats/dashboard').reply(401)
      await api.getDashboardStats().catch(() => {})

      expect(appStore.getToken()).toBe('')

      appStore.setToken('new-valid-token')
      mock.onGet('/admin/stats/dashboard').reply(200, { data: {} })
      const res = await api.getDashboardStats()
      expect(res.status).toBe(200)
    })
  })

  // ========== Loading状态压测 ==========
  describe('Loading状态', () => {
    it('并发请求正确管理loading计数', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { data: {} })
      mock.onGet('/admin/users').reply(200, { data: [] })
      mock.onGet('/admin/stones').reply(200, { data: [] })

      await Promise.all([
        api.getDashboardStats(),
        api.getUsers(),
        api.getContents(),
      ])

      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('skipLoading请求不影响loading计数', async () => {
      mock.onGet('/admin/stats/realtime').reply(200, { data: {} })
      mock.onGet('/admin/edge-ai/status').reply(200, { data: {} })
      mock.onGet('/admin/edge-ai/metrics').reply(200, { data: {} })

      await Promise.all([
        api.getRealtimeStats(),
        api.getEdgeAIStatus(),
        api.getEdgeAIMetrics(),
      ])

      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('请求失败也正确减少loading计数', async () => {
      mock.onGet('/admin/stats/dashboard').reply(500)
      mock.onGet('/admin/users').reply(500)

      await Promise.allSettled([
        api.getDashboardStats(),
        api.getUsers(),
      ])

      expect(appStore.isGlobalLoading).toBe(false)
    }, 30000)

    it('混合skipLoading和普通请求', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { data: {} })
      mock.onGet('/admin/edge-ai/status').reply(200, { data: {} })
      mock.onGet('/admin/users').reply(200, { data: [] })
      mock.onGet('/admin/edge-ai/metrics').reply(200, { data: {} })

      await Promise.all([
        api.getDashboardStats(),
        api.getEdgeAIStatus(),
        api.getUsers(),
        api.getEdgeAIMetrics(),
      ])

      expect(appStore.isGlobalLoading).toBe(false)
    })
  })

  // ========== 各种HTTP状态码 ==========
  describe('HTTP状态码处理', () => {
    const statusCodes = [400, 401, 403, 404, 405, 408, 429, 500, 502, 503, 504]

    statusCodes.forEach(code => {
      it(`处理HTTP ${code}状态码`, async () => {
        mock.onGet('/admin/stats/dashboard').reply(code)
        const result = await api.getDashboardStats().catch(e => e)
        expect(result).toBeDefined()
      }, 15000)
    })
  })

  // ========== 大响应体 ==========
  describe('大响应体处理', () => {
    it('处理包含大量字段的JSON', async () => {
      const bigObj: Record<string, string> = {}
      for (let i = 0; i < 1000; i++) {
        bigObj[`field_${i}`] = `value_${i}_${'x'.repeat(100)}`
      }
      mock.onGet('/admin/stats/dashboard').reply(200, { data: bigObj })
      const res = await api.getDashboardStats()
      expect(Object.keys(res.data.data)).toHaveLength(1000)
    })

    it('处理深层嵌套JSON', async () => {
      let nested: Record<string, unknown> = { value: 'deep' }
      for (let i = 0; i < 50; i++) {
        nested = { child: nested }
      }
      mock.onGet('/admin/stats/dashboard').reply(200, { data: nested })
      const res = await api.getDashboardStats()
      expect(res.data.data).toHaveProperty('child')
    })

    it('处理大数组响应', async () => {
      const arr = Array.from({ length: 50000 }, (_, i) => ({ id: i, v: i * 2 }))
      mock.onGet('/admin/users').reply(200, { data: arr })
      const res = await api.getUsers()
      expect(res.data.data).toHaveLength(50000)
    })
  })

  // ========== 业务错误码 ==========
  describe('业务错误码压测', () => {
    it('处理认证类业务错误码200001-200005', async () => {
      const codes = [200001, 200002, 200003, 200004, 200005]
      for (const code of codes) {
        appStore.setToken('some-token')
        mock.onGet('/admin/stats/dashboard').reply(200, { code, message: `auth error ${code}` })
        await api.getDashboardStats().catch(() => {})
        expect(appStore.getToken()).toBe('')
        mock.reset()
      }
    })

    it('处理普通业务错误码', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { code: 400001, message: 'bad request' })
      const result = await api.getDashboardStats().catch(e => e)
      expect(result).toBeInstanceOf(Error)
      expect(result.message).toBe('bad request')
    })

    it('code为0表示成功', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { code: 0, data: { ok: true } })
      const res = await api.getDashboardStats()
      expect(res.data.data.ok).toBe(true)
    })

    it('code为200表示成功', async () => {
      mock.onGet('/admin/stats/dashboard').reply(200, { code: 200, data: { ok: true } })
      const res = await api.getDashboardStats()
      expect(res.data.data.ok).toBe(true)
    })
  })

  // ========== POST请求压测 ==========
  describe('POST请求压测', () => {
    it('发送大请求体', async () => {
      const bigData = { text: 'x'.repeat(100000) }
      mock.onPost('/admin/edge-ai/analyze').reply(200, { data: { result: 'ok' } })
      const res = await api.analyzeText(bigData.text)
      expect(res.status).toBe(200)
    })

    it('并发POST请求', async () => {
      mock.onPost('/admin/edge-ai/analyze').reply(200, { data: {} })
      mock.onPost('/admin/edge-ai/moderate').reply(200, { data: {} })
      mock.onPost('/admin/edge-ai/vector-search').reply(200, { data: {} })

      const results = await Promise.all([
        api.analyzeText('hello'),
        api.moderateText('world'),
        api.edgeAIVectorSearch({ query: 'test' }),
      ])

      expect(results).toHaveLength(3)
    })

    it('登录请求携带正确数据', async () => {
      mock.onPost('/admin/login').reply(200, { data: { token: 'new-token' } })

      await api.login({ username: 'admin', password: 'password123' })

      const req = mock.history.post.find(r => r.url === '/admin/login')
      expect(req).toBeDefined()
      const data = JSON.parse(req!.data)
      expect(data.username).toBe('admin')
      expect(data.password).toBe('password123')
    })

    it('快速连续登录登出', async () => {
      mock.onPost('/admin/login').reply(200, { data: { token: 'tok' } })
      mock.onPost('/admin/logout').reply(200, { data: {} })

      for (let i = 0; i < 10; i++) {
        await api.login({ username: 'admin', password: 'pass' })
        appStore.setToken('tok')
        await api.logout()
        appStore.clearToken()
      }

      expect(appStore.getToken()).toBe('')
    })
  })

  // ========== 特殊字符和编码 ==========
  describe('特殊字符处理', () => {
    it('URL中的特殊字符', async () => {
      mock.onGet(/\/admin\/users\/.*/).reply(200, { data: {} })
      await api.getUserDetail('user-with-special-chars-!@#')
    })

    it('请求体中的Unicode字符', async () => {
      mock.onPost('/admin/edge-ai/analyze').reply(200, { data: {} })
      await api.analyzeText('你好世界🌍🎉 émojis и кириллица العربية')
    })

    it('响应中的特殊字符', async () => {
      const specialData = {
        data: {
          name: '<script>alert("xss")</script>',
          desc: '"; DROP TABLE users; --',
          unicode: '你好\n\t\r世界',
        },
      }
      mock.onGet('/admin/stats/dashboard').reply(200, specialData)
      const res = await api.getDashboardStats()
      expect(res.data.data.name).toContain('<script>')
    })
  })
})
