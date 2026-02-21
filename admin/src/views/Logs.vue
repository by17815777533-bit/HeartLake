<!--
  @file Logs.vue
  @brief 操作日志页面
-->

<template>
  <div class="logs-page">
    <el-card shadow="never" class="filter-card">
      <el-form :model="filters" inline>
        <el-form-item label="操作人">
          <el-input v-model="filters.operator" placeholder="管理员账号" clearable style="width: 140px" />
        </el-form-item>
        <el-form-item label="操作类型">
          <el-select v-model="filters.action" placeholder="全部" clearable style="width: 140px">
            <el-option label="登录" value="login" />
            <el-option label="封禁用户" value="ban_user" />
            <el-option label="解封用户" value="unban_user" />
            <el-option label="删除内容" value="delete_content" />
            <el-option label="审核通过" value="approve" />
            <el-option label="审核拒绝" value="reject" />
            <el-option label="修改配置" value="config" />
          </el-select>
        </el-form-item>
        <el-form-item label="时间范围">
          <el-date-picker v-model="filters.dateRange" type="daterange" range-separator="至"
            start-placeholder="开始日期" end-placeholder="结束日期" value-format="YYYY-MM-DD" style="width: 240px" />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" @click="handleSearch">搜索</el-button>
          <el-button @click="handleReset">重置</el-button>
        </el-form-item>
      </el-form>
    </el-card>

    <el-card shadow="never">
      <el-table v-loading="loading" :data="logList" stripe>
        <el-table-column prop="id" label="ID" width="80" />
        <el-table-column prop="admin_id" label="操作人" width="120" />
        <el-table-column label="操作类型" width="120">
          <template #default="{ row }">
            <el-tag :type="getActionType(row.action)" size="small">{{ getActionLabel(row.action) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作对象" width="150">
          <template #default="{ row }">{{ row.target_type ? `${row.target_type}:${row.target_id}` : '-' }}</template>
        </el-table-column>
        <el-table-column prop="details" label="详情" min-width="200" show-overflow-tooltip />
        <el-table-column prop="created_at" label="操作时间" width="180" />
      </el-table>

      <div class="pagination-wrapper">
        <el-pagination v-model:current-page="pagination.page" v-model:page-size="pagination.pageSize"
          :total="pagination.total" :page-sizes="[20, 50, 100]" layout="total, sizes, prev, pager, next"
          @size-change="handleSizeChange" @current-change="fetchLogs" />
      </div>
    </el-card>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import api from '@/api'
import { ElMessage } from 'element-plus'
import { getErrorMessage } from '@/utils/errorHelper'

const loading = ref(false)
const logList = ref([])
const filters = reactive({ operator: '', action: '', dateRange: null })
const pagination = reactive({ page: 1, pageSize: 20, total: 0 })

const actionMap = {
  login: { label: '登录', type: 'info', icon: '🔑' },
  ban_user: { label: '封禁用户', type: 'danger', icon: '🚫' },
  unban_user: { label: '解封用户', type: 'success', icon: '✓' },
  delete_content: { label: '删除内容', type: 'danger', icon: '🗑' },
  approve: { label: '审核通过', type: 'success', icon: '✓' },
  reject: { label: '审核拒绝', type: 'warning', icon: '✗' },
  config: { label: '修改配置', type: 'primary', icon: '⚙' },
}

const getActionLabel = (action) => `${actionMap[action]?.icon || ''} ${actionMap[action]?.label || action}`
const getActionType = (action) => actionMap[action]?.type || 'info'

const fetchLogs = async () => {
  loading.value = true
  try {
    const params = { page: pagination.page, page_size: pagination.pageSize, ...filters }
    if (filters.dateRange?.length === 2) {
      params.start_date = filters.dateRange[0]
      params.end_date = filters.dateRange[1]
    }
    delete params.dateRange
    const res = await api.getOperationLogs(params)
    logList.value = res.data?.list || []
    pagination.total = res.data?.total || 0
  } catch (e) {
    console.error('获取操作日志失败:', e)
    ElMessage.error(getErrorMessage(e, '获取操作日志失败'))
    logList.value = []
  } finally {
    loading.value = false
  }
}

const handleSizeChange = () => { pagination.page = 1; fetchLogs() }
const handleSearch = () => {
  filters.operator = filters.operator.trim()
  if (filters.operator.length > 50) {
    ElMessage.warning('操作人名称过长')
    return
  }
  pagination.page = 1
  fetchLogs()
}
const handleReset = () => { Object.assign(filters, { operator: '', action: '', dateRange: null }); pagination.page = 1; fetchLogs() }

onMounted(() => fetchLogs())
</script>

<style lang="scss" scoped>
.logs-page {
  // filter-card
  .filter-card {
    margin-bottom: 16px;
    background: rgba(26, 26, 62, 0.6);
    backdrop-filter: blur(16px);
    border: 1px solid rgba(242, 204, 143, 0.08);
    border-radius: 16px;
  }

  // el-card
  :deep(.el-card) {
    background: rgba(26, 26, 62, 0.6);
    backdrop-filter: blur(16px);
    border: 1px solid rgba(242, 204, 143, 0.08);
    border-radius: 16px;
  }

  // el-form 筛选栏
  :deep(.el-form-item__label) {
    color: #B8A99A;
  }

  :deep(.el-input__wrapper),
  :deep(.el-select__wrapper) {
    background: rgba(20, 20, 50, 0.6);
    border: 1px solid rgba(242, 204, 143, 0.12);
    border-radius: 8px;
    box-shadow: none;
    color: #F0E6D3;

    &:hover,
    &.is-focus {
      border-color: rgba(242, 204, 143, 0.3);
      box-shadow: 0 0 0 1px rgba(242, 204, 143, 0.1);
    }
  }

  :deep(.el-input__inner) {
    color: #F0E6D3;

    &::placeholder {
      color: #7A6F63;
    }
  }

  :deep(.el-select__placeholder) {
    color: #7A6F63;
  }

  :deep(.el-select__selected-item span) {
    color: #F0E6D3;
  }

  // date-picker
  :deep(.el-date-editor) {
    --el-date-editor-width: 240px;

    .el-input__wrapper {
      background: rgba(20, 20, 50, 0.6);
      border: 1px solid rgba(242, 204, 143, 0.12);
      border-radius: 8px;
      box-shadow: none;
    }

    .el-range-input {
      color: #F0E6D3;

      &::placeholder {
        color: #7A6F63;
      }
    }

    .el-range-separator {
      color: #7A6F63;
    }

    .el-icon {
      color: #7A6F63;
    }
  }

  // 按钮
  :deep(.el-button--primary) {
    background: linear-gradient(135deg, #F2CC8F, #E8A87C);
    border: none;
    color: #1a1a3e;
    font-weight: 600;
    border-radius: 8px;

    &:hover {
      background: linear-gradient(135deg, #f5d9a8, #edb78f);
    }
  }

  :deep(.el-button:not(.el-button--primary)) {
    background: rgba(242, 204, 143, 0.08);
    border: 1px solid rgba(242, 204, 143, 0.15);
    color: #B8A99A;
    border-radius: 8px;

    &:hover {
      border-color: rgba(242, 204, 143, 0.3);
      color: #F2CC8F;
    }
  }

  // el-table
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

    .el-table__row--striped td.el-table__cell {
      background: rgba(242, 204, 143, 0.02);
    }
  }

  // loading 遮罩
  :deep(.el-loading-mask) {
    background: rgba(20, 20, 50, 0.6);
  }

  // pagination
  .pagination-wrapper {
    margin-top: 20px;
    display: flex;
    justify-content: flex-end;
  }

  :deep(.el-pagination) {
    --el-pagination-bg-color: transparent;
    --el-pagination-button-bg-color: rgba(26, 26, 62, 0.5);
    --el-pagination-hover-color: #F2CC8F;
    --el-pagination-text-color: #B8A99A;
    --el-pagination-button-color: #B8A99A;
    --el-pagination-button-disabled-color: #7A6F63;
    --el-pagination-button-disabled-bg-color: transparent;

    .el-pager li {
      background: rgba(26, 26, 62, 0.5);
      color: #B8A99A;
      border-radius: 6px;

      &.is-active {
        background: linear-gradient(135deg, #F2CC8F, #E8A87C);
        color: #1a1a3e;
        font-weight: 600;
      }
    }

    button {
      background: rgba(26, 26, 62, 0.5);
      color: #B8A99A;
      border-radius: 6px;
    }

    .el-pagination__total,
    .el-pagination__sizes .el-select__wrapper {
      color: #B8A99A;
    }
  }
}
</style>
