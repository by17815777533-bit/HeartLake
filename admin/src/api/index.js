import axios from 'axios'
import { ElMessage } from 'element-plus'
import router from '@/router'
import { useAppStore } from '@/stores'

const http = axios.create({
  baseURL: '/api',
  timeout: 15000,
  xsrfCookieName: 'csrf_token',
  xsrfHeaderName: 'X-CSRF-Token',
})

// 防止 401 重复跳转的标志
let isRedirectingToLogin = false

// 请求拦截器：注入 token + 全局 loading
http.interceptors.request.use(config => {
  const appStore = useAppStore()
  const token = appStore.getToken()
  if (token) config.headers.Authorization = `Bearer ${token}`
  // 全局 loading（可通过 config.skipLoading 跳过）
  if (!config.skipLoading) {
    appStore.startLoading()
  }
  return config
})

// 响应拦截器：统一处理错误 + 全局 loading
http.interceptors.response.use(
  response => {
    const appStore = useAppStore()
    if (!response.config.skipLoading) {
      appStore.stopLoading()
    }
    return response
  },
  error => {
    const appStore = useAppStore()
    if (!error.config?.skipLoading) {
      appStore.stopLoading()
    }
    if (error.response) {
      const { status, data } = error.response
      if (status === 401) {
        appStore.clearToken()
        if (!isRedirectingToLogin && router.currentRoute.value.path !== '/login') {
          isRedirectingToLogin = true
          ElMessage.error('登录已过期，请重新登录')
          router.push('/login').finally(() => {
            isRedirectingToLogin = false
          })
        }
      } else if (status === 403) {
        ElMessage.error(data?.message || '没有权限执行此操作')
      } else if (status === 429) {
        const retryAfter = error.response.headers['retry-after']
        ElMessage.warning(retryAfter
          ? `操作过于频繁，请 ${retryAfter} 秒后再试`
          : '操作过于频繁，请稍后再试')
      } else if (status >= 500) {
        ElMessage.error('服务器错误，请稍后重试')
      }
    } else if (error.code === 'ECONNABORTED') {
      ElMessage.error('请求超时，请检查网络')
    } else {
      ElMessage.error('网络连接失败，请检查网络')
    }
    // 返回统一的 rejected promise，附带简洁 message 供视图层使用
    const msg = error.response?.data?.message || error.message || '请求失败'
    return Promise.reject(new Error(msg))
  }
)

export { http }

export default {
  // Auth
  login: data => http.post('/admin/login', data),
  // Dashboard stats
  getDashboardStats: () => http.get('/admin/stats/dashboard'),
  getRealtimeStats: () => http.get('/admin/stats/realtime', { skipLoading: true }),
  getUserGrowthStats: days => http.get('/admin/stats/user-growth', { params: { days } }),
  getMoodDistribution: () => http.get('/admin/stats/mood-distribution'),
  getMoodTrend: days => http.get('/admin/stats/mood-distribution', { params: { days, type: 'trend' } }),
  getTrendingTopics: () => http.get('/admin/stats/trending-topics'),
  getActiveTimeStats: () => http.get('/admin/stats/active-time'),
  getPrivacyStats: () => http.get('/admin/edge-ai/privacy-budget', { skipLoading: true }),
  getResonanceStats: () => http.get('/recommendations/emotion-trends', { skipLoading: true }),
  // Users
  getUsers: params => http.get('/admin/users', { params }),
  banUser: (userId, reason) => http.post(`/admin/users/${userId}/ban`, { reason }),
  unbanUser: userId => http.post(`/admin/users/${userId}/unban`),
  // Content
  getStones: params => http.get('/admin/stones', { params }),
  getBoats: params => http.get('/admin/boats', { params }),
  deleteStone: (id, reason) => http.delete(`/admin/stones/${id}`, { data: { reason } }),
  deleteBoat: (id, reason) => http.delete(`/admin/boats/${id}`, { data: { reason } }),
  // Reports
  getReports: params => http.get('/admin/reports', { params }),
  handleReport: (id, data) => http.post(`/admin/reports/${id}/handle`, data),
  // Moderation
  getPendingModeration: params => http.get('/admin/moderation/pending', { params }),
  getModerationHistory: params => http.get('/admin/moderation/history', { params }),
  approveContent: id => http.post(`/admin/moderation/${id}/approve`),
  rejectContent: (id, reason) => http.post(`/admin/moderation/${id}/reject`, { reason }),
  // Sensitive words
  getSensitiveWords: params => http.get('/admin/sensitive-words', { params }),
  addSensitiveWord: data => http.post('/admin/sensitive-words', data),
  updateSensitiveWord: (id, data) => http.put(`/admin/sensitive-words/${id}`, data),
  deleteSensitiveWord: id => http.delete(`/admin/sensitive-words/${id}`),
  // Settings
  getSystemConfig: () => http.get('/admin/config'),
  updateSystemConfig: data => http.put('/admin/config', data),
  testAIConnection: () => http.get('/admin/edge-ai/status'),
  broadcastMessage: data => http.post('/admin/broadcast', data),
  // Logs
  getOperationLogs: params => http.get('/admin/logs', { params }),
  // Admin info
  getAdminInfo: () => http.get('/admin/info'),
  // Edge AI
  getEdgeAIStatus: () => http.get('/admin/edge-ai/status', { skipLoading: true }),
  getEdgeAIMetrics: () => http.get('/admin/edge-ai/metrics', { skipLoading: true }),
  getEmotionPulse: () => http.get('/admin/edge-ai/emotion-pulse', { skipLoading: true }),
  triggerFederatedAggregation: data => http.post('/admin/edge-ai/federated/aggregate', data),
  getPrivacyBudget: () => http.get('/admin/edge-ai/privacy-budget', { skipLoading: true }),
  edgeAIVectorSearch: data => http.post('/admin/edge-ai/vector-search', data),
  getEdgeAIConfig: () => http.get('/admin/edge-ai/config'),
  updateEdgeAIConfig: data => http.put('/admin/edge-ai/config', data),
  // AI Analysis
  analyzeText: text => http.post('/admin/edge-ai/analyze', { text }),
  moderateText: text => http.post('/admin/edge-ai/moderate', { text }),
  // Recommendations
  getTrendingContent: () => http.get('/recommendations/trending', { skipLoading: true }),
  getEmotionTrends: () => http.get('/recommendations/emotion-trends', { skipLoading: true }),
  getRecommendationStats: () => http.get('/recommendations/stones', { skipLoading: true }),
}
