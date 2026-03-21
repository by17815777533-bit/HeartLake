/**
 * @file index.ts
 * @brief 核心 HTTP 通信模块及全站 API 注册注册表
 *
 * 包装底层 Axios 引擎以构建前端数据获取层统一规范。包含以下安全及架构特性：
 * - 自动化拦截模型：自动组装和注入符合 Oauth / JWT 及 PASETO 标准的 Bearer Token，挂载全局加载态计数器。
 *   包含路由切换及相同参数并发情况下的请求去重（基于 CancelToken 或 AbortController）。
 * - 响应降级及容错：统一梳理并拦截应用层非成功状态（响应 200 即代码为非零标识），处理幂等情况下的接口异常。
 *   特定场景（GET 协议类接口，指定无尽休眠或超时等场景下）执行渐进式指数退避重试（最高限额三次）。
 * - 认证衰退安全：鉴权失败（如 HTTP 401 及特定业务校验错误码区间 200001-200005）统一下达强制登出及单一实例防重入的重定向任务。
 * - 防跨站攻击体系 (CSRF)：支持主动抓取预置 `csrf_token` cookie 并追加防御头 `X-CSRF-Token` 以保护长轮询及提交。
 * - 多维耗时补偿策略：默认接口 15 秒通讯截断，为边缘计算聚合 (EdgeAI) 及流式上传/导出配置 60 秒延长窗口。
 */
import axios, { type AxiosRequestConfig, type AxiosResponse, type InternalAxiosRequestConfig } from 'axios'
import { ElMessage } from 'element-plus'
import router from '@/router'
import { useAppStore } from '@/stores'
import { getBusinessMessage } from '@/utils/errorHelper'
import { ADMIN_LOGIN_PATH } from '@/utils/adminRoutes'

/**
 * @brief 自定义 Axios 顶级请求配置拓展
 */
interface CustomAxiosRequestConfig extends AxiosRequestConfig {
  /** @brief 标识此事务是否主动静默全局 UI 刷新阻塞遮罩 */
  skipLoading?: boolean
  /** @brief 标识此事务在遭逢认证错误是否忽略自动退场指令 */
  skipAuthRedirect?: boolean
}

/**
 * @brief 自定义 Axios 拦截器专属内部拓展配置
 */
interface CustomInternalConfig extends InternalAxiosRequestConfig {
  skipLoading?: boolean
  skipAuthRedirect?: boolean
  /** @brief 标记系统化自动重试机制已激活及当下重试顺位 */
  _retryCount?: number
}

/**
 * @brief 带特定业务语义指纹的扩展异常协议模型
 */
interface BusinessError extends Error {
  /** @brief 后台规培定义的独立业务领域异常溯源码 */
  _businessCode?: number
  /** @brief 网络通讯层的底层附带响应对象，支持溯源追踪 */
  response?: AxiosResponse
}

/** 
 * @brief HttpClient 底层通信实例 
 * @details 采用 VITE 宏指令提取基准微服务基址，兼收 CSRF 标准配置并统一下发请求策略。
 */
export const http = axios.create({
  baseURL: import.meta.env.VITE_API_BASE_URL || '/api',
  timeout: 15000,
  xsrfCookieName: 'csrf_token',
  xsrfHeaderName: 'X-CSRF-Token',
})

/**
 * @brief 阻塞重定向互斥锁，规避弱网或接口齐发引发的登出死锁
 */
let isRedirectingToLogin = false

/**
 * @brief 本地响应授权崩塌及清理策略核心函数
 * 
 * 摧毁现有证书，发起路由出栈处理。
 * 
 * @param msg 指引层外抛的具体用户视界提示语句
 */
function handleAuthFailure(msg?: string): void {
  const appStore = useAppStore()
  appStore.clearToken()
  if (!isRedirectingToLogin && router.currentRoute.value.path !== ADMIN_LOGIN_PATH) {
    isRedirectingToLogin = true
    ElMessage.error(msg || '登录已过期，请重新登录')
    router.push(ADMIN_LOGIN_PATH).finally(() => {
      isRedirectingToLogin = false
    })
  }
}

/**
 * @brief 会话请求注册池，键值为哈希标志符，绑定网络终止手柄对象
 */
const pendingRequests = new Map<string, AbortController>()

