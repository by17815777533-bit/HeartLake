<!--
  @file Logs.vue
  @brief 操作日志页面
-->

<template>
  <div class="logs-page">
    <el-card
      shadow="never"
      class="filter-card"
    >
      <el-form
        :model="filters"
        inline
        aria-label="日志筛选"
      >
        <el-form-item label="操作人">
          <el-input
            v-model="filters.operator"
            placeholder="管理员账号"
            clearable
            style="width: 140px"
          />
        </el-form-item>
        <el-form-item label="操作类型">
          <el-select
            v-model="filters.action"
            placeholder="全部"
            clearable
            style="width: 140px"
          >
            <el-option
              label="登录"
              value="login"
            />
            <el-option
              label="封禁用户"
              value="ban_user"
            />
            <el-option
              label="解封用户"
              value="unban_user"
            />
            <el-option
              label="删除内容"
              value="delete_content"
            />
            <el-option
              label="审核通过"
              value="approve"
            />
            <el-option
              label="审核拒绝"
              value="reject"
            />
            <el-option
              label="修改配置"
              value="config"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="时间范围">
          <el-date-picker
            v-model="filters.dateRange"
            type="daterange"
            range-separator="至"
            start-placeholder="开始日期"
            end-placeholder="结束日期"
            value-format="YYYY-MM-DD"
            style="width: 240px"
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

    <el-card shadow="never">
      <el-table
        v-loading="loading"
        :data="logList"
        stripe
        aria-label="操作日志列表"
      >
        <el-table-column
          prop="id"
          label="ID"
          width="80"
        />
        <el-table-column
          prop="admin_id"
          label="操作人"
          width="120"
        />
        <el-table-column
          label="操作类型"
          width="120"
        >
          <template #default="{ row }">
            <el-tag
              :type="getActionType(row.action)"
              size="small"
            >
              {{ getActionLabel(row.action) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column
          label="操作对象"
          width="150"
        >
          <template #default="{ row }">
            {{ row.target_type ? `${row.target_type}:${row.target_id}` : '-' }}
          </template>
        </el-table-column>
        <el-table-column
          prop="details"
          label="详情"
          min-width="200"
          show-overflow-tooltip
        />
        <el-table-column
          prop="created_at"
          label="操作时间"
          width="180"
        />
      </el-table>

      <div class="pagination-wrapper">
        <el-pagination
          v-model:current-page="pagination.page"
          v-model:page-size="pagination.pageSize"
          :total="pagination.total"
          :page-sizes="[20, 50, 100]"
          layout="total, sizes, prev, pager, next"
          @size-change="handleSizeChange"
          @current-change="handleCurrentChange"
        />
      </div>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import api from '@/api'
import { ElMessage } from 'element-plus'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'

const loading = ref(false)
const logList = ref([])
const filters = reactive({ operator: '', action: '', dateRange: null })
const { pagination, buildParams, handleSizeChange, handleCurrentChange, handleSearch, handleReset } = useTablePagination(fetchLogs, {
  filters,
  defaultFilters: { operator: '', action: '', dateRange: null },
  beforeSearch: () => {
    filters.operator = filters.operator.trim()
    if (filters.operator.length > 50) {
      ElMessage.warning('操作人名称过长')
      return false
    }
  },
})

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

async function fetchLogs() {
  loading.value = true
  try {
    const extra: Record<string, unknown> = { ...filters }
    if (filters.dateRange?.length === 2) {
      extra.start_date = filters.dateRange[0]
      extra.end_date = filters.dateRange[1]
    }
    delete extra.dateRange
    const params = buildParams(extra)
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

onMounted(() => fetchLogs())
</script>

<style lang="scss" scoped>
.logs-page {
  padding: 20px;

  .filter-card {
    margin-bottom: 16px;
  }

  .pagination-wrapper {
    margin-top: 20px;
    display: flex;
    justify-content: flex-end;
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

  // ── 输入框 ──
  :deep(.el-input) {
    .el-input__wrapper {
      background: rgba(20, 20, 50, 0.6);
      border: 1px solid rgba(242, 204, 143, 0.15);
      border-radius: 8px;
      box-shadow: none;

      .el-input__inner {
        color: #F0E6D3;

        &::placeholder {
          color: #7A6F63;
        }
      }

      &:hover {
        border-color: rgba(242, 204, 143, 0.3);
      }

      &.is-focus {
        border-color: #F2CC8F;
        box-shadow: 0 0 8px rgba(242, 204, 143, 0.15);
      }
    }
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

  // ── 日期选择器 ──
  :deep(.el-date-editor) {
    .el-input__wrapper {
      background: rgba(20, 20, 50, 0.6);
      border: 1px solid rgba(242, 204, 143, 0.15);
      border-radius: 8px;
      box-shadow: none;

      .el-range-input {
        color: #F0E6D3;
        background: transparent;

        &::placeholder {
          color: #7A6F63;
        }
      }

      .el-range-separator {
        color: #7A6F63;
      }

      .el-icon {
        color: #B8A99A;
      }

      &:hover {
        border-color: rgba(242, 204, 143, 0.3);
      }

      &.is-focus {
        border-color: #F2CC8F;
        box-shadow: 0 0 8px rgba(242, 204, 143, 0.15);
      }
    }
  }

  // ── 按钮 ──
  :deep(.el-button--primary) {
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

  // ── 标签（5种类型） ──
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

  // ── Loading 遮罩 ──
  :deep(.el-loading-mask) {
    background: rgba(20, 20, 50, 0.7);
    backdrop-filter: blur(4px);
  }
}
</style>
