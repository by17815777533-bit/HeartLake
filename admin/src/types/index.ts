import type { AxiosResponse } from 'axios'

// ── 业务错误 ──
export interface BusinessError extends Error {
  _businessCode?: number
  response?: AxiosResponse
}

// ── 用户 ──
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

export interface UserInfo {
  username: string
  role?: 'admin' | 'super_admin' | string
  [key: string]: unknown
}

// ── 内容 ──
export interface ContentItem {
  id: string | number
  user_id: string
  nickname?: string
  type: string
  content: string
  mood?: string
  status: string
  created_at: string
  likes_count?: number
  replies_count?: number
}

// ── 举报 ──
export interface Report {
  id: string | number
  reporter_id: string
  reporter_nickname?: string
  target_id: string
  target_type: string
  reason: string
  status: string
  content_preview?: string
  created_at: string
  handled_at?: string
  handler?: string
  result?: string
}

// ── 审核 ──
export interface ModerationItem {
  id: string | number
  moderation_id?: string | number
  content_id?: string | number
  content_type?: string
  user_id: string
  nickname?: string
  content: string
  type: string
  status: string
  ai_score?: number
  ai_reason?: string
  created_at: string
  reviewed_at?: string
  reviewer?: string
}

// ── 敏感词 ──
export interface SensitiveWord {
  id: string | number
  word: string
  level: string
  category?: string
  created_at?: string
  updated_at?: string
}

// ── 操作日志 ──
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
export interface DashboardStats {
  total_users: number
  today_stones: number
  online_users?: number
  pending_reports: number
}

export interface RealtimeStats {
  total_users?: number
  online_users: number
  today_stones?: number
  pending_reports?: number
  new_report?: number
  new_moderation?: number
}

export interface GrowthDataItem {
  date: string
  count: number
}

export interface MoodDistributionItem {
  mood_type?: string
  mood?: string
  count: number
}

export interface MoodTrendItem {
  date: string
  mood: string
  mood_type?: string
  count: number
}

export interface ActiveTimeItem {
  hour: number
  count: number
}

export interface TrendingTopic {
  keyword: string
  count: number
  trend?: string
}

export interface TrendingContentItem {
  id: string | number
  content: string
  likes?: number
  score?: number
}

// ── 隐私 ──
export interface PrivacyStats {
  queryCount: number
  epsilonUsed: number
  epsilonTotal: number
  protectedUsers: number
}

// ── 情绪共鸣 ──
export interface ResonanceStats {
  todayMatches: number
  avgScore: number
  topMood: string
  successRate: number
}

// ── Edge AI ──
export interface EdgeAISubsystem {
  name: string
  status: string
  uptime?: number
  version?: string
  [key: string]: unknown
}

export interface EdgeAIStatus {
  subsystems: EdgeAISubsystem[]
  overall_status: string
  [key: string]: unknown
}

export interface EdgeAIMetrics {
  cpu_usage?: number
  memory_usage?: number
  requests_per_second?: number
  avg_latency_ms?: number
  [key: string]: unknown
}

// ── 系统设置 ──
export interface SystemConfig {
  site_name?: string
  max_stone_length?: number
  max_daily_stones?: number
  [key: string]: unknown
}

export interface AIConfig {
  enabled?: boolean
  model?: string
  sensitivity?: number
  [key: string]: unknown
}

// ── WebSocket ──
export interface WSMessage {
  type: string
  data?: unknown
  token?: string
}

export type WSListener = (data: unknown) => void

// ── 分页 ──
export interface Pagination {
  page: number
  pageSize: number
  total: number
}

export interface TablePaginationOptions<F = Record<string, unknown>> {
  defaultPageSize?: number
  filters?: F | null
  defaultFilters?: F | null
  beforeSearch?: (() => boolean | void) | null
}

// ── 情绪脉搏 ──
export interface EmotionPulseData {
  temperature?: number
  value?: number
  label?: string
}

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
export interface SensitiveCodeRange {
  min: number
  max: number
  msg: string
}

// ── ECharts 回调参数（tooltip formatter 用） ──
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
export interface HandleReportParams {
  action: string
  note?: string
}

export interface AddSensitiveWordParams {
  word: string
  level: string
  replacement?: string
}

export interface UpdateSensitiveWordParams {
  word?: string
  level?: string
  replacement?: string
}

export interface BroadcastMessageParams {
  message: string
  level: string
}

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

export interface FederatedAggregationParams {
  round?: number
  [key: string]: unknown
}

export interface VectorSearchParams {
  query: string
  top_k?: number
  [key: string]: unknown
}
