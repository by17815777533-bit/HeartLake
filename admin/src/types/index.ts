/**
 * 管理后台全局类型定义
 *
 * 所有 interface 与后端 C++ DTO / JSON 响应保持字段名一致（snake_case），
 * 前端内部使用 camelCase 的场景通过 composable 做映射，类型层不做转换。
 */

import type { AxiosResponse } from 'axios'

// ── 业务错误 ──

/** 携带后端业务错误码的 Error 扩展，供 axios 拦截器和 errorHelper 统一处理 */
export interface BusinessError extends Error {
  /** 后端返回的六位业务错误码，如 200002 表示 token 过期 */
  _businessCode?: number
  /** 原始 axios 响应，用于读取 HTTP 状态码和响应体 */
  response?: AxiosResponse
}

// ── 用户 ──

/** 用户列表行数据，对应 GET /admin/users 响应中的单条记录 */
export interface User {
  user_id: string
  username: string
  nickname: string
  status: 'active' | 'banned'
  stones_count?: number
  boat_count?: number
  created_at: string
  last_active_at: string
}

/** 登录后存入 sessionStorage 的管理员身份摘要 */
export interface UserInfo {
  username: string
  role?: 'admin' | 'super_admin' | string
  [key: string]: unknown
}

// ── 内容 ──

/** 内容管理列表行，涵盖石头和纸船两种类型 */
export interface ContentItem {
  id: string | number
  user_id: string
  nickname?: string
  user?: {
    nickname?: string
  }
  /** 内容类型：stone / boat */
  type: string
  content: string
  mood?: string
  /** 内容状态：normal / hidden / deleted */
  status: string
  created_at: string
  likes_count?: number
  replies_count?: number
}

// ── 举报 ──

/** 举报记录，对应 GET /admin/reports 响应 */
export interface Report {
  id: string | number
  reporter_id: string
  reporter_nickname?: string
  target_id: string
  /** 举报目标类型：stone / boat / user */
  target_type: string
  reason: string
  /** pending / handled / dismissed */
  status: string
  content_preview?: string
  created_at: string
  handled_at?: string
  handler?: string
  result?: string
}

// ── 审核 ──

/** 内容审核队列条目，包含 AI 预审分数和人工复核信息 */
export interface ModerationItem {
  id: string | number
  moderation_id?: string | number
  content_id?: string | number
  content_type?: string
  user_id: string
  nickname?: string
  content: string
  type: string
  /** pending / approved / rejected */
  status: string
  /** AI 预审风险分（0-1），越高越可能违规 */
  ai_score?: number
  ai_reason?: string
  result?: string
  created_at: string
  reviewed_at?: string
  reviewer?: string
}

// ── 敏感词 ──

/** 敏感词条目，对应风险词典 CRUD */
export interface SensitiveWord {
  id: string | number
  word: string
  /** 风险等级：low / medium / high / critical */
  level: string
  category?: string
  created_at?: string
  updated_at?: string
}

// ── 操作日志 ──

/** 管理员操作日志，用于审计追踪 */
export interface OperationLog {
  id: string | number
  operator: string
  action: string
  target?: string
  detail?: string
  ip?: string
  created_at: string
}

// ── Dashboard ──

/** Dashboard 核心统计，来自 GET /api/admin/stats/dashboard */
export interface DashboardStats {
  total_users: number
  today_stones: number
  online_users?: number
  pending_reports: number
}

/** WebSocket 推送的实时统计增量 */
export interface RealtimeStats {
  total_users?: number
  online_users: number
  today_stones?: number
  pending_reports?: number
  new_report?: number
  new_moderation?: number
}

/** 用户增长曲线数据点 */
export interface GrowthDataItem {
  date: string
  count: number
}

/** 情绪分布饼图数据点 */
export interface MoodDistributionItem {
  mood_type?: string
  mood?: string
  count: number
}

/** 情绪趋势折线数据点，按日期 + 情绪类型聚合 */
export interface MoodTrendItem {
  date: string
  mood: string
  mood_type?: string
  count: number
}

/** 24 小时活跃时段柱状图数据点 */
export interface ActiveTimeItem {
  /** 0-23 对应 0:00-23:00 */
  hour: number
  count: number
}

/** 热门话题条目 */
export interface TrendingTopic {
  keyword: string
  count: number
  /** up / down / stable */
  trend?: string
}

/** 热门内容条目，用于 AI 趋势面板 */
export interface TrendingContentItem {
  id: string | number
  content: string
  likes?: number
  score?: number
}

// ── 隐私 ──

