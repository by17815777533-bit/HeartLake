/**
 * @file edgeai.test.ts
 * @brief EdgeAI 相关 API 调用测试
 */
import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import MockAdapter from 'axios-mock-adapter'
import { setActivePinia, createPinia } from 'pinia'
import api, { http } from '@/api'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), warning: vi.fn(), success: vi.fn() },
}))

vi.mock('@/router', () => ({
  default: {
    push: vi.fn().mockResolvedValue(undefined),
    currentRoute: { value: { path: '/edge-ai' } },
  },
}))

vi.mock('@/utils/errorHelper', () => ({
  getBusinessMessage: vi.fn(() => ''),
}))

const statusData = {
  data: {
    engine_version: '1.0.0',
    uptime_seconds: 86400,
    subsystems: {
      emotion: { status: 'running', latency_ms: 12 },
      vector: { status: 'running', latency_ms: 8 },
      federated: { status: 'idle', latency_ms: 0 },
      privacy: { status: 'running', latency_ms: 5 },
    },
    nodes: [
      { node_id: 'node-1', status: 'online', last_heartbeat: '2025-01-01T00:00:00Z' },
      { node_id: 'node-2', status: 'offline', last_heartbeat: '2025-01-01T00:00:00Z' },
    ],
  },
}

const metricsData = {
  data: {
    inference_count: 12345,
    avg_latency_ms: 15.3,
    error_rate: 0.002,
    throughput_qps: 42.5,
    memory_usage_mb: 256,
  },
}

const pulseData = {
  data: {
    avgScore: 0.65,
    stddev: 0.12,
    trend: 'stable',
    sampleCount: 50,
    windowSeconds: 300,
  },
}

const privacyBudgetData = {
  data: {
    epsilon_used: 3.5,
    epsilon_total: 10.0,
    delta: 1e-5,
    queries_today: 128,
    budget_reset_at: '2025-01-02T00:00:00Z',
  },
}

const configData = {
  data: {
    emotion_model: 'bert-emotion-v2',
    vector_dim: 128,
    hnsw_m: 16,
    hnsw_ef: 200,
    privacy_epsilon: 10.0,
    federated_rounds: 5,
    federated_min_clients: 3,
  },
}

const vectorSearchResult = {
  data: {
    results: [
      { id: 'vec-1', score: 0.95, content: '相似内容1' },
      { id: 'vec-2', score: 0.87, content: '相似内容2' },
    ],
  },
}

const analyzeResult = {
  data: {
    sentiment: 'positive',
    confidence: 0.89,
    emotions: { joy: 0.7, sadness: 0.1, anger: 0.05 },
  },
}

const moderateResult = {
  data: {
    safe: false,
    reason: '包含敏感词',
    score: 0.92,
    matched_words: ['敏感词1'],
  },
}

