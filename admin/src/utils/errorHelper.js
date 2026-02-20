/**
 * @file errorHelper.js
 * @brief 统一错误处理工具函数
 */

// 生产环境标志
const isProd = import.meta.env.PROD

/**
 * 获取用户友好的错误提示信息
 * 生产环境隐藏服务端详细错误，开发环境保留完整信息
 * @param {Error|Object} error - 错误对象
 * @param {string} fallback - 默认提示文案
 * @returns {string}
 */
export function getErrorMessage(error, fallback = '操作失败，请稍后重试') {
  if (isProd) {
    // 生产环境：仅展示通用提示，不暴露服务端细节
    const status = error?.response?.status
    if (status === 400) return '请求参数有误'
    if (status === 401) return '登录已过期，请重新登录'
    if (status === 403) return '没有权限执行此操作'
    if (status === 404) return '请求的资源不存在'
    if (status === 429) return '操作过于频繁，请稍后再试'
    if (status >= 500) return '服务器繁忙，请稍后重试'
    return fallback
  }
  // 开发环境：展示详细错误信息便于调试
  return error?.response?.data?.message || error?.message || fallback
}