/** 差分隐私预算统计，对应 GET /admin/edge-ai/privacy-budget */
export interface PrivacyStats {
  queryCount: number
  /** 已消耗的 epsilon 值 */
  epsilonUsed: number
  /** 总 epsilon 预算 */
  epsilonTotal: number
  protectedUsers: number
}

// ── 情绪共鸣 ──

/** DTW 情绪共鸣引擎统计 */
export interface ResonanceStats {
  todayMatches: number
  avgScore: number
  topMood: string
  successRate: number
}

// ── Edge AI ──

/** EdgeAI 子系统状态描述 */
export interface EdgeAISubsystem {
  name: string
  /** running / stopped / degraded */
  status: string
  uptime?: number
  version?: string
  [key: string]: unknown
}

/** EdgeAI 整体状态，包含各子系统列表 */
export interface EdgeAIStatus {
  subsystems: EdgeAISubsystem[]
  overall_status: string
  [key: string]: unknown
}

/** EdgeAI 性能指标快照 */
export interface EdgeAIMetrics {
  cpu_usage?: number
  memory_usage?: number
  requests_per_second?: number
  avg_latency_ms?: number
  [key: string]: unknown
}

// ── 系统设置 ──

/** 系统全局配置，对应 GET /admin/config 中的 system 段 */
export interface SystemConfig {
  site_name?: string
  max_stone_length?: number
  max_daily_stones?: number
  [key: string]: unknown
}

/** AI 模块配置 */
export interface AIConfig {
  enabled?: boolean
  model?: string
  sensitivity?: number
  [key: string]: unknown
}

// ── WebSocket ──

/** WebSocket 消息帧结构 */
export interface WSMessage {
  type: string
  data?: unknown
  token?: string
}

/** WebSocket 事件监听回调签名 */
export type WSListener = (data: unknown) => void

// ── 分页 ──

/** 通用分页状态 */
export interface Pagination {
  page: number
  pageSize: number
  total: number
}

/** useTablePagination composable 的初始化选项 */
export interface TablePaginationOptions<F = Record<string, unknown>> {
  defaultPageSize?: number
  /** 外部筛选条件的 reactive 对象引用，重置时会被 defaultFilters 覆盖 */
  filters?: F | null
  defaultFilters?: F | null
  /** 搜索前校验钩子，返回 false 可阻止本次搜索 */
  beforeSearch?: (() => boolean | void) | null
}

// ── 情绪脉搏 ──

/** 情绪温度仪表盘数据 */
export interface EmotionPulseData {
  temperature?: number
  value?: number
  label?: string
}

/** 情绪趋势折线数据点（积极/中性/消极三维） */
export interface EmotionTrendItem {
  date?: string
  day?: string
  positive?: number
  pos?: number
  neutral?: number
  neu?: number
  negative?: number
  neg?: number
}

// ── 敏感码范围 ──

/** 生产环境错误码脱敏区间，落入区间的业务码统一返回 msg */
export interface SensitiveCodeRange {
  min: number
  max: number
  msg: string
}

// ── ECharts 回调参数（tooltip formatter 用） ──

/** ECharts tooltip formatter 回调参数的最小类型约束 */
export interface EChartsTooltipParam {
  componentType?: string
  seriesType?: string
  seriesIndex?: number
  seriesName?: string
  name: string
  dataIndex?: number
  data?: unknown
  value: number | number[] | string
  color?: string
  marker?: string
  percent?: number
}

// ── API 具体参数类型 ──

/** 处理举报请求体 */
export interface HandleReportParams {
  /** approve / reject / dismiss */
  action: string
  note?: string
}

/** 新增敏感词请求体 */
export interface AddSensitiveWordParams {
  word: string
  level: string
  replacement?: string
}

/** 更新敏感词请求体（部分更新） */
export interface UpdateSensitiveWordParams {
  word?: string
  level?: string
  replacement?: string
}

/** 全站广播消息请求体 */
export interface BroadcastMessageParams {
  message: string
  /** info / warning / danger */
  level: string
}

/** 系统设置保存请求体，三个配置段均为可选 */
export interface SaveConfigPayload {
  system?: {
    name?: string
    description?: string
    allow_register?: boolean
    allow_anonymous?: boolean
  }
  ai?: {
    provider?: string
    api_key?: string
    base_url?: string
    model?: string
    enable_sentiment?: boolean
    enable_auto_reply?: boolean
  }
  rate?: {
    stone_per_hour?: number
    boat_per_hour?: number
    message_per_minute?: number
    max_content_length?: number
  }
}

/** 联邦学习手动聚合请求体 */
export interface FederatedAggregationParams {
  round?: number
  [key: string]: unknown
}

/** HNSW 向量搜索请求体 */
export interface VectorSearchParams {
  query: string
  top_k?: number
  [key: string]: unknown
}
