<!--
  配置区重构：
  - 表单分组更像一张策略面板，而不是默认设置页
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col :span="24">
      <el-card
        shadow="hover"
        class="chart-card config-card"
      >
        <template #header>
          <div class="config-head">
            <div>
              <span class="config-eyebrow">Engine Policy</span>
              <h3>辅助功能设置</h3>
            </div>
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
                  <el-option label="ONNX Runtime" value="onnx" />
                  <el-option label="TensorFlow Lite" value="tflite" />
                  <el-option label="WASM" value="wasm" />
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
                  <el-option label="LRU" value="lru" />
                  <el-option label="LFU" value="lfu" />
                  <el-option label="TTL" value="ttl" />
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
                  <el-option label="轻量级 (Fast)" value="fast" />
                  <el-option label="标准 (Standard)" value="standard" />
                  <el-option label="高精度 (Accurate)" value="accurate" />
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

<style scoped lang="scss">
.config-card {
  position: relative;
  overflow: hidden;
}

.config-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;

  h3 {
    margin: 8px 0 0;
    color: var(--hl-ink);
    font-family: var(--hl-font-display);
    font-size: clamp(22px, 3vw, 28px);
    line-height: 1.08;
  }
}

.config-eyebrow {
  display: inline-flex;
  min-height: 26px;
  align-items: center;
  padding: 0 10px;
  border-radius: 999px;
  background: rgba(17, 62, 74, 0.08);
  color: var(--m3-primary);
  font-family: var(--hl-font-mono);
  font-size: 10px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.config-form {
  padding-top: 2px;
}

.config-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  margin-top: 12px;
}

@media (max-width: 900px) {
  .config-head {
    flex-direction: column;
  }
}
</style>
