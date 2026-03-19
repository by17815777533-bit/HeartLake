<!--
  内容管理页面 -- 石头与纸船的查看、筛选、删除

  功能：
  - 按类型（石头/纸船）、状态（已发布/待审核/已删除）、关键词筛选
  - 关键词输入 300ms 防抖，减少无效请求
  - 删除操作强制输入原因（>=5字），根据内容类型调用不同 API
  - 内容详情弹窗展示完整文本
-->

<template>
  <div class="content-page">
    <!-- 筛选 -->
    <el-card
      shadow="never"
      class="filter-card"
    >
      <el-form
        :model="filters"
        inline
        aria-label="内容筛选"
      >
        <el-form-item label="类型">
          <el-select
            v-model="filters.type"
            placeholder="全部"
            clearable
            style="width: 120px"
          >
            <el-option
              label="石头"
              value="stone"
            />
            <el-option
              label="纸船"
              value="boat"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="状态">
          <el-select
            v-model="filters.status"
            placeholder="全部"
            clearable
            style="width: 120px"
          >
            <el-option
              label="已发布"
              value="published"
            />
            <el-option
              label="待审核"
              value="pending"
            />
            <el-option
              label="已删除"
              value="deleted"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="关键词">
          <el-input
            v-model="filters.keyword"
            placeholder="搜索内容"
            clearable
            @input="onKeywordInput"
          />
        </el-form-item>
        <el-form-item>
          <el-button
            type="primary"
            @click="handleSearch"
          >
            搜索
          </el-button>
          <el-button @click="handleReset">
            重置
          </el-button>
        </el-form-item>
      </el-form>
    </el-card>

    <!-- 内容列表 -->
    <el-card shadow="never">
      <el-table
        v-loading="loading"
        :data="contentList"
        stripe
        aria-label="内容列表"
      >
        <el-table-column
          prop="id"
          label="ID"
          width="100"
        />
        <el-table-column
          label="类型"
          width="80"
        >
          <template #default="{ row }">
            <el-tag
              :type="row.type === 'stone' ? 'primary' : 'success'"
              size="small"
            >
              {{ row.type === 'stone' ? '石头' : '纸船' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column
          label="内容"
          min-width="300"
        >
          <template #default="{ row }">
            <p class="content-text">
              {{ row.content?.substring(0, 100) }}{{ row.content?.length > 100 ? '...' : '' }}
            </p>
          </template>
        </el-table-column>
        <el-table-column
          label="作者"
          width="120"
        >
          <template #default="{ row }">
            {{ row.user?.nickname || '匿名' }}
          </template>
        </el-table-column>
        <el-table-column
          label="状态"
          width="100"
        >
          <template #default="{ row }">
            <el-tag
              :type="getStatusType(row.status)"
              size="small"
            >
              {{ getStatusLabel(row.status) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column
          prop="created_at"
          label="发布时间"
          width="180"
        />
        <el-table-column
          label="操作"
          width="150"
          fixed="right"
        >
          <template #default="{ row }">
            <el-button
              type="primary"
              link
              @click="viewContent(row)"
            >
              查看
            </el-button>
            <el-button
              type="danger"
              link
              @click="deleteContent(row)"
            >
              删除
            </el-button>
          </template>
        </el-table-column>
        <template #empty>
          <el-empty
            description="暂无内容数据"
            :image-size="120"
          />
        </template>
      </el-table>

      <div class="pagination-wrapper">
        <el-pagination
          v-model:current-page="pagination.page"
          v-model:page-size="pagination.pageSize"
          :total="pagination.total"
          :page-sizes="[10, 20, 50]"
          layout="total, sizes, prev, pager, next"
          @size-change="handleSizeChange"
          @current-change="handleCurrentChange"
        />
      </div>
    </el-card>

    <!-- 内容详情弹窗 -->
    <el-dialog
      v-model="detailVisible"
      title="内容详情"
      width="600px"
      aria-labelledby="content-detail-title"
    >
      <div v-if="currentContent">
        <el-descriptions
          :column="2"
          border
        >
          <el-descriptions-item label="ID">
            {{ currentContent.id }}
          </el-descriptions-item>
          <el-descriptions-item label="类型">
            {{ currentContent.type === 'stone' ? '石头' : '纸船' }}
          </el-descriptions-item>
          <el-descriptions-item label="作者">
            {{ currentContent.user?.nickname || '匿名' }}
          </el-descriptions-item>
          <el-descriptions-item label="状态">
            {{ getStatusLabel(currentContent.status) }}
          </el-descriptions-item>
          <el-descriptions-item
            label="发布时间"
            :span="2"
          >
            {{ currentContent.created_at }}
          </el-descriptions-item>
          <el-descriptions-item
            label="内容"
            :span="2"
          >
            <div class="detail-content">
              {{ currentContent.content }}
            </div>
          </el-descriptions-item>
        </el-descriptions>
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import type { ContentItem } from '@/types'

const loading = ref(false)
const contentList = ref<ContentItem[]>([])
const detailVisible = ref(false)
const currentContent = ref<ContentItem | null>(null)

// 搜索关键词输入防抖，避免每次按键都触发请求
let keywordDebounceTimer: ReturnType<typeof setTimeout> | null = null
const onKeywordInput = () => {
  if (keywordDebounceTimer) clearTimeout(keywordDebounceTimer)
  keywordDebounceTimer = setTimeout(() => {
    handleSearch()
  }, 300)
}
onUnmounted(() => {
  if (keywordDebounceTimer) clearTimeout(keywordDebounceTimer)
})

const filters = reactive({
  type: '',
  status: '',
  keyword: '',
})

const { pagination, buildParams, handleSizeChange, handleCurrentChange, handleSearch, handleReset } = useTablePagination(fetchContent, {
  filters,
  defaultFilters: { type: '', status: '', keyword: '' },
  beforeSearch: () => {
    filters.keyword = filters.keyword.trim()
    if (filters.keyword.length > 100) {
      ElMessage.warning('搜索关键词过长，请精简后重试')
      return false
    }
  },
})

/** 内容状态 → el-tag type 映射 */
const getStatusType = (status: string) => {
  const map: Record<string, string> = { published: 'success', pending: 'warning', deleted: 'danger' }
  return map[status] || 'info'
}

/** 内容状态 → 中文标签映射 */
const getStatusLabel = (status: string) => {
  const map: Record<string, string> = { published: '已发布', pending: '待审核', deleted: '已删除' }
  return map[status] || status
}

/**
 * 拉取内容列表，兼容后端 stones/boats/list 三种响应字段名。
 * 统一映射为前端 ContentItem 结构。
 */
async function fetchContent() {
  loading.value = true
  try {
    const params = buildParams(filters)
    const currentPage = Number(params.page || 1)
    const pageSize = Number(params.page_size || 20)

    if (filters.type === 'boat') {
      const res = await api.getBoats(params)
      const resData = res.data?.data || res.data || {}
      const list = resData.list || []
      contentList.value = list.map(item => ({
        id: item.boat_id || item.id,
        type: 'boat',
        content: item.content,
        user: { nickname: item.author_nickname || item.nickname },
        status: item.status,
        created_at: item.created_at,
      }))
      pagination.total = resData.total || 0
      return
    }

    if (filters.type === 'stone') {
      const res = await api.getStones(params)
      const resData = res.data?.data || res.data || {}
      const list = resData.list || []
      contentList.value = list.map(item => ({
        id: item.stone_id || item.id,
        type: 'stone',
        content: item.content,
        user: { nickname: item.author_nickname || item.nickname },
        status: item.status,
        created_at: item.created_at,
      }))
      pagination.total = resData.total || 0
      return
    }

    // 后端当前未提供统一 contents 路由，默认页在前端合并石头和纸船。
    const mergedPageSize = currentPage * pageSize
    const [stonesRes, boatsRes] = await Promise.all([
      api.getStones({ page: 1, page_size: mergedPageSize }),
      api.getBoats({ page: 1, page_size: mergedPageSize }),
    ])
    const stonesData = stonesRes.data?.data || stonesRes.data || {}
    const boatsData = boatsRes.data?.data || boatsRes.data || {}
    const mergedList = [
      ...(stonesData.list || []).map(item => ({
        id: item.stone_id || item.id,
        type: 'stone',
        content: item.content,
        user: { nickname: item.author_nickname || item.nickname },
        status: item.status,
        created_at: item.created_at,
      })),
      ...(boatsData.list || []).map(item => ({
        id: item.boat_id || item.id,
        type: 'boat',
        content: item.content,
        user: { nickname: item.author_nickname || item.nickname },
        status: item.status,
        created_at: item.created_at,
      })),
    ]

    mergedList.sort((a, b) => String(b.created_at).localeCompare(String(a.created_at)))
    const start = (currentPage - 1) * pageSize
    contentList.value = mergedList.slice(start, start + pageSize)
    pagination.total = Number(stonesData.total || 0) + Number(boatsData.total || 0)
  } catch (e) {
    console.error('获取内容列表失败:', e)
    ElMessage.error(getErrorMessage(e, '获取内容列表失败'))
    contentList.value = []
    pagination.total = 0
  } finally {
    loading.value = false
  }
}

/** 打开内容详情弹窗 */
const viewContent = (row: { id: string; type: string; content: string; user?: { nickname: string }; status: string; created_at: string }) => {
  currentContent.value = row
  detailVisible.value = true
}

/**
 * 删除内容，需输入原因（>=5字）。
 * 根据内容类型分别调用 deleteStone / deleteBoat 接口。
 */
const deleteContent = async (row: { id: string; type: string }) => {
  const { value: reason } = await ElMessageBox.prompt('请输入删除原因', '删除内容', {
    confirmButtonText: '确定',
    cancelButtonText: '取消',
    inputPattern: /.{5,}/,
    inputErrorMessage: '删除原因至少需要5个字符',
  }).catch(() => ({ value: null }))

  if (!reason) return

  try {
    if (row.type === 'stone') {
      await api.deleteStone(row.id, reason)
    } else {
      await api.deleteBoat(row.id, reason)
    }
    ElMessage.success('删除成功')
    fetchContent()
  } catch (e) {
    console.error('删除内容失败:', e)
    ElMessage.error(getErrorMessage(e, '删除失败'))
  }
}

onMounted(() => {
  fetchContent()
})
</script>

<style lang="scss" scoped>
.content-page {
  .filter-card {
    margin-bottom: 16px;
  }

  .content-text {
    margin: 0;
    color: var(--m3-on-surface-variant);
    line-height: 1.5;
  }

  .pagination-wrapper {
    margin-top: 20px;
    display: flex;
    justify-content: flex-end;
  }

  .detail-content {
    color: var(--m3-on-surface);
    white-space: pre-wrap;
    line-height: 1.6;
  }
}
</style>
