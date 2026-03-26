<!--
  智能辅助页面

  统一到新的管理台语言：
  - 叙事型页首 + 指标带，弱化旧的技术大屏感
  - 为性能、联动、检索、设置提供一致的雾面容器
  - 让“智能辅助”更像值守与陪伴工具，而不是开发演示页
-->

<template>
  <div class="edge-workbench ops-page">
    <section class="edge-grid">
      <article class="edge-card edge-card--engine">
        <div class="edge-head">
          <div>
            <span class="edge-eyebrow" :class="{ 'edge-eyebrow--warning': edgeIssues.length > 0 }">
              {{ engineEyebrow }}
            </span>
            <h2>智能辅助引擎</h2>
          </div>
          <span class="edge-chip">{{ currentTime }}</span>
        </div>

        <div class="engine-total">
          <span>在线节点</span>
          <div class="engine-total__value">
            {{ activeNodesDisplay }}
            <small v-if="activeNodesDisplay !== '--'">个</small>
          </div>
          <p>{{ heroDescription }}</p>
        </div>

        <div v-if="edgeIssues.length" class="edge-status-banner" :class="edgeIssueTone">
          <strong>{{ edgeIssueTitle }}</strong>
          <p>{{ edgeIssueSummary }}</p>
        </div>

        <div class="engine-actions">
          <button type="button" class="engine-action" :disabled="loading" @click="refreshAll">
            <el-icon><RefreshRight /></el-icon>
            <span>刷新状态</span>
          </button>
          <button
            type="button"
            class="engine-action"
            :disabled="federated.aggregating"
            @click="triggerAggregation"
          >
            <span>触发聚合</span>
          </button>
        </div>

        <div class="engine-stack">
          <article class="engine-mini-card is-blue">
            <span>平均响应</span>
            <strong>{{ avgLatencyDisplay }}</strong>
            <small>{{ throughputDisplay }}</small>
          </article>
          <article class="engine-mini-card is-mint">
            <span>保护余量</span>
            <strong>{{ privacyRemainingDisplay }}</strong>
            <small>{{ privacyQueriesDisplay }}</small>
          </article>
          <button
            type="button"
            class="engine-mini-card engine-mini-card--more"
            @click="loadConfig({ notify: true })"
          >
            <strong>配置</strong>
            <small>{{ configCardLabel }}</small>
          </button>
        </div>
      </article>

      <article class="edge-card edge-card--perf">
        <div class="edge-head">
          <div>
            <span class="edge-eyebrow">性能</span>
            <h3>性能概览</h3>
          </div>
        </div>

        <OpsMiniBars v-if="canShowPerformance" :items="performanceMetrics" />
        <p v-else class="edge-empty-copy">{{ performanceEmptyCopy }}</p>
      </article>

      <article class="edge-card edge-card--queue">
        <div class="edge-head">
          <div>
            <span class="edge-eyebrow">链路</span>
            <h3>节点与链路</h3>
          </div>
          <button type="button" class="text-action" @click="loadConfig({ notify: true })">
            查看配置
          </button>
        </div>

        <div v-if="engineRows.length" class="queue-list">
          <article v-for="row in engineRows" :key="row.id" class="queue-item">
            <div class="queue-item__avatar">
              {{ row.badge }}
            </div>
            <div class="queue-item__copy">
              <strong>{{ row.title }}</strong>
              <span>{{ row.meta }}</span>
            </div>
            <div class="queue-item__value" :class="row.tone">
              {{ row.value }}
            </div>
          </article>
        </div>
        <p v-else class="edge-empty-copy">{{ queueEmptyCopy }}</p>
      </article>

      <article class="edge-card edge-card--guide">
        <div class="edge-head">
          <div>
            <span class="edge-eyebrow">建议</span>
            <h3>预算建议</h3>
          </div>
        </div>

        <p class="guide-copy">
          {{ edgeGuideCopy }}
        </p>

        <button type="button" class="guide-button" @click="triggerAggregation">立即处理</button>
      </article>

      <article class="edge-card edge-card--chart">
        <div class="edge-head">
          <div>
            <span class="edge-eyebrow">情绪</span>
            <h3>情绪脉搏曲线</h3>
          </div>
          <span class="edge-chip">{{ lastUpdateDisplay }}</span>
        </div>

        <v-chart
          v-if="canShowEmotionChart"
          :option="emotionPulseLineOption"
          autoresize
          class="edge-chart"
          role="img"
          aria-label="情绪脉搏曲线"
        />
        <p v-else class="edge-empty-copy">{{ emotionEmptyCopy }}</p>
      </article>

      <article class="edge-card edge-card--score">
        <div class="edge-head">
          <div>
            <span class="edge-eyebrow">评分</span>
            <h3>引擎评分</h3>
          </div>
        </div>

        <template v-if="canShowHealthScore">
          <OpsGaugeMeter :value="edgeHealthScore" :max="100" :label="edgeHealthLabel" />

          <div class="score-meta">
            <span>缓存命中 {{ engineStatus.cacheHitRate }}%</span>
            <span>本轮协同 {{ federated.currentRound }}</span>
            <span>噪声级别 {{ privacyNoiseLabel }}</span>
          </div>
        </template>
        <p v-else class="edge-empty-copy">{{ healthScoreEmptyCopy }}</p>
      </article>
    </section>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted, onUnmounted } from 'vue'
