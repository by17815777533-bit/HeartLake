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
              <el-tag type="success" size="small">30s 刷新</el-tag>
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
              <span>情绪分析</span>
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
                :color="'#1565C0'"
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
                  :color="row.load > 80 ? '#C62828' : row.load > 50 ? '#E65100' : '#2E7D32'" />
              </template>
            </el-table-column>
            <el-table-column label="延迟" width="80">
              <template #default="{ row }">
                <span :style="{ color: row.latency > 100 ? '#C62828' : row.latency > 50 ? '#E65100' : '#2E7D32' }">
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

    <!-- AI 工具箱 -->
    <el-row :gutter="20" class="section-row">
      <el-col :xs="24" :md="12">
        <el-card shadow="hover" class="module-card">
          <template #header>
            <div class="card-header">
              <span><el-icon><DataAnalysis /></el-icon> 情感分析测试</span>
            </div>
          </template>
          <div class="ai-tool-panel">
            <el-input
              v-model="sentimentTool.text"
              type="textarea"
              :rows="3"
              placeholder="输入文本，测试情感分析..."
              maxlength="500"
              show-word-limit
            />
            <el-button
              type="primary"
              @click="doSentimentAnalysis"
              :loading="sentimentTool.loading"
              :disabled="!sentimentTool.text.trim()"
              style="margin-top: 12px"
            >分析</el-button>
            <div class="tool-result" v-if="sentimentTool.result">
              <div class="result-item">
                <span class="result-label">情感得分</span>
                <el-progress
                  :percentage="Math.round(sentimentTool.result.score * 100)"
                  :color="sentimentTool.result.score >= 0.6 ? '#2E7D32' : sentimentTool.result.score >= 0.4 ? '#E65100' : '#C62828'"
                  :stroke-width="12"
                />
              </div>
              <div class="result-item">
                <span class="result-label">情绪类型</span>
                <el-tag :type="sentimentTool.result.emotion === 'positive' ? 'success' : sentimentTool.result.emotion === 'negative' ? 'danger' : 'info'">
                  {{ sentimentTool.result.emotion || '-' }}
                </el-tag>
              </div>
              <div class="result-item">
                <span class="result-label">置信度</span>
                <span class="result-value">{{ ((sentimentTool.result.confidence ?? 0) * 100).toFixed(1) }}%</span>
              </div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :md="12">
        <el-card shadow="hover" class="module-card">
          <template #header>
            <div class="card-header">
              <span><el-icon><CircleCheck /></el-icon> 内容审核测试</span>
            </div>
          </template>
          <div class="ai-tool-panel">
            <el-input
              v-model="moderationTool.text"
              type="textarea"
              :rows="3"
              placeholder="输入文本，测试内容审核..."
              maxlength="500"
              show-word-limit
            />
            <el-button
              type="primary"
              @click="doContentModeration"
              :loading="moderationTool.loading"
              :disabled="!moderationTool.text.trim()"
              style="margin-top: 12px"
            >审核</el-button>
            <div class="tool-result" v-if="moderationTool.result">
              <div class="result-item">
                <span class="result-label">审核结果</span>
                <el-tag :type="moderationTool.result.pass ? 'success' : 'danger'" size="large">
                  {{ moderationTool.result.pass ? '通过' : '未通过' }}
                </el-tag>
              </div>
              <div class="result-item">
                <span class="result-label">风险等级</span>
                <el-tag :type="moderationTool.result.risk === 'low' ? 'success' : moderationTool.result.risk === 'medium' ? 'warning' : 'danger'">
                  {{ { low: '低风险', medium: '中风险', high: '高风险' }[moderationTool.result.risk] || moderationTool.result.risk || '-' }}
                </el-tag>
              </div>
              <div class="result-item" v-if="moderationTool.result.reason">
                <span class="result-label">原因</span>
                <span class="result-value reason-text">{{ moderationTool.result.reason }}</span>
              </div>
            </div>
          </div>
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
    color: engineStatus.avgLatency < 50 ? '#2E7D32' : engineStatus.avgLatency < 100 ? '#E65100' : '#C62828',
  },
  {
    label: '缓存命中率',
    value: `${engineStatus.cacheHitRate}%`,
    percent: engineStatus.cacheHitRate,
    color: engineStatus.cacheHitRate > 80 ? '#2E7D32' : engineStatus.cacheHitRate > 50 ? '#E65100' : '#C62828',
  },
  {
    label: '吞吐量',
    value: `${engineStatus.throughput} req/s`,
    percent: Math.min(100, (engineStatus.throughput / 500) * 100),
    color: '#1565C0',
  },
  {
    label: '推理次数',
    value: engineStatus.inferenceCount.toLocaleString(),
    percent: Math.min(100, (engineStatus.inferenceCount / 10000) * 100),
    color: '#545F71',
  },
])

