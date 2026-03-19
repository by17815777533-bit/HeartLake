<!--
  智能辅助页面

  统一到新的管理台语言：
  - 叙事型页首 + 指标带，弱化旧的技术大屏感
  - 为性能、联动、检索、设置提供一致的雾面容器
  - 让“智能辅助”更像值守与陪伴工具，而不是开发演示页
-->

<template>
  <div class="edge-ai ops-page">
    <OpsPageHero
      eyebrow="陪伴引擎"
      title="智能辅助"
      :description="heroDescription"
      :status="engineStatus.enabled ? '陪伴中' : '需检查'"
      :chips="heroChips"
    >
      <template #actions>
        <el-button
          type="primary"
          :icon="RefreshRight"
          :loading="loading"
          @click="refreshAll"
        >
          刷新状态
        </el-button>
      </template>
    </OpsPageHero>

    <OpsMetricStrip :items="summaryItems" />

    <OpsSignalDeck :items="edgeSignals" />

    <PerformanceSection
      :performance-metrics="performanceMetrics"
      :emotion-gauge-option="emotionGaugeOption"
      :emotion-pulse-line-option="emotionPulseLineOption"
    />

    <FederatedPrivacySection
      :federated="federated"
      :privacy="privacy"
      :privacy-budget-color="privacyBudgetColor"
      :privacy-pie-option="privacyPieOption"
      @trigger-aggregation="triggerAggregation"
      @refresh-federated="loadFederatedStatus"
    />

    <VectorNodesSection
      v-model:vector-query="vectorQuery"
      :vector-searching="vectorSearching"
      :vector-searched="vectorSearched"
      :vector-results="vectorResults"
      :edge-nodes="edgeNodes"
      @search="doVectorSearch"
    />

    <RAGSystemSection :rag-stats="ragStats" />

    <ConfigSection
      :edge-config="edgeConfig"
      :config-loading="configLoading"
      :config-saving="configSaving"
      @save="saveConfig"
      @reset="loadConfig"
    />

    <AIToolboxSection
      :sentiment-tool="sentimentTool"
      :moderation-tool="moderationTool"
      @analyze-sentiment="doSentimentAnalysis"
      @moderate-content="doContentModeration"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted, onUnmounted } from 'vue'
import { ElMessage } from 'element-plus'
import { RefreshRight } from '@element-plus/icons-vue'
import api from '@/api'
import dayjs from 'dayjs'
import OpsPageHero from '@/components/OpsPageHero.vue'
import OpsMetricStrip from '@/components/OpsMetricStrip.vue'
import OpsSignalDeck from '@/components/OpsSignalDeck.vue'
import { getErrorMessage } from '@/utils/errorHelper'
import PerformanceSection from './edge-ai/PerformanceSection.vue'
import FederatedPrivacySection from './edge-ai/FederatedPrivacySection.vue'
import VectorNodesSection from './edge-ai/VectorNodesSection.vue'
import RAGSystemSection from './edge-ai/RAGSystemSection.vue'
import ConfigSection from './edge-ai/ConfigSection.vue'
import AIToolboxSection from './edge-ai/AIToolboxSection.vue'

const loading = ref(false)
const lastUpdateTime = ref(dayjs().format('HH:mm:ss'))
const edgeTimeFormat = 'MM月DD日 HH:mm'
const currentTime = ref(dayjs().format(edgeTimeFormat))