import { ElMessage } from 'element-plus'
import { RefreshRight } from '@element-plus/icons-vue'
import VChart from 'vue-echarts'
import api from '@/api'
import dayjs from 'dayjs'
import { getErrorMessage } from '@/utils/errorHelper'
import { normalizePayloadRecord } from '@/utils/collectionPayload'
import OpsMiniBars from '@/components/OpsMiniBars.vue'
import OpsGaugeMeter from '@/components/OpsGaugeMeter.vue'
import { createSoftBaseline } from '@/utils/chartSignals'

const loading = ref(false)
const lastUpdateTime = ref('')
const edgeTimeFormat = 'MM月DD日 HH:mm'
const currentTime = ref(dayjs().format(edgeTimeFormat))

type EdgeLoaderKey = 'status' | 'metrics' | 'emotion' | 'federated' | 'privacy' | 'config'

const normalizeEdgePayload = (payload: unknown) => normalizePayloadRecord(payload)
let edgeMetricsRequest: Promise<Record<string, unknown>> | null = null
let edgeMetricsSnapshot: { at: number; data: Record<string, unknown> } | null = null

const fetchEdgeMetricsPayload = async () => {
  const now = Date.now()
  if (edgeMetricsSnapshot && now - edgeMetricsSnapshot.at < 1000) {
    return edgeMetricsSnapshot.data
  }
  if (edgeMetricsRequest) {
    return edgeMetricsRequest
  }

  edgeMetricsRequest = api.getEdgeAIMetrics().then(({ data }) => {
    const normalized = normalizeEdgePayload(data)
    edgeMetricsSnapshot = { at: Date.now(), data: normalized }
    return normalized
  }).finally(() => {
    edgeMetricsRequest = null
  })

  return edgeMetricsRequest
}

const edgeLoaderLabels: Record<EdgeLoaderKey, string> = {
  status: '引擎状态',
  metrics: '性能指标',
  emotion: '情绪脉搏',
  federated: '联邦状态',
  privacy: '隐私预算',
  config: '引擎配置',
}

const edgeLoaders = reactive<Record<EdgeLoaderKey, { loaded: boolean; error: string }>>({
  status: { loaded: false, error: '' },
  metrics: { loaded: false, error: '' },
  emotion: { loaded: false, error: '' },
  federated: { loaded: false, error: '' },
  privacy: { loaded: false, error: '' },
  config: { loaded: false, error: '' },
})

function markLoaderSuccess(key: EdgeLoaderKey) {
  edgeLoaders[key].loaded = true
  edgeLoaders[key].error = ''
}

function markLoaderFailure(key: EdgeLoaderKey, error: unknown, fallbackMessage: string) {
  edgeLoaders[key].error = getErrorMessage(error, fallbackMessage)
}

const edgeIssues = computed(() =>
  (Object.entries(edgeLoaderLabels) as Array<[EdgeLoaderKey, string]>).flatMap(([key, label]) => {
    const state = edgeLoaders[key]
    if (!state.error) return []
    return [
      {
        key,
        stale: state.loaded,
        summary: state.loaded
          ? `${label}刷新失败，当前显示上次成功结果。`
          : `${label}加载失败。`,
        detail: state.error,
      },
    ]
  }),
)

const hasBlockingIssue = computed(() => edgeIssues.value.some((issue) => !issue.stale))
const hasStaleIssue = computed(() => edgeIssues.value.some((issue) => issue.stale))
const edgeIssueTone = computed(() => (hasBlockingIssue.value ? 'is-error' : 'is-stale'))
const edgeIssueTitle = computed(() =>
  hasBlockingIssue.value ? '部分模块加载失败' : '部分模块显示旧数据',
)
const edgeIssueSummary = computed(() =>
  edgeIssues.value.map((issue) => `${issue.summary}${issue.detail}`).join('；'),
)

const engineStatus = reactive({
  enabled: false,
  modulesLoaded: 0,
  totalModules: 0,
  uptime: '0h',
  inferenceCount: 0,
  cacheHitRate: 0,
  avgLatency: 0,
  throughput: 0,
  activeNodes: 0,
})

const performanceMetrics = computed(() => [
  {
    label: '响应速度',
    value: Math.max(0, 100 - Math.round((engineStatus.avgLatency / 220) * 100)),
    display: `${engineStatus.avgLatency}ms`,
  },
  {
    label: '命中效率',
    value: Math.max(0, engineStatus.cacheHitRate),
    display: `${engineStatus.cacheHitRate}%`,
  },
  {
    label: '处理速度',
    value: Math.max(0, Math.min(100, Math.round((engineStatus.throughput / 500) * 100))),
    display: `${engineStatus.throughput} req/s`,
  },
  {
    label: '累计调用',
    value: Math.max(0, Math.min(100, Math.round((engineStatus.inferenceCount / 10000) * 100))),
    display: engineStatus.inferenceCount.toLocaleString(),
  },
])

const emotionPulse = reactive({
  temperature: 50,
  trend: 'stable',
  history: [] as number[],
  timestamps: [] as string[],
})

const emotionTrendLabel = computed(() => {
  const map: Record<string, string> = {
    positive: '回暖',
    neutral: '平稳',
    negative: '偏低',
    stable: '平稳',
  }
  return map[emotionPulse.trend] || emotionPulse.trend || '平稳'
})

