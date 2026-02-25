<!--
  @file Content.vue
  @brief Content 组件
  Created by 林子怡
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

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'

const loading = ref(false)
const contentList = ref([])
const detailVisible = ref(false)
const currentContent = ref(null)

const filters = reactive({
  type: '',
  status: '',
  keyword: '',
})

const { pagination, handleSizeChange, handleCurrentChange, handleSearch, handleReset } = useTablePagination(fetchContent, {
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

const getStatusType = (status) => {
  const map = { published: 'success', pending: 'warning', deleted: 'danger' }
  return map[status] || 'info'
}

const getStatusLabel = (status) => {
  const map = { published: '已发布', pending: '待审核', deleted: '已删除' }
  return map[status] || status
}

const fetchContent = async () => {
  loading.value = true
  try {
    const params = {
      page: pagination.page,
      page_size: pagination.pageSize,
    }
    if (filters.status) params.status = filters.status
    if (filters.keyword) params.keyword = filters.keyword

    let res
    if (filters.type === 'boat') {
      res = await api.getBoats(params)
    } else {
      res = await api.getStones(params)
    }

    const resData = res.data?.data || res.data || {}
    const list = resData.stones || resData.boats || resData.list || []
    contentList.value = list.map(item => ({
      id: item.stone_id || item.boat_id || item.id,
      type: filters.type || item.type || 'stone',
      content: item.content,
      user: { nickname: item.author_nickname || item.nickname },
      status: item.status,
      created_at: item.created_at,
    }))
    pagination.total = resData.total || 0
  } catch (e) {
    console.error('获取内容列表失败:', e)
    contentList.value = []
    pagination.total = 0
  } finally {
    loading.value = false
  }
}

const viewContent = (row) => {
  currentContent.value = row
  detailVisible.value = true
}

// M-6: 删除内容需输入原因
const deleteContent = async (row) => {
  const { value: reason } = await ElMessageBox.prompt('请输入删除原因', '删除内容', {
    confirmButtonText: '确定',
    cancelButtonText: '取消',
    inputPattern: /\S+/,
    inputErrorMessage: '请输入删除原因',
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
