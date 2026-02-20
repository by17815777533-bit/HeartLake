<!--
  @file EdgeAI.vue
  @brief 边缘AI管理页面 - 状态监控 + 性能指标 + 联邦学习 + 隐私预算
  Created by 林子怡
-->

<template>
  <div class="edge-ai">
    <!-- 页面标题 -->
    <div class="page-header">
      <div class="welcome-section">
        <h1>边缘AI管理</h1>
        <p class="welcome-sub">Edge AI 引擎状态监控与管理 · {{ currentTime }}</p>
      </div>
      <div class="header-actions">
        <el-button type="primary" :icon="Refresh" @click="refreshAll" :loading="loading">
          刷新数据
        </el-button>
        <span class="update-time">最后更新: {{ lastUpdateTime }}</span>
      </div>
    </div>

    <!-- 技术标签 -->
    <div class="tech-badges">
      <span class="tech-badge" v-for="badge in techBadges" :key="badge.label">
        <span class="badge-icon">{{ badge.icon }}</span>
        <span class="badge-label">{{ badge.label }}</span>
      </span>
    </div>

    <!-- 状态卡片 -->
    <el-row :gutter="20" class="stats-cards">
      <el-col :xs="24" :sm="12" :md="6" v-for="card in statusCards" :key="card.title">
        <el-card shadow="hover" class="stat-card" :style="{ borderTop: `4px solid ${card.color}` }">
          <div class="stat-content">
            <div class="stat-info">
              <div class="stat-value">{{ card.value }}</div>
              <div class="stat-title">{{ card.title }}</div>
            </div>
            <div class="stat-icon" :style="{ background: `${card.color}15`, color: card.color }">
              <el-icon :size="32"><component :is="card.icon" /></el-icon>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 性能指标 + 情绪脉搏 -->
    <el-row :gutter="20" class="charts-row">
      <el-col :xs="24" :md="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>性能指标</span>
              <el-tag type="success" size="small">实时</el-tag>
            </div>
          </template>
          <div class="metrics-grid">
            <div class="metric-item" v-for="m in performanceMetrics" :key="m.label">
              <div class="metric-value" :style="{ color: m.color }">{{ m.value }}</div>
              <div class="metric-label">{{ m.label }}</div>
              <el-progress
                :percentage="m.percent"
                :color="m.color"
                :stroke-width="6"
                :show-text="false"
              />
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :md="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>社区情绪脉搏</span>
              <el-tag type="warning" size="small">30s 刷新</el-tag>
            </div>
          </template>
          <div class="emotion-pulse-container">
            <div class="gauge-wrapper">
              <v-chart :option="emotionGaugeOption" autoresize style="height: 220px" />
            </div>
            <div class="pulse-line-wrapper">
              <v-chart :option="emotionLineOption" autoresize style="height: 220px" />
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 联邦学习 + 隐私预算 -->
    <el-row :gutter="20" class="charts-row">
      <el-col :xs="24" :md="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>联邦学习控制</span>
              <el-tag :type="federated.status === 'aggregating' ? 'warning' : 'info'" size="small">
                {{ federated.status === 'aggregating' ? '聚合中' : '就绪' }}
              </el-tag>
            </div>
          </template>
          <div class="federated-panel">
            <div class="fed-stats">
              <div class="fed-stat-item">
                <span class="fed-stat-value">{{ federated.currentRound }}</span>
                <span class="fed-stat-label">当前轮次</span>
              </div>
              <div class="fed-stat-item">
                <span class="fed-stat-value">{{ federated.participatingNodes }}</span>
                <span class="fed-stat-label">参与节点</span>
              </div>
              <div class="fed-stat-item">
                <span class="fed-stat-value">{{ federated.modelAccuracy }}</span>
                <span class="fed-stat-label">模型精度</span>
              </div>
              <div class="fed-stat-item">
                <span class="fed-stat-value">{{ federated.convergenceRate }}</span>
                <span class="fed-stat-label">收敛速率</span>
              </div>
            </div>
            <div class="fed-progress">
              <span class="fed-progress-label">聚合进度</span>
              <el-progress
                :percentage="federated.aggregationProgress"
                :stroke-width="12"
                :color="'#4285F4'"
                striped
                striped-flow
              />
            </div>
            <div class="fed-actions">
              <el-button
                type="primary"
                @click="triggerAggregation"
                :loading="federated.aggregating"
                :disabled="federated.status === 'aggregating'"
              >
                触发聚合
              </el-button>
              <el-button @click="loadFederatedStatus">刷新状态</el-button>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :md="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>差分隐私预算</span>
              <el-tag size="small">ε 监控</el-tag>
            </div>
          </template>
          <div class="privacy-panel">
            <div class="budget-overview">
              <div class="budget-main">
                <div class="epsilon">ε = {{ privacy.epsilonUsed }} / {{ privacy.epsilonTotal }}</div>
                <el-progress
                  :percentage="privacy.epsilonPercent"
                  :stroke-width="16"
                  :color="privacyBudgetColor"
                />
                <span class="budget-hint">剩余预算: {{ privacy.epsilonRemaining }}</span>
              </div>
            </div>
            <div class="budget-chart">
              <v-chart :option="privacyPieOption" autoresize style="height: 200px" />
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 向量搜索测试 + 边缘节点列表 -->
    <el-row :gutter="20" class="charts-row">
      <el-col :xs="24" :md="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>向量搜索测试</span>
              <el-tag type="primary" size="small">语义检索</el-tag>
            </div>
          </template>
          <div class="vector-search-panel">
            <div class="search-input-row">
              <el-input
                v-model="vectorQuery"
                placeholder="输入文本，搜索语义相似内容..."
                :prefix-icon="Search"
                clearable
                @keyup.enter="doVectorSearch"
              />
              <el-button type="primary" @click="doVectorSearch" :loading="vectorSearching">
                搜索
              </el-button>
            </div>
            <div class="search-results" v-if="vectorResults.length">
              <div class="search-result-item" v-for="(item, idx) in vectorResults" :key="idx">
                <div class="result-score">
                  <el-tag :type="item.score > 0.8 ? 'success' : item.score > 0.5 ? 'warning' : 'info'" size="small">
                    {{ (item.score * 100).toFixed(1) }}%
                  </el-tag>
                </div>
                <div class="result-content">{{ item.content }}</div>
              </div>
            </div>
            <el-empty v-else-if="vectorSearched" description="未找到相似内容" :image-size="80" />
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :md="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>边缘节点列表</span>
              <el-tag size="small">{{ edgeNodes.length }} 个节点</el-tag>
            </div>
          </template>
          <el-table :data="edgeNodes" stripe style="width: 100%" max-height="320" size="small">
            <el-table-column prop="nodeId" label="节点ID" width="100" />
            <el-table-column prop="name" label="名称" min-width="100" />
            <el-table-column label="状态" width="80">
              <template #default="{ row }">
                <el-tag :type="row.status === 'online' ? 'success' : row.status === 'busy' ? 'warning' : 'danger'" size="small">
                  {{ row.status === 'online' ? '在线' : row.status === 'busy' ? '繁忙' : '离线' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="负载" width="100">
              <template #default="{ row }">
                <el-progress :percentage="row.load" :stroke-width="6" :show-text="true"
                  :color="row.load > 80 ? '#EA4335' : row.load > 50 ? '#FBBC04' : '#34A853'" />
              </template>
            </el-table-column>
            <el-table-column label="延迟" width="80">
              <template #default="{ row }">
                <span :style="{ color: row.latency > 100 ? '#EA4335' : row.latency > 50 ? '#FBBC04' : '#34A853' }">
                  {{ row.latency }}ms
                </span>
              </template>
            </el-table-column>
          </el-table>
        </el-card>
      </el-col>
    </el-row>

    <!-- 配置管理 -->
    <el-row :gutter="20" class="charts-row">
      <el-col :span="24">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>Edge AI 配置管理</span>
              <el-tag type="info" size="small">运行时参数</el-tag>
            </div>
          </template>
          <el-form :model="edgeConfig" label-width="160px" class="config-form" v-loading="configLoading">
            <el-row :gutter="24">
              <el-col :xs="24" :md="12">
                <el-form-item label="推理引擎">
                  <el-select v-model="edgeConfig.inferenceEngine" style="width: 100%">
                    <el-option label="ONNX Runtime" value="onnx" />
                    <el-option label="TensorFlow Lite" value="tflite" />
                    <el-option label="WASM" value="wasm" />
                  </el-select>
                </el-form-item>
              </el-col>
              <el-col :xs="24" :md="12">
                <el-form-item label="缓存策略">
                  <el-select v-model="edgeConfig.cacheStrategy" style="width: 100%">
                    <el-option label="LRU" value="lru" />
                    <el-option label="LFU" value="lfu" />
                    <el-option label="TTL" value="ttl" />
                  </el-select>
                </el-form-item>
              </el-col>
              <el-col :xs="24" :md="12">
                <el-form-item label="最大推理批次">
                  <el-input-number v-model="edgeConfig.maxBatchSize" :min="1" :max="64" style="width: 100%" />
                </el-form-item>
              </el-col>
              <el-col :xs="24" :md="12">
                <el-form-item label="缓存大小 (MB)">
                  <el-input-number v-model="edgeConfig.cacheSizeMB" :min="16" :max="1024" :step="16" style="width: 100%" />
                </el-form-item>
              </el-col>
              <el-col :xs="24" :md="12">
                <el-form-item label="隐私预算上限 (ε)">
                  <el-input-number v-model="edgeConfig.maxEpsilon" :min="0.1" :max="10" :step="0.1" :precision="1" style="width: 100%" />
                </el-form-item>
              </el-col>
              <el-col :xs="24" :md="12">
                <el-form-item label="联邦学习轮次间隔 (s)">
                  <el-input-number v-model="edgeConfig.federatedInterval" :min="60" :max="3600" :step="60" style="width: 100%" />
                </el-form-item>
              </el-col>
              <el-col :xs="24" :md="12">
                <el-form-item label="情绪分析模型">
                  <el-select v-model="edgeConfig.emotionModel" style="width: 100%">
                    <el-option label="轻量级 (Fast)" value="fast" />
                    <el-option label="标准 (Standard)" value="standard" />
                    <el-option label="高精度 (Accurate)" value="accurate" />
                  </el-select>
                </el-form-item>
              </el-col>
              <el-col :xs="24" :md="12">
                <el-form-item label="启用向量搜索">
                  <el-switch v-model="edgeConfig.vectorSearchEnabled" />
                </el-form-item>
              </el-col>
            </el-row>
            <div class="config-actions">
              <el-button type="primary" @click="saveConfig" :loading="configSaving">保存配置</el-button>
              <el-button @click="loadConfig">重置</el-button>
            </div>
          </el-form>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, onUnmounted } from 'vue'
import VChart from 'vue-echarts'
import api from '@/api'
import dayjs from 'dayjs'
import {
  Refresh, Cpu, Monitor, Connection, Stopwatch,
  DataAnalysis, Setting, Search, CircleCheck
} from '@element-plus/icons-vue'

// ========== 响应式状态 ==========
const loading = ref(false)
const lastUpdateTime = ref(dayjs().format('HH:mm:ss'))
const currentTime = computed(() => dayjs().format('YYYY-MM-DD HH:mm'))

// 技术标签
const techBadges = [
  { icon: '🧠', label: 'Edge Inference' },
  { icon: '🔗', label: 'Federated Learning' },
  { icon: '🛡️', label: 'Differential Privacy' },
  { icon: '📊', label: 'Emotion Pulse' },
  { icon: '🔍', label: 'Vector Search' },
  { icon: '⚡', label: 'Real-time Analytics' },
]

// 引擎状态
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

// 性能指标
const performanceMetrics = computed(() => [
  {
    label: '推理延迟',
    value: `${engineStatus.avgLatency}ms`,
    percent: Math.min(100, (engineStatus.avgLatency / 200) * 100),
    color: engineStatus.avgLatency < 50 ? '#34A853' : engineStatus.avgLatency < 100 ? '#FBBC04' : '#EA4335',
  },
  {
    label: '缓存命中率',
    value: `${engineStatus.cacheHitRate}%`,
    percent: engineStatus.cacheHitRate,
    color: engineStatus.cacheHitRate > 80 ? '#34A853' : engineStatus.cacheHitRate > 50 ? '#FBBC04' : '#EA4335',
  },
  {
    label: '吞吐量',
    value: `${engineStatus.throughput} req/s`,
    percent: Math.min(100, (engineStatus.throughput / 500) * 100),
    color: '#4285F4',
  },
  {
    label: '推理次数',
    value: engineStatus.inferenceCount.toLocaleString(),
    percent: Math.min(100, (engineStatus.inferenceCount / 10000) * 100),
    color: '#1A73E8',
  },
])

// 状态卡片
const statusCards = computed(() => [
  {
    title: '引擎状态',
    value: engineStatus.enabled ? '运行中' : '已停止',
    color: engineStatus.enabled ? '#34A853' : '#EA4335',
    icon: Cpu,
  },
  {
    title: '模块加载',
    value: `${engineStatus.modulesLoaded}/${engineStatus.totalModules}`,
    color: '#4285F4',
    icon: Monitor,
  },
  {
    title: '运行时间',
    value: engineStatus.uptime,
    color: '#FBBC04',
    icon: Stopwatch,
  },
  {
    title: '活跃节点',
    value: engineStatus.activeNodes,
    color: '#1A73E8',
    icon: Connection,
  },
])

// 情绪脉搏
const emotionPulse = reactive({
  temperature: 50,
  trend: 'stable',
  history: [],
  timestamps: [],
})

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
          [0.3, '#4285F4'],
          [0.5, '#34A853'],
          [0.7, '#FBBC04'],
          [1, '#EA4335'],
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
    axisLabel: { color: '#9AA0A6', fontSize: 10, distance: -40 },
    title: { offsetCenter: [0, '75%'], fontSize: 13, color: '#5f6368' },
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

const emotionLineOption = computed(() => ({
  tooltip: { trigger: 'axis' },
  grid: { top: 20, right: 20, bottom: 30, left: 45 },
  xAxis: {
    type: 'category',
    data: emotionPulse.timestamps,
    axisLabel: { color: '#9AA0A6', fontSize: 10 },
    axisLine: { lineStyle: { color: '#E8EAED' } },
  },
  yAxis: {
    type: 'value',
    min: 0,
    max: 100,
    axisLabel: { color: '#9AA0A6', fontSize: 10 },
    splitLine: { lineStyle: { color: '#F1F3F4' } },
  },
  series: [{
    type: 'line',
    data: emotionPulse.history,
    smooth: true,
    symbol: 'circle',
    symbolSize: 6,
    lineStyle: { color: '#4285F4', width: 2 },
    itemStyle: { color: '#4285F4' },
    areaStyle: {
      color: {
        type: 'linear',
        x: 0, y: 0, x2: 0, y2: 1,
        colorStops: [
          { offset: 0, color: 'rgba(66,133,244,0.25)' },
          { offset: 1, color: 'rgba(66,133,244,0.02)' },
        ],
      },
    },
  }],
}))

// 联邦学习
const federated = reactive({
  status: 'idle',
  currentRound: 0,
  participatingNodes: 0,
  modelAccuracy: '0%',
  convergenceRate: '0%',
  aggregationProgress: 0,
  aggregating: false,
})

// 隐私预算
const privacy = reactive({
  epsilonUsed: 0,
  epsilonTotal: 10,
  epsilonPercent: 0,
  deltaValue: '1e-5',
  noiseLevel: 'medium',
  queriesRemaining: 0,
  allocation: [],
})

const privacyPieOption = computed(() => ({
  tooltip: { trigger: 'item', formatter: '{b}: ε={c} ({d}%)' },
  legend: { bottom: 0, textStyle: { color: '#5f6368', fontSize: 11 } },
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
          { value: 3, name: '情绪分析', itemStyle: { color: '#4285F4' } },
          { value: 2, name: '内容审核', itemStyle: { color: '#34A853' } },
          { value: 2.5, name: '推荐系统', itemStyle: { color: '#FBBC04' } },
          { value: 1.5, name: '向量搜索', itemStyle: { color: '#EA4335' } },
          { value: 1, name: '预留', itemStyle: { color: '#9AA0A6' } },
        ],
  }],
}))