const emotionPulseLineOption = computed(() => ({
  tooltip: {
    trigger: 'axis',
    backgroundColor: 'rgba(36, 49, 75, 0.92)',
    borderColor: 'rgba(232, 239, 255, 0.14)',
    borderWidth: 1,
    textStyle: { color: '#f4f7ff', fontSize: 12 },
    padding: [10, 12],
    extraCssText: 'box-shadow: 0 16px 34px rgba(43, 58, 94, 0.24); border-radius: 14px;',
  },
  grid: { top: 24, right: 18, bottom: 26, left: 40 },
  xAxis: {
    type: 'category',
    data: emotionPulse.timestamps,
    boundaryGap: false,
    axisLabel: { color: '#7c8baa', fontSize: 10 },
    axisLine: { show: false, lineStyle: { color: '#d4deef' } },
    axisTick: { show: false },
  },
  yAxis: {
    type: 'value',
    min: 0,
    max: 100,
    axisLabel: { color: '#7c8baa', fontSize: 10 },
    axisLine: { show: false },
    axisTick: { show: false },
    splitLine: { lineStyle: { color: 'rgba(141, 161, 206, 0.16)', type: 'dashed' } },
  },
  series: [
    {
      name: '情绪波峰',
      type: 'line',
      data: emotionPulse.history,
      smooth: 0.45,
      symbol: 'circle',
      showSymbol: false,
      symbolSize: 7,
      lineStyle: { color: '#8eaefd', width: 3.2 },
      itemStyle: { color: '#8eaefd' },
      areaStyle: {
        color: {
          type: 'linear',
          x: 0,
          y: 0,
          x2: 0,
          y2: 1,
          colorStops: [
            { offset: 0, color: 'rgba(142,174,253,0.38)' },
            { offset: 1, color: 'rgba(142,174,253,0.08)' },
          ],
        },
      },
    },
    {
      name: '情绪基线',
      type: 'line',
      data: createSoftBaseline(emotionPulse.history),
      smooth: 0.5,
      symbol: 'none',
      lineStyle: { color: '#283245', width: 2.5 },
      itemStyle: { color: '#283245' },
    },
  ],
}))

const federated = reactive({
  status: 'idle',
  currentRound: 0,
  participatingNodes: 0,
  modelAccuracy: '0%',
  convergenceRate: '0%',
  aggregationProgress: 0,
  aggregating: false,
})

const privacy = reactive({
  epsilonUsed: 0,
  epsilonTotal: 10,
  epsilonPercent: 0,
  epsilonRemaining: 10,
  deltaValue: '1e-5',
  noiseLevel: 'medium',
  queriesRemaining: 0,
  allocation: [] as Array<{ value: number; name: string; itemStyle: { color: string } }>,
})

const edgeNodes = ref<Array<Record<string, unknown>>>([])

const edgeConfig = reactive({
  inferenceEngine: 'onnx',
  cacheStrategy: 'lru',
  maxBatchSize: 32,
  cacheSizeMB: 256,
  maxEpsilon: 1.0,
  federatedInterval: 300,
  emotionModel: 'standard',
  vectorSearchEnabled: true,
  inferenceTimeout: 5000,
  cacheEnabled: true,
  cacheTTL: 3600,
  federatedEnabled: true,
  privacyEpsilon: 1.0,
})
const privacyNoiseLabel = computed(() => {
  const map: Record<string, string> = {
    low: '轻度扰动',
    medium: '中等扰动',
    high: '高强度扰动',
  }
  return map[privacy.noiseLevel] || privacy.noiseLevel || '--'
})

const engineEyebrow = computed(() => {
  if (hasBlockingIssue.value) return '部分异常'
  if (hasStaleIssue.value) return '显示旧数据'
  return engineStatus.enabled ? '已启用' : '已暂停'
})

const activeNodesDisplay = computed(() =>
  edgeLoaders.status.loaded ? String(engineStatus.activeNodes) : '--',
)

const avgLatencyDisplay = computed(() =>
  edgeLoaders.metrics.loaded ? `${engineStatus.avgLatency}ms` : '--',
)

const throughputDisplay = computed(() =>
  edgeLoaders.metrics.loaded ? `当前吞吐 ${engineStatus.throughput} req/s` : '当前吞吐 --',
)

const privacyRemainingDisplay = computed(() =>
  edgeLoaders.privacy.loaded ? `ε ${privacy.epsilonRemaining}` : 'ε --',
)

const privacyQueriesDisplay = computed(() =>
  edgeLoaders.privacy.loaded ? `剩余查询 ${privacy.queriesRemaining}` : '剩余查询 --',
)

const configCardLabel = computed(() => {
  if (edgeLoaders.config.error) {
    return edgeLoaders.config.loaded ? '配置陈旧' : '配置异常'
  }
  return '查看参数'
})

const lastUpdateDisplay = computed(() =>
  lastUpdateTime.value ? `最近完整刷新 ${lastUpdateTime.value}` : '尚未成功刷新',
)

const canShowPerformance = computed(() => edgeLoaders.metrics.loaded)
const performanceEmptyCopy = computed(() => {
  if (edgeLoaders.metrics.error) {
    return edgeLoaders.metrics.loaded ? '性能指标刷新失败，当前仅保留上次成功快照。' : '性能指标不可用。'
  }
  return '正在加载性能指标。'
})