/**
 * @brief 创建网络请求身份摘要
 * 
 * 支持比对请求五元组及内容序列。若前后双请求匹配该摘要，先导请求即视作无效作废（防重复并发机制）。
 * 
 * @param config 当前拦截周期的请求组态对象
 * @return 构造哈希字符串
 */
function getRequestKey(config: CustomInternalConfig): string {
  const params = config.params ? JSON.stringify(config.params, Object.keys(config.params).sort()) : ''
  const data = config.data ? (typeof config.data === 'string' ? config.data : JSON.stringify(config.data)) : ''
  return `${config.method}:${config.url}:${params}:${data}`
}

/** 
 * @brief 全局抛弃排队池中待决口网络事务 
 * @details 多使用于 SPA 路由栈清场等大型生命周期场景。
 */
export function clearPendingRequests(): void {
  pendingRequests.forEach(controller => controller.abort())
  pendingRequests.clear()
}

/** 
 * @brief 旧版请求清洗 API 暴露别名支持
 */
export function cancelAllRequests(): void {
  clearPendingRequests()
}

/**
 * 判断错误是否由主动取消请求引起。
 * 兼容 axios 的 CanceledError 与 AbortController 取消场景。
 */
export function isRequestCanceled(error: unknown): boolean {
  if (axios.isCancel(error)) return true
  if (!error || typeof error !== 'object') return false

  const maybeError = error as { code?: string; name?: string; message?: string }
  return maybeError.code === 'ERR_CANCELED'
    || maybeError.name === 'CanceledError'
    || maybeError.message === 'canceled'
}

/** 
 * @brief Axios 全局请求门神拦截装置
 * @details 第一时间推入验证字典，排遣未完结相同冲突体，挂载事务标识信号标。同时触犯阻塞性遮罩。
 */
