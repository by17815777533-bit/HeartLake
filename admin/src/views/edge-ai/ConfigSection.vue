<!--
  EdgeAI 引擎配置管理面板

  表单字段：
  - 推理超时(ms)、最大批处理大小、缓存开关及 TTL
  - 联邦学习开关、隐私 epsilon 值、情绪模型选择、向量搜索开关
  保存调用 PUT /admin/edge-ai/config，重置从 GET /admin/edge-ai/config 重新加载
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
            <span>辅助功能设置</span>
            <el-tag
              type="info"
              size="small"
            >
              当前设置
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
              <el-form-item label="回复方式">
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
              <el-form-item label="记录方式">
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
              <el-form-item label="每次处理上限">
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
              <el-form-item label="临时缓存大小 (MB)">
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
              <el-form-item label="保护额度上限">
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
              <el-form-item label="自动更新间隔 (s)">
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
              <el-form-item label="判断精细度">
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
              <el-form-item label="开启内容检索">
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
