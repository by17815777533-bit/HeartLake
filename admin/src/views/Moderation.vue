<!--
  @file Moderation.vue
  @brief 内容审核管理页面
-->

<template>
  <div class="moderation-page">
    <el-tabs v-model="activeTab" @tab-change="handleTabChange">
      <!-- 待审核 -->
      <el-tab-pane label="待审核" name="pending">
        <el-table v-loading="loading" :data="pendingList" stripe>
          <el-table-column prop="moderation_id" label="ID" width="80" />
          <el-table-column label="类型" width="80">
            <template #default="{ row }">
              <el-tag :type="row.content_type === 'stone' ? 'primary' : 'success'" size="small">
                {{ row.content_type === 'stone' ? '石头' : '纸船' }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column label="内容" min-width="250">
            <template #default="{ row }">
              <p class="content-preview">{{ row.content?.substring(0, 80) }}{{ row.content?.length > 80 ? '...' : '' }}</p>
            </template>
          </el-table-column>
          <el-table-column label="触发原因" width="150">
            <template #default="{ row }">
              <el-tag type="warning" size="small">{{ row.ai_reason || 'AI检测' }}</el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="created_at" label="提交时间" width="170" />
          <el-table-column label="操作" width="180" fixed="right">
            <template #default="{ row }">
              <el-button type="success" link @click="handleApprove(row)">通过</el-button>
              <el-button type="danger" link @click="handleReject(row)">拒绝</el-button>
              <el-button type="primary" link @click="viewDetail(row)">详情</el-button>
            </template>
          </el-table-column>
        </el-table>
        <div class="pagination-wrapper">
          <el-pagination v-model:current-page="pagination.page" v-model:page-size="pagination.pageSize"
            :total="pagination.total" :page-sizes="[10, 20, 50]" layout="total, sizes, prev, pager, next"
            @size-change="handlePendingSizeChange" @current-change="fetchPending" />
        </div>
      </el-tab-pane>

      <!-- 审核历史 -->
      <el-tab-pane label="审核历史" name="history">
        <el-card shadow="never" class="filter-card">
          <el-form :model="historyFilters" inline>
            <el-form-item label="结果">
              <el-select v-model="historyFilters.result" placeholder="全部" clearable style="width: 120px">
                <el-option label="通过" value="approved" />
                <el-option label="拒绝" value="rejected" />
              </el-select>
            </el-form-item>
            <el-form-item>
              <el-button type="primary" @click="fetchHistory">搜索</el-button>
            </el-form-item>
          </el-form>
        </el-card>
        <el-table v-loading="historyLoading" :data="historyList" stripe>
          <el-table-column prop="moderation_id" label="ID" width="80" />
          <el-table-column label="内容摘要" min-width="200">
            <template #default="{ row }">{{ row.content?.substring(0, 50) }}...</template>
          </el-table-column>
          <el-table-column label="审核结果" width="100">
            <template #default="{ row }">
              <el-tag :type="row.result === 'approved' ? 'success' : 'danger'" size="small">
                {{ row.result === 'approved' ? '通过' : '拒绝' }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="moderator" label="操作人" width="100" />
          <el-table-column prop="moderated_at" label="处理时间" width="170" />
        </el-table>
        <div class="pagination-wrapper">
          <el-pagination v-model:current-page="historyPagination.page" :total="historyPagination.total"
            :page-sizes="[20, 50]" layout="total, prev, pager, next" @current-change="fetchHistory" />
        </div>
      </el-tab-pane>
    </el-tabs>

    <!-- 详情弹窗 -->
    <el-dialog v-model="detailVisible" title="内容详情" width="600px">
      <div v-if="currentItem">
        <el-descriptions :column="2" border>
          <el-descriptions-item label="ID">{{ currentItem.moderation_id || currentItem.content_id }}</el-descriptions-item>
          <el-descriptions-item label="类型">{{ currentItem.content_type === 'stone' ? '石头' : '纸船' }}</el-descriptions-item>
          <el-descriptions-item label="触发原因" :span="2">{{ currentItem.ai_reason || '自动检测' }}</el-descriptions-item>
          <el-descriptions-item label="内容" :span="2">
            <div class="detail-content">{{ currentItem.content }}</div>
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

const activeTab = ref('pending')
const loading = ref(false)
const historyLoading = ref(false)
const pendingList = ref([])
const historyList = ref([])
const detailVisible = ref(false)
const currentItem = ref(null)

const pagination = reactive({ page: 1, pageSize: 20, total: 0 })
const historyPagination = reactive({ page: 1, pageSize: 20, total: 0 })
const historyFilters = reactive({ result: '' })

const fetchPending = async () => {
  loading.value = true
  try {
    const res = await api.getPendingModeration({ page: pagination.page, page_size: pagination.pageSize })
    pendingList.value = res.data?.list || []
    pagination.total = res.data?.total || 0
  } catch (e) {
    // M-7: 空 catch 块添加 console.error
    console.error('获取待审核列表失败:', e)
    ElMessage.error(getErrorMessage(e, '获取待审核列表失败'))
    pendingList.value = []
  } finally {
    loading.value = false
  }
}

const fetchHistory = async () => {
  historyLoading.value = true
  try {
    const res = await api.getModerationHistory({ page: historyPagination.page, page_size: historyPagination.pageSize, ...historyFilters })
    historyList.value = res.data?.list || []
    historyPagination.total = res.data?.total || 0
  } catch (e) {
    // M-7: 空 catch 块添加 console.error
    console.error('获取审核历史失败:', e)
    ElMessage.error(getErrorMessage(e, '获取审核历史失败'))
    historyList.value = []
  } finally {
    historyLoading.value = false
  }
}

const handlePendingSizeChange = () => { pagination.page = 1; fetchPending() }

const handleTabChange = (tab) => { tab === 'pending' ? fetchPending() : fetchHistory() }

// M-6: 审核通过添加二次确认
const handleApprove = async (row) => {
  try {
    await ElMessageBox.confirm('确定通过该内容的审核吗？', '审核确认', {
      confirmButtonText: '确定通过',
      cancelButtonText: '取消',
      type: 'info',
    })
  } catch {
    return // 用户取消
  }
  try {
    await api.approveContent(row.moderation_id || row.content_id)
    ElMessage.success('已通过')
    fetchPending()
  } catch (e) {
    // M-7: 空 catch 块添加 console.error
    console.error('审核通过操作失败:', e)
    ElMessage.error(getErrorMessage(e, '操作失败'))
  }
}

const handleReject = async (row) => {
  const { value: reason } = await ElMessageBox.prompt('请输入拒绝原因', '拒绝内容', {
    confirmButtonText: '确定', cancelButtonText: '取消', inputPattern: /\S+/, inputErrorMessage: '请输入原因'
  }).catch(() => ({ value: null }))
  if (!reason) return
  try {
    await api.rejectContent(row.moderation_id || row.content_id, reason)
    ElMessage.success('已拒绝')
    fetchPending()
  } catch (e) {
    // M-7: 空 catch 块添加 console.error
    console.error('审核拒绝操作失败:', e)
    ElMessage.error(getErrorMessage(e, '操作失败'))
  }
}

const viewDetail = (row) => { currentItem.value = row; detailVisible.value = true }

onMounted(() => fetchPending())
</script>

<style lang="scss" scoped>
.moderation-page {
  :deep(.el-tabs__item) {
    color: #B8A99A;

    &.is-active {
      color: #F2CC8F;
    }
  }

  :deep(.el-tabs__active-bar) {
    background-color: #F2CC8F;
  }

  .filter-card {
    margin-bottom: 16px;
    background: rgba(26, 26, 62, 0.6);
    backdrop-filter: blur(16px);
    border: 1px solid rgba(242, 204, 143, 0.08);
    border-radius: 16px;

    :deep(.el-card__body) {
      padding: 16px 20px;
    }
  }

  :deep(.el-card) {
    background: rgba(26, 26, 62, 0.6);
    backdrop-filter: blur(16px);
    border: 1px solid rgba(242, 204, 143, 0.08);
    border-radius: 16px;
  }

  :deep(.el-table) {
    background: transparent;
    --el-table-bg-color: transparent;
    --el-table-tr-bg-color: transparent;
    --el-table-header-bg-color: rgba(26, 26, 62, 0.5);
    --el-table-row-hover-bg-color: rgba(242, 204, 143, 0.06);
    --el-table-border-color: rgba(242, 204, 143, 0.06);

    th {
      color: #B8A99A !important;
      font-weight: 500;
    }

    td {
      color: #F0E6D3;
      border-bottom-color: rgba(242, 204, 143, 0.06);
    }
  }

  .content-preview {
    margin: 0;
    color: #B8A99A;
    line-height: 1.5;
  }

  .pagination-wrapper {
    margin-top: 20px;
    display: flex;
    justify-content: flex-end;
  }

  .detail-content {
    color: #F0E6D3;
    white-space: pre-wrap;
    line-height: 1.6;
    max-height: 300px;
    overflow-y: auto;
  }
}
</style>
