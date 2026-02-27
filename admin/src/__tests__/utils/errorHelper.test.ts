/**
 * errorHelper 工具函数测试
 */
import { describe, it, expect, vi, beforeEach } from 'vitest'

// 默认测试开发环境 (PROD = false)
describe('errorHelper (dev mode)', () => {
  let getBusinessMessage: typeof import('@/utils/errorHelper').getBusinessMessage
  let getErrorMessage: typeof import('@/utils/errorHelper').getErrorMessage

  beforeEach(async () => {
    vi.resetModules()
    vi.stubEnv('PROD', false)
    const mod = await import('@/utils/errorHelper')
    getBusinessMessage = mod.getBusinessMessage
    getErrorMessage = mod.getErrorMessage
  })

  describe('getBusinessMessage', () => {
    it('code 为 0 返回 undefined', () => {
      expect(getBusinessMessage(0)).toBeUndefined()
    })

    it('code 为 200 返回 undefined', () => {
      expect(getBusinessMessage(200)).toBeUndefined()
    })

    it('code 为 null 返回 undefined', () => {
      expect(getBusinessMessage(null as any)).toBeUndefined()
    })

    it('通用错误 100001 返回正确消息', () => {
      expect(getBusinessMessage(100001)).toBe('无效的请求')
    })

    it('参数无效 100002', () => {
      expect(getBusinessMessage(100002)).toBe('参数无效')
    })

    it('缺少必需参数 100003', () => {
      expect(getBusinessMessage(100003)).toBe('缺少必需参数')
    })

    it('服务器内部错误 100500', () => {
      expect(getBusinessMessage(100500)).toBe('服务器内部错误')
    })

    it('认证错误 200001', () => {
      expect(getBusinessMessage(200001)).toBe('未授权')
    })

    it('登录过期 200002', () => {
      expect(getBusinessMessage(200002)).toBe('登录已过期，请重新登录')
    })

    it('无效凭证 200003', () => {
      expect(getBusinessMessage(200003)).toBe('无效的登录凭证')
    })

    it('权限不足 200004', () => {
      expect(getBusinessMessage(200004)).toBe('权限不足')
    })

    it('用户不存在 300001', () => {
      expect(getBusinessMessage(300001)).toBe('用户不存在')
    })

    it('内容不存在 400001', () => {
      expect(getBusinessMessage(400001)).toBe('内容不存在')
    })

    it('AI服务不可用 500001', () => {
      expect(getBusinessMessage(500001)).toBe('AI服务暂时不可用')
    })

    it('数据库错误 600001 (dev 模式直接返回)', () => {
      expect(getBusinessMessage(600001)).toBe('数据库错误')
    })

    it('网络错误 700001 (dev 模式直接返回)', () => {
      expect(getBusinessMessage(700001)).toBe('网络错误')
    })

    it('未知错误码返回 undefined', () => {
      expect(getBusinessMessage(999999)).toBeUndefined()
    })
  })

  describe('getErrorMessage', () => {
    it('有 _businessCode 时返回业务消息', () => {
      const err = Object.assign(new Error('test'), { _businessCode: 200001 })
      expect(getErrorMessage(err)).toBe('未授权')
    })

    it('dev 模式下返回 response.data.message', () => {
      const err = Object.assign(new Error('test'), {
        response: { status: 400, data: { message: '详细错误信息' } },
      })
      expect(getErrorMessage(err)).toBe('详细错误信息')
    })

    it('dev 模式下 fallback 到 error.message', () => {
      const err = new Error('原始错误')
      expect(getErrorMessage(err as any)).toBe('原始错误')
    })

    it('使用自定义 fallback', () => {
      expect(getErrorMessage({} as any, '自定义提示')).toBe('自定义提示')
    })

    it('默认 fallback', () => {
      expect(getErrorMessage({} as any)).toBe('操作失败，请稍后重试')
    })
  })
})
describe('errorHelper (prod mode)', () => {
  let getBusinessMessage: typeof import('@/utils/errorHelper').getBusinessMessage
  let getErrorMessage: typeof import('@/utils/errorHelper').getErrorMessage

  beforeEach(async () => {
    vi.resetModules()
    vi.stubEnv('PROD', true)
    // import.meta.env.PROD 在 vitest 中通过 stubEnv 控制
    // 但 errorHelper 在模块顶层读取 import.meta.env.PROD
    // 需要 resetModules 后重新 import
    const mod = await import('@/utils/errorHelper')
    getBusinessMessage = mod.getBusinessMessage
    getErrorMessage = mod.getErrorMessage
  })

  describe('getBusinessMessage (prod)', () => {
    it('数据库错误码 600001 返回脱敏消息', () => {
      const msg = getBusinessMessage(600001)
      expect(msg).toBe('服务器繁忙，请稍后重试')
    })

    it('数据库错误码 600004 返回脱敏消息', () => {
      expect(getBusinessMessage(600004)).toBe('服务器繁忙，请稍后重试')
    })

    it('网络错误码 700001 返回脱敏消息', () => {
      expect(getBusinessMessage(700001)).toBe('服务器繁忙，请稍后重试')
    })

    it('网络错误码 700003 返回脱敏消息', () => {
      expect(getBusinessMessage(700003)).toBe('服务器繁忙，请稍后重试')
    })

    it('非敏感错误码正常返回', () => {
      expect(getBusinessMessage(200001)).toBe('未授权')
    })
  })

  describe('getErrorMessage (prod)', () => {
    it('HTTP 400 返回通用提示', () => {
      const err = Object.assign(new Error(''), { response: { status: 400 } })
      expect(getErrorMessage(err as any)).toBe('请求参数有误')
    })

    it('HTTP 401 返回登录过期', () => {
      const err = Object.assign(new Error(''), { response: { status: 401 } })
      expect(getErrorMessage(err as any)).toBe('登录已过期，请重新登录')
    })

    it('HTTP 403 返回权限不足', () => {
      const err = Object.assign(new Error(''), { response: { status: 403 } })
      expect(getErrorMessage(err as any)).toBe('没有权限执行此操作')
    })

    it('HTTP 404 返回资源不存在', () => {
      const err = Object.assign(new Error(''), { response: { status: 404 } })
      expect(getErrorMessage(err as any)).toBe('请求的资源不存在')
    })

    it('HTTP 429 返回频率限制', () => {
      const err = Object.assign(new Error(''), { response: { status: 429 } })
      expect(getErrorMessage(err as any)).toBe('操作过于频繁，请稍后再试')
    })

    it('HTTP 500 返回服务器繁忙', () => {
      const err = Object.assign(new Error(''), { response: { status: 500 } })
      expect(getErrorMessage(err as any)).toBe('服务器繁忙，请稍后重试')
    })

    it('HTTP 502 返回服务器繁忙', () => {
      const err = Object.assign(new Error(''), { response: { status: 502 } })
      expect(getErrorMessage(err as any)).toBe('服务器繁忙，请稍后重试')
    })

    it('无 status 时返回 fallback', () => {
      expect(getErrorMessage({} as any)).toBe('操作失败，请稍后重试')
    })

    it('businessCode 优先于 HTTP status', () => {
      const err = Object.assign(new Error(''), {
        _businessCode: 300001,
        response: { status: 500 },
      })
      expect(getErrorMessage(err as any)).toBe('用户不存在')
    })
  })
})