// 向量搜索
const vectorSearch = reactive({
  query: '',
  searching: false,
  results: [],
})

// 边缘节点
const edgeNodes = ref([])

// 配置管理
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
const configSaving = ref(false)

// ========== 数据加载函数 ==========
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
  } catch { /* 静默 */ }
}

async function loadEmotionPulse() {
  try {
    const { data } = await api.getEmotionPulse()
    const ep = data.data || data
    emotionPulse.temperature = ep.temperature ?? ep.value ?? 50
    emotionPulse.trend = ep.trend ?? 'stable'
    if (Array.isArray(ep.history)) {
      emotionPulse.history = ep.history.map(h => h.value ?? h)
      emotionPulse.timestamps = ep.history.map(h => h.time ?? h.timestamp ?? '')
    }
  } catch { /* 静默 */ }
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
  } catch { /* 静默 */ }
}

async function loadPrivacyBudget() {
  try {
    const { data } = await api.getPrivacyBudget()
    const p = data.data || data
    Object.assign(privacy, {
      epsilonUsed: p.epsilonUsed ?? p.epsilon_used ?? 0,
      epsilonTotal: p.epsilonTotal ?? p.epsilon_total ?? 10,
      epsilonPercent: p.epsilonPercent ?? p.epsilon_percent ?? 0,
      deltaValue: p.deltaValue ?? p.delta_value ?? '1e-5',
      noiseLevel: p.noiseLevel ?? p.noise_level ?? 'medium',
      queriesRemaining: p.queriesRemaining ?? p.queries_remaining ?? 0,
    })
    if (Array.isArray(p.allocation)) {
      const colors = ['#4285F4', '#34A853', '#FBBC04', '#EA4335', '#9AA0A6', '#1A73E8']
      privacy.allocation = p.allocation.map((a, i) => ({
        value: a.value ?? a.epsilon,
        name: a.name ?? a.label,
        itemStyle: { color: colors[i % colors.length] },
      }))
    }
  } catch { /* 静默 */ }
}

