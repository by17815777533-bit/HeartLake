<!--
  @file Reports.vue
  @brief Reports 组件
  Created by 林子怡
-->

<template>
  <div class="reports-page">
    <!-- 筛选 -->
    <el-card shadow="never" class="filter-card">
      <el-form :model="filters" inline>
        <el-form-item label="状态">
          <el-select v-model="filters.status" placeholder="全部" clearable style="width: 120px">
            <el-option label="待处理" value="pending" />
            <el-option label="已处理" value="handled" />
            <el-option label="已忽略" value="ignored" />
          </el-select>
        </el-form-item>
        <el-form-item label="类型">
          <el-select v-model="filters.type" placeholder="全部" clearable style="width: 120px">
            <el-option label="垃圾信息" value="spam" />
            <el-option label="骚扰辱骂" value="harassment" />
            <el-option label="不当内容" value="inappropriate" />
            <el-option label="暴力内容" value="violence" />
            <el-option label="其他" value="other" />
          </el-select>
        </el-form-item>
        <el-form-item>
          <el-button type="primary" @click="handleSearch">搜索</el-button>
          <el-button @click="handleReset">重置</el-button>
        </el-form-item>
      </el-form>
    </el-card>

    <!-- 举报列表 -->
    <el-card shadow="never">
      <el-table v-loading="loading" :data="reportList" stripe>
        <el-table-column prop="id" label="ID" width="100" />
        <el-table-column label="举报类型" width="100">
          <template #default="{ row }">
            <el-tag size="small" :color="getTypeColor(row.type)" style="border:none;color:#fff">{{ getTypeLabel(row.type) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column label="举报原因" min-width="200">
          <template #default="{ row }">{{ row.reason }}</template>
        </el-table-column>
        <el-table-column label="被举报内容" min-width="200">
          <template #default="{ row }">
            {{ row.target_content?.substring(0, 50) }}{{ row.target_content?.length > 50 ? '...' : '' }}
          </template>
        </el-table-column>
        <el-table-column label="状态" width="100">
          <template #default="{ row }">
            <el-tag :type="getStatusType(row.status)" size="small">
              {{ getStatusLabel(row.status) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="举报时间" width="180" />
        <el-table-column label="操作" width="200" fixed="right">
          <template #default="{ row }">
            <template v-if="row.status === 'pending'">
              <el-button type="success" link @click="handleReport(row, 'handled')">
                处理
              </el-button>
              <el-button type="info" link @click="handleReport(row, 'ignored')">
                忽略
              </el-button>
            </template>
            <span v-else class="handled-text">已{{ getStatusLabel(row.status) }}</span>
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
          @current-change="fetchReports"
        />
      </div>
    </el-card>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'
import { getErrorMessage } from '@/utils/errorHelper'

const loading = ref(false)
const reportList = ref([])

const filters = reactive({
  status: '',
  type: '',
})

const pagination = reactive({
  page: 1,
  pageSize: 20,
  total: 0,
})

const getTypeLabel = (type) => {
  const map = { spam: '垃圾信息', harassment: '骚扰辱骂', inappropriate: '不当内容', violence: '暴力内容', other: '其他' }
  return map[type] || type
}

const getTypeColor = (type) => {
  const map = { spam: '#909399', harassment: '#E07A5F', inappropriate: '#F2CC8F', violence: '#E6A23C', other: '#81B29A' }
  return map[type] || '#909399'
}

const getStatusType = (status) => {
  const map = { pending: 'warning', handled: 'success', ignored: 'info' }
  return map[status] || 'info'
}

const getStatusLabel = (status) => {
  const map = { pending: '待处理', handled: '已处理', ignored: '已忽略' }
  return map[status] || status
}

const fetchReports = async () => {
  loading.value = true
  try {
    const res = await api.getReports({
      page: pagination.page,
      page_size: pagination.pageSize,
      ...filters,
    })
    reportList.value = res.data?.list || []
    pagination.total = res.data?.total || 0
  } catch (e) {
    console.error('获取举报列表失败:', e)
    ElMessage.error(getErrorMessage(e, '获取举报列表失败'))
    reportList.value = []
    pagination.total = 0
  } finally {
    loading.value = false
  }
}

const handleSizeChange = () => {
  pagination.page = 1
  fetchReports()
}

const handleSearch = () => {
  pagination.page = 1
  fetchReports()
}

const handleReset = () => {
  Object.assign(filters, { status: '', type: '' })
  pagination.page = 1
  fetchReports()
}

const handleReport = async (row, action) => {
  const actionText = action === 'handled' ? '处理' : '忽略'

  try {
    await ElMessageBox.confirm(`确定要${actionText}此举报吗？`, '确认', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning',
    })
  } catch {
    return // 用户取消
  }

  try {
    await api.handleReport(row.id, { action, note: `管理员${actionText}` })
    ElMessage.success(`${actionText}成功`)
    fetchReports()
  } catch (e) {
    ElMessage.error(getErrorMessage(e, `${actionText}失败`))
  }
}

onMounted(() => {
  fetchReports()
})
</script>

<style lang="scss" scoped>
.reports-page {
  padding: 20px;

  .filter-card {
    margin-bottom: 16px;
  }

  .pagination-wrapper {
    margin-top: 20px;
    display: flex;
    justify-content: flex-end;
  }

  .handled-text {
    color: #7A6F63;
    font-size: 14px;
  }

  // ── 光遇主题：毛玻璃卡片 ──
  :deep(.el-card) {
    background: rgba(26, 26, 62, 0.6);
    backdrop-filter: blur(16px);
    border: 1px solid rgba(242, 204, 143, 0.12);
    border-radius: 16px;
    color: #F0E6D3;

    .el-card__body {
      color: #F0E6D3;
    }
  }

  // ── 表单标签 ──
  :deep(.el-form-item__label) {
    color: #B8A99A;
  }

  // ── 下拉选择器 ──
  :deep(.el-select) {
    .el-select__wrapper {
      background: rgba(20, 20, 50, 0.6);
      border: 1px solid rgba(242, 204, 143, 0.15);
      border-radius: 8px;
      box-shadow: none;
      color: #F0E6D3;

      &:hover {
        border-color: rgba(242, 204, 143, 0.3);
      }

      &.is-focused {
        border-color: #F2CC8F;
        box-shadow: 0 0 8px rgba(242, 204, 143, 0.15);
      }

      .el-select__selected-item {
        color: #F0E6D3;
      }

      .el-select__placeholder {
        color: #7A6F63;
      }

      .el-select__suffix {
        color: #B8A99A;
      }
    }
  }

  :deep(.el-select__popper) {
    background: rgba(26, 26, 62, 0.95);
    backdrop-filter: blur(16px);
    border: 1px solid rgba(242, 204, 143, 0.12);
    border-radius: 12px;

    .el-select-dropdown__item {
      color: #B8A99A;

      &.is-hovering {
        background: rgba(242, 204, 143, 0.08);
        color: #F2CC8F;
      }

      &.is-selected {
        color: #F2CC8F;
        font-weight: 600;
      }
    }
  }

  // ── 按钮 ──
  :deep(.el-button--primary:not(.is-link)) {
    background: linear-gradient(135deg, #F2CC8F, #E8A87C);
    border: none;
    color: #141432;
    font-weight: 600;
    border-radius: 8px;

    &:hover {
      background: linear-gradient(135deg, #E8A87C, #E07A5F);
      box-shadow: 0 4px 12px rgba(242, 204, 143, 0.3);
    }
  }

  :deep(.el-button:not(.el-button--primary):not(.el-button--success):not(.el-button--danger):not(.el-button--warning):not(.el-button--info):not(.is-link)) {
    background: rgba(20, 20, 50, 0.6);
    border: 1px solid rgba(242, 204, 143, 0.2);
    color: #F0E6D3;
    border-radius: 8px;

    &:hover {
      border-color: #F2CC8F;
      color: #F2CC8F;
    }
  }

  // ── 链接按钮 ──
  :deep(.el-button.is-link) {
    &.el-button--success {
      color: #81B29A;

      &:hover {
        color: #6a9e84;
      }
    }

    &.el-button--info {
      color: #B8A99A;

      &:hover {
        color: #F2CC8F;
      }
    }
  }

  // ── 表格 ──
  :deep(.el-table) {
    background: transparent;
    color: #F0E6D3;
    --el-table-border-color: rgba(242, 204, 143, 0.08);
    --el-table-header-bg-color: rgba(20, 20, 50, 0.5);
    --el-table-header-text-color: #B8A99A;
    --el-table-row-hover-bg-color: rgba(242, 204, 143, 0.06);
    --el-table-bg-color: transparent;
    --el-table-tr-bg-color: transparent;
    --el-table-text-color: #F0E6D3;

    .el-table__header-wrapper th {
      background: rgba(20, 20, 50, 0.5);
      color: #B8A99A;
      font-weight: 600;
      border-bottom: 1px solid rgba(242, 204, 143, 0.1);
    }

    .el-table__body-wrapper {
      tr {
        background: transparent;

        td {
          border-bottom: 1px solid rgba(242, 204, 143, 0.05);
        }
      }

      .el-table__row--striped td {
        background: rgba(20, 20, 50, 0.3);
      }
    }

    .el-table__empty-block {
      background: transparent;
      color: #7A6F63;
    }
  }

  // ── 标签 ──
  :deep(.el-tag) {
    border: none;
    border-radius: 6px;

    &.el-tag--info {
      background: rgba(184, 169, 154, 0.15);
      color: #B8A99A;
    }

    &.el-tag--danger {
      background: rgba(224, 122, 95, 0.15);
      color: #E07A5F;
    }

    &.el-tag--success {
      background: rgba(129, 178, 154, 0.15);
      color: #81B29A;
    }

    &.el-tag--warning {
      background: rgba(232, 168, 124, 0.15);
      color: #E8A87C;
    }

    &.el-tag--primary {
      background: rgba(123, 104, 174, 0.15);
      color: #7B68AE;
    }
  }

  // ── 分页 ──
  :deep(.el-pagination) {
    --el-pagination-bg-color: transparent;
    --el-pagination-text-color: #B8A99A;
    --el-pagination-button-disabled-bg-color: transparent;

    .el-pager li {
      background: transparent;
      color: #B8A99A;
      border-radius: 6px;

      &.is-active {
        background: linear-gradient(135deg, #F2CC8F, #E8A87C);
        color: #141432;
        font-weight: 600;
      }

      &:hover:not(.is-active) {
        color: #F2CC8F;
      }
    }

    .btn-prev,
    .btn-next {
      background: transparent;
      color: #B8A99A;

      &:hover {
        color: #F2CC8F;
      }
    }

    .el-pagination__total {
      color: #7A6F63;
    }

    .el-pagination__sizes .el-select {
      .el-select__wrapper {
        background: rgba(20, 20, 50, 0.6);
        border: 1px solid rgba(242, 204, 143, 0.15);
      }
    }
  }

  // ── 确认弹窗 ──
  :deep(.el-message-box) {
    background: rgba(26, 26, 62, 0.95);
    backdrop-filter: blur(20px);
    border: 1px solid rgba(242, 204, 143, 0.12);
    border-radius: 16px;

    .el-message-box__title {
      color: #F0E6D3;
    }

    .el-message-box__content {
      color: #B8A99A;
    }
  }

  // ── Loading 遮罩 ──
  :deep(.el-loading-mask) {
    background: rgba(20, 20, 50, 0.7);
    backdrop-filter: blur(4px);
  }
}
</style>
