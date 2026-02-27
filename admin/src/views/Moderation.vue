<!--
  内容审核管理页面
-->

<template>
  <div class="moderation-page">
    <el-tabs
      v-model="activeTab"
      @tab-change="handleTabChange"
    >
      <!-- 待审核 -->
      <el-tab-pane
        label="待审核"
        name="pending"
      >
        <el-table
          v-loading="loading"
          :data="pendingList"
          stripe
          aria-label="待审核内容列表"
        >
          <el-table-column
            prop="moderation_id"
            label="ID"
            width="80"
          />
          <el-table-column
            label="类型"
            width="80"
          >
            <template #default="{ row }">
              <el-tag
                :type="row.content_type === 'stone' ? 'primary' : 'success'"
                size="small"
              >
                {{ row.content_type === 'stone' ? '石头' : '纸船' }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column
            label="内容"
            min-width="250"
          >
            <template #default="{ row }">
              <p class="content-preview">
                {{ row.content?.substring(0, 80) }}{{ row.content?.length > 80 ? '...' : '' }}
              </p>
            </template>
          </el-table-column>
          <el-table-column
            label="触发原因"
            width="150"
          >
            <template #default="{ row }">
              <el-tag
                type="warning"
                size="small"
              >
                {{ row.ai_reason || 'AI检测' }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column
            prop="created_at"
            label="提交时间"
            width="170"
          />
          <el-table-column
            label="操作"
            width="180"
            fixed="right"
          >
            <template #default="{ row }">
              <el-button
                type="success"
                link
                @click="handleApprove(row)"
              >
                通过
              </el-button>
              <el-button
                type="danger"
                link
                @click="handleReject(row)"
              >
                拒绝
              </el-button>
              <el-button
                type="primary"
                link
                @click="viewDetail(row)"
              >
                详情
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
            @size-change="handlePendingSizeChange"
            @current-change="handlePendingCurrentChange"
          />
        </div>
      </el-tab-pane>

      <!-- 审核历史 -->
      <el-tab-pane
        label="审核历史"
        name="history"
      >
        <el-card
          shadow="never"
          class="filter-card"
        >
          <el-form
            :model="historyFilters"
            inline
            aria-label="审核历史筛选"
          >
            <el-form-item label="结果">
              <el-select
                v-model="historyFilters.result"
                placeholder="全部"
                clearable
                style="width: 120px"
              >
                <el-option
                  label="通过"
                  value="approved"
                />
                <el-option
                  label="拒绝"
                  value="rejected"
                />
              </el-select>
            </el-form-item>
            <el-form-item>
              <el-button
                type="primary"
                @click="fetchHistory"
              >
                搜索
              </el-button>
            </el-form-item>
          </el-form>
        </el-card>
        <el-table
          v-loading="historyLoading"
          :data="historyList"
          stripe
          aria-label="审核历史列表"
        >
          <el-table-column
            prop="moderation_id"
            label="ID"
            width="80"
          />
          <el-table-column
            label="内容摘要"
            min-width="200"
          >
            <template #default="{ row }">
              {{ row.content?.substring(0, 50) }}...
            </template>
          </el-table-column>
          <el-table-column
            label="审核结果"
            width="100"
          >
            <template #default="{ row }">
              <el-tag
                :type="row.result === 'approved' ? 'success' : 'danger'"
                size="small"
              >
                {{ row.result === 'approved' ? '通过' : '拒绝' }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column
            prop="moderator"
            label="操作人"
            width="100"
          />
          <el-table-column
            prop="moderated_at"
            label="处理时间"
            width="170"
          />
        </el-table>
        <div class="pagination-wrapper">
          <el-pagination
            v-model:current-page="historyPagination.page"
            v-model:page-size="historyPagination.pageSize"
            :total="historyPagination.total"
            :page-sizes="[20, 50]"
            layout="total, sizes, prev, pager, next"
            @size-change="handleHistorySizeChange"
            @current-change="handleHistoryCurrentChange"
          />
        </div>
      </el-tab-pane>
    </el-tabs>

    <!-- 详情弹窗 -->
    <el-dialog
      v-model="detailVisible"
      title="内容详情"
      width="600px"
      aria-labelledby="moderation-detail-title"
    >
      <div v-if="currentItem">
        <el-descriptions
          :column="2"
          border
        >
          <el-descriptions-item label="ID">
            {{ currentItem.moderation_id || currentItem.content_id }}
          </el-descriptions-item>
          <el-descriptions-item label="类型">
            {{ currentItem.content_type === 'stone' ? '石头' : '纸船' }}
          </el-descriptions-item>
          <el-descriptions-item
            label="触发原因"
            :span="2"
          >
            {{ currentItem.ai_reason || '自动检测' }}
          </el-descriptions-item>
          <el-descriptions-item
            label="内容"
            :span="2"
          >
            <div class="detail-content">
              {{ currentItem.content }}
            </div>
          </el-descriptions-item>
        </el-descriptions>
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import type { ModerationItem } from '@/types'

const activeTab = ref('pending')
const loading = ref(false)
const historyLoading = ref(false)
const pendingList = ref<ModerationItem[]>([])
const historyList = ref<ModerationItem[]>([])
const detailVisible = ref(false)
const currentItem = ref<ModerationItem | null>(null)

const historyFilters = reactive({ result: '' })

const { pagination, buildParams: buildPendingParams, handleSizeChange: handlePendingSizeChange, handleCurrentChange: handlePendingCurrentChange } = useTablePagination(fetchPending)
const {
  pagination: historyPagination,
  buildParams: buildHistoryParams,
  handleSizeChange: handleHistorySizeChange,
  handleCurrentChange: handleHistoryCurrentChange,
} = useTablePagination(fetchHistory, {
  filters: historyFilters,
  defaultFilters: { result: '' },
})

async function fetchPending() {
  loading.value = true
  try {
    const res = await api.getPendingModeration(buildPendingParams())
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

async function fetchHistory() {
  historyLoading.value = true
  try {
    const res = await api.getModerationHistory(buildHistoryParams(historyFilters))
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

const handleTabChange = (tab: string) => { tab === 'pending' ? fetchPending() : fetchHistory() }

// M-6: 审核通过添加二次确认
const handleApprove = async (row: ModerationItem) => {
  try {
    await ElMessageBox.confirm('确定通过该内容的审核吗？', '审核确认', {
      confirmButtonText: '确定通过',
      cancelButtonText: '取消',
      type: 'info',
    })
  } catch {
    return // 用户取消
  }
  // L-25: 乐观更新 — 先从本地列表移除，再异步请求后端
  const itemId = row.moderation_id || row.content_id
  const removedIndex = pendingList.value.findIndex(
    (item) => (item.moderation_id || item.content_id) === itemId
  )
  const removedItem = removedIndex >= 0 ? pendingList.value.splice(removedIndex, 1)[0] : null
  try {
    await api.approveContent(itemId)
    ElMessage.success('已通过')
    fetchPending() // 异步刷新获取最新数据
  } catch (e) {
    console.error('审核通过操作失败:', e)
    ElMessage.error(getErrorMessage(e, '操作失败'))
    // 回滚：操作失败时恢复被移除的项
    if (removedItem && removedIndex >= 0) {
      pendingList.value.splice(removedIndex, 0, removedItem)
    }
  }
}

const handleReject = async (row: ModerationItem) => {
  const { value: reason } = await ElMessageBox.prompt('请输入拒绝原因', '拒绝内容', {
    confirmButtonText: '确定', cancelButtonText: '取消', inputPattern: /\S+/, inputErrorMessage: '请输入原因'
  }).catch(() => ({ value: null }))
  if (!reason) return
  // L-25: 乐观更新 — 先从本地列表移除，再异步请求后端
  const itemId = row.moderation_id || row.content_id
  const removedIndex = pendingList.value.findIndex(
    (item) => (item.moderation_id || item.content_id) === itemId
  )
  const removedItem = removedIndex >= 0 ? pendingList.value.splice(removedIndex, 1)[0] : null
  try {
    await api.rejectContent(itemId, reason)
    ElMessage.success('已拒绝')
    fetchPending()
  } catch (e) {
    console.error('审核拒绝操作失败:', e)
    ElMessage.error(getErrorMessage(e, '操作失败'))
    if (removedItem && removedIndex >= 0) {
      pendingList.value.splice(removedIndex, 0, removedItem)
    }
  }
}

const viewDetail = (row: ModerationItem) => { currentItem.value = row; detailVisible.value = true }

onMounted(() => fetchPending())
</script>

<style lang="scss" scoped>
.moderation-page {
  .filter-card {
    margin-bottom: 16px;
  }

  .content-preview {
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
    max-height: 300px;
    overflow-y: auto;
  }
}
</style>
