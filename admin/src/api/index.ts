import axios, { type AxiosRequestConfig, type AxiosResponse, type InternalAxiosRequestConfig } from 'axios'
import { ElMessage } from 'element-plus'
import router from '@/router'
import { useAppStore } from '@/stores'
import { getBusinessMessage } from '@/utils/errorHelper'

// 扩展 Axios 类型：支持 skipLoading 自定义配置
interface CustomAxiosRequestConfig extends AxiosRequestConfig {
  skipLoading?: boolean
}

interface CustomInternalConfig extends InternalAxiosRequestConfig {
  skipLoading?: boolean
  _retryCount?: number
}

// 业务错误类型
interface BusinessError extends Error {
  _businessCode?: number
  response?: AxiosResponse
}

export const http = axios.create({
  baseURL: import.meta.env.VITE_API_BASE_URL || '/api',
  timeout: 15000,
  xsrfCookieName: 'csrf_token',
  xsrfHeaderName: 'X-CSRF-Token',
})

// 防止 401 重复跳转的标志
let isRedirectingToLogin = false

/**
 * L-22: 统一认证失败处理 —— 业务码 200001-200005 和 HTTP 401 共用
 * 清除本地 token，显示提示，跳转登录页（防重复跳转）
 */
function handleAuthFailure(msg?: string): void {
  const appStore = useAppStore()
  appStore.clearToken()
  if (!isRedirectingToLogin && router.currentRoute.value.path !== '/login') {
    isRedirectingToLogin = true
    ElMessage.error(msg || '登录已过期，请重新登录')
    router.push('/login').finally(() => {
      isRedirectingToLogin = false
    })
  }
}

// 请求取消机制：基于 AbortController
const pendingRequests = new Map<string, AbortController>()

/**
 * 生成请求唯一标识，用于去重
 * 包含 method + url + 序列化的 params/data，避免同 URL 不同参数的请求被误取消
 */
function getRequestKey(config: CustomInternalConfig): string {
  const params = config.params ? JSON.stringify(config.params, Object.keys(config.params).sort()) : ''
  const data = config.data ? (typeof config.data === 'string' ? config.data : JSON.stringify(config.data)) : ''
  return `${config.method}:${config.url}:${params}:${data}`
}

export function cancelAllRequests(): void {
  pendingRequests.forEach(controller => controller.abort())
  pendingRequests.clear()
}

// 请求拦截器：注入 token + 全局 loading + 请求去重
http.interceptors.request.use((config: CustomInternalConfig) => {
  const appStore = useAppStore()
  const token = appStore.getToken()
  if (token) config.headers.Authorization = `Bearer ${token}`

  // 请求去重：相同 method:url 自动取消前一个
  const key = getRequestKey(config)
  if (pendingRequests.has(key)) {
    pendingRequests.get(key)!.abort()
  }
  const controller = new AbortController()
  config.signal = controller.signal
  pendingRequests.set(key, controller)

  // 全局 loading（可通过 config.skipLoading 跳过）
  if (!config.skipLoading) {
    appStore.startLoading()
  }
  return config
})

