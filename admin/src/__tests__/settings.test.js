/**
 * @file settings.test.js
 * @brief Settings 相关 API 调用测试
 */
import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import MockAdapter from 'axios-mock-adapter'
import { setActivePinia, createPinia } from 'pinia'
import api, { http } from '@/api'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), warning: vi.fn(), success: vi.fn() },
  ElMessageBox: { confirm: vi.fn().mockResolvedValue('confirm') },
}))

vi.mock('@/router', () => ({
  default: {
    push: vi.fn().mockResolvedValue(undefined),
    currentRoute: { value: { path: '/settings' } },
  },
}))

const configData = {
  data: {
    system: { name: '心湖', description: '测试描述', allow_register: true, allow_anonymous: false },
    ai: { provider: 'deepseek', api_key: 'sk-test', base_url: 'https://api.deepseek.com', model: 'deepseek-chat' },
    rate: { stone_per_hour: 15, boat_per_hour: 50, message_per_minute: 60, max_content_length: 2000 },
  },
}

describe('Settings API', () => {
  let mock

  beforeEach(() => {
    localStorage.clear()
    vi.clearAllMocks()
    setActivePinia(createPinia())
    mock = new MockAdapter(http)
  })

  afterEach(() => { mock.restore() })

  describe('获取系统配置', () => {
    it('应正确调用 config 接口', async () => {
      mock.onGet('/admin/config').reply(200, configData)
      const res = await api.getSystemConfig()
      expect(res.data.data.system.name).toBe('心湖')
      expect(res.data.data.rate.stone_per_hour).toBe(15)
    })

    it('应返回完整的三组配置', async () => {
      mock.onGet('/admin/config').reply(200, configData)
      const res = await api.getSystemConfig()
      const data = res.data.data
      expect(data).toHaveProperty('system')
      expect(data).toHaveProperty('ai')
      expect(data).toHaveProperty('rate')
    })

    it('接口错误应抛出异常', async () => {
      mock.onGet('/admin/config').reply(500)
      await expect(api.getSystemConfig()).rejects.toThrow()
    })
  })

  describe('更新系统配置', () => {
    it('应正确调用 PUT 接口', async () => {
      mock.onPut('/admin/config').reply(200, { code: 0 })
      await api.updateSystemConfig({ system: { name: '新名称' } })
      expect(mock.history.put[0].url).toBe('/admin/config')
    })

    it('应传递 snake_case 格式的配置数据', async () => {
      mock.onPut('/admin/config').reply(200, { code: 0 })
      const payload = {
        rate: {
          stone_per_hour: 20,
          boat_per_hour: 30,
          message_per_minute: 120,
          max_content_length: 3000,
        },
      }
      await api.updateSystemConfig(payload)
      const body = JSON.parse(mock.history.put[0].data)
      expect(body.rate.stone_per_hour).toBe(20)
      expect(body.rate.boat_per_hour).toBe(30)
    })

    it('保存 AI 配置应包含所有字段', async () => {
      mock.onPut('/admin/config').reply(200, { code: 0 })
      const payload = {
        ai: {
          provider: 'openai',
          api_key: 'sk-new',
          base_url: 'https://api.openai.com',
          model: 'gpt-4',
          enable_sentiment: false,
          enable_auto_reply: false,
        },
      }
      await api.updateSystemConfig(payload)
      const body = JSON.parse(mock.history.put[0].data)
      expect(body.ai.provider).toBe('openai')
      expect(body.ai.api_key).toBe('sk-new')
    })

    it('更新失败应抛出异常', async () => {
      mock.onPut('/admin/config').reply(500)
      await expect(api.updateSystemConfig({})).rejects.toThrow()
    })
  })

  describe('测试AI连接', () => {
    it('应调用 test-ai 接口', async () => {
      mock.onPost('/admin/config/test-ai').reply(200, { code: 0 })
      await api.testAIConnection()
      expect(mock.history.post[0].url).toBe('/admin/config/test-ai')
    })

    it('连接失败应抛出异常', async () => {
      mock.onPost('/admin/config/test-ai').reply(500)
      await expect(api.testAIConnection()).rejects.toThrow()
    })
  })

  describe('广播消息', () => {
    it('应正确调用广播接口', async () => {
      mock.onPost('/admin/broadcast').reply(200, { code: 0 })
      await api.broadcastMessage({ message: '测试广播', level: 'info' })
      expect(mock.history.post[0].url).toBe('/admin/broadcast')
      const body = JSON.parse(mock.history.post[0].data)
      expect(body.message).toBe('测试广播')
      expect(body.level).toBe('info')
    })

    it('支持不同级别', async () => {
      mock.onPost('/admin/broadcast').reply(200, { code: 0 })
      for (const level of ['info', 'success', 'warning', 'error']) {
        mock.resetHistory()
        await api.broadcastMessage({ message: '测试', level })
        const body = JSON.parse(mock.history.post[0].data)
        expect(body.level).toBe(level)
      }
    })

    it('广播失败应抛出异常', async () => {
      mock.onPost('/admin/broadcast').reply(500)
      await expect(api.broadcastMessage({ message: '测试', level: 'info' })).rejects.toThrow()
    })
  })
})
