<!--
  @file ConfigSection.vue
  @brief Edge AI 配置管理面板
  Created by 林子怡
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col :span="24">
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          <div class="card-header">
            <span>Edge AI 配置管理</span>
            <el-tag
              type="info"
              size="small"
            >
              运行时参数
            </el-tag>
          </div>
        </template>
        <el-form
          v-loading="configLoading"
          :model="edgeConfig"
          label-width="160px"
          class="config-form"
        >
          <el-row :gutter="24">
            <el-col
              :xs="24"
              :md="12"
            >
              <el-form-item label="推理引擎">
                <el-select
                  v-model="edgeConfig.inferenceEngine"
                  style="width: 100%"
                >
                  <el-option
                    label="ONNX Runtime"
                    value="onnx"
                  />
                  <el-option
                    label="TensorFlow Lite"
                    value="tflite"
                  />
                  <el-option
                    label="WASM"
                    value="wasm"
                  />
                </el-select>
              </el-form-item>
            </el-col>
            <el-col
              :xs="24"
              :md="12"
            >
              <el-form-item label="缓存策略">
                <el-select
                  v-model="edgeConfig.cacheStrategy"
                  style="width: 100%"
                >
                  <el-option
                    label="LRU"
                    value="lru"
                  />
                  <el-option
                    label="LFU"
                    value="lfu"
                  />
                  <el-option
                    label="TTL"
                    value="ttl"
                  />
                </el-select>
              </el-form-item>
            </el-col>
            <el-col
              :xs="24"
              :md="12"
            >
              <el-form-item label="最大推理批次">
                <el-input-number
                  v-model="edgeConfig.maxBatchSize"
                  :min="1"
                  :max="64"
                  style="width: 100%"
                />
              </el-form-item>
            </el-col>
            <el-col
              :xs="24"
              :md="12"
            >
              <el-form-item label="缓存大小 (MB)">
                <el-input-number
                  v-model="edgeConfig.cacheSizeMB"
                  :min="16"
                  :max="1024"
                  :step="16"
                  style="width: 100%"
                />
              </el-form-item>
            </el-col>
            <el-col
              :xs="24"
              :md="12"
            >
              <el-form-item label="隐私预算上限 (ε)">
                <el-input-number
                  v-model="edgeConfig.maxEpsilon"
                  :min="0.1"
                  :max="10"
                  :step="0.1"
                  :precision="1"
                  style="width: 100%"
                />
              </el-form-item>
            </el-col>
            <el-col
              :xs="24"
              :md="12"
            >
              <el-form-item label="联邦学习轮次间隔 (s)">
                <el-input-number
                  v-model="edgeConfig.federatedInterval"
                  :min="60"
                  :max="3600"
                  :step="60"
                  style="width: 100%"
                />
              </el-form-item>
            </el-col>
            <el-col
              :xs="24"
              :md="12"
            >
              <el-form-item label="情绪分析模型">
                <el-select
                  v-model="edgeConfig.emotionModel"
                  style="width: 100%"
                >
                  <el-option
                    label="轻量级 (Fast)"
                    value="fast"
                  />
                  <el-option
                    label="标准 (Standard)"
                    value="standard"
                  />
                  <el-option
                    label="高精度 (Accurate)"
                    value="accurate"
                  />
                </el-select>
              </el-form-item>
            </el-col>
            <el-col
              :xs="24"
              :md="12"
            >
              <el-form-item label="启用向量搜索">
                <el-switch v-model="edgeConfig.vectorSearchEnabled" />
              </el-form-item>
            </el-col>
          </el-row>
          <div class="config-actions">
            <el-button
              type="primary"
              :loading="configSaving"
              @click="$emit('save')"
            >
              保存配置
            </el-button>
            <el-button @click="$emit('reset')">
              重置
            </el-button>
          </div>
        </el-form>
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
interface EdgeConfig {
  inferenceEngine: string
  cacheStrategy: string
  maxBatchSize: number
  cacheSizeMB: number
  maxEpsilon: number
  federatedInterval: number
  emotionModel: string
  vectorSearchEnabled: boolean
}

defineProps<{
  edgeConfig: EdgeConfig
  configLoading: boolean
  configSaving: boolean
}>()

defineEmits<{
  (e: 'save'): void
  (e: 'reset'): void
}>()
</script>
