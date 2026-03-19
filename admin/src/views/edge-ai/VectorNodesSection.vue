<!--
  检索 + 节点区重构：
  - 左侧做成语义检索工作台
  - 右侧把原表格换成节点卡列，更接近运营态势板
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <el-col
      :xs="24"
      :lg="12"
    >
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          <div class="vector-head">
            <div>
              <span class="vector-eyebrow">Semantic Search</span>
              <h3>内容检索</h3>
            </div>
            <el-tag
              type="primary"
              size="small"
            >
              语义检索
            </el-tag>
          </div>
        </template>
        <div class="vector-search-panel">
          <div class="search-input-row">
            <el-input
              :model-value="vectorQuery"
              placeholder="输入一句话，查找相关内容..."
              :prefix-icon="Search"
              clearable
              maxlength="200"
              show-word-limit
              @update:model-value="$emit('update:vectorQuery', $event)"
              @keyup.enter="$emit('search')"
            />
            <el-button
              type="primary"
              :loading="vectorSearching"
              @click="$emit('search')"
            >
              查找
            </el-button>
          </div>

          <div
            v-if="vectorResults.length"
            class="search-results"
          >
            <article
              v-for="(item, idx) in vectorResults"
              :key="idx"
              class="search-result-item"
            >
              <div class="result-score">
                <el-tag
                  :type="item.score > 0.8 ? 'success' : item.score > 0.5 ? 'warning' : 'info'"
                  size="small"
                >
                  {{ (item.score * 100).toFixed(1) }}%
                </el-tag>
              </div>
              <div class="result-content">
                <strong>相似片段 {{ idx + 1 }}</strong>
                <p>{{ item.content }}</p>
              </div>
            </article>
          </div>

          <el-empty
            v-else-if="vectorSearched"
            description="没有找到相关内容"
            :image-size="80"
          />
        </div>
      </el-card>
    </el-col>

    <el-col
      :xs="24"
      :lg="12"
    >
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          <div class="vector-head">
            <div>
              <span class="vector-eyebrow">Node Mesh</span>
              <h3>服务节点</h3>
            </div>
            <el-tag size="small">
              {{ edgeNodes.length }} 个节点
            </el-tag>
          </div>
        </template>

        <div class="node-list">
          <article
            v-for="node in edgeNodes"
            :key="node.nodeId"
            class="node-card"
          >
            <div class="node-card__top">
              <div>
                <strong>{{ node.name }}</strong>
                <span>{{ node.nodeId }}</span>
              </div>
              <el-tag
                :type="node.status === 'online' ? 'success' : node.status === 'busy' ? 'warning' : 'danger'"
                size="small"
              >
                {{ node.status === 'online' ? '在线' : node.status === 'busy' ? '繁忙' : '离线' }}
              </el-tag>
            </div>

            <div class="node-card__metrics">
              <div class="node-chip">
                <span>忙碌度</span>
                <strong>{{ node.load }}%</strong>
              </div>
              <div class="node-chip">
                <span>响应</span>
                <strong>{{ node.latency }}ms</strong>
              </div>
            </div>

            <el-progress
              :percentage="node.load"
              :stroke-width="8"
              :show-text="false"
              :color="node.load > 80 ? '#a5483e' : node.load > 50 ? '#b67a42' : '#4d8f6b'"
            />
          </article>

          <el-empty
            v-if="!edgeNodes.length"
            description="暂无节点数据"
            :image-size="72"
          />
        </div>
      </el-card>
    </el-col>
  </el-row>
</template>

<script setup lang="ts">
import { Search } from '@element-plus/icons-vue'

interface VectorResult {
  score: number
  content: string
}

interface EdgeNode {
  nodeId: string
  name: string
  status: string
  load: number
  latency: number
}

defineProps<{
  vectorQuery: string
  vectorSearching: boolean
  vectorSearched: boolean
  vectorResults: VectorResult[]
  edgeNodes: EdgeNode[]
}>()

defineEmits<{
  (e: 'update:vectorQuery', val: string): void
  (e: 'search'): void
}>()
</script>

<style scoped lang="scss">
.vector-head {
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

.vector-eyebrow {
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

.vector-search-panel {
  display: grid;
  gap: 18px;
}

.search-input-row {
  display: grid;
  grid-template-columns: 1fr auto;
  gap: 12px;
}

.search-results,
.node-list {
  display: grid;
  gap: 14px;
}

.search-result-item,
.node-card {
  padding: 16px;
  border-radius: 22px;
  border: 1px solid rgba(123, 149, 160, 0.14);
  background: rgba(255, 255, 255, 0.66);
}

.search-result-item {
  display: grid;
  grid-template-columns: auto 1fr;
  gap: 14px;
  align-items: start;
}

.result-content {
  strong {
    display: block;
    color: var(--hl-ink);
    font-size: 13px;
    font-weight: 700;
  }

  p {
    margin: 8px 0 0;
    color: var(--hl-ink-soft);
    font-size: 13px;
    line-height: 1.7;
  }
}

.node-card__top {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;

  strong {
    display: block;
    color: var(--hl-ink);
    font-size: 14px;
    font-weight: 700;
  }

  span {
    display: block;
    margin-top: 6px;
    color: var(--hl-ink-soft);
    font-size: 12px;
  }
}

.node-card__metrics {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin: 14px 0;
}

.node-chip {
  padding: 12px 14px;
  border-radius: 16px;
  background: rgba(17, 62, 74, 0.05);

  span {
    display: block;
    color: var(--hl-ink-soft);
    font-size: 12px;
  }

  strong {
    display: block;
    margin-top: 6px;
    color: var(--hl-ink);
    font-size: 15px;
    font-weight: 700;
  }
}

@media (max-width: 900px) {
  .vector-head {
    flex-direction: column;
  }

  .search-input-row {
    grid-template-columns: 1fr;
  }
}
</style>