http.interceptors.request.use((config: CustomInternalConfig) => {
  const appStore = useAppStore()
  const token = appStore.getToken()
  if (token) config.headers.Authorization = `Bearer ${token}`

  // 相同 method:url:params 的请求自动取消前一个，防止重复提交
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

/** 
 * @brief Axios 全局响应归档拦截装置
 * @details 负责脱钩排队池对应事务键值；收起前端动画负反馈，拆解与剥离响应状态并分类传递异常流或正反馈数据实体。
 */
http.interceptors.response.use(
  (response: AxiosResponse) => {
    pendingRequests.delete(getRequestKey(response.config as CustomInternalConfig))
    const appStore = useAppStore()
    if (!(response.config as CustomInternalConfig).skipLoading) {
      appStore.stopLoading()
    }

    // HTTP 200 但 code 非 0/200 表示业务层失败
    const { code, message } = response.data ?? {}
    if (code != null && code !== 0 && code !== 200) {
      // 认证类业务错误码 (200001-200005)：清 token 跳转登录
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

    // GET 请求遇到可重试状态码时自动重试（指数退避，最多3次）
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

    // HTTP 401：token 过期或无效，跳转登录
    if (axiosError.response?.status === 401) {
      const config = axiosError.config as CustomInternalConfig | undefined
      if (!config?.skipAuthRedirect) {
        handleAuthFailure()
      }
    } else if (!axios.isCancel(error)) {
      ElMessage.error(axiosError.message || '网络请求失败')
    }

    return Promise.reject(error)
  },
)

/** 
 * @brief 通用不定参数映射表 
 */
type Params = Record<string, unknown>

type VectorSearchPayload = VectorSearchParams & { topK?: number }

type AliasPair = readonly [camelKey: string, snakeKey: string]

function camelToSnakeKey(key: string): string {
  return key.replace(/[A-Z]/g, (match) => `_${match.toLowerCase()}`)
}

function mirrorAliasPair(payload: Params, [camelKey, snakeKey]: AliasPair): void {
  if (payload[snakeKey] == null && payload[camelKey] != null) {
    payload[snakeKey] = payload[camelKey]
  }
  if (payload[camelKey] == null && payload[snakeKey] != null) {
    payload[camelKey] = payload[snakeKey]
  }
}

function buildSnakeMirroredPayload(data: Params, aliasPairs: readonly AliasPair[] = []): Params {
  const payload: Params = { ...data }
  Object.entries(data).forEach(([key, value]) => {
    const snakeKey = camelToSnakeKey(key)
    if (snakeKey !== key && payload[snakeKey] === undefined) {
      payload[snakeKey] = value
    }
  })
  aliasPairs.forEach((pair) => mirrorAliasPair(payload, pair))
  return payload
}

function buildEdgeAIConfigPayload(data: Params): Params {
  return buildSnakeMirroredPayload(data)
}

function buildVectorSearchPayload(data: VectorSearchPayload): Params {
  return buildSnakeMirroredPayload(data, [['topK', 'top_k']])
}

/** 
 * @brief 重量级事务强制截断常量
 */
const LONG_TIMEOUT = 60000

/** 
 * @brief 通行证口令握手包裹模型 
 */
interface LoginPayload {
  username: string
  password: string
}

// 高频 API 的具体参数类型
import type {
  HandleReportParams, AddSensitiveWordParams, UpdateSensitiveWordParams,
  BroadcastMessageParams, SaveConfigPayload,
  FederatedAggregationParams, VectorSearchParams,
} from '@/types'

/**
 * @brief 封禁管控专用指令集
 */
interface BanUserParams {
  reason: string
}

/**
 * @brief 后端全类目业务门面通讯实例集
 */
export default {
  // Auth
  login: (data: LoginPayload) => http.post('/admin/login', data),
  logout: () => http.post('/admin/logout'),
  // Dashboard
  getDashboardStats: () => http.get('/admin/stats/dashboard'),
  getRealtimeStats: () => http.get('/admin/stats/realtime', { skipLoading: true } as CustomAxiosRequestConfig),
  getUserGrowthStats: (range: string) => http.get(`/admin/stats/user-growth?days=${range}`, { skipLoading: true } as CustomAxiosRequestConfig),
  getMoodDistribution: () => http.get('/admin/stats/mood-distribution', { skipLoading: true } as CustomAxiosRequestConfig),
  getMoodTrend: (range: string) => http.get(`/admin/stats/mood-trend?days=${range}`, { skipLoading: true } as CustomAxiosRequestConfig),
  getTrendingTopics: () => http.get('/admin/stats/trending-topics', { skipLoading: true } as CustomAxiosRequestConfig),
  getActiveTimeStats: () => http.get('/admin/stats/active-time', { skipLoading: true } as CustomAxiosRequestConfig),
  // Users
  getUsers: (params?: Params) => http.get('/admin/users', { params }),
  getUserDetail: (id: string) => http.get(`/admin/users/${id}`),
  banUser: (id: string, data: BanUserParams) => http.post(`/admin/users/${id}/ban`, data),
  unbanUser: (id: string) => http.post(`/admin/users/${id}/unban`),
  // Content
  getContents: (params?: Params) => http.get('/admin/content', { params }),
  deleteContent: (id: string, reason: string, type: 'stone' | 'boat' = 'stone') =>
    http.delete(`/admin/${type === 'boat' ? 'boats' : 'stones'}/${id}`, { data: { reason } }),
  getStones: (params?: Params) => http.get('/admin/stones', { params }),
  getBoats: (params?: Params) => http.get('/admin/boats', { params }),
  deleteStone: (id: string, reason: string) => http.delete(`/admin/stones/${id}`, { data: { reason } }),
  deleteBoat: (id: string, reason: string) => http.delete(`/admin/boats/${id}`, { data: { reason } }),
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
  edgeAIVectorSearch: (data: VectorSearchPayload) => http.post(
    '/admin/edge-ai/vector-search',
    buildVectorSearchPayload(data),
    { timeout: LONG_TIMEOUT },
  ),
  getEdgeAIConfig: () => http.get('/admin/edge-ai/config'),
  updateEdgeAIConfig: (data: Params) => http.put(
    '/admin/edge-ai/config',
    buildEdgeAIConfigPayload(data),
    { timeout: LONG_TIMEOUT },
  ),
  // AI Analysis
  analyzeText: (text: string) => http.post('/admin/edge-ai/analyze', { text }, { timeout: LONG_TIMEOUT }),
  moderateText: (text: string) => http.post('/admin/edge-ai/moderate', { text }, { timeout: LONG_TIMEOUT }),
  // Recommendation System
  getTrendingContent: (params?: Params) => http.get(
    '/recommendations/trending',
    { params, skipLoading: true, skipAuthRedirect: true } as CustomAxiosRequestConfig,
  ),
  getEmotionTrends: () => http.get('/recommendations/emotion-trends', { skipLoading: true, skipAuthRedirect: true } as CustomAxiosRequestConfig),
  getRecommendationStats: () => http.get('/recommendations/stones', { skipLoading: true, skipAuthRedirect: true } as CustomAxiosRequestConfig),
}
