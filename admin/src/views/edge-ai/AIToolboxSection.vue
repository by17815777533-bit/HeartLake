<!--
  工具箱区重构：
  - 情感分析和内容审核统一为两张实验卡
-->

<template>
  <el-row
    :gutter="20"
    class="section-row"
  >
    <el-col
      :xs="24"
      :lg="12"
    >
      <el-card
        shadow="hover"
        class="module-card toolbox-card"
      >
        <template #header>
          <div class="tool-head">
            <span><el-icon><DataAnalysis /></el-icon> 情绪判断</span>
            <span class="tool-note">实验台</span>
          </div>
        </template>
        <div class="ai-tool-panel">
          <el-input
            v-model="sentimentTool.text"
            type="textarea"
            :rows="4"
            placeholder="输入内容，看看整体情绪..."
            maxlength="500"
            show-word-limit
          />
          <el-button
            type="primary"
            :loading="sentimentTool.loading"
            :disabled="!sentimentTool.text.trim()"
            class="tool-action"
            @click="$emit('analyzeSentiment')"
          >
            开始判断
          </el-button>
          <div
            v-if="sentimentTool.result"
            class="tool-result"
          >
            <div class="result-item">
              <span class="result-label">判断结果</span>
              <el-tag
                :type="sentimentTool.result.sentiment === 'positive' ? 'success' : sentimentTool.result.sentiment === 'negative' ? 'danger' : 'info'"
                size="large"
              >
                {{ { positive: '积极', negative: '消极', neutral: '中性' }[sentimentTool.result.sentiment] || sentimentTool.result.sentiment }}
              </el-tag>
            </div>
            <div class="result-item">
              <span class="result-label">把握度</span>
              <el-progress
                :percentage="Math.round((sentimentTool.result.confidence || 0) * 100)"
                :stroke-width="10"
                :color="(sentimentTool.result.confidence || 0) > 0.8 ? '#4d8f6b' : '#b67a42'"
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

    <el-col
      :xs="24"
      :lg="12"
    >
      <el-card
        shadow="hover"
        class="module-card toolbox-card"
      >
        <template #header>
          <div class="tool-head">
            <span><el-icon><CircleCheck /></el-icon> 内容检查</span>
            <span class="tool-note">实验台</span>
          </div>
        </template>
        <div class="ai-tool-panel">
          <el-input
            v-model="moderationTool.text"
            type="textarea"
            :rows="4"
            placeholder="输入内容，检查是否需要关注..."
            maxlength="500"
            show-word-limit
          />
          <el-button
            type="primary"
            :loading="moderationTool.loading"
            :disabled="!moderationTool.text.trim()"
            class="tool-action"
            @click="$emit('moderateContent')"
          >
            开始检查
          </el-button>
          <div
            v-if="moderationTool.result"
            class="tool-result"
          >
            <div class="result-item">
              <span class="result-label">检查结果</span>
              <el-tag
                :type="moderationTool.result.pass ? 'success' : 'danger'"
                size="large"
              >
                {{ moderationTool.result.pass ? '通过' : '未通过' }}
              </el-tag>
            </div>
            <div class="result-item">
              <span class="result-label">风险程度</span>
              <el-tag :type="moderationTool.result.risk === 'low' ? 'success' : moderationTool.result.risk === 'medium' ? 'warning' : 'danger'">
                {{ { low: '低风险', medium: '中风险', high: '高风险' }[moderationTool.result.risk] || moderationTool.result.risk || '-' }}
              </el-tag>
            </div>
            <div
              v-if="moderationTool.result.reason"
              class="result-item"
            >
              <span class="result-label">说明</span>
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

<style scoped lang="scss">
.toolbox-card {
  position: relative;
  overflow: hidden;
}

.tool-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  color: var(--hl-ink);
  font-size: 16px;
  font-weight: 700;
}

.tool-note {
  color: var(--hl-ink-soft);
  font-size: 12px;
  font-weight: 600;
}

.ai-tool-panel {
  display: grid;
  gap: 14px;
}

.tool-action {
  width: fit-content;
}

.tool-result {
  display: grid;
  gap: 14px;
  padding: 16px;
  border-radius: 20px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.66);
}

.result-item {
  display: grid;
  gap: 10px;
}

.result-label {
  color: var(--hl-ink-soft);
  font-size: 12px;
  font-weight: 600;
}

.emotion-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.reason-text {
  color: var(--hl-ink);
  font-size: 13px;
  line-height: 1.7;
}
</style>