describe('EdgeAI API', () => {
  let mock: MockAdapter
  let noRetryId: number
  let cleanupId: number

  beforeEach(() => {
    localStorage.clear()
    sessionStorage.clear()
    vi.clearAllMocks()
    setActivePinia(createPinia())
    mock = new MockAdapter(http)
    noRetryId = http.interceptors.request.use((config) => {
      ;(config as any)._retryCount = Infinity
      return config
    })
    cleanupId = http.interceptors.response.use(undefined, (error) => {
      if (error?.config) {
        try { error.config = JSON.parse(JSON.stringify(error.config)) }
        catch { error.config = {} }
      }
      return Promise.reject(error)
    })
  })

  afterEach(() => {
    http.interceptors.request.eject(noRetryId)
    http.interceptors.response.eject(cleanupId)
    mock.restore()
  })

  describe('状态与指标', () => {
    it('应正确获取 EdgeAI 状态', async () => {
      mock.onGet('/admin/edge-ai/status').reply(200, statusData)
      const res = await api.getEdgeAIStatus()
      expect(res.data.data.engine_version).toBe('1.0.0')
      expect(res.data.data.subsystems.emotion.status).toBe('running')
    })

    it('应包含节点信息', async () => {
      mock.onGet('/admin/edge-ai/status').reply(200, statusData)
      const res = await api.getEdgeAIStatus()
      expect(res.data.data.nodes).toHaveLength(2)
      expect(res.data.data.nodes[0].status).toBe('online')
    })

    it('应正确获取性能指标', async () => {
      mock.onGet('/admin/edge-ai/metrics').reply(200, metricsData)
      const res = await api.getEdgeAIMetrics()
      expect(res.data.data.inference_count).toBe(12345)
      expect(res.data.data.avg_latency_ms).toBe(15.3)
    })

    it('状态接口错误应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/status').reply(400)
      await expect(api.getEdgeAIStatus()).rejects.toThrow()
    })

    it('指标接口错误应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/metrics').reply(400)
      await expect(api.getEdgeAIMetrics()).rejects.toThrow()
    })
  })

  describe('情绪脉搏', () => {
    it('应正确获取脉搏数据', async () => {
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(200, pulseData)
      const res = await api.getEmotionPulse()
      expect(res.data.data.avgScore).toBe(0.65)
      expect(res.data.data.trend).toBe('stable')
    })

    it('脉搏接口错误应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(400)
      await expect(api.getEmotionPulse()).rejects.toThrow()
    })
  })

  describe('隐私预算', () => {
    it('应正确获取隐私预算', async () => {
      mock.onGet('/admin/edge-ai/privacy-budget').reply(200, privacyBudgetData)
      const res = await api.getPrivacyBudget()
      expect(res.data.data.epsilon_used).toBe(3.5)
      expect(res.data.data.epsilon_total).toBe(10.0)
    })

    it('隐私预算接口错误应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/privacy-budget').reply(400)
      await expect(api.getPrivacyBudget()).rejects.toThrow()
    })
  })

  describe('向量搜索', () => {
    it('应正确执行向量搜索', async () => {
      mock.onPost('/admin/edge-ai/vector-search').reply(200, vectorSearchResult)
      const res = await api.edgeAIVectorSearch({ query: '测试', top_k: 5 })
      expect(res.data.data.results).toHaveLength(2)
      expect(res.data.data.results[0].score).toBe(0.95)
    })

    it('向量搜索失败应抛出异常', async () => {
      mock.onPost('/admin/edge-ai/vector-search').reply(500)
      await expect(api.edgeAIVectorSearch({ query: '测试' })).rejects.toThrow()
    })
  })

  describe('AI 分析', () => {
    it('应正确分析文本情感', async () => {
      mock.onPost('/admin/edge-ai/analyze').reply(200, analyzeResult)
      const res = await api.analyzeText('我今天很开心')
      expect(res.data.data.sentiment).toBe('positive')
      expect(res.data.data.confidence).toBe(0.89)
    })

    it('应正确审核文本内容', async () => {
      mock.onPost('/admin/edge-ai/moderate').reply(200, moderateResult)
      const res = await api.moderateText('包含敏感词1的内容')
      expect(res.data.data.safe).toBe(false)
      expect(res.data.data.matched_words).toContain('敏感词1')
    })
  })

  describe('配置管理', () => {
    it('应正确获取 EdgeAI 配置', async () => {
      mock.onGet('/admin/edge-ai/config').reply(200, configData)
      const res = await api.getEdgeAIConfig()
      expect(res.data.data.emotion_model).toBe('bert-emotion-v2')
      expect(res.data.data.vector_dim).toBe(128)
    })

    it('应正确更新 EdgeAI 配置', async () => {
      const payload = { emotion_model: 'bert-emotion-v3', vector_dim: 256 }
      mock.onPut('/admin/edge-ai/config').reply(200, { code: 0 })
      await api.updateEdgeAIConfig(payload)
      expect(mock.history.put[0].url).toBe('/admin/edge-ai/config')
      const body = JSON.parse(mock.history.put[0].data as string)
      expect(body.emotion_model).toBe('bert-emotion-v3')
      expect(body.vector_dim).toBe(256)
    })

    it('获取配置失败应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/config').reply(400)
      await expect(api.getEdgeAIConfig()).rejects.toThrow()
    })

    it('更新配置失败应抛出异常', async () => {
      mock.onPut('/admin/edge-ai/config').reply(500)
      await expect(api.updateEdgeAIConfig({})).rejects.toThrow()
    })
  })

  describe('数据完整性', () => {
    it('所有 EdgeAI 接口应能并发调用', async () => {
      mock.onGet('/admin/edge-ai/status').reply(200, statusData)
      mock.onGet('/admin/edge-ai/metrics').reply(200, metricsData)
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(200, pulseData)
      mock.onGet('/admin/edge-ai/privacy-budget').reply(200, privacyBudgetData)
      mock.onGet('/admin/edge-ai/config').reply(200, configData)

      const results = await Promise.all([
        api.getEdgeAIStatus(),
        api.getEdgeAIMetrics(),
        api.getEmotionPulse(),
        api.getPrivacyBudget(),
        api.getEdgeAIConfig(),
      ])

      expect(results).toHaveLength(5)
      results.forEach((r) => expect(r.status).toBe(200))
    })
  })
})