const canShowEmotionChart = computed(() => edgeLoaders.emotion.loaded && emotionPulse.history.length > 0)
const emotionEmptyCopy = computed(() => {
  if (edgeLoaders.emotion.error) {
    return edgeLoaders.emotion.loaded ? '情绪脉搏刷新失败，当前仅保留摘要状态。' : '情绪脉搏不可用。'
  }
  if (edgeLoaders.emotion.loaded) {
    return '暂无情绪脉搏历史。'
  }
  return '正在加载情绪脉搏。'
})

const queueEmptyCopy = computed(() => {
  if (edgeLoaders.metrics.error) {
    return edgeLoaders.metrics.loaded ? '节点与链路刷新失败，当前无可展示的节点快照。' : '节点与链路数据不可用。'
  }
  if (edgeLoaders.metrics.loaded) {
    return '暂无节点数据。'
  }
  return '正在加载节点与链路。'
})

const canShowHealthScore = computed(
  () => edgeLoaders.status.loaded && edgeLoaders.metrics.loaded && edgeLoaders.privacy.loaded,
)

const healthScoreEmptyCopy = computed(() => {
  if (edgeIssues.value.length) {
    return hasBlockingIssue.value
      ? '缺少关键状态，暂时无法计算引擎评分。'
      : '评分依赖的数据已陈旧，请先完成一次成功刷新。'
  }
  return '正在计算引擎评分。'
})

const heroDescription = computed(() => {
  if (edgeIssues.value.length) {
    return edgeIssueSummary.value
  }

  const statusText = engineStatus.enabled
    ? `当前有 ${engineStatus.activeNodes} 个服务节点在线`
    : '当前辅助引擎未处于运行状态'

  return `${currentTime.value} 的陪伴链路以${emotionTrendLabel.value}为主，${statusText}，平均响应 ${engineStatus.avgLatency}ms，剩余保护预算 ε ${privacy.epsilonRemaining} / ${privacy.epsilonTotal}。`
})

const engineRows = computed(() => {
  return edgeNodes.value.slice(0, 5).map((node, index) => ({
    id: node.id ?? index,
    badge: String(node.name ?? node.node_id ?? `N${index + 1}`)
      .slice(0, 1)
      .toUpperCase(),
    title: node.name ?? node.node_id ?? `边缘节点 ${index + 1}`,
    meta: node.status ?? '未知状态',
    value: node.latency ? `${node.latency}ms` : `${node.load ?? node.qps ?? '--'}`,
    tone: node.status === 'running' || node.status === 'online' ? 'is-up' : 'is-neutral',
  }))
})

const edgeGuideCopy = computed(() => {
  if (hasBlockingIssue.value) {
    return '当前面板存在未恢复的模块错误，先处理加载失败项，再触发新的聚合任务。'
  }
  if (hasStaleIssue.value) {
    return '当前展示包含旧数据，先完成一次成功刷新，再决定是否继续放大自动化处理能力。'
  }
  if (privacy.epsilonPercent >= 80) {
    return '当前预算接近上限，建议先压低高频调用，再安排下一轮聚合，避免保护额度被快速吃空。'
  }
  if (!engineStatus.enabled) {
    return '当前辅助引擎未完全启动，先恢复节点可用性，再放大自动化处理能力。'
  }
  return '当前链路总体可控，优先盯住高延迟节点和缓存命中率，把自动辅助维持在稳定区间。'
})

const edgeHealthScore = computed(() => {
  let score = 92
  if (!engineStatus.enabled) score -= 24
  score -= Math.min(20, Math.round(engineStatus.avgLatency / 18))
  score -= Math.min(14, Math.max(0, 80 - engineStatus.cacheHitRate) / 4)
  score -= Math.min(18, Math.max(0, privacy.epsilonPercent - 60) / 2)
  return Math.max(0, Math.min(100, Math.round(score)))
})

const edgeHealthLabel = computed(() => {
  if (edgeHealthScore.value >= 82) return '平稳'
  if (edgeHealthScore.value >= 66) return '可控'
  return '需检修'
})

async function loadStatus() {
  try {
    const { data } = await api.getEdgeAIStatus()
    const s = normalizeEdgePayload(data)
    Object.assign(engineStatus, {
      enabled: s.enabled ?? s.status === 'running',
      modulesLoaded: s.modulesLoaded ?? s.modules_loaded ?? 0,
      totalModules: s.totalModules ?? s.total_modules ?? 0,
      uptime: s.uptime ?? '0h',
      inferenceCount: s.inferenceCount ?? s.inference_count ?? 0,
      cacheHitRate: s.cacheHitRate ?? s.cache_hit_rate ?? 0,
      avgLatency: s.avgLatency ?? s.avg_latency ?? 0,
      throughput: s.throughput ?? 0,
      activeNodes: s.activeNodes ?? s.active_nodes ?? 0,
    })
    markLoaderSuccess('status')
    return true
  } catch (error) {
    markLoaderFailure('status', error, '加载引擎状态失败')
    return false
  }
}

async function loadMetrics() {
  try {
    const m = await fetchEdgeMetricsPayload()
    if (m.avgLatency != null || m.avg_latency != null)
      engineStatus.avgLatency = Number(m.avgLatency ?? m.avg_latency) || 0
    if (m.cacheHitRate != null || m.cache_hit_rate != null)
      engineStatus.cacheHitRate = Number(m.cacheHitRate ?? m.cache_hit_rate) || 0
    if (m.throughput != null) engineStatus.throughput = Number(m.throughput) || 0
    if (m.inferenceCount != null || m.inference_count != null)
      engineStatus.inferenceCount = Number(m.inferenceCount ?? m.inference_count) || 0
    edgeNodes.value = Array.isArray(m.nodes) ? m.nodes : []
    markLoaderSuccess('metrics')
    return true
  } catch (error) {
    markLoaderFailure('metrics', error, '加载性能指标失败')
    return false
  }
}

