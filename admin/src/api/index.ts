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

// 请求取消机制：基于 AbortController
const pendingRequests = new Map<string, AbortController>()

function getRequestKey(config: CustomInternalConfig): string {
  return `${config.method}:${config.url}`
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
        appStore.clearToken()
        if (!isRedirectingToLogin && router.currentRoute.value.path !== '/login') {
          isRedirectingToLogin = true
          ElMessage.error(getBusinessMessage(code) || '登录已过期，请重新登录')
          router.push('/login').finally(() => {
            isRedirectingToLogin = false
          })
        }
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

    // HTTP 401：token 过期或无效
    if (axiosError.response?.status === 401) {
      const appStore = useAppStore()
      appStore.clearToken()
      if (!isRedirectingToLogin && router.currentRoute.value.path !== '/login') {
        isRedirectingToLogin = true
        ElMessage.error('登录已过期，请重新登录')
        router.push('/login').finally(() => {
          isRedirectingToLogin = false
        })
      }
    } else if (!axios.isCancel(error)) {
      ElMessage.error(axiosError.message || '网络请求失败')
    }

    return Promise.reject(error)
  },
)

type Params = Record<string, unknown>

export default {
  // Auth
  login: (data: Params) => http.post('/admin/login', data),
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
  banUser: (id: string, data: Params) => http.post(`/admin/users/${id}/ban`, data),
  unbanUser: (id: string) => http.post(`/admin/users/${id}/unban`),
  // Content
  getContents: (params?: Params) => http.get('/admin/contents', { params }),
  deleteContent: (id: string) => http.delete(`/admin/contents/${id}`),
  // Reports
  getReports: (params?: Params) => http.get('/admin/reports', { params }),
  handleReport: (id: string, data: Params) => http.post(`/admin/reports/${id}/handle`, data),
  // Moderation
  getPendingModeration: (params?: Params) => http.get('/admin/moderation/pending', { params }),
  getModerationHistory: (params?: Params) => http.get('/admin/moderation/history', { params }),
  approveContent: (id: string) => http.post(`/admin/moderation/${id}/approve`),
  rejectContent: (id: string, reason: string) => http.post(`/admin/moderation/${id}/reject`, { reason }),
  // Sensitive words
  getSensitiveWords: (params?: Params) => http.get('/admin/sensitive-words', { params }),
  addSensitiveWord: (data: Params) => http.post('/admin/sensitive-words', data),
  updateSensitiveWord: (id: string | number, data: Params) => http.put(`/admin/sensitive-words/${id}`, data),
  deleteSensitiveWord: (id: string | number) => http.delete(`/admin/sensitive-words/${id}`),
  // Settings
  getSystemConfig: () => http.get('/admin/config'),
  updateSystemConfig: (data: Params) => http.put('/admin/config', data),
  testAIConnection: () => http.get('/admin/edge-ai/status'),
  broadcastMessage: (data: Params) => http.post('/admin/broadcast', data),
  // Logs
  getOperationLogs: (params?: Params) => http.get('/admin/logs', { params }),
  // Admin info
  getAdminInfo: () => http.get('/admin/info'),
  // Edge AI
  getEdgeAIStatus: () => http.get('/admin/edge-ai/status', { skipLoading: true } as CustomAxiosRequestConfig),
  getEdgeAIMetrics: () => http.get('/admin/edge-ai/metrics', { skipLoading: true } as CustomAxiosRequestConfig),
  getEmotionPulse: () => http.get('/admin/edge-ai/emotion-pulse', { skipLoading: true } as CustomAxiosRequestConfig),
  triggerFederatedAggregation: (data: Params) => http.post('/admin/edge-ai/federated/aggregate', data),
  getPrivacyBudget: () => http.get('/admin/edge-ai/privacy-budget', { skipLoading: true } as CustomAxiosRequestConfig),
  getPrivacyStats: () => http.get('/admin/edge-ai/privacy-budget', { skipLoading: true } as CustomAxiosRequestConfig),
  getResonanceStats: () => http.get('/admin/edge-ai/emotion-pulse', { skipLoading: true } as CustomAxiosRequestConfig),
  edgeAIVectorSearch: (data: Params) => http.post('/admin/edge-ai/vector-search', data),
  getEdgeAIConfig: () => http.get('/admin/edge-ai/config'),
  updateEdgeAIConfig: (data: Params) => http.put('/admin/edge-ai/config', data),
  // AI Analysis
  analyzeText: (text: string) => http.post('/admin/edge-ai/analyze', { text }),
  moderateText: (text: string) => http.post('/admin/edge-ai/moderate', { text }),
  // Recommendations
  getTrendingContent: () => http.get('/recommendations/trending', { skipLoading: true } as CustomAxiosRequestConfig),
  getEmotionTrends: () => http.get('/recommendations/emotion-trends', { skipLoading: true } as CustomAxiosRequestConfig),
  getRecommendationStats: () => http.get('/recommendations/stones', { skipLoading: true } as CustomAxiosRequestConfig),
}
