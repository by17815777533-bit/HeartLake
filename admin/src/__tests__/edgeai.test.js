/**
 * @file edgeai.test.js
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
      { id: 'stone-101', score: 0.95, content: '测试内容1' },
      { id: 'stone-202', score: 0.87, content: '测试内容2' },
    ],
    total: 2,
    latency_ms: 8,
  },
}

const aggregationResult = {
  data: {
    round: 6,
    participants: 5,
    accuracy_delta: 0.02,
    status: 'completed',
  },
}

describe('EdgeAI API', () => {
  let mock

  beforeEach(() => {
    localStorage.clear()
    vi.clearAllMocks()
    setActivePinia(createPinia())
    mock = new MockAdapter(http)
  })

  afterEach(() => { mock.restore() })

  describe('引擎状态', () => {
    it('应正确获取引擎状态', async () => {
      mock.onGet('/admin/edge-ai/status').reply(200, statusData)
      const res = await api.getEdgeAIStatus()
      expect(res.data.data.engine_version).toBe('1.0.0')
      expect(res.data.data.nodes).toHaveLength(2)
    })

    it('应包含子系统状态', async () => {
      mock.onGet('/admin/edge-ai/status').reply(200, statusData)
      const res = await api.getEdgeAIStatus()
      const subs = res.data.data.subsystems
      expect(subs.emotion.status).toBe('running')
      expect(subs.federated.status).toBe('idle')
    })

    it('接口错误应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/status').reply(500)
      await expect(api.getEdgeAIStatus()).rejects.toThrow()
    })
  })

  describe('性能指标', () => {
    it('应正确获取性能指标', async () => {
      mock.onGet('/admin/edge-ai/metrics').reply(200, metricsData)
      const res = await api.getEdgeAIMetrics()
      expect(res.data.data.inference_count).toBe(12345)
      expect(res.data.data.avg_latency_ms).toBe(15.3)
      expect(res.data.data.throughput_qps).toBe(42.5)
    })

    it('接口错误应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/metrics').reply(500)
      await expect(api.getEdgeAIMetrics()).rejects.toThrow()
    })
  })

  describe('情绪脉搏', () => {
    it('应正确获取脉搏数据', async () => {
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(200, pulseData)
      const res = await api.getEmotionPulse()
      expect(res.data.data.avgScore).toBe(0.65)
      expect(res.data.data.trend).toBe('stable')
      expect(res.data.data.sampleCount).toBe(50)
    })

    it('接口错误应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/emotion-pulse').reply(500)
      await expect(api.getEmotionPulse()).rejects.toThrow()
    })
  })

  describe('隐私预算', () => {
    it('应正确获取隐私预算', async () => {
      mock.onGet('/admin/edge-ai/privacy-budget').reply(200, privacyBudgetData)
      const res = await api.getPrivacyBudget()
      expect(res.data.data.epsilon_used).toBe(3.5)
      expect(res.data.data.epsilon_total).toBe(10.0)
      expect(res.data.data.queries_today).toBe(128)
    })

    it('接口错误应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/privacy-budget').reply(500)
      await expect(api.getPrivacyBudget()).rejects.toThrow()
    })
  })

  describe('联邦聚合', () => {
    it('应正确触发联邦聚合', async () => {
      mock.onPost('/admin/edge-ai/federated/aggregate').reply(200, aggregationResult)
      const payload = { min_clients: 3, target_rounds: 1 }
      const res = await api.triggerFederatedAggregation(payload)
      expect(res.data.data.status).toBe('completed')
      expect(res.data.data.participants).toBe(5)
    })

    it('应传递聚合参数', async () => {
      mock.onPost('/admin/edge-ai/federated/aggregate').reply(200, aggregationResult)
      const payload = { min_clients: 5, target_rounds: 3 }
      await api.triggerFederatedAggregation(payload)
      const body = JSON.parse(mock.history.post[0].data)
      expect(body.min_clients).toBe(5)
      expect(body.target_rounds).toBe(3)
    })

    it('聚合失败应抛出异常', async () => {
      mock.onPost('/admin/edge-ai/federated/aggregate').reply(500)
      await expect(api.triggerFederatedAggregation({})).rejects.toThrow()
    })
  })

  describe('向量搜索', () => {
    it('应正确执行向量搜索', async () => {
      mock.onPost('/admin/edge-ai/vector-search').reply(200, vectorSearchResult)
      const payload = { query: '温暖的故事', topK: 10 }
      const res = await api.edgeAIVectorSearch(payload)
      expect(res.data.data.results).toHaveLength(2)
      expect(res.data.data.results[0].score).toBe(0.95)
    })

    it('应传递搜索参数', async () => {
      mock.onPost('/admin/edge-ai/vector-search').reply(200, vectorSearchResult)
      const payload = { query: '测试查询', topK: 5, threshold: 0.8 }
      await api.edgeAIVectorSearch(payload)
      const body = JSON.parse(mock.history.post[0].data)
      expect(body.query).toBe('测试查询')
      expect(body.topK).toBe(5)
    })

    it('搜索失败应抛出异常', async () => {
      mock.onPost('/admin/edge-ai/vector-search').reply(500)
      await expect(api.edgeAIVectorSearch({})).rejects.toThrow()
    })
  })

  describe('EdgeAI 配置', () => {
    it('应正确获取配置', async () => {
      mock.onGet('/admin/edge-ai/config').reply(200, configData)
      const res = await api.getEdgeAIConfig()
      expect(res.data.data.emotion_model).toBe('bert-emotion-v2')
      expect(res.data.data.vector_dim).toBe(128)
      expect(res.data.data.hnsw_m).toBe(16)
    })

    it('应正确更新配置', async () => {
      mock.onPut('/admin/edge-ai/config').reply(200, { code: 0 })
      const payload = {
        emotion_model: 'bert-emotion-v3',
        vector_dim: 256,
        privacy_epsilon: 8.0,
      }
      await api.updateEdgeAIConfig(payload)
      expect(mock.history.put[0].url).toBe('/admin/edge-ai/config')
      const body = JSON.parse(mock.history.put[0].data)
      expect(body.emotion_model).toBe('bert-emotion-v3')
      expect(body.vector_dim).toBe(256)
    })

    it('获取配置失败应抛出异常', async () => {
      mock.onGet('/admin/edge-ai/config').reply(500)
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
      results.forEach(r => expect(r.status).toBe(200))
    })
  })
})