async function loadEmotionPulse() {
  try {
    const { data } = await api.getEmotionPulse()
    const ep = normalizeEdgePayload(data)
    const avgScore = ep.avg_score ?? ep.avgScore ?? ep.temperature ?? ep.value ?? 0
    const normalizedTemp =
      avgScore <= 1 && avgScore >= -1 ? Math.round((avgScore + 1) * 50) : Math.round(avgScore)
    emotionPulse.temperature = Math.max(0, Math.min(100, normalizedTemp))
    emotionPulse.trend = ep.dominant_mood ?? ep.dominantMood ?? ep.trend ?? 'stable'

    if (Array.isArray(ep.history)) {
      const history = ep.history as Array<Record<string, unknown> | number | string>
      emotionPulse.history = history.map((item) => {
        if (typeof item === 'object' && item !== null) {
          const entry = item as Record<string, unknown>
          return Number(entry.value ?? item) || 0
        }
        return Number(item) || 0
      })
      emotionPulse.timestamps = history.map((item) => {
        if (typeof item === 'object' && item !== null) {
          const entry = item as Record<string, unknown>
          return String(entry.time ?? entry.timestamp ?? '')
        }
        return ''
      })
    } else if (ep.mood_distribution && typeof ep.mood_distribution === 'object') {
      const values = Object.values(ep.mood_distribution).map((value) => Number(value) || 0)
      const sum = values.reduce((acc, curr) => acc + curr, 0)
      emotionPulse.history = sum > 0 ? values.map((value) => Math.round((value / sum) * 100)) : []
      emotionPulse.timestamps = Object.keys(ep.mood_distribution)
    } else {
      emotionPulse.history = []
      emotionPulse.timestamps = []
    }
    markLoaderSuccess('emotion')
    return true
  } catch (error) {
    markLoaderFailure('emotion', error, '加载情绪脉搏失败')
    return false
  }
}

async function loadFederatedStatus() {
  try {
    const m = await fetchEdgeMetricsPayload()
    if (m.federated) {
      Object.assign(federated, {
        status: m.federated.status ?? 'idle',
        currentRound: m.federated.currentRound ?? m.federated.current_round ?? 0,
        participatingNodes: m.federated.participatingNodes ?? m.federated.participating_nodes ?? 0,
        modelAccuracy: m.federated.modelAccuracy ?? m.federated.model_accuracy ?? '0%',
        convergenceRate: m.federated.convergenceRate ?? m.federated.convergence_rate ?? '0%',
        aggregationProgress:
          m.federated.aggregationProgress ?? m.federated.aggregation_progress ?? 0,
      })
    } else {
      Object.assign(federated, {
        status: 'idle',
        currentRound: 0,
        participatingNodes: 0,
        modelAccuracy: '0%',
        convergenceRate: '0%',
        aggregationProgress: 0,
      })
    }
    markLoaderSuccess('federated')
    return true
  } catch (error) {
    markLoaderFailure('federated', error, '加载联邦状态失败')
    return false
  }
}

async function loadPrivacyBudget() {
  try {
    const { data } = await api.getPrivacyBudget()
    const p = normalizeEdgePayload(data)
    const epsilonTotal = Number(p.epsilonTotal ?? p.epsilon_total ?? p.total_budget ?? 10) || 10
    const epsilonUsed = Number(p.epsilonUsed ?? p.epsilon_used ?? p.consumed ?? 0) || 0
    const epsilonPercentRaw = p.epsilonPercent ?? p.epsilon_percent ?? p.utilization_percent
    const epsilonPercent =
      epsilonPercentRaw != null
        ? Number(epsilonPercentRaw) || 0
        : Math.min(100, Math.max(0, (epsilonUsed / epsilonTotal) * 100))

    Object.assign(privacy, {
      epsilonUsed,
      epsilonTotal,
      epsilonPercent: Number(epsilonPercent.toFixed(2)),
      epsilonRemaining: Number((epsilonTotal - epsilonUsed).toFixed(2)),
      deltaValue: p.deltaValue ?? p.delta_value ?? '1e-5',
      noiseLevel: p.noiseLevel ?? p.noise_level ?? 'medium',
      queriesRemaining: p.queriesRemaining ?? p.queries_remaining ?? p.query_count ?? 0,
    })

    privacy.allocation = Array.isArray(p.allocation)
      ? (p.allocation as Array<Record<string, unknown>>).map((item, index: number) => ({
          value: Number(item.value ?? item.epsilon) || 0,
          name: item.name ?? item.label ?? `模块 ${index + 1}`,
          itemStyle: { color: ['#315b6f', '#4d8f6b', '#b67a42', '#a35f5f', '#5d6d77', '#7c6975'][index % 6] },
        }))
      : []
    markLoaderSuccess('privacy')
    return true
  } catch (error) {
    markLoaderFailure('privacy', error, '加载隐私预算失败')
    return false
  }
}

