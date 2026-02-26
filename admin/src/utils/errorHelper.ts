/**
 * @file errorHelper.ts
 * @brief 统一错误处理工具函数
 * 与后端 ErrorCode.h / ErrorCode.cpp 保持一致
 */

import type { BusinessError, SensitiveCodeRange } from '@/types'

// 生产环境标志
const isProd: boolean = import.meta.env.PROD

/**
 * 业务错误码 → 用户友好中文提示
 * 与后端 ErrorCode enum (ErrorCode.h) 及 messageZhMap (ErrorCode.cpp) 一一对应
 */
const businessCodeMap: Record<number, string> = {
  // 通用错误 1xxxxx
  100001: '无效的请求',
  100002: '参数无效',
  100003: '缺少必需参数',
  100500: '服务器内部错误',
  100503: '服务暂时不可用',
  // 认证错误 2xxxxx
  200001: '未授权',
  200002: '登录已过期，请重新登录',
  200003: '无效的登录凭证',
  200004: '权限不足',
  200005: '请先登录',
  // 用户错误 3xxxxx
  300001: '用户不存在',
  300002: '用户已存在',
  300003: '邮箱已被注册',
  300004: '邮箱格式不正确',
  300005: '密码不正确',
  300006: '密码强度不够',
  300007: '验证码错误',
  300008: '验证码已过期',
  300009: '邮箱未验证',
  300010: '账户已被停用',
  300011: '账户已删除',
  // 内容错误 4xxxxx
  400001: '内容不存在',
  400002: '内容已被删除',
  400003: '内容过长',
  400004: '内容不能为空',
  400005: '内容包含敏感信息',
  400006: '内容审核未通过',
  400007: '操作过于频繁，请稍后再试',
  400008: '内容重复',
  // 好友错误 41xxxx
  410001: '好友不存在',
  410002: '已经是好友了',
  410003: '好友请求已存在',
  410004: '不能添加自己为好友',
  410005: '好友数量已达上限',
  // AI 错误 5xxxxx
  500001: 'AI服务暂时不可用',
  500002: 'AI服务响应异常',
  500003: 'AI服务响应超时',
  500004: 'AI服务配额已用完',
  500005: 'AI服务返回无效',
  // 数据库错误 6xxxxx
  600001: '数据库错误',
  600002: '数据库连接失败',
  600003: '数据已存在',
  600004: '数据关联约束冲突',
  // 网络错误 7xxxxx
  700001: '网络错误',
  700002: '请求超时',
  700003: '上游服务错误',
}

// 生产环境对服务端内部错误码做脱敏，只返回通用提示
const sensitiveCodeRanges: SensitiveCodeRange[] = [
  { min: 600001, max: 600004, msg: '服务器繁忙，请稍后重试' },
  { min: 700001, max: 700003, msg: '服务器繁忙，请稍后重试' },
]

/**
 * 根据业务错误码获取用户友好提示
 * @param code - 后端业务错误码
 * @returns 匹配到的提示，未匹配返回 undefined
 */
export function getBusinessMessage(code: number): string | undefined {
  if (code == null || code === 0 || code === 200) return undefined
  if (isProd) {
    for (const range of sensitiveCodeRanges) {
      if (code >= range.min && code <= range.max) return range.msg
    }
  }
  return businessCodeMap[code]
}

/**
 * 获取用户友好的错误提示信息
 * 优先使用业务错误码，其次 HTTP 状态码，最后 fallback
 * @param error - 错误对象（可携带 _businessCode 属性）
 * @param fallback - 默认提示文案
 * @returns 用户友好的错误提示
 */
export function getErrorMessage(error: BusinessError, fallback = '操作失败，请稍后重试'): string {
  // 1. 优先检查业务错误码
  const bizCode = error?._businessCode
  if (bizCode) {
    const bizMsg = getBusinessMessage(bizCode)
    if (bizMsg) return bizMsg
  }

  if (isProd) {
    // 2. 生产环境：按 HTTP 状态码返回通用提示
    const status = error?.response?.status
    if (status === 400) return '请求参数有误'
    if (status === 401) return '登录已过期，请重新登录'
    if (status === 403) return '没有权限执行此操作'
    if (status === 404) return '请求的资源不存在'
    if (status === 429) return '操作过于频繁，请稍后再试'
    if (status != null && status >= 500) return '服务器繁忙，请稍后重试'
    return fallback
  }
  // 开发环境下有意返回原始错误详情（含 response.data.message 和 error.message），
  // 便于开发者快速定位问题根因，生产环境已在上方做了脱敏处理
  return error?.response?.data?.message || error?.message || fallback
}
