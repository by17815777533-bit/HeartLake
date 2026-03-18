<!--
  心湖智能（Edge AI 管理）页面 -- 状态监控 + 性能指标 + 联邦学习 + 隐私预算

  组件结构（7 个子组件位于 views/edge-ai/ 目录下）：
  - EdgeAIHeader: 页面头部、技术标签、状态卡片
  - PerformanceSection: 性能指标仪表盘、情绪脉搏图表
  - FederatedPrivacySection: 联邦学习控制面板、隐私预算可视化
  - VectorNodesSection: HNSW 向量搜索工具、边缘节点列表
  - RAGSystemSection: 双记忆 RAG 系统指标展示
  - ConfigSection: 引擎配置管理表单
  - AIToolboxSection: 情感分析/内容审核在线调试工具

  本文件作为子组件的协调层，保留所有响应式状态和数据加载逻辑。
  定时器统一由 timerIds 数组管理，onUnmounted 时批量清理。
-->

<template>
  <div class="edge-ai">
    <!-- 页面头部 + 技术标签 + 状态卡片 -->
    <EdgeAIHeader
      :current-time="currentTime"
      :last-update-time="lastUpdateTime"
      :loading="loading"
      :tech-badges="techBadges"
      :status-cards="statusCards"
      @refresh="refreshAll"
    />

    <!-- 性能指标 + 情绪分析 -->
    <PerformanceSection
      :performance-metrics="performanceMetrics"
      :emotion-gauge-option="emotionGaugeOption"
      :emotion-pulse-line-option="emotionPulseLineOption"
    />

    <!-- 联邦学习 + 隐私预算 -->
    <FederatedPrivacySection
      :federated="federated"
      :privacy="privacy"
      :privacy-budget-color="privacyBudgetColor"
      :privacy-pie-option="privacyPieOption"
      @trigger-aggregation="triggerAggregation"
      @refresh-federated="loadFederatedStatus"
    />

    <!-- 向量搜索 + 边缘节点 -->
    <VectorNodesSection
      v-model:vector-query="vectorQuery"
      :vector-searching="vectorSearching"
      :vector-searched="vectorSearched"
      :vector-results="vectorResults"
      :edge-nodes="edgeNodes"
      @search="doVectorSearch"
    />

    <!-- 双记忆RAG系统 -->
    <RAGSystemSection :rag-stats="ragStats" />

    <!-- 配置管理 -->
    <ConfigSection
      :edge-config="edgeConfig"
      :config-loading="configLoading"
      :config-saving="configSaving"
      @save="saveConfig"
      @reset="loadConfig"
    />

    <!-- AI 工具箱 -->
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
import api from '@/api'
import dayjs from 'dayjs'
import { getErrorMessage } from '@/utils/errorHelper'
import { Cpu, Monitor, Connection, Stopwatch } from '@element-plus/icons-vue'
import EdgeAIHeader from './edge-ai/EdgeAIHeader.vue'
import PerformanceSection from './edge-ai/PerformanceSection.vue'
import FederatedPrivacySection from './edge-ai/FederatedPrivacySection.vue'
import VectorNodesSection from './edge-ai/VectorNodesSection.vue'
import RAGSystemSection from './edge-ai/RAGSystemSection.vue'
import ConfigSection from './edge-ai/ConfigSection.vue'
import AIToolboxSection from './edge-ai/AIToolboxSection.vue'


// ========== 响应式状态 ==========

/** 全局加载标志，刷新全部数据时为 true */
const loading = ref(false)
const lastUpdateTime = ref(dayjs().format('HH:mm:ss'))
const currentTime = computed(() => dayjs().format('YYYY-MM-DD HH:mm'))

/** 技术能力标签，展示在页面头部 */
const techBadges = [
  { icon: '建议', label: '内容建议' },
  { icon: '检查', label: '内容检查' },
  { icon: '情绪', label: '情绪观察' },
  { icon: '参考', label: '历史参考' },
  { icon: '整理', label: '自动整理' },
  { icon: '支持', label: '处理支持' },
]

/** 引擎运行状态，由 loadStatus() 填充 */
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