async function loadConfig(options: { notify?: boolean } = {}) {
  try {
    const { data } = await api.getEdgeAIConfig()
    const c = normalizeEdgePayload(data)
    Object.assign(edgeConfig, {
      inferenceEngine: c.inferenceEngine ?? c.inference_engine ?? edgeConfig.inferenceEngine,
      cacheStrategy: c.cacheStrategy ?? c.cache_strategy ?? edgeConfig.cacheStrategy,
      maxBatchSize: c.maxBatchSize ?? c.max_batch_size ?? 32,
      cacheSizeMB: c.cacheSizeMB ?? c.cache_size_mb ?? edgeConfig.cacheSizeMB,
      maxEpsilon:
        c.maxEpsilon ??
        c.max_epsilon ??
        c.privacyEpsilon ??
        c.privacy_epsilon ??
        edgeConfig.maxEpsilon,
      federatedInterval:
        c.federatedInterval ?? c.federated_interval ?? edgeConfig.federatedInterval,
      emotionModel: c.emotionModel ?? c.emotion_model ?? 'standard',
      vectorSearchEnabled: c.vectorSearchEnabled ?? c.vector_search_enabled ?? true,
      inferenceTimeout: c.inferenceTimeout ?? c.inference_timeout ?? 5000,
      cacheEnabled: c.cacheEnabled ?? c.cache_enabled ?? true,
      cacheTTL: c.cacheTTL ?? c.cache_ttl ?? 3600,
      federatedEnabled: c.federatedEnabled ?? c.federated_enabled ?? true,
      privacyEpsilon: c.privacyEpsilon ?? c.privacy_epsilon ?? c.maxEpsilon ?? c.max_epsilon ?? 1.0,
    })
    markLoaderSuccess('config')
    if (options.notify) {
      ElMessage.success('配置已同步')
    }
    return true
  } catch (error) {
    markLoaderFailure('config', error, '加载引擎配置失败')
    if (options.notify) {
      ElMessage.error(getErrorMessage(error, '加载引擎配置失败'))
    }
    return false
  }
}

async function triggerAggregation() {
  federated.aggregating = true
  try {
    await api.triggerFederatedAggregation({ round: federated.currentRound + 1 })
    federated.status = 'aggregating'
    const refreshed = await loadFederatedStatus()
    if (!refreshed) {
      ElMessage.warning('聚合已触发，但联邦状态刷新失败，页面显示为旧数据')
      return
    }
    ElMessage.success('已触发聚合')
  } catch (error) {
    ElMessage.error(getErrorMessage(error, '触发聚合失败'))
  } finally {
    federated.aggregating = false
  }
}

async function refreshAll() {
  loading.value = true
  currentTime.value = dayjs().format(edgeTimeFormat)
  try {
    const results = await Promise.all([
      loadStatus(),
      loadMetrics(),
      loadEmotionPulse(),
      loadFederatedStatus(),
      loadPrivacyBudget(),
      loadConfig(),
    ])
    if (results.every(Boolean)) {
      lastUpdateTime.value = dayjs().format('HH:mm:ss')
    }
  } finally {
    loading.value = false
  }
}

const timerIds: ReturnType<typeof setInterval>[] = []

function addTimer(fn: () => void, interval: number): void {
  const id = setInterval(fn, interval)
  timerIds.push(id)
}

onMounted(() => {
  refreshAll()
  addTimer(() => {
    currentTime.value = dayjs().format(edgeTimeFormat)
  }, 60000)
  addTimer(() => {
    currentTime.value = dayjs().format(edgeTimeFormat)
    loadEmotionPulse()
    loadMetrics()
  }, 30000)
})

onUnmounted(() => {
  timerIds.forEach((id) => clearInterval(id))
  timerIds.length = 0
})
</script>

<style lang="scss" scoped>
.edge-workbench {
  --edge-gap: 18px;
  --edge-radius: 28px;

  padding-bottom: 6px;
  font-variant-numeric: tabular-nums;
}

.edge-grid {
  display: grid;
  grid-template-columns: minmax(0, 1.46fr) minmax(184px, 0.78fr) minmax(252px, 0.96fr);
  grid-template-rows: minmax(182px, 0.98fr) minmax(138px, 0.74fr) minmax(282px, 1fr);
  grid-template-areas:
    'engine perf queue'
    'engine guide queue'
    'chart chart score';
  gap: var(--edge-gap);
}

.edge-card {
  position: relative;
  overflow: hidden;
  min-height: 0;
  padding: 24px;
  border-radius: var(--edge-radius);
  border: 1px solid rgba(133, 156, 201, 0.12);
  background: linear-gradient(180deg, rgba(251, 253, 255, 0.98), rgba(241, 247, 255, 0.98));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 22px 42px rgba(104, 128, 173, 0.12);
}

.edge-card--engine {
  grid-area: engine;
  min-height: 418px;
}
.edge-card--perf {
  grid-area: perf;
  min-height: 182px;
  background: linear-gradient(180deg, rgba(215, 227, 255, 0.98), rgba(204, 218, 255, 0.98));
}
.edge-card--queue {
  grid-area: queue;
  min-height: 418px;
  background: linear-gradient(180deg, rgba(214, 237, 231, 0.98), rgba(204, 233, 226, 0.98));
}
.edge-card--guide {
  grid-area: guide;
  min-height: 138px;
  background: linear-gradient(180deg, rgba(221, 242, 236, 0.98), rgba(211, 238, 232, 0.98));
}
.edge-card--chart {
  grid-area: chart;
  min-height: 282px;
  background: linear-gradient(180deg, rgba(241, 247, 255, 0.98), rgba(230, 239, 255, 0.98));
}
.edge-card--score {
  grid-area: score;
  min-height: 282px;
  background: linear-gradient(180deg, rgba(250, 252, 255, 0.98), rgba(241, 246, 255, 0.98));
}

