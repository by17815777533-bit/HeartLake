/**
 * @file moderation.test.ts
 * @brief Moderation.vue API 调用测试
 */
import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import MockAdapter from 'axios-mock-adapter'
import { setActivePinia, createPinia } from 'pinia'
import api, { http } from '@/api'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), warning: vi.fn(), success: vi.fn() },
  ElMessageBox: {
    confirm: vi.fn().mockResolvedValue('confirm'),
    prompt: vi.fn().mockResolvedValue({ value: '违规内容' }),
  },
}))

vi.mock('@/router', () => ({
  default: {
    push: vi.fn().mockResolvedValue(undefined),
    currentRoute: { value: { path: '/moderation' } },
  },
}))

interface ModerationListItem {
  moderation_id: number
  content_type?: string
  content: string
  ai_reason?: string
  result?: string
  moderator?: string
}

interface ModerationResponse {
  data: {
    list: ModerationListItem[]
    total: number
  }
}

const pendingData: ModerationResponse = {
  data: {
    list: [
      { moderation_id: 1, content_type: 'stone', content: '测试内容1', ai_reason: 'AI检测' },
      { moderation_id: 2, content_type: 'boat', content: '测试内容2', ai_reason: '敏感词' },
    ],
    total: 2,
  },
}

const historyData: ModerationResponse = {
  data: {
    list: [
      { moderation_id: 10, content: '历史内容', result: 'approved', moderator: 'admin' },
    ],
    total: 1,
  },
}

describe('Moderation API', () => {
  let mock: MockAdapter

  beforeEach(() => {
    localStorage.clear()
    sessionStorage.clear()
    vi.clearAllMocks()
    setActivePinia(createPinia())
    mock = new MockAdapter(http)
  })

  afterEach(() => { mock.restore() })

  describe('获取待审核列表', () => {
    it('应正确调用 pending 接口', async () => {
      mock.onGet('/admin/moderation/pending').reply(200, pendingData)
      const res = await api.getPendingModeration({ page: 1, page_size: 20 })
      expect(res.data.data.list).toHaveLength(2)
      expect(res.data.data.list[0].content_type).toBe('stone')
    })

    it('应传递分页参数', async () => {
      mock.onGet('/admin/moderation/pending').reply(200, pendingData)
      await api.getPendingModeration({ page: 2, page_size: 10 })
      const req = mock.history.get[0]
      expect(req.params.page).toBe(2)
      expect(req.params.page_size).toBe(10)
    })

    it('接口错误应抛出异常', async () => {
      mock.onGet('/admin/moderation/pending').reply(400)
      await expect(api.getPendingModeration({})).rejects.toThrow()
    }, 15000)
  })

  describe('获取审核历史', () => {
    it('应正确调用 history 接口', async () => {
      mock.onGet('/admin/moderation/history').reply(200, historyData)
      const res = await api.getModerationHistory({ page: 1, page_size: 20 })
      expect(res.data.data.list).toHaveLength(1)
      expect(res.data.data.list[0].result).toBe('approved')
    })

    it('应支持筛选参数', async () => {
      mock.onGet('/admin/moderation/history').reply(200, historyData)
      await api.getModerationHistory({ page: 1, page_size: 20, result: 'rejected' })
      const req = mock.history.get[0]
      expect(req.params.result).toBe('rejected')
    })
  })

  describe('审核操作', () => {
    it('通过审核应调用 approve 接口', async () => {
      mock.onPost('/admin/moderation/1/approve').reply(200, { code: 0 })
      await api.approveContent('1')
      expect(mock.history.post[0].url).toBe('/admin/moderation/1/approve')
    })

    it('拒绝审核应调用 reject 接口并传递原因', async () => {
      mock.onPost('/admin/moderation/2/reject').reply(200, { code: 0 })
      await api.rejectContent('2', '违规内容')
      expect(mock.history.post[0].url).toBe('/admin/moderation/2/reject')
      const body = JSON.parse(mock.history.post[0].data as string)
      expect(body.reason).toBe('违规内容')
    })

    it('通过审核失败应抛出异常', async () => {
      mock.onPost('/admin/moderation/99/approve').reply(500)
      await expect(api.approveContent('99')).rejects.toThrow()
    })

    it('拒绝审核失败应抛出异常', async () => {
      mock.onPost('/admin/moderation/99/reject').reply(500)
      await expect(api.rejectContent('99', '原因')).rejects.toThrow()
    })
  })

  describe('分页逻辑', () => {
    it('不同页码应发送不同请求', async () => {
      mock.onGet('/admin/moderation/pending').reply(200, pendingData)
      await api.getPendingModeration({ page: 1, page_size: 20 })
      await api.getPendingModeration({ page: 3, page_size: 10 })
      expect(mock.history.get).toHaveLength(2)
      expect(mock.history.get[1].params.page).toBe(3)
    })
  })
})