/** 四项性能指标，驱动 PerformanceSection 的进度条渲染 */
const performanceMetrics = computed(() => [
  {
    label: '响应速度',
    value: `${engineStatus.avgLatency}ms`,
    percent: Math.min(100, (engineStatus.avgLatency / 200) * 100),
    color: engineStatus.avgLatency < 50 ? '#2E7D32' : engineStatus.avgLatency < 100 ? '#E65100' : '#C62828',
  },
  {
    label: '命中效率',
    value: `${engineStatus.cacheHitRate}%`,
    percent: engineStatus.cacheHitRate,
    color: engineStatus.cacheHitRate > 80 ? '#2E7D32' : engineStatus.cacheHitRate > 50 ? '#E65100' : '#C62828',
  },
  {
    label: '处理速度',
    value: `${engineStatus.throughput} req/s`,
    percent: Math.min(100, (engineStatus.throughput / 500) * 100),
    color: '#1565C0',
  },
  {
    label: '累计调用',
    value: engineStatus.inferenceCount.toLocaleString(),
    percent: Math.min(100, (engineStatus.inferenceCount / 10000) * 100),
    color: '#545F71',
  },
])

/** 头部四张状态卡片 */
const statusCards = computed(() => [
  {
    title: '服务状态',
    value: engineStatus.enabled ? '运行中' : '已停止',
    color: engineStatus.enabled ? '#2E7D32' : '#C62828',
    icon: Cpu,
  },
  {
    title: '可用功能',
    value: `${engineStatus.modulesLoaded}/${engineStatus.totalModules}`,
    color: '#1565C0',
    icon: Monitor,
  },
  {
    title: '累计运行',
    value: engineStatus.uptime,
    color: '#E65100',
    icon: Stopwatch,
  },
  {
    title: '当前节点',
    value: engineStatus.activeNodes,
    color: '#545F71',
    icon: Connection,
  },
])

/** 情绪脉搏数据，驱动仪表盘和历史折线图 */
const emotionPulse = reactive({
  temperature: 50,
  trend: 'stable',
  history: [],
  timestamps: [],
})

/** 情绪温度仪表盘 ECharts option */
const emotionGaugeOption = computed(() => ({
  series: [{
    type: 'gauge',
    center: ['50%', '60%'],
    radius: '90%',
    startAngle: 200,
    endAngle: -20,
    min: 0,
    max: 100,
    splitNumber: 10,
    axisLine: {
      lineStyle: {
        width: 16,
        color: [
          [0.3, '#1565C0'],
          [0.5, '#2E7D32'],
          [0.7, '#E65100'],
          [1, '#C62828'],
        ],
      },
    },
    pointer: {
      icon: 'path://M12.8,0.7l12,40.1H0.7L12.8,0.7z',
      length: '55%',
      width: 8,
      offsetCenter: [0, '-10%'],
      itemStyle: { color: 'auto' },
    },
    axisTick: { length: 8, lineStyle: { color: 'auto', width: 1.5 } },
    splitLine: { length: 14, lineStyle: { color: 'auto', width: 2.5 } },
    axisLabel: { color: '#44474E', fontSize: 10, distance: -40 },
    title: { offsetCenter: [0, '75%'], fontSize: 13, color: '#44474E' },
    detail: {
      fontSize: 28,
      offsetCenter: [0, '45%'],
      valueAnimation: true,
      color: 'auto',
      formatter: '{value}°',
    },
    data: [{ value: emotionPulse.temperature, name: '情绪温度' }],
  }],
}))

/** 情绪脉搏历史折线图 ECharts option */
const emotionPulseLineOption = computed(() => ({
  tooltip: { trigger: 'axis' },
  grid: { top: 20, right: 20, bottom: 30, left: 45 },
  xAxis: {
    type: 'category',
    data: emotionPulse.timestamps,
    axisLabel: { color: '#44474E', fontSize: 10 },
    axisLine: { lineStyle: { color: '#C4C6CF' } },
  },
  yAxis: {
    type: 'value',
    min: 0,
    max: 100,
    axisLabel: { color: '#44474E', fontSize: 10 },
    splitLine: { lineStyle: { color: '#C4C6CF' } },
  },
  series: [{
    type: 'line',
    data: emotionPulse.history,
    smooth: true,
    symbol: 'circle',
    symbolSize: 6,
    lineStyle: { color: '#1565C0', width: 2 },
    itemStyle: { color: '#1565C0' },
    areaStyle: {
      color: {
        type: 'linear',
        x: 0, y: 0, x2: 0, y2: 1,
        colorStops: [
          { offset: 0, color: 'rgba(21,101,192,0.25)' },
          { offset: 1, color: 'rgba(21,101,192,0.02)' },
        ],
      },
    },
  }],
}))