.edge-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 14px;

  h2,
  h3 {
    margin: 8px 0 0;
    color: var(--hl-ink);
    font-size: 21px;
    font-weight: 680;
    letter-spacing: -0.04em;
    line-height: 1.12;
  }

  h3 {
    font-size: 19px;
  }
}

.edge-eyebrow,
.edge-chip {
  display: inline-flex;
  align-items: center;
  min-height: 32px;
  padding: 0 12px;
  border-radius: 999px;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.06em;
}

.edge-eyebrow {
  background: rgba(255, 255, 255, 0.72);
  color: var(--hl-ink-soft);
  text-transform: uppercase;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.88);
}

.edge-eyebrow--warning {
  background: rgba(255, 240, 220, 0.88);
  color: #8d4f26;
}

.edge-chip {
  background: rgba(255, 255, 255, 0.8);
  color: var(--hl-ink);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.9);
}

.engine-total {
  margin-top: 18px;

  > span {
    display: block;
    color: var(--hl-ink-soft);
    font-size: 12px;
  }

  p {
    max-width: 24rem;
    margin-top: 10px;
    color: var(--hl-ink-soft);
    font-size: 13px;
    line-height: 1.7;
  }
}

.edge-status-banner {
  margin-top: 16px;
  padding: 14px 16px;
  border-radius: 18px;
  border: 1px solid rgba(163, 95, 95, 0.14);
  background: rgba(255, 247, 244, 0.9);

  strong {
    display: block;
    color: #8d4f26;
    font-size: 12px;
    font-weight: 700;
  }

  p {
    margin: 6px 0 0;
    color: #7a5a49;
    font-size: 12px;
    line-height: 1.6;
  }
}

.edge-status-banner.is-stale {
  border-color: rgba(182, 122, 66, 0.16);
  background: rgba(255, 249, 240, 0.9);
}

.edge-status-banner.is-error {
  border-color: rgba(163, 95, 95, 0.18);
  background: rgba(255, 244, 241, 0.92);
}

.engine-total__value {
  margin-top: 8px;
  color: var(--hl-ink);
  font-size: clamp(46px, 5.4vw, 58px);
  font-weight: 800;
  line-height: 1;
  letter-spacing: -0.05em;

  small {
    margin-left: 8px;
    color: #8da5db;
    font-size: 22px;
    font-weight: 700;
  }
}

.engine-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 18px;
}

.engine-action {
  min-width: 124px;
  height: 42px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 0 16px;
  border: none;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.88);
  color: var(--hl-ink);
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 12px 18px rgba(113, 138, 184, 0.08);
  transition: var(--m3-transition);
}

.engine-stack {
  display: grid;
  grid-template-columns: 1fr 1fr minmax(94px, 0.64fr);
  gap: 12px;
  align-items: end;
  margin-top: 28px;
}

.engine-mini-card {
  min-height: 134px;
  padding: 16px;
  border: none;
  border-radius: 24px;
  text-align: left;
  box-shadow: 0 18px 32px rgba(120, 146, 194, 0.16);

  span,
  small {
    display: block;
  }

  span {
    color: rgba(35, 45, 67, 0.74);
    font-size: 11px;
  }

  strong {
    display: block;
    margin-top: 24px;
    color: #1d2740;
    font-size: 20px;
    font-weight: 700;
  }

  small {
    margin-top: 8px;
    color: rgba(35, 45, 67, 0.62);
    font-size: 11px;
  }
}

.engine-mini-card.is-blue {
  background: linear-gradient(180deg, rgba(177, 204, 255, 0.92), rgba(166, 194, 255, 0.96));
}

.engine-mini-card.is-mint {
  background: linear-gradient(180deg, rgba(209, 241, 236, 0.92), rgba(196, 236, 229, 0.96));
}

.engine-mini-card--more {
  min-height: 138px;
  display: grid;
  align-content: center;
  justify-items: center;
  gap: 6px;
  background: #212121;
  box-shadow: 0 20px 34px rgba(33, 33, 33, 0.22);

  strong,
  small {
    display: block;
  }

  strong {
    color: #ffffff;
    font-size: 16px;
    font-weight: 700;
    letter-spacing: 0.08em;
  }

  small {
    color: rgba(255, 255, 255, 0.68);
    font-size: 10px;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }
}

.queue-list {
  display: grid;
  gap: 8px;
  margin-top: 16px;
}

.queue-item {
  display: grid;
  grid-template-columns: 40px minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
  padding: 11px 0;
  border-bottom: 1px solid rgba(122, 163, 157, 0.16);
}

.queue-item:last-child {
  border-bottom: none;
}

.queue-item__avatar {
  width: 40px;
  height: 40px;
  display: grid;
  place-items: center;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.78);
  color: var(--hl-ink);
  font-weight: 700;
}