async function loadConfig() {
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
  } catch { /* 静默 */ }
}

// ========== 操作函数 ==========
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

async function doVectorSearch() {
  if (!vectorSearch.query.trim()) return
  vectorSearch.searching = true
  try {
    const { data } = await api.edgeAIVectorSearch({ text: vectorSearch.query })
    const r = data.data || data
    vectorSearch.results = Array.isArray(r) ? r : (r.results ?? [])
  } catch {
    vectorSearch.results = []
  } finally {
    vectorSearch.searching = false
  }
}

async function saveConfig() {
  configSaving.value = true
  try {
    await api.updateEdgeAIConfig(edgeConfig)
  } catch { /* 静默 */ } finally {
    configSaving.value = false
  }
}

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

function getNodeStatusType(status) {
  const map = { online: 'success', offline: 'danger', syncing: 'warning', idle: 'info' }
  return map[status] || 'info'
}

function getNodeStatusLabel(status) {
  const map = { online: '在线', offline: '离线', syncing: '同步中', idle: '空闲' }
  return map[status] || status
}

// ========== 生命周期 ==========
let pulseTimer = null

onMounted(() => {
  refreshAll()
  pulseTimer = setInterval(() => {
    loadEmotionPulse()
    loadMetrics()
  }, 30000)
})

onUnmounted(() => {
  clearInterval(pulseTimer)
  pulseTimer = null
})
</script>