// 响应拦截器：统一处理错误 + 全局 loading + 清理 pending
http.interceptors.response.use(
  (response: AxiosResponse) => {
    pendingRequests.delete(getRequestKey(response.config as CustomInternalConfig))
    const appStore = useAppStore()
    if (!(response.config as CustomInternalConfig).skipLoading) {
      appStore.stopLoading()
    }

    // 检查业务错误码：HTTP 200 但 code 非 0/200 表示业务失败
    const { code, message } = response.data ?? {}
    if (code != null && code !== 0 && code !== 200) {
      // 认证类业务错误码 (200001-200005)：清 token 并跳转登录
      if (code >= 200001 && code <= 200005) {
        handleAuthFailure(getBusinessMessage(code) || '登录已过期，请重新登录')
      }
      const bizError: BusinessError = new Error(message || '操作失败')
      bizError._businessCode = code
      bizError.response = response
      return Promise.reject(bizError)
    }

    return response
  },
  (error: unknown) => {
    if (axios.isCancel(error)) return Promise.reject(error)

    const axiosError = error as { config?: CustomInternalConfig; response?: AxiosResponse; message?: string }
    if (axiosError.config) {
      pendingRequests.delete(getRequestKey(axiosError.config))
      const appStore = useAppStore()
      if (!axiosError.config.skipLoading) {
        appStore.stopLoading()
      }
    }

    // 可重试的 HTTP 状态码（网络抖动、服务端临时过载）
    const RETRYABLE_CODES = new Set([408, 429, 500, 502, 503, 504])
    const MAX_RETRIES = 3

    if (
      axiosError.config &&
      axiosError.response &&
      RETRYABLE_CODES.has(axiosError.response.status) &&
      axiosError.config.method?.toUpperCase() === 'GET'
    ) {
      const retryCount = axiosError.config._retryCount ?? 0
      if (retryCount < MAX_RETRIES) {
        axiosError.config._retryCount = retryCount + 1
        const delay = Math.min(1000 * 2 ** retryCount, 8000)
        return new Promise(resolve => setTimeout(resolve, delay)).then(() =>
          http.request(axiosError.config!),
        )
      }
    }

    // HTTP 401：token 过期或无效
    if (axiosError.response?.status === 401) {
      handleAuthFailure()
    } else if (!axios.isCancel(error)) {
      ElMessage.error(axiosError.message || '网络请求失败')
    }

    return Promise.reject(error)
  },
)

// 通用参数类型，用于灵活的查询/请求体
type Params = Record<string, unknown>

// L-15: 针对耗时操作的差异化超时配置（默认 15s 不够用）
const LONG_TIMEOUT = 60000 // 文件上传、数据导出、AI分析等慢操作用 60s

// 登录请求参数
interface LoginPayload {
  username: string
  password: string
}

// 高频 API 具体参数类型
import type {
  HandleReportParams, AddSensitiveWordParams, UpdateSensitiveWordParams,
  BroadcastMessageParams, SaveConfigPayload,
  FederatedAggregationParams, VectorSearchParams,
} from '@/types'

interface BanUserParams {
  reason: string
}