// 状态卡片
const statusCards = computed(() => [
  {
    title: '引擎状态',
    value: engineStatus.enabled ? '运行中' : '已停止',
    color: engineStatus.enabled ? '#2E7D32' : '#C62828',
    icon: Cpu,
  },
  {
    title: '模块加载',
    value: `${engineStatus.modulesLoaded}/${engineStatus.totalModules}`,
    color: '#1565C0',
    icon: Monitor,
  },
  {
    title: '运行时间',
    value: engineStatus.uptime,
    color: '#E65100',
    icon: Stopwatch,
  },
  {
    title: '活跃节点',
    value: engineStatus.activeNodes,
    color: '#545F71',
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

const emotionLineOption = computed(() => ({
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
          { value: 3, name: '情绪分析', itemStyle: { color: '#1565C0' } },
          { value: 2, name: '内容审核', itemStyle: { color: '#2E7D32' } },
          { value: 2.5, name: '推荐系统', itemStyle: { color: '#E65100' } },
          { value: 1.5, name: '向量搜索', itemStyle: { color: '#C62828' } },
          { value: 1, name: '预留', itemStyle: { color: '#545F71' } },
        ],
  }],
}))

// 向量搜索
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

// AI 工具箱
const sentimentTool = reactive({ text: '', loading: false, result: null })
const moderationTool = reactive({ text: '', loading: false, result: null })

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

