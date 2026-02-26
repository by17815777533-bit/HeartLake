<!--
  @file VectorNodesSection.vue
  @brief 向量搜索测试 + 边缘节点列表
  Created by 林子怡
-->

<template>
  <el-row
    :gutter="20"
    class="charts-row"
  >
    <!-- 向量搜索测试 -->
    <el-col
      :xs="24"
      :md="12"
    >
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          <div class="card-header">
            <span>向量搜索测试</span>
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
              placeholder="输入文本，搜索语义相似内容..."
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
              搜索
            </el-button>
          </div>
          <div
            v-if="vectorResults.length"
            class="search-results"
          >
            <div
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
                {{ item.content }}
              </div>
            </div>
          </div>
          <el-empty
            v-else-if="vectorSearched"
            description="未找到相似内容"
            :image-size="80"
          />
        </div>
      </el-card>
    </el-col>

    <!-- 边缘节点列表 -->
    <el-col
      :xs="24"
      :md="12"
    >
      <el-card
        shadow="hover"
        class="chart-card"
      >
        <template #header>
          <div class="card-header">
            <span>边缘节点列表</span>
            <el-tag size="small">
              {{ edgeNodes.length }} 个节点
            </el-tag>
          </div>
        </template>
        <el-table
          :data="edgeNodes"
          stripe
          style="width: 100%"
          max-height="320"
          size="small"
        >
          <el-table-column
            prop="nodeId"
            label="节点ID"
            width="100"
          />
          <el-table-column
            prop="name"
            label="名称"
            min-width="100"
          />
          <el-table-column
            label="状态"
            width="80"
          >
            <template #default="{ row }">
              <el-tag
                :type="row.status === 'online' ? 'success' : row.status === 'busy' ? 'warning' : 'danger'"
                size="small"
              >
                {{ row.status === 'online' ? '在线' : row.status === 'busy' ? '繁忙' : '离线' }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column
            label="负载"
            width="100"
          >
            <template #default="{ row }">
              <el-progress
                :percentage="row.load"
                :stroke-width="6"
                :show-text="true"
                :color="row.load > 80 ? '#C62828' : row.load > 50 ? '#E65100' : '#2E7D32'"
              />
            </template>
          </el-table-column>
          <el-table-column
            label="延迟"
            width="80"
          >
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