export default {
  // Auth
  login: (data: LoginPayload) => http.post('/admin/login', data),
  logout: () => http.post('/admin/logout'),
  // Dashboard
  getDashboardStats: () => http.get('/admin/dashboard/stats'),
  getRealtimeStats: () => http.get('/admin/realtime-stats', { skipLoading: true } as CustomAxiosRequestConfig),
  getUserGrowthStats: (range: string) => http.get(`/admin/dashboard/user-growth?range=${range}`, { skipLoading: true } as CustomAxiosRequestConfig),
  getMoodDistribution: () => http.get('/admin/dashboard/mood-distribution', { skipLoading: true } as CustomAxiosRequestConfig),
  getMoodTrend: (range: string) => http.get(`/admin/dashboard/mood-trend?range=${range}`, { skipLoading: true } as CustomAxiosRequestConfig),
  getTrendingTopics: () => http.get('/admin/dashboard/trending-topics', { skipLoading: true } as CustomAxiosRequestConfig),
  getActiveTimeStats: () => http.get('/admin/dashboard/active-time', { skipLoading: true } as CustomAxiosRequestConfig),
  // Users
  getUsers: (params?: Params) => http.get('/admin/users', { params }),
  getUserDetail: (id: string) => http.get(`/admin/users/${id}`),
  banUser: (id: string, data: BanUserParams) => http.post(`/admin/users/${id}/ban`, data),
  unbanUser: (id: string) => http.post(`/admin/users/${id}/unban`),
  // Content
  getContents: (params?: Params) => http.get('/admin/contents', { params }),
  deleteContent: (id: string, reason: string) => http.delete(`/admin/contents/${id}`, { data: { reason } }),
  getStones: (params?: Params) => http.get('/admin/contents/stones', { params }),
  getBoats: (params?: Params) => http.get('/admin/contents/boats', { params }),
  deleteStone: (id: string, reason: string) => http.delete(`/admin/contents/stones/${id}`, { data: { reason } }),
  deleteBoat: (id: string, reason: string) => http.delete(`/admin/contents/boats/${id}`, { data: { reason } }),
  // Reports
  getReports: (params?: Params) => http.get('/admin/reports', { params }),
  handleReport: (id: string, data: HandleReportParams) => http.post(`/admin/reports/${id}/handle`, data),
  // Moderation
  getPendingModeration: (params?: Params) => http.get('/admin/moderation/pending', { params }),
  getModerationHistory: (params?: Params) => http.get('/admin/moderation/history', { params }),
  approveContent: (id: string) => http.post(`/admin/moderation/${id}/approve`),
  rejectContent: (id: string, reason: string) => http.post(`/admin/moderation/${id}/reject`, { reason }),
  // Sensitive words
  getSensitiveWords: (params?: Params) => http.get('/admin/sensitive-words', { params }),
  addSensitiveWord: (data: AddSensitiveWordParams) => http.post('/admin/sensitive-words', data),
  updateSensitiveWord: (id: string | number, data: UpdateSensitiveWordParams) => http.put(`/admin/sensitive-words/${id}`, data),
  deleteSensitiveWord: (id: string | number) => http.delete(`/admin/sensitive-words/${id}`),
  // Settings
  getSystemConfig: () => http.get('/admin/config'),
  updateSystemConfig: (data: SaveConfigPayload) => http.put('/admin/config', data),
  broadcastMessage: (data: BroadcastMessageParams) => http.post('/admin/broadcast', data, { timeout: LONG_TIMEOUT }),
  // Logs
  getOperationLogs: (params?: Params) => http.get('/admin/logs', { params }),
  // Admin info
  getAdminInfo: () => http.get('/admin/info'),
  // Edge AI
  getEdgeAIStatus: () => http.get('/admin/edge-ai/status', { skipLoading: true } as CustomAxiosRequestConfig),
  getEdgeAIMetrics: () => http.get('/admin/edge-ai/metrics', { skipLoading: true } as CustomAxiosRequestConfig),
  getEmotionPulse: () => http.get('/admin/edge-ai/emotion-pulse', { skipLoading: true } as CustomAxiosRequestConfig),
  triggerFederatedAggregation: (data: FederatedAggregationParams) => http.post('/admin/edge-ai/federated/aggregate', data, { timeout: LONG_TIMEOUT }),
  getPrivacyBudget: () => http.get('/admin/edge-ai/privacy-budget', { skipLoading: true } as CustomAxiosRequestConfig),
  edgeAIVectorSearch: (data: VectorSearchParams) => http.post('/admin/edge-ai/vector-search', data, { timeout: LONG_TIMEOUT }),
  getEdgeAIConfig: () => http.get('/admin/edge-ai/config'),
  updateEdgeAIConfig: (data: Params) => http.put('/admin/edge-ai/config', data, { timeout: LONG_TIMEOUT }),
  // AI Analysis
  analyzeText: (text: string) => http.post('/admin/edge-ai/analyze', { text }, { timeout: LONG_TIMEOUT }),
  moderateText: (text: string) => http.post('/admin/edge-ai/moderate', { text }, { timeout: LONG_TIMEOUT }),
  // Recommendations
  getTrendingContent: () => http.get('/recommendations/trending', { skipLoading: true } as CustomAxiosRequestConfig),
  getEmotionTrends: () => http.get('/recommendations/emotion-trends', { skipLoading: true } as CustomAxiosRequestConfig),
  getRecommendationStats: () => http.get('/recommendations/stones', { skipLoading: true } as CustomAxiosRequestConfig),
}