async function doSentimentAnalysis() {
  if (!sentimentTool.text.trim()) return
  sentimentTool.loading = true
  sentimentTool.result = null
  try {
    const { data } = await api.analyzeText(sentimentTool.text)
    const r = data.data || data
    sentimentTool.result = {
      score: r.score ?? r.sentiment_score ?? 0,
      emotion: r.emotion ?? r.sentiment ?? 'neutral',
      confidence: r.confidence ?? r.conf ?? 0,
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

<style lang="scss" scoped>
/* ============================================
   Material Design 3 主题
   ============================================ */

// ==========================================
// 根容器
// ==========================================
.edge-ai {
  min-height: 100vh;
  padding: 24px;
  background: var(--m3-surface);
  color: var(--m3-on-surface);
  font-family: 'Inter', 'PingFang SC', sans-serif;
}

// ==========================================
// 页面头部
// ==========================================
.page-header {
  margin-bottom: 28px;

  .welcome-section {
    h1 {
      font-size: 28px;
      font-weight: 700;
      color: var(--m3-primary);
      margin-bottom: 6px;
      letter-spacing: 1px;
    }

    .welcome-sub {
      font-size: 14px;
      color: var(--m3-on-surface-variant);
      letter-spacing: 0.5px;
    }
  }

  .header-actions {
    display: flex;
    align-items: center;
    gap: 16px;

    .update-time {
      font-size: 12px;
      color: var(--m3-on-surface-variant);
      opacity: 0.7;
    }
  }
}

// ==========================================
// 技术徽章
// ==========================================
.tech-badges {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-bottom: 24px;

  .tech-badge {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 6px 14px;
    background: var(--m3-surface-container);
    border: 1px solid var(--m3-outline-variant);
    border-radius: var(--m3-shape-corner-full);
    font-size: 12px;
    color: var(--m3-on-surface);
    transition: all var(--m3-motion-duration-short2) var(--m3-motion-easing-standard);
    cursor: default;

    &:hover {
      background: var(--m3-surface-container-high);
      border-color: var(--m3-primary);
      transform: translateY(-1px);
    }

    .badge-icon {
      font-size: 14px;
      opacity: 0.9;
    }

    .badge-label {
      font-weight: 500;
    }
  }
}

// ==========================================
// 状态卡片
// ==========================================
.stats-cards {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 16px;
  margin-bottom: 24px;

  .stat-card {
    background: var(--m3-surface-container);
    border: 1px solid var(--m3-outline-variant);
    border-radius: var(--m3-shape-corner-large);
    padding: 20px;
    transition: all var(--m3-motion-duration-medium2) var(--m3-motion-easing-standard);
    overflow: hidden;
    position: relative;

    &:hover {
      transform: translateY(-3px);
      box-shadow: var(--m3-elevation-2);
      border-color: var(--m3-primary);
    }

    .stat-content {
      display: flex;
      align-items: center;
      justify-content: space-between;
    }

    .stat-info {
      .stat-value {
        font-size: 28px;
        font-weight: 700;
        color: var(--m3-primary);
        line-height: 1.2;
      }

      .stat-title {
        font-size: 13px;
        color: var(--m3-on-surface-variant);
        margin-top: 4px;
      }
    }

    .stat-icon {
      font-size: 36px;
      opacity: 0.6;
    }
  }
}

// ==========================================
// 图表行
// ==========================================
.charts-row {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
  gap: 20px;
  margin-bottom: 24px;

  .chart-card {
    background: var(--m3-surface-container);
    border: 1px solid var(--m3-outline-variant);
    border-radius: var(--m3-shape-corner-large);
    padding: 20px;
    transition: all var(--m3-motion-duration-medium2) var(--m3-motion-easing-standard);

    &:hover {
      border-color: var(--m3-outline);
    }

    .card-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 16px;
      font-size: 15px;
      font-weight: 600;
      color: var(--m3-on-surface);

      span {
        color: var(--m3-primary);
      }
    }
  }
}

// ==========================================
// 性能指标网格
// ==========================================
.metrics-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
  gap: 12px;
  margin-top: 16px;

  .metric-item {
    background: var(--m3-surface-container-high);
    border: 1px solid var(--m3-outline-variant);
    border-radius: var(--m3-shape-corner-medium);
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
.emotion-pulse-container {
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
.federated-panel {
  .fed-stats {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
    gap: 12px;
    margin-bottom: 16px;

    .fed-stat-item {
      background: var(--m3-surface-container-high);
      border: 1px solid var(--m3-outline-variant);
      border-radius: var(--m3-shape-corner-medium);
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
.privacy-panel {
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
.vector-search-panel {
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
      background: var(--m3-surface-container-high);
      border: 1px solid var(--m3-outline-variant);
      border-radius: var(--m3-shape-corner-medium);
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
        color: var(--m3-on-surface);
        line-height: 1.5;
      }
    }
  }
}

// ==========================================
// 配置管理
// ==========================================
.module-card {
  background: var(--m3-surface-container);
  border: 1px solid var(--m3-outline-variant);
  border-radius: var(--m3-shape-corner-large);
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
    border-top: 1px solid var(--m3-outline-variant);
  }
}

// ==========================================
// AI 工具面板
// ==========================================
.ai-tool-panel {
  .tool-result {
    margin-top: 16px;
    padding: 16px;
    background: var(--m3-surface-container-high);
    border: 1px solid var(--m3-outline-variant);
    border-radius: var(--m3-shape-corner-medium);
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
:deep(.el-card) {
  background: var(--m3-surface-container);
  border: 1px solid var(--m3-outline-variant);
  border-radius: var(--m3-shape-corner-large);
  color: var(--m3-on-surface);
  --el-card-bg-color: transparent;

  .el-card__header {
    border-bottom: 1px solid var(--m3-outline-variant);
    color: var(--m3-on-surface);
    padding: 16px 20px;
    font-weight: 600;
  }

  .el-card__body {
    color: var(--m3-on-surface);
  }
}

// el-button
:deep(.el-button) {
  --el-button-bg-color: transparent;
  --el-button-border-color: var(--m3-outline);
  --el-button-text-color: var(--m3-on-surface);
  --el-button-hover-bg-color: var(--m3-surface-container-high);
  --el-button-hover-border-color: var(--m3-primary);
  --el-button-hover-text-color: var(--m3-primary);
  border-radius: var(--m3-shape-corner-small);
  transition: all var(--m3-motion-duration-short2) var(--m3-motion-easing-standard);
}

:deep(.el-button--primary) {
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

:deep(.el-button--success) {
  --el-button-bg-color: var(--m3-success-container);
  border-color: var(--m3-success) !important;
  color: var(--m3-success) !important;

  &:hover {
    background: var(--m3-success-container) !important;
  }
}

:deep(.el-button--warning) {
  --el-button-bg-color: var(--m3-tertiary-container);
  border-color: var(--m3-tertiary) !important;
  color: var(--m3-tertiary) !important;

  &:hover {
    background: var(--m3-tertiary-container) !important;
  }
}

:deep(.el-button--danger) {
  --el-button-bg-color: var(--m3-error-container);
  border-color: var(--m3-error) !important;
  color: var(--m3-error) !important;

  &:hover {
    background: var(--m3-error-container) !important;
  }
}

// el-input
:deep(.el-input) {
  .el-input__wrapper {
    background: var(--m3-surface-container-highest);
    border: 1px solid var(--m3-outline-variant);
    border-radius: var(--m3-shape-corner-small);
    box-shadow: none !important;
    transition: all var(--m3-motion-duration-short2) var(--m3-motion-easing-standard);

    &:hover {
      border-color: var(--m3-outline);
    }

    &.is-focus {
      border-color: var(--m3-primary);
    }
  }

  .el-input__inner {
    color: var(--m3-on-surface);

    &::placeholder {
      color: var(--m3-on-surface-variant);
      opacity: 0.6;
    }
  }
}

// el-textarea
:deep(.el-textarea) {
  .el-textarea__inner {
    background: var(--m3-surface-container-highest);
    border: 1px solid var(--m3-outline-variant);
    border-radius: var(--m3-shape-corner-small);
    color: var(--m3-on-surface);
    box-shadow: none !important;
    transition: all var(--m3-motion-duration-short2) var(--m3-motion-easing-standard);

    &:hover {
      border-color: var(--m3-outline);
    }

    &:focus {
      border-color: var(--m3-primary);
    }

    &::placeholder {
      color: var(--m3-on-surface-variant);
      opacity: 0.6;
    }
  }
}

// el-select
:deep(.el-select) {
  .el-select__wrapper {
    background: var(--m3-surface-container-highest) !important;
    border: 1px solid var(--m3-outline-variant);
    border-radius: var(--m3-shape-corner-small);
    box-shadow: none !important;

    &:hover {
      border-color: var(--m3-outline);
    }
  }

  .el-select__selected-item {
    color: var(--m3-on-surface);
  }

  .el-select__placeholder {
    color: var(--m3-on-surface-variant);
    opacity: 0.6;
  }
}

// el-input-number
:deep(.el-input-number) {
  .el-input__wrapper {
    background: var(--m3-surface-container-highest);
    border: 1px solid var(--m3-outline-variant);
    border-radius: var(--m3-shape-corner-small);
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
:deep(.el-switch) {
  --el-switch-on-color: var(--m3-primary);
  --el-switch-off-color: var(--m3-surface-container-highest);
}

// el-progress
:deep(.el-progress) {
  .el-progress-bar__outer {
    background: var(--m3-surface-container-highest);
    border-radius: var(--m3-shape-corner-small);
  }

  .el-progress-bar__inner {
    background: var(--m3-primary);
    border-radius: var(--m3-shape-corner-small);
  }

  .el-progress__text {
    color: var(--m3-on-surface-variant);
    font-size: 12px !important;
  }
}

// el-table
:deep(.el-table) {
  --el-table-bg-color: transparent;
  --el-table-tr-bg-color: transparent;
  --el-table-header-bg-color: var(--m3-surface-container);
  --el-table-row-hover-bg-color: var(--m3-surface-container-high);
  --el-table-border-color: var(--m3-outline-variant);
  --el-table-text-color: var(--m3-on-surface);
  --el-table-header-text-color: var(--m3-on-surface-variant);
  background: transparent;
  color: var(--m3-on-surface);

  &::before {
    display: none;
  }

  th.el-table__cell {
    background: var(--m3-surface-container) !important;
    border-bottom: 1px solid var(--m3-outline-variant) !important;
    font-weight: 600;
    font-size: 12px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: var(--m3-on-surface-variant);
  }

  td.el-table__cell {
    border-bottom: 1px solid var(--m3-outline-variant) !important;
  }

  .el-table__body tr:hover > td {
    background: var(--m3-surface-container-high) !important;
  }
}

// el-tag
:deep(.el-tag) {
  border-radius: var(--m3-shape-corner-small);
  border: none;
  font-size: 11px;
  font-weight: 500;
}

:deep(.el-tag--success) {
  background: var(--m3-success-container);
  color: var(--m3-success);
}

:deep(.el-tag--warning) {
  background: var(--m3-tertiary-container);
  color: var(--m3-tertiary);
}

:deep(.el-tag--danger) {
  background: var(--m3-error-container);
  color: var(--m3-error);
}

:deep(.el-tag--info) {
  background: var(--m3-secondary-container);
  color: var(--m3-secondary);
}

// el-form
:deep(.el-form) {
  .el-form-item__label {
    color: var(--m3-on-surface-variant);
    font-size: 13px;
  }
}

// v-loading
:deep(.el-loading-mask) {
  background: rgba(0, 0, 0, 0.5);
}

:deep(.el-loading-spinner) {
  .circular .path {
    stroke: var(--m3-primary);
  }

  .el-loading-text {
    color: var(--m3-on-surface-variant);
  }
}

// el-empty
:deep(.el-empty) {
  .el-empty__description p {
    color: var(--m3-on-surface-variant);
  }
}

// ==========================================
// 响应式
// ==========================================
@media (max-width: 768px) {
  .edge-ai {
    padding: 16px;
  }

  .charts-row {
    grid-template-columns: 1fr;
  }

  .emotion-pulse-container {
    flex-direction: column;
    height: auto;

    .gauge-wrapper,
    .pulse-line-wrapper {
      flex: none;
      min-height: 220px;
    }
  }

  .stats-cards {
    grid-template-columns: repeat(2, 1fr);
  }

  .page-header .welcome-section h1 {
    font-size: 22px;
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
</style>
