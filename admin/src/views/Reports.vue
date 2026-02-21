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

  :deep(.el-button:not(.el-button--primary):not(.el-button--success):not(.el-button--info):not(.el-button--danger)) {
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

    // 去掉 stripe 的默认灰色背景
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

  // 举报类型 tag 保持原有颜色映射（template 中已内联 color）

  // handled-text
  .handled-text {
    color: #7A6F63;
    font-size: 14px;
  }

  // link 按钮
  :deep(.el-button.is-link) {
    &.el-button--success {
      color: #81B29A;
      &:hover { color: #9ecab5; }
    }
    &.el-button--info {
      color: #B8A99A;
      &:hover { color: #F0E6D3; }
    }
  }
}
</style>