.queue-item__copy {
  min-width: 0;

  strong {
    display: block;
    color: var(--hl-ink);
    font-size: 13px;
    font-weight: 700;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  span {
    display: block;
    margin-top: 4px;
    color: var(--hl-ink-soft);
    font-size: 11px;
  }
}

.queue-item__value {
  font-size: 13px;
  font-weight: 700;
}

.queue-item__value.is-up {
  color: #5aaf9d;
}
.queue-item__value.is-neutral {
  color: #6f7c96;
}

.text-action {
  border: none;
  background: transparent;
  color: var(--hl-ink-soft);
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
}

.edge-empty-copy {
  margin: 18px 0 0;
  color: var(--hl-ink-soft);
  font-size: 13px;
  line-height: 1.7;
}

.edge-card--guide::before,
.edge-card--guide::after {
  content: '';
  position: absolute;
  border-radius: 999px;
  border: 1px solid rgba(129, 157, 230, 0.26);
}

.edge-card--guide::before {
  width: 112px;
  height: 112px;
  right: 22px;
  bottom: 18px;
}

.edge-card--guide::after {
  width: 74px;
  height: 74px;
  right: 88px;
  bottom: 44px;
}

.guide-copy {
  max-width: 28ch;
  margin: 14px 0 0;
  color: var(--hl-ink);
  font-size: 14px;
  line-height: 1.65;
}

.guide-button {
  margin-top: 18px;
  height: 40px;
  padding: 0 18px;
  border: none;
  border-radius: 999px;
  background: linear-gradient(180deg, #8cb2ff, #789cf2);
  color: #ffffff;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 14px 26px rgba(120, 156, 242, 0.22);
}

.edge-chart {
  height: 234px;
  margin-top: 16px;
}

.score-meta {
  display: grid;
  gap: 6px;
  margin-top: 16px;
  text-align: center;

  span {
    color: var(--hl-ink-soft);
    font-size: 12px;
  }
}

@media (max-width: 1180px) and (min-width: 961px) {
  .edge-workbench {
    --edge-gap: 14px;
    --edge-radius: 24px;
  }

  .edge-grid {
    grid-template-columns: minmax(0, 1.38fr) minmax(152px, 0.72fr) minmax(218px, 0.88fr);
    grid-template-rows: minmax(154px, 0.96fr) minmax(118px, 0.74fr) minmax(236px, 1fr);
  }

  .edge-card {
    padding: 18px;
  }

  .edge-head {
    gap: 10px;

    h2,
    h3 {
      font-size: 18px;
    }

    h3 {
      font-size: 16px;
    }
  }

  .edge-eyebrow,
  .edge-chip {
    min-height: 28px;
    padding: 0 10px;
    font-size: 9px;
  }

  .engine-total {
    margin-top: 14px;

    > span,
    p {
      font-size: 11px;
    }
  }

  .engine-total__value {
    margin-top: 6px;
    font-size: clamp(40px, 5vw, 50px);

    small {
      font-size: 18px;
    }
  }

  .engine-actions {
    margin-top: 14px;
    gap: 8px;
  }

  .engine-action {
    min-width: 116px;
    height: 38px;
    padding: 0 12px;
    font-size: 11px;
  }

  .engine-stack {
    grid-template-columns: 1fr 1fr minmax(82px, 0.62fr);
    gap: 10px;
    margin-top: 20px;
  }

  .engine-mini-card {
    min-height: 116px;
    padding: 14px;
    border-radius: 20px;

    span {
      font-size: 10px;
    }

    strong {
      margin-top: 18px;
      font-size: 18px;
    }

    small {
      font-size: 10px;
    }
  }

  .engine-mini-card--more {
    min-height: 120px;
  }

  .queue-list {
    margin-top: 14px;
  }

  .queue-item {
    grid-template-columns: 34px minmax(0, 1fr) auto;
    gap: 10px;
    padding: 10px 0;
  }

  .queue-item__avatar {
    width: 34px;
    height: 34px;
    font-size: 11px;
  }

  .queue-item__copy {
    strong {
      font-size: 12px;
    }

    span {
      font-size: 10px;
    }
  }

  .queue-item__value {
    font-size: 11px;
  }

  .guide-copy {
    margin-top: 12px;
    font-size: 13px;
  }

  .guide-button {
    margin-top: 14px;
    height: 38px;
    padding: 0 16px;
    font-size: 11px;
  }

  .edge-card--guide::before {
    width: 84px;
    height: 84px;
    right: 16px;
    bottom: 14px;
  }

  .edge-card--guide::after {
    width: 56px;
    height: 56px;
    right: 60px;
    bottom: 36px;
  }

  .edge-chart {
    height: 200px;
    margin-top: 14px;
  }

  .score-meta {
    margin-top: 12px;
  }

  .edge-card--score :deep(.ops-gauge-meter__content strong) {
    font-size: clamp(30px, 3vw, 40px);
  }
}

@media (max-width: 960px) and (min-width: 761px) {
  .edge-grid {
    grid-template-columns: minmax(0, 1.18fr) minmax(0, 0.82fr);
    grid-template-rows: none;
    grid-template-areas:
      'engine engine'
      'perf guide'
      'queue queue'
      'chart chart'
      'score score';
  }
}

@media (max-width: 760px) {
  .edge-grid {
    grid-template-columns: 1fr;
    grid-template-rows: none;
    grid-template-areas:
      'engine'
      'perf'
      'guide'
      'queue'
      'chart'
      'score';
  }

  .engine-stack {
    grid-template-columns: 1fr;
  }

  .engine-mini-card--more {
    min-height: 76px;
  }
}
</style>