const techBadges = [
  { icon: '建议', label: '内容建议' },
  { icon: '检查', label: '内容检查' },
  { icon: '情绪', label: '情绪观察' },
  { icon: '参考', label: '历史参考' },
  { icon: '整理', label: '自动整理' },
  { icon: '支持', label: '处理支持' },
]

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
    value: `${engineStatus.avgLatency}ms`,
    percent: Math.min(100, (engineStatus.avgLatency / 200) * 100),
    color: engineStatus.avgLatency < 50 ? '#4d8f6b' : engineStatus.avgLatency < 100 ? '#b67a42' : '#a35f5f',
  },
  {
    label: '命中效率',
    value: `${engineStatus.cacheHitRate}%`,
    percent: engineStatus.cacheHitRate,
    color: engineStatus.cacheHitRate > 80 ? '#4d8f6b' : engineStatus.cacheHitRate > 50 ? '#b67a42' : '#a35f5f',
  },
  {
    label: '处理速度',
    value: `${engineStatus.throughput} req/s`,
    percent: Math.min(100, (engineStatus.throughput / 500) * 100),
    color: '#315b6f',
  },
  {
    label: '累计调用',
    value: engineStatus.inferenceCount.toLocaleString(),
    percent: Math.min(100, (engineStatus.inferenceCount / 10000) * 100),
    color: '#5d6d77',
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

const emotionGaugeOption = computed(() => ({
  series: [{
    type: 'gauge',
    center: ['50%', '58%'],
    radius: '92%',
    startAngle: 210,
    endAngle: -30,
    min: 0,
    max: 100,
    splitNumber: 8,
    axisLine: {
      lineStyle: {
        width: 18,
        color: [
          [0.25, '#6f9dab'],
          [0.5, '#4d8f6b'],
          [0.75, '#d09a54'],
          [1, '#a35f5f'],
        ],
      },
    },
    pointer: {
      icon: 'path://M10 0 L20 42 L0 42 Z',
      length: '54%',
      width: 10,
      offsetCenter: [0, '-10%'],
      itemStyle: { color: '#1f3942' },
    },
    anchor: {
      show: true,
      size: 14,
      itemStyle: { color: '#f6fafb', borderColor: '#1f3942', borderWidth: 3 },
    },
    axisTick: { length: 7, lineStyle: { color: 'auto', width: 1.4 } },
    splitLine: { length: 14, lineStyle: { color: 'auto', width: 2.4 } },
    axisLabel: { color: '#5f7882', fontSize: 10, distance: -42 },
    title: { offsetCenter: [0, '22%'], fontSize: 13, color: '#5f7882' },
    detail: {
      fontSize: 30,
      offsetCenter: [0, '48%'],
      valueAnimation: true,
      color: '#213840',
      formatter: '{value}°',
    },
    data: [{ value: emotionPulse.temperature, name: '情绪温度' }],
  }],
}))

const emotionPulseLineOption = computed(() => ({
  tooltip: {
    trigger: 'axis',
    backgroundColor: 'rgba(15, 28, 34, 0.92)',
    borderColor: 'rgba(208, 221, 226, 0.16)',
    borderWidth: 1,
    textStyle: { color: '#edf5f7', fontSize: 12 },
    padding: [10, 12],
    extraCssText: 'box-shadow: 0 16px 34px rgba(2, 10, 14, 0.22); border-radius: 14px;',
  },
  grid: { top: 24, right: 20, bottom: 28, left: 42 },
  xAxis: {
    type: 'category',
    data: emotionPulse.timestamps,
    boundaryGap: false,
    axisLabel: { color: '#5f7882', fontSize: 10 },
    axisLine: { lineStyle: { color: '#b8c7cd' } },
  },
  yAxis: {
    type: 'value',
    min: 0,
    max: 100,
    axisLabel: { color: '#5f7882', fontSize: 10 },
    splitLine: { lineStyle: { color: 'rgba(89, 118, 129, 0.12)' } },
  },
  series: [{
    type: 'line',
    data: emotionPulse.history,
    smooth: true,
    symbol: 'circle',
    showSymbol: false,
    symbolSize: 7,
    lineStyle: { color: '#315b6f', width: 2.6 },
    itemStyle: { color: '#315b6f' },
    areaStyle: {
      color: {
        type: 'linear',
        x: 0,
        y: 0,
        x2: 0,
        y2: 1,
        colorStops: [
          { offset: 0, color: 'rgba(49,91,111,0.26)' },
          { offset: 1, color: 'rgba(49,91,111,0.02)' },
        ],
      },
    },
  }],
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

const privacyBudgetColor = computed(() => {
  if (privacy.epsilonPercent < 50) return '#4d8f6b'
  if (privacy.epsilonPercent < 80) return '#b67a42'
  return '#a35f5f'
})

const privacyPieOption = computed(() => ({
  tooltip: {
    trigger: 'item',
    backgroundColor: 'rgba(15, 28, 34, 0.92)',
    borderColor: 'rgba(208, 221, 226, 0.16)',
    borderWidth: 1,
    textStyle: { color: '#edf5f7', fontSize: 12 },
    padding: [10, 12],
    formatter: '{b}: ε={c} ({d}%)',
  },
  legend: {
    bottom: 0,
    icon: 'circle',
    itemGap: 16,
    textStyle: { color: '#5f7882', fontSize: 11 },
  },
  series: [{
    type: 'pie',
    radius: ['50%', '74%'],
    center: ['50%', '42%'],
    avoidLabelOverlap: true,
    itemStyle: { borderRadius: 12, borderColor: '#f8fbfc', borderWidth: 3 },
    label: { show: false },
    emphasis: { label: { show: true, fontSize: 13, fontWeight: 'bold' } },
    data: privacy.allocation.length > 0
      ? privacy.allocation
      : [
          { value: 3, name: '情绪判断', itemStyle: { color: '#315b6f' } },
          { value: 2, name: '内容检查', itemStyle: { color: '#4d8f6b' } },
          { value: 2.5, name: '智能回复', itemStyle: { color: '#b67a42' } },
          { value: 1.5, name: '内容检索', itemStyle: { color: '#a35f5f' } },
          { value: 1, name: '预留', itemStyle: { color: '#5d6d77' } },
        ],
  }],
}))

const vectorSearch = reactive({
  query: '',
  searching: false,
  results: [] as Array<Record<string, unknown>>,
})
const vectorSearched = ref(false)
const vectorQuery = computed({
  get: () => vectorSearch.query,
  set: (value: string) => { vectorSearch.query = value },
})
const vectorSearching = computed(() => vectorSearch.searching)
const vectorResults = computed(() => vectorSearch.results)

const edgeNodes = ref<Array<Record<string, any>>>([])
const ragStats = ref<Record<string, any>>({})

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
const configLoading = ref(false)
const configSaving = ref(false)

const sentimentTool = reactive({ text: '', loading: false, result: null as null | Record<string, any> })
const moderationTool = reactive({ text: '', loading: false, result: null as null | Record<string, any> })

const formatCount = (value: number) => value.toLocaleString()
const privacyNoiseLabel = computed(() => {
  const map: Record<string, string> = {
    low: '轻度扰动',
    medium: '中等扰动',
    high: '高强度扰动',
  }
  return map[privacy.noiseLevel] || privacy.noiseLevel || '中等扰动'
})

const heroDescription = computed(() => {
  const statusText = engineStatus.enabled
    ? `当前有 ${engineStatus.activeNodes} 个服务节点在线`
    : '当前辅助引擎未处于运行状态'

  return `${currentTime.value} 的陪伴链路以${emotionTrendLabel.value}为主，${statusText}，平均响应 ${engineStatus.avgLatency}ms，保护额度剩余 ${privacy.epsilonRemaining}。`
})

const heroChips = computed(() => [
  ...techBadges.map(({ label }) => label),
  `情绪水温 ${emotionPulse.temperature}°`,
].slice(0, 5))

const summaryItems = computed(() => [
  {
    label: '服务状态',
    value: engineStatus.enabled ? '运行中' : '已停止',
    note: `已加载 ${engineStatus.modulesLoaded}/${engineStatus.totalModules} 个功能`,
    tone: 'lake' as const,
  },
  {
    label: '平均响应',
    value: `${engineStatus.avgLatency}ms`,
    note: `当前吞吐 ${engineStatus.throughput} req/s`,
    tone: 'amber' as const,
  },
  {
    label: '协同节点',
    value: formatCount(federated.participatingNodes),
    note: `第 ${federated.currentRound} 轮 · 准确度 ${federated.modelAccuracy}`,
    tone: 'sage' as const,
  },
  {
    label: '保护余量',
    value: `${privacy.epsilonRemaining}`,
    note: `已使用 ${privacy.epsilonPercent}% · ${privacyNoiseLabel.value}`,
    tone: 'rose' as const,
  },
])

const edgeSignals = computed(() => [
  {
    label: '最后巡看',
    value: lastUpdateTime.value,
    note: `${currentTime.value} 已同步关键面板`,
    badge: '巡看完成',
    tone: 'lake' as const,
  },
  {
    label: '情绪水温',
    value: `${emotionPulse.temperature}°`,
    note: `${emotionTrendLabel.value} · 最近波动持续观察中`,
    badge: '每 30 秒更新',
    tone: 'amber' as const,
  },
  {
    label: '在线节点',
    value: formatCount(engineStatus.activeNodes),
    note: `累计调用 ${formatCount(engineStatus.inferenceCount)} 次`,
    badge: engineStatus.enabled ? '引擎运行中' : '需要检查',
    tone: 'sage' as const,
  },
])

async function loadStatus() {
  try {
    const { data } = await api.getEdgeAIStatus()
    const s = data.data || data
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
  } catch {
    // 静默降级
  }
}

async function loadMetrics() {
  try {
    const { data } = await api.getEdgeAIMetrics()
    const m = data.data || data
    if (m.avgLatency != null || m.avg_latency != null) engineStatus.avgLatency = Number(m.avgLatency ?? m.avg_latency) || 0
    if (m.cacheHitRate != null || m.cache_hit_rate != null) engineStatus.cacheHitRate = Number(m.cacheHitRate ?? m.cache_hit_rate) || 0
    if (m.throughput != null) engineStatus.throughput = Number(m.throughput) || 0
    if (m.inferenceCount != null || m.inference_count != null) engineStatus.inferenceCount = Number(m.inferenceCount ?? m.inference_count) || 0
    if (Array.isArray(m.nodes)) {
      edgeNodes.value = m.nodes
    }
    if (m.dual_memory_rag) {
      ragStats.value = m.dual_memory_rag
    }
  } catch {
    // 静默降级
  }
}

async function loadEmotionPulse() {
  try {
    const { data } = await api.getEmotionPulse()
    const ep = data.data || data
    const avgScore = ep.avg_score ?? ep.avgScore ?? ep.temperature ?? ep.value ?? 0
    const normalizedTemp = avgScore <= 1 && avgScore >= -1
      ? Math.round((avgScore + 1) * 50)
      : Math.round(avgScore)
    emotionPulse.temperature = Math.max(0, Math.min(100, normalizedTemp))
    emotionPulse.trend = ep.dominant_mood ?? ep.dominantMood ?? ep.trend ?? 'stable'

    if (Array.isArray(ep.history)) {
      emotionPulse.history = ep.history.map((item: any) => Number(item.value ?? item) || 0)
      emotionPulse.timestamps = ep.history.map((item: any) => item.time ?? item.timestamp ?? '')
    } else if (ep.mood_distribution && typeof ep.mood_distribution === 'object') {
      const values = Object.values(ep.mood_distribution).map((value) => Number(value) || 0)
      const sum = values.reduce((acc, curr) => acc + curr, 0)
      emotionPulse.history = sum > 0 ? values.map((value) => Math.round((value / sum) * 100)) : []
      emotionPulse.timestamps = Object.keys(ep.mood_distribution)
    }
  } catch {
    // 静默降级
  }
}

async function loadFederatedStatus() {
  try {
    const { data } = await api.getEdgeAIMetrics()
    const m = data.data || data
    if (m.federated) {
      Object.assign(federated, {
        status: m.federated.status ?? 'idle',
        currentRound: m.federated.currentRound ?? m.federated.current_round ?? 0,
        participatingNodes: m.federated.participatingNodes ?? m.federated.participating_nodes ?? 0,
        modelAccuracy: m.federated.modelAccuracy ?? m.federated.model_accuracy ?? '0%',
        convergenceRate: m.federated.convergenceRate ?? m.federated.convergence_rate ?? '0%',
        aggregationProgress: m.federated.aggregationProgress ?? m.federated.aggregation_progress ?? 0,
      })
    }
  } catch {
    // 静默降级
  }
}

async function loadPrivacyBudget() {
  try {
    const { data } = await api.getPrivacyBudget()
    const p = data.data || data
    const epsilonTotal = Number(p.epsilonTotal ?? p.epsilon_total ?? p.total_budget ?? 10) || 10
    const epsilonUsed = Number(p.epsilonUsed ?? p.epsilon_used ?? p.consumed ?? 0) || 0
    const epsilonPercentRaw = p.epsilonPercent ?? p.epsilon_percent ?? p.utilization_percent
    const epsilonPercent = epsilonPercentRaw != null
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

    if (Array.isArray(p.allocation)) {
      const colors = ['#315b6f', '#4d8f6b', '#b67a42', '#a35f5f', '#5d6d77', '#7c6975']
      privacy.allocation = p.allocation.map((item: any, index: number) => ({
        value: Number(item.value ?? item.epsilon) || 0,
        name: item.name ?? item.label ?? `模块 ${index + 1}`,
        itemStyle: { color: colors[index % colors.length] },
      }))
    }
  } catch {
    // 静默降级
  }
}

async function loadConfig() {
  configLoading.value = true
  try {
    const { data } = await api.getEdgeAIConfig()
    const c = data.data || data
    Object.assign(edgeConfig, {
      inferenceEngine: c.inferenceEngine ?? c.inference_engine ?? edgeConfig.inferenceEngine,
      cacheStrategy: c.cacheStrategy ?? c.cache_strategy ?? edgeConfig.cacheStrategy,
      maxBatchSize: c.maxBatchSize ?? c.max_batch_size ?? 32,
      cacheSizeMB: c.cacheSizeMB ?? c.cache_size_mb ?? edgeConfig.cacheSizeMB,
      maxEpsilon: c.maxEpsilon ?? c.max_epsilon ?? c.privacyEpsilon ?? c.privacy_epsilon ?? edgeConfig.maxEpsilon,
      federatedInterval: c.federatedInterval ?? c.federated_interval ?? edgeConfig.federatedInterval,
      emotionModel: c.emotionModel ?? c.emotion_model ?? 'standard',
      vectorSearchEnabled: c.vectorSearchEnabled ?? c.vector_search_enabled ?? true,
      inferenceTimeout: c.inferenceTimeout ?? c.inference_timeout ?? 5000,
      cacheEnabled: c.cacheEnabled ?? c.cache_enabled ?? true,
      cacheTTL: c.cacheTTL ?? c.cache_ttl ?? 3600,
      federatedEnabled: c.federatedEnabled ?? c.federated_enabled ?? true,
      privacyEpsilon: c.privacyEpsilon ?? c.privacy_epsilon ?? c.maxEpsilon ?? c.max_epsilon ?? 1.0,
    })
  } catch {
    // 静默降级
  } finally {
    configLoading.value = false
  }
}

async function triggerAggregation() {
  federated.aggregating = true
  try {
    await api.triggerFederatedAggregation({ round: federated.currentRound + 1 })
    federated.status = 'aggregating'
    await loadFederatedStatus()
  } catch {
    // 静默降级
  } finally {
    federated.aggregating = false
  }
}

async function doSentimentAnalysis() {
  if (!sentimentTool.text.trim()) return
  sentimentTool.loading = true
  sentimentTool.result = null
  try {
    const { data } = await api.analyzeText(sentimentTool.text)
    const result = data.data || data
    sentimentTool.result = {
      score: result.score ?? result.sentiment_score ?? 0,
      sentiment: result.emotion ?? result.sentiment ?? 'neutral',
      confidence: result.confidence ?? result.conf ?? 0,
      emotions: result.emotions ?? [],
    }
  } catch {
    sentimentTool.result = null
  } finally {
    sentimentTool.loading = false
  }
}

async function doContentModeration() {
  if (!moderationTool.text.trim()) return
  moderationTool.loading = true
  moderationTool.result = null
  try {
    const { data } = await api.moderateText(moderationTool.text)
    const result = data.data || data
    moderationTool.result = {
      pass: result.pass ?? result.passed ?? result.approved ?? true,
      risk: result.risk ?? result.risk_level ?? 'low',
      reason: result.reason ?? result.reject_reason ?? '',
    }
  } catch {
    moderationTool.result = null
  } finally {
    moderationTool.loading = false
  }
}

async function doVectorSearch() {
  if (!vectorSearch.query.trim()) return
  vectorSearched.value = false
  vectorSearch.searching = true
  try {
    const { data } = await api.edgeAIVectorSearch({
      query: vectorSearch.query,
      topK: 10,
    })
    const result = data.data || data
    const raw = Array.isArray(result) ? result : (result.results ?? [])
    vectorSearch.results = raw.map((item: any) => ({
      ...item,
      score: Number(item.score ?? item.similarity ?? 0),
      content: item.content ?? item.text ?? item.id ?? '未知内容',
    }))
  } catch {
    vectorSearch.results = []
  } finally {
    vectorSearched.value = true
    vectorSearch.searching = false
  }
}

async function saveConfig() {
  configSaving.value = true
  try {
    await api.updateEdgeAIConfig(edgeConfig)
    ElMessage.success('配置保存成功')
  } catch (error) {
    ElMessage.error(getErrorMessage(error, '保存配置失败'))
  } finally {
    configSaving.value = false
  }
}

async function refreshAll() {
  loading.value = true
  currentTime.value = dayjs().format(edgeTimeFormat)
  try {
    await Promise.allSettled([
      loadStatus(),
      loadMetrics(),
      loadEmotionPulse(),
      loadFederatedStatus(),
      loadPrivacyBudget(),
      loadConfig(),
    ])
    lastUpdateTime.value = dayjs().format('HH:mm:ss')
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
.edge-ai {
  min-height: 100%;
  padding: 8px 0 22px;
}

.edge-ai :deep(.charts-row),
.edge-ai :deep(.section-row) {
  margin-bottom: 22px;
}

.edge-ai :deep(.chart-card),
.edge-ai :deep(.module-card) {
  border-radius: 28px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background:
    linear-gradient(180deg, rgba(254, 253, 251, 0.94), rgba(242, 247, 248, 0.96)),
    radial-gradient(circle at right top, rgba(123, 160, 173, 0.12), transparent 28%);
  box-shadow: 0 22px 46px rgba(10, 23, 31, 0.06);
  overflow: hidden;
}

.edge-ai :deep(.chart-card .el-card__header),
.edge-ai :deep(.module-card .el-card__header) {
  padding: 18px 22px 14px;
  border-bottom: 1px solid rgba(24, 36, 47, 0.07);
  background: rgba(255, 255, 255, 0.36);
}

.edge-ai :deep(.chart-card .el-card__body),
.edge-ai :deep(.module-card .el-card__body) {
  padding: 22px;
}

.edge-ai :deep(.card-header) {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  color: var(--hl-ink);
  font-size: 16px;
  font-weight: 600;
}

.edge-ai :deep(.chart-card:hover),
.edge-ai :deep(.module-card:hover) {
  border-color: rgba(17, 62, 74, 0.16);
}

.edge-ai :deep(.metrics-grid) {
  gap: 14px;
}

.edge-ai :deep(.metric-item),
.edge-ai :deep(.fed-stat-item),
.edge-ai :deep(.rag-stat-item),
.edge-ai :deep(.search-result-item) {
  border-radius: 20px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.66);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.4);
}

.edge-ai :deep(.metric-item) {
  padding: 16px 14px;
}

.edge-ai :deep(.metric-value),
.edge-ai :deep(.fed-stat-value),
.edge-ai :deep(.rag-stat-value) {
  color: var(--hl-ink);
  font-family: var(--hl-font-display);
}

.edge-ai :deep(.metric-label),
.edge-ai :deep(.fed-stat-label),
.edge-ai :deep(.rag-stat-label),
.edge-ai :deep(.rag-stat-desc),
.edge-ai :deep(.budget-hint),
.edge-ai :deep(.result-label) {
  color: var(--hl-ink-soft);
}

.edge-ai :deep(.emotion-pulse-container) {
  gap: 18px;
  min-height: 260px;
}

.edge-ai :deep(.fed-actions) {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  margin-top: 18px;
}

.edge-ai :deep(.fed-progress-label) {
  display: inline-block;
  margin-bottom: 10px;
  color: var(--hl-ink-soft);
}

.edge-ai :deep(.budget-main .epsilon) {
  margin-bottom: 14px;
  font-family: var(--hl-font-display);
  font-size: clamp(28px, 3vw, 34px);
  color: var(--hl-ink);
}

.edge-ai :deep(.search-results) {
  display: grid;
  gap: 12px;
  margin-top: 18px;
}

.edge-ai :deep(.search-result-item) {
  display: grid;
  grid-template-columns: auto 1fr;
  gap: 14px;
  align-items: start;
  padding: 14px;
}

.edge-ai :deep(.result-content),
.edge-ai :deep(.reason-text) {
  color: var(--hl-ink);
  line-height: 1.7;
}

.edge-ai :deep(.config-form) {
  padding-top: 4px;
}

.edge-ai :deep(.config-actions) {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  padding-top: 10px;
}

.edge-ai :deep(.ai-tool-panel) {
  display: grid;
  gap: 14px;
}

.edge-ai :deep(.tool-result) {
  display: grid;
  gap: 12px;
  margin-top: 6px;
  padding: 16px;
  border-radius: 20px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.62);
}

.edge-ai :deep(.result-item) {
  display: grid;
  gap: 8px;
}

.edge-ai :deep(.emotion-tags) {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.edge-ai :deep(.el-input__wrapper),
.edge-ai :deep(.el-textarea__inner),
.edge-ai :deep(.el-select__wrapper),
.edge-ai :deep(.el-input-number),
.edge-ai :deep(.el-input-number .el-input__wrapper) {
  border-radius: 16px;
  box-shadow: 0 0 0 1px rgba(123, 149, 160, 0.16) inset;
  background: rgba(255, 255, 255, 0.82);
}

.edge-ai :deep(.el-form-item__label) {
  color: var(--hl-ink-soft);
  font-weight: 600;
}

.edge-ai :deep(.el-table) {
  --el-table-header-bg-color: rgba(245, 248, 249, 0.92);
  --el-table-row-hover-bg-color: rgba(241, 246, 247, 0.92);
  --el-table-border-color: rgba(123, 149, 160, 0.12);
  border-radius: 18px;
  overflow: hidden;
}

.edge-ai :deep(.el-tag) {
  border-radius: 999px;
}

.edge-ai :deep(.el-progress-bar__outer) {
  background: rgba(24, 36, 47, 0.08);
}

@media (max-width: 900px) {
  .edge-ai-ribbon {
    grid-template-columns: 1fr;
  }

  .edge-ai :deep(.emotion-pulse-container) {
    flex-direction: column;
    height: auto;
  }

  .edge-ai :deep(.gauge-wrapper),
  .edge-ai :deep(.pulse-line-wrapper) {
    min-height: 220px;
  }
}
</style>
