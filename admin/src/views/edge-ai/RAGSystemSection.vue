<!--
  RAG 记忆区重构：
  - 把四项指标做成统一的记忆卡组
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col :span="24">
      <el-card
        shadow="hover"
        class="chart-card rag-card"
      >
        <template #header>
          <div class="rag-head">
            <div>
              <span class="rag-eyebrow">Memory Ledger</span>
              <h3>历史记录概览</h3>
            </div>
            <el-tag
              type="success"
              size="small"
            >
              服务参考
            </el-tag>
          </div>
        </template>

        <div class="rag-grid">
          <article class="rag-stat-item">
            <span>有记录的用户</span>
            <strong>{{ ragStats.active_users || 0 }}</strong>
            <p>已经形成可参考轨迹的用户规模。</p>
          </article>

          <article class="rag-stat-item">
            <span>近期对话条目</span>
            <strong>{{ ragStats.total_short_term_entries || 0 }}</strong>
            <p>最近互动，每人最多 {{ ragStats.max_short_term_entries || 5 }} 条。</p>
          </article>

          <article class="rag-stat-item">
            <span>长期关怀档案</span>
            <strong>{{ ragStats.users_with_long_term_profile || 0 }}</strong>
            <p>近 {{ ragStats.long_term_retention_days || 30 }} 天长期记录。</p>
          </article>

          <article class="rag-stat-item">
            <span>平均情绪指数</span>
            <strong>{{ (ragStats.avg_emotion_score || 0).toFixed(2) }}</strong>
            <el-progress
              :percentage="Math.min(100, Math.round((ragStats.avg_emotion_score || 0) * 100))"
              :stroke-width="8"
              :show-text="false"
              :color="(ragStats.avg_emotion_score || 0) > 0.6 ? '#4d8f6b' : (ragStats.avg_emotion_score || 0) > 0.3 ? '#b67a42' : '#a35f5f'"
            />
          </article>
        </div>
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

<style scoped lang="scss">
.rag-card {
  position: relative;
  overflow: hidden;
}

.rag-head {
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

.rag-eyebrow {
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

.rag-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
}

.rag-stat-item {
  padding: 18px;
  border-radius: 22px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.66);

  span {
    display: block;
    color: var(--hl-ink-soft);
    font-size: 12px;
  }

  strong {
    display: block;
    margin: 10px 0 12px;
    color: var(--hl-ink);
    font-family: var(--hl-font-display);
    font-size: clamp(24px, 3vw, 32px);
    line-height: 1.04;
  }

  p {
    margin: 12px 0 0;
    color: var(--hl-ink-soft);
    font-size: 12px;
    line-height: 1.7;
  }
}

@media (max-width: 1100px) {
  .rag-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 900px) {
  .rag-head {
    flex-direction: column;
  }
}

@media (max-width: 640px) {
  .rag-grid {
    grid-template-columns: 1fr;
  }
}
</style>
