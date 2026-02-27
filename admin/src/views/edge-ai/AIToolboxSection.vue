<!--
  AI 工具箱 - 情感分析测试 + 内容审核测试
-->

<template>
  <el-row
    :gutter="20"
    class="section-row"
  >
    <!-- 情感分析测试 -->
    <el-col
      :xs="24"
      :md="12"
    >
      <el-card
        shadow="hover"
        class="module-card"
      >
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
            :loading="sentimentTool.loading"
            :disabled="!sentimentTool.text.trim()"
            style="margin-top: 12px"
            @click="$emit('analyzeSentiment')"
          >
            分析
          </el-button>
          <div
            v-if="sentimentTool.result"
            class="tool-result"
          >
            <div class="result-item">
              <span class="result-label">情感倾向</span>
              <el-tag
                :type="sentimentTool.result.sentiment === 'positive' ? 'success' : sentimentTool.result.sentiment === 'negative' ? 'danger' : 'info'"
                size="large"
              >
                {{ { positive: '积极', negative: '消极', neutral: '中性' }[sentimentTool.result.sentiment] || sentimentTool.result.sentiment }}
              </el-tag>
            </div>
            <div class="result-item">
              <span class="result-label">置信度</span>
              <el-progress
                :percentage="Math.round((sentimentTool.result.confidence || 0) * 100)"
                :stroke-width="10"
                :color="(sentimentTool.result.confidence || 0) > 0.8 ? '#2E7D32' : '#E65100'"
              />
            </div>
            <div
              v-if="sentimentTool.result.emotions"
              class="result-item"
            >
              <span class="result-label">情绪标签</span>
              <div class="emotion-tags">
                <el-tag
                  v-for="em in sentimentTool.result.emotions"
                  :key="em"
                  size="small"
                  type="info"
                >
                  {{ em }}
                </el-tag>
              </div>
            </div>
          </div>
        </div>
      </el-card>
    </el-col>

    <!-- 内容审核测试 -->
    <el-col
      :xs="24"
      :md="12"
    >
      <el-card
        shadow="hover"
        class="module-card"
      >
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
            :loading="moderationTool.loading"
            :disabled="!moderationTool.text.trim()"
            style="margin-top: 12px"
            @click="$emit('moderateContent')"
          >
            审核
          </el-button>
          <div
            v-if="moderationTool.result"
            class="tool-result"
          >
            <div class="result-item">
              <span class="result-label">审核结果</span>
              <el-tag
                :type="moderationTool.result.pass ? 'success' : 'danger'"
                size="large"
              >
                {{ moderationTool.result.pass ? '通过' : '未通过' }}
              </el-tag>
            </div>
            <div class="result-item">
              <span class="result-label">风险等级</span>
              <el-tag :type="moderationTool.result.risk === 'low' ? 'success' : moderationTool.result.risk === 'medium' ? 'warning' : 'danger'">
                {{ { low: '低风险', medium: '中风险', high: '高风险' }[moderationTool.result.risk] || moderationTool.result.risk || '-' }}
              </el-tag>
            </div>
            <div
              v-if="moderationTool.result.reason"
              class="result-item"
            >
              <span class="result-label">原因</span>
              <span class="result-value reason-text">{{ moderationTool.result.reason }}</span>
            </div>
          </div>
        </div>
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
import { DataAnalysis, CircleCheck } from '@element-plus/icons-vue'

interface SentimentResult {
  sentiment: string
  confidence?: number
  emotions?: string[]
}

interface ModerationResult {
  pass: boolean
  risk: string
  reason?: string
}

interface SentimentTool {
  text: string
  loading: boolean
  result: SentimentResult | null
}

interface ModerationTool {
  text: string
  loading: boolean
  result: ModerationResult | null
}

defineProps<{
  sentimentTool: SentimentTool
  moderationTool: ModerationTool
}>()

defineEmits<{
  (e: 'analyzeSentiment'): void
  (e: 'moderateContent'): void
}>()
</script>