/** 联邦学习状态 */
const federated = reactive({
  status: 'idle',
  currentRound: 0,
  participatingNodes: 0,
  modelAccuracy: '0%',
  convergenceRate: '0%',
  aggregationProgress: 0,
  aggregating: false,
})

/** 差分隐私预算状态 */
const privacy = reactive({
  epsilonUsed: 0,
  epsilonTotal: 10,
  epsilonPercent: 0,
  epsilonRemaining: 10,
  deltaValue: '1e-5',
  noiseLevel: 'medium',
  queriesRemaining: 0,
  allocation: [],
})

/** 隐私预算进度条颜色，随消耗比例从绿→橙→红变化 */
const privacyBudgetColor = computed(() => {
  if (privacy.epsilonPercent < 50) return '#2E7D32'
  if (privacy.epsilonPercent < 80) return '#E65100'
  return '#C62828'
})

/** 隐私预算各子系统分配饼图 ECharts option */
const privacyPieOption = computed(() => ({
  tooltip: { trigger: 'item', formatter: '{b}: ε={c} ({d}%)' },
  legend: { bottom: 0, textStyle: { color: '#44474E', fontSize: 11 } },
  series: [{
    type: 'pie',
    radius: ['40%', '65%'],
    center: ['50%', '45%'],
    avoidLabelOverlap: true,
    itemStyle: { borderRadius: 6, borderColor: '#fff', borderWidth: 2 },
    label: { show: false },
    emphasis: { label: { show: true, fontSize: 13, fontWeight: 'bold' } },
        data: privacy.allocation.length > 0
      ? privacy.allocation
      : [
          { value: 3, name: '情绪判断', itemStyle: { color: '#1565C0' } },
          { value: 2, name: '内容检查', itemStyle: { color: '#2E7D32' } },
          { value: 2.5, name: '智能回复', itemStyle: { color: '#E65100' } },
          { value: 1.5, name: '内容检索', itemStyle: { color: '#C62828' } },
          { value: 1, name: '预留', itemStyle: { color: '#545F71' } },
        ],
  }],
}))

/** 向量搜索状态 */
const vectorSearch = reactive({
  query: '',
  searching: false,
  results: [],
})
const vectorSearched = ref(false)
const vectorQuery = computed({
  get: () => vectorSearch.query,
  set: (value) => { vectorSearch.query = value },
})
const vectorSearching = computed(() => vectorSearch.searching)
const vectorResults = computed(() => vectorSearch.results)

/** 边缘推理节点列表 */
const edgeNodes = ref([])

/** 双记忆 RAG 系统指标 */
const ragStats = ref({})

/** 引擎配置表单数据 */
const edgeConfig = reactive({
  inferenceTimeout: 5000,
  maxBatchSize: 32,
  cacheEnabled: true,
  cacheTTL: 3600,
  federatedEnabled: true,
  privacyEpsilon: 1.0,
  emotionModel: 'standard',
  vectorSearchEnabled: true,
})
const configLoading = ref(false)
const configSaving = ref(false)

/** 情感分析在线调试工具状态 */
const sentimentTool = reactive({ text: '', loading: false, result: null })
/** 内容审核在线调试工具状态 */
const moderationTool = reactive({ text: '', loading: false, result: null })

// ========== 数据加载函数 ==========

/** 加载引擎整体状态（运行/停止、模块数、运行时间等） */
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
  } catch { /* 静默 */ }
}

/** 加载性能指标、节点列表和 RAG 统计 */
async function loadMetrics() {
  try {
    const { data } = await api.getEdgeAIMetrics()
    const m = data.data || data
    if (m.avgLatency != null) engineStatus.avgLatency = m.avgLatency ?? m.avg_latency
    if (m.cacheHitRate != null) engineStatus.cacheHitRate = m.cacheHitRate ?? m.cache_hit_rate
    if (m.throughput != null) engineStatus.throughput = m.throughput
    if (m.inferenceCount != null) engineStatus.inferenceCount = m.inferenceCount ?? m.inference_count
    // 节点列表
    if (Array.isArray(m.nodes)) {
      edgeNodes.value = m.nodes
    }
    // 双记忆RAG指标
    if (m.dual_memory_rag) {
      ragStats.value = m.dual_memory_rag
    }
  } catch { /* 静默 */ }
}

