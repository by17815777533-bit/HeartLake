<!--
  双记忆 RAG 系统指标面板

  展示心湖守护链路的核心指标：
  - 短期记忆（会话上下文）和长期记忆（用户画像）的命中率
  - 检索延迟、文档总量、索引更新频率等运维指标
  数据来自 EdgeAI metrics 接口的 dual_memory_rag 字段
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
            <span>历史记录概览</span>
            <el-tag
              type="success"
              size="small"
            >
              服务参考
            </el-tag>
          </div>
        </template>
        <el-row :gutter="20">
          <el-col
            :xs="24"
            :md="6"
          >
            <div class="rag-stat-item">
              <div class="rag-stat-value">
                {{ ragStats.active_users || 0 }}
              </div>
              <div class="rag-stat-label">
                有记录的用户
              </div>
            </div>
          </el-col>
          <el-col
            :xs="24"
            :md="6"
          >
            <div class="rag-stat-item">
              <div class="rag-stat-value">
                {{ ragStats.total_short_term_entries || 0 }}
              </div>
              <div class="rag-stat-label">
                近期对话条目
              </div>
              <div class="rag-stat-desc">
                最近互动（每人最多 {{ ragStats.max_short_term_entries || 5 }} 条）
              </div>
            </div>
          </el-col>
          <el-col
            :xs="24"
            :md="6"
          >
            <div class="rag-stat-item">
              <div class="rag-stat-value">
                {{ ragStats.users_with_long_term_profile || 0 }}
              </div>
              <div class="rag-stat-label">
                长期关怀档案
              </div>
              <div class="rag-stat-desc">
                近 {{ ragStats.long_term_retention_days || 30 }} 天长期记录
              </div>
            </div>
          </el-col>
          <el-col
            :xs="24"
            :md="6"
          >
            <div class="rag-stat-item">
              <div class="rag-stat-value">
                {{ (ragStats.avg_emotion_score || 0).toFixed(2) }}
              </div>
              <div class="rag-stat-label">
                平均情绪指数
              </div>
              <el-progress
                :percentage="Math.min(100, Math.round((ragStats.avg_emotion_score || 0) * 100))"
                :stroke-width="8"
                :color="(ragStats.avg_emotion_score || 0) > 0.6 ? '#2E7D32' : (ragStats.avg_emotion_score || 0) > 0.3 ? '#E65100' : '#C62828'"
                style="margin-top: 8px"
              />
            </div>
          </el-col>
        </el-row>
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
interface RAGStats {
  active_users?: number
  total_short_term_entries?: number
  max_short_term_entries?: number
  users_with_long_term_profile?: number
  long_term_retention_days?: number
  avg_emotion_score?: number
}

defineProps<{
  ragStats: RAGStats
}>()
</script>