/**
 * 加载情绪脉搏数据。
 * 支持多种后端响应格式：avg_score / temperature / history / mood_distribution
 */
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
      emotionPulse.history = ep.history.map(h => h.value ?? h)
      emotionPulse.timestamps = ep.history.map(h => h.time ?? h.timestamp ?? '')
    } else if (ep.mood_distribution && typeof ep.mood_distribution === 'object') {
      const values = Object.values(ep.mood_distribution).map(v => Number(v) || 0)
      const sum = values.reduce((acc, curr) => acc + curr, 0)
      if (sum > 0) {
        emotionPulse.history = values.map(v => Math.round((v / sum) * 100))
      }
      emotionPulse.timestamps = Object.keys(ep.mood_distribution)
    }
  } catch { /* 静默 */ }
}

/** 加载联邦学习状态（轮次、节点数、准确率、收敛率） */
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
  } catch { /* 静默 */ }
}

/** 加载差分隐私预算（epsilon 消耗/总量/分配） */
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
      const colors = ['#1565C0', '#2E7D32', '#E65100', '#C62828', '#545F71', '#6E5676']
      privacy.allocation = p.allocation.map((a, i) => ({
        value: a.value ?? a.epsilon,
        name: a.name ?? a.label,
        itemStyle: { color: colors[i % colors.length] },
      }))
    }
  } catch { /* 静默 */ }
}

/** 加载引擎配置表单数据 */
async function loadConfig() {
  configLoading.value = true
  try {
    const { data } = await api.getEdgeAIConfig()
    const c = data.data || data
    Object.assign(edgeConfig, {
      inferenceTimeout: c.inferenceTimeout ?? c.inference_timeout ?? 5000,
      maxBatchSize: c.maxBatchSize ?? c.max_batch_size ?? 32,
      cacheEnabled: c.cacheEnabled ?? c.cache_enabled ?? true,
      cacheTTL: c.cacheTTL ?? c.cache_ttl ?? 3600,
      federatedEnabled: c.federatedEnabled ?? c.federated_enabled ?? true,
      privacyEpsilon: c.privacyEpsilon ?? c.privacy_epsilon ?? 1.0,
      emotionModel: c.emotionModel ?? c.emotion_model ?? 'standard',
      vectorSearchEnabled: c.vectorSearchEnabled ?? c.vector_search_enabled ?? true,
    })
  } catch { /* 静默 */ } finally {
    configLoading.value = false
  }
}

// ========== 操作函数 ==========

/** 手动触发联邦学习聚合（下一轮） */
async function triggerAggregation() {
  federated.aggregating = true
  try {
    await api.triggerFederatedAggregation({ round: federated.currentRound + 1 })
    federated.status = 'aggregating'
    await loadFederatedStatus()
  } catch { /* 静默 */ } finally {
    federated.aggregating = false
  }
}

/** 调用情感分析接口，展示分数/类别/置信度 */
async function doSentimentAnalysis() {
  if (!sentimentTool.text.trim()) return
  sentimentTool.loading = true
  sentimentTool.result = null
  try {
    const { data } = await api.analyzeText(sentimentTool.text)
    const r = data.data || data
    sentimentTool.result = {
      score: r.score ?? r.sentiment_score ?? 0,
      sentiment: r.emotion ?? r.sentiment ?? 'neutral',
      confidence: r.confidence ?? r.conf ?? 0,
    }
  } catch {
    sentimentTool.result = null
  } finally {
    sentimentTool.loading = false
  }
}

/** 调用内容审核接口，展示通过/拒绝/风险等级/原因 */
async function doContentModeration() {
  if (!moderationTool.text.trim()) return
  moderationTool.loading = true
  moderationTool.result = null
  try {
    const { data } = await api.moderateText(moderationTool.text)
    const r = data.data || data
    moderationTool.result = {
      pass: r.pass ?? r.passed ?? r.approved ?? true,
      risk: r.risk ?? r.risk_level ?? 'low',
      reason: r.reason ?? r.reject_reason ?? '',
    }
  } catch {
    moderationTool.result = null
  } finally {
    moderationTool.loading = false
  }
}

/** 执行 HNSW 向量搜索，结果按相似度排序 */
async function doVectorSearch() {
  if (!vectorSearch.query.trim()) return
  vectorSearched.value = false
  vectorSearch.searching = true
  try {
    const { data } = await api.edgeAIVectorSearch({
      query: vectorSearch.query,
      topK: 10,
    })
    const r = data.data || data
    const raw = Array.isArray(r) ? r : (r.results ?? [])
    vectorSearch.results = raw.map((item) => ({
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

/** 保存引擎配置到后端 */
async function saveConfig() {
  configSaving.value = true
  try {
    await api.updateEdgeAIConfig(edgeConfig)
    ElMessage.success('配置保存成功')
  } catch (e) {
    ElMessage.error(getErrorMessage(e, '保存配置失败'))
  } finally {
    configSaving.value = false
  }
}

/** 并行刷新所有数据面板 */
async function refreshAll() {
  loading.value = true
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


// ========== 生命周期 ==========

/** 统一管理所有定时器 ID，onUnmounted 中批量清理 */
const timerIds: ReturnType<typeof setInterval>[] = []

/** 注册定时器并记录 ID，确保卸载时能全部清理 */
function addTimer(fn: () => void, interval: number): void {
  const id = setInterval(fn, interval)
  timerIds.push(id)
}

onMounted(() => {
  refreshAll()
  // 每30秒刷新情绪脉搏和性能指标
  addTimer(() => {
    loadEmotionPulse()
    loadMetrics()
  }, 30000)
})

onUnmounted(() => {
  timerIds.forEach(id => clearInterval(id))
  timerIds.length = 0
})
</script>

<style lang="scss">
/* ============================================
   Material Design 3 主题
   ============================================ */

// ==========================================
// 根容器
// ==========================================
.edge-ai {
  min-height: 100%;
  padding: 8px 0 18px;
  color: var(--hl-ink);
  font-family: var(--hl-font-body);
}

// ==========================================
// 图表行
// ==========================================
.edge-ai .charts-row {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
  gap: 20px;
  margin-bottom: 24px;

  .chart-card {
    background: linear-gradient(180deg, rgba(255, 250, 242, 0.94), rgba(247, 241, 232, 0.98));
    border: 1px solid rgba(24, 36, 47, 0.08);
    border-radius: 24px;
    padding: 20px;
    transition: all var(--m3-motion-duration-medium2) var(--m3-motion-easing-standard);

    &:hover {
      border-color: rgba(35, 73, 99, 0.16);
    }

    .card-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 16px;
      font-size: 15px;
      font-weight: 600;
      color: var(--hl-ink);

      span {
        color: var(--hl-ink);
      }
    }
  }
}

// ==========================================
// 性能指标网格
// ==========================================
.edge-ai .metrics-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
  gap: 12px;
  margin-top: 16px;

  .metric-item {
    background: rgba(255, 255, 255, 0.56);
    border: 1px solid rgba(24, 36, 47, 0.08);
    border-radius: 18px;
    padding: 14px 12px;
    text-align: center;
    transition: all var(--m3-motion-duration-short2) var(--m3-motion-easing-standard);

    &:hover {
      border-color: var(--m3-primary);
      background: var(--m3-surface-container-highest);
    }

    .metric-value {
      font-size: 20px;
      font-weight: 700;
      color: var(--m3-primary);
    }

    .metric-label {
      font-size: 11px;
      color: var(--m3-on-surface-variant);
      margin-top: 4px;
    }
  }
}

// ==========================================
// 情感脉搏
// ==========================================
.edge-ai .emotion-pulse-container {
  display: flex;
  gap: 16px;
  height: 300px;

  .gauge-wrapper {
    flex: 0 0 45%;
    min-height: 280px;
  }

  .pulse-line-wrapper {
    flex: 1;
    min-height: 280px;
  }
}

// ==========================================
// 联邦学习面板
// ==========================================
.edge-ai .federated-panel {
  .fed-stats {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
    gap: 12px;
    margin-bottom: 16px;

    .fed-stat-item {
      background: rgba(255, 255, 255, 0.56);
      border: 1px solid rgba(24, 36, 47, 0.08);
      border-radius: 18px;
      padding: 14px;
      text-align: center;

      .fed-stat-value {
        font-size: 22px;
        font-weight: 700;
        color: var(--m3-primary);
      }

      .fed-stat-label {
        font-size: 11px;
        color: var(--m3-on-surface-variant);
        margin-top: 4px;
      }
    }
  }

  .fed-progress {
    margin: 16px 0;

    .fed-progress-label {
      display: flex;
      justify-content: space-between;
      font-size: 12px;
      color: var(--m3-on-surface-variant);
      margin-bottom: 8px;
    }
  }

  .fed-actions {
    display: flex;
    gap: 10px;
    margin-top: 16px;
  }
}

// ==========================================
// 隐私预算面板
// ==========================================
.edge-ai .privacy-panel {
  .budget-overview {
    display: flex;
    align-items: center;
    gap: 24px;
    margin-bottom: 16px;

    .budget-main {
      text-align: center;

      .epsilon {
        font-size: 36px;
        font-weight: 700;
        color: var(--m3-primary);
      }

      .budget-hint {
        font-size: 11px;
        color: var(--m3-on-surface-variant);
        margin-top: 2px;
      }
    }

    .budget-chart {
      flex: 1;
      min-height: 200px;
    }
  }
}

// ==========================================
// 向量搜索面板
// ==========================================
.edge-ai .vector-search-panel {
  .search-input-row {
    display: flex;
    gap: 10px;
    margin-bottom: 16px;
  }

  .search-results {
    display: flex;
    flex-direction: column;
    gap: 10px;

    .search-result-item {
      background: rgba(255, 255, 255, 0.56);
      border: 1px solid rgba(24, 36, 47, 0.08);
      border-radius: 18px;
      padding: 14px 16px;
      display: flex;
      align-items: center;
      gap: 14px;
      transition: all var(--m3-motion-duration-short2) var(--m3-motion-easing-standard);

      &:hover {
        border-color: var(--m3-primary);
        background: var(--m3-surface-container-highest);
      }

      .result-score {
        font-size: 18px;
        font-weight: 700;
        color: var(--m3-primary);
        min-width: 50px;
        text-align: center;
      }

      .result-content {
        flex: 1;
        font-size: 13px;
        color: var(--hl-ink-soft);
        line-height: 1.5;
      }
    }
  }
}

// ==========================================
// 配置管理
// ==========================================
.edge-ai .module-card {
  background: linear-gradient(180deg, rgba(255, 250, 242, 0.94), rgba(247, 241, 232, 0.98));
  border: 1px solid rgba(24, 36, 47, 0.08);
  border-radius: 24px;
  padding: 20px;
  margin-bottom: 16px;

  .config-form {
    margin-top: 12px;
  }

  .config-actions {
    display: flex;
    justify-content: flex-end;
    gap: 10px;
    margin-top: 16px;
    padding-top: 16px;
    border-top: 1px solid rgba(24, 36, 47, 0.08);
  }
}

// ==========================================
// AI 工具面板
// ==========================================
.edge-ai .ai-tool-panel {
  .tool-result {
    margin-top: 16px;
    padding: 16px;
    background: rgba(255, 255, 255, 0.56);
    border: 1px solid rgba(24, 36, 47, 0.08);
    border-radius: 18px;
    display: flex;
    flex-direction: column;
    gap: 12px;

    .result-item {
      display: flex;
      justify-content: space-between;
      align-items: center;

      .result-label {
        font-size: 13px;
        color: var(--m3-on-surface-variant);
        min-width: 80px;
      }

      .result-value {
        font-size: 15px;
        font-weight: 600;
        color: var(--m3-primary);

        &.reason-text {
          font-size: 13px;
          font-weight: 400;
          color: var(--m3-error);
          text-align: right;
          max-width: 200px;
          word-break: break-all;
        }
      }

      .el-progress {
        flex: 1;
        margin-left: 12px;
      }
    }
  }
}

// ==========================================
// Element Plus 组件覆盖 — Material Design 3
// ==========================================

// el-card
.edge-ai :deep(.el-card) {
  background: var(--m3-surface-container);
  border: 1px solid rgba(24, 36, 47, 0.08);
  border-radius: 24px;
  color: var(--hl-ink);
  --el-card-bg-color: transparent;

  .el-card__header {
    border-bottom: 1px solid rgba(24, 36, 47, 0.08);
    color: var(--hl-ink);
    padding: 18px 22px;
    font-weight: 600;
    background: rgba(255, 255, 255, 0.38);
  }

  .el-card__body {
    color: var(--hl-ink);
  }
}

// el-button
.edge-ai :deep(.el-button) {
  --el-button-bg-color: transparent;
  --el-button-border-color: rgba(24, 36, 47, 0.12);
  --el-button-text-color: var(--hl-ink);
  --el-button-hover-bg-color: rgba(255, 255, 255, 0.72);
  --el-button-hover-border-color: var(--m3-primary);
  --el-button-hover-text-color: var(--m3-primary);
  border-radius: 14px;
  transition: all var(--m3-motion-duration-short2) var(--m3-motion-easing-standard);
}

.edge-ai :deep(.el-button--primary) {
  --el-button-bg-color: var(--m3-primary);
  background: var(--m3-primary) !important;
  border: none !important;
  color: var(--m3-on-primary) !important;
  font-weight: 600;

  &:hover {
    background: var(--m3-primary-container) !important;
    color: var(--m3-on-primary-container) !important;
  }
}

.edge-ai :deep(.el-button--success) {
  --el-button-bg-color: var(--m3-success-container);
  border-color: var(--m3-success) !important;
  color: var(--m3-success) !important;

  &:hover {
    background: var(--m3-success-container) !important;
  }
}

.edge-ai :deep(.el-button--warning) {
  --el-button-bg-color: var(--m3-tertiary-container);
  border-color: var(--m3-tertiary) !important;
  color: var(--m3-tertiary) !important;

  &:hover {
    background: var(--m3-tertiary-container) !important;
  }
}

.edge-ai :deep(.el-button--danger) {
  --el-button-bg-color: var(--m3-error-container);
  border-color: var(--m3-error) !important;
  color: var(--m3-error) !important;

  &:hover {
    background: var(--m3-error-container) !important;
  }
}

// el-input
.edge-ai :deep(.el-input) {
  .el-input__wrapper {
    background: rgba(255, 255, 255, 0.8);
    border: 1px solid rgba(24, 36, 47, 0.12);
    border-radius: 14px;
    box-shadow: none !important;
    transition: all var(--m3-motion-duration-short2) var(--m3-motion-easing-standard);

    &:hover {
      border-color: rgba(24, 36, 47, 0.2);
    }

    &.is-focus {
      border-color: var(--m3-primary);
      box-shadow: 0 0 0 3px rgba(35, 73, 99, 0.1) !important;
    }
  }

  .el-input__inner {
    color: var(--hl-ink);

    &::placeholder {
      color: var(--m3-on-surface-variant);
      opacity: 0.6;
    }
  }
}

// el-textarea
.edge-ai :deep(.el-textarea) {
  .el-textarea__inner {
    background: rgba(255, 255, 255, 0.8);
    border: 1px solid rgba(24, 36, 47, 0.12);
    border-radius: 14px;
    color: var(--hl-ink);
    box-shadow: none !important;
    transition: all var(--m3-motion-duration-short2) var(--m3-motion-easing-standard);

    &:hover {
      border-color: rgba(24, 36, 47, 0.2);
    }

    &:focus {
      border-color: var(--m3-primary);
      box-shadow: 0 0 0 3px rgba(35, 73, 99, 0.1) !important;
    }

    &::placeholder {
      color: var(--m3-on-surface-variant);
      opacity: 0.6;
    }
  }
}

// el-select
.edge-ai :deep(.el-select) {
  .el-select__wrapper {
    background: rgba(255, 255, 255, 0.8) !important;
    border: 1px solid rgba(24, 36, 47, 0.12);
    border-radius: 14px;
    box-shadow: none !important;

    &:hover {
      border-color: rgba(24, 36, 47, 0.2);
    }
  }

  .el-select__selected-item {
    color: var(--hl-ink);
  }

  .el-select__placeholder {
    color: var(--m3-on-surface-variant);
    opacity: 0.6;
  }
}

// el-input-number
.edge-ai :deep(.el-input-number) {
  .el-input__wrapper {
    background: rgba(255, 255, 255, 0.8);
    border: 1px solid rgba(24, 36, 47, 0.12);
    border-radius: 14px;
  }

  .el-input-number__decrease,
  .el-input-number__increase {
    background: transparent;
    border-color: var(--m3-outline-variant);
    color: var(--m3-on-surface-variant);

    &:hover {
      color: var(--m3-primary);
    }
  }
}

// el-switch
.edge-ai :deep(.el-switch) {
  --el-switch-on-color: var(--m3-primary);
  --el-switch-off-color: rgba(24, 36, 47, 0.12);
}

// el-progress
.edge-ai :deep(.el-progress) {
  .el-progress-bar__outer {
    background: rgba(24, 36, 47, 0.08);
    border-radius: 999px;
  }

  .el-progress-bar__inner {
    background: var(--m3-primary);
    border-radius: 999px;
  }

  .el-progress__text {
    color: var(--hl-ink-soft);
    font-size: 12px !important;
  }
}

// el-table
.edge-ai :deep(.el-table) {
  --el-table-bg-color: transparent;
  --el-table-tr-bg-color: transparent;
  --el-table-header-bg-color: rgba(255, 255, 255, 0.56);
  --el-table-row-hover-bg-color: rgba(255, 255, 255, 0.72);
  --el-table-border-color: rgba(24, 36, 47, 0.08);
  --el-table-text-color: var(--hl-ink);
  --el-table-header-text-color: var(--hl-ink-soft);
  background: transparent;
  color: var(--hl-ink);

  &::before {
    display: none;
  }

  th.el-table__cell {
    background: rgba(255, 255, 255, 0.56) !important;
    border-bottom: 1px solid rgba(24, 36, 47, 0.08) !important;
    font-weight: 600;
    font-size: 12px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: var(--hl-ink-soft);
  }

  td.el-table__cell {
    border-bottom: 1px solid rgba(24, 36, 47, 0.08) !important;
  }

  .el-table__body tr:hover > td {
    background: rgba(255, 255, 255, 0.72) !important;
  }
}

// el-tag
.edge-ai :deep(.el-tag) {
  border-radius: 999px;
  border: none;
  font-size: 11px;
  font-weight: 500;
}

.edge-ai :deep(.el-tag--success) {
  background: var(--m3-success-container);
  color: var(--m3-success);
}

.edge-ai :deep(.el-tag--warning) {
  background: var(--m3-tertiary-container);
  color: var(--m3-tertiary);
}

.edge-ai :deep(.el-tag--danger) {
  background: var(--m3-error-container);
  color: var(--m3-error);
}

.edge-ai :deep(.el-tag--info) {
  background: var(--m3-secondary-container);
  color: var(--m3-secondary);
}

// el-form
.edge-ai :deep(.el-form) {
  .el-form-item__label {
    color: var(--hl-ink-soft);
    font-size: 13px;
  }
}

// v-loading
.edge-ai :deep(.el-loading-mask) {
  background: rgba(0, 0, 0, 0.5);
}

.edge-ai :deep(.el-loading-spinner) {
  .circular .path {
    stroke: var(--m3-primary);
  }

  .el-loading-text {
    color: var(--hl-paper);
  }
}

// el-empty
.edge-ai :deep(.el-empty) {
  .el-empty__description p {
    color: var(--hl-ink-soft);
  }
}

// ==========================================
// 响应式
// ==========================================
@media (max-width: 768px) {
  .edge-ai {
    padding: 8px 0 18px;
  }

  .edge-ai .charts-row {
    grid-template-columns: 1fr;
  }

  .edge-ai .emotion-pulse-container {
    flex-direction: column;
    height: auto;

    .gauge-wrapper,
    .pulse-line-wrapper {
      flex: none;
      min-height: 220px;
    }
  }

  .edge-ai .vector-search-panel .search-input-row {
    flex-direction: column;
  }
}

// ==========================================
// 滚动条美化
// ==========================================
.edge-ai {
  &::-webkit-scrollbar {
    width: 6px;
  }

  &::-webkit-scrollbar-track {
    background: transparent;
  }

  &::-webkit-scrollbar-thumb {
    background: var(--m3-outline-variant);
    border-radius: 3px;

    &:hover {
      background: var(--m3-outline);
    }
  }
}

.rag-stat-item {
  text-align: center;
  padding: 16px 8px;

  .rag-stat-value {
    font-size: 28px;
    font-weight: 700;
    color: var(--m3-primary);
    line-height: 1.2;
  }

  .rag-stat-label {
    font-size: 13px;
    color: var(--m3-on-surface);
    margin-top: 6px;
    font-weight: 500;
  }

  .rag-stat-desc {
    font-size: 11px;
    color: var(--m3-outline);
    margin-top: 4px;
  }
}
</style>
