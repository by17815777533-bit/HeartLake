<!--
  @file Reports.vue
  @brief Reports 组件
  Created by 林子怡
-->

<template>
  <div class="reports-page">
    <!-- 筛选 -->
    <el-card
      shadow="never"
      class="filter-card"
    >
      <el-form
        :model="filters"
        inline
      >
        <el-form-item label="状态">
          <el-select
            v-model="filters.status"
            placeholder="全部"
            clearable
            style="width: 120px"
          >
            <el-option
              label="待处理"
              value="pending"
            />
            <el-option
              label="已处理"
              value="handled"
            />
            <el-option
              label="已忽略"
              value="ignored"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="类型">
          <el-select
            v-model="filters.type"
            placeholder="全部"
            clearable
            style="width: 120px"
          >
            <el-option
              label="垃圾信息"
              value="spam"
            />
            <el-option
              label="骚扰辱骂"
              value="harassment"
            />
            <el-option
              label="不当内容"
              value="inappropriate"
            />
            <el-option
              label="暴力内容"
              value="violence"
            />
            <el-option
              label="其他"
              value="other"
            />
          </el-select>
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

    <!-- 举报列表 -->
    <el-card shadow="never">
      <el-table
        v-loading="loading"
        :data="reportList"
        stripe
      >
        <el-table-column
          prop="id"
          label="ID"
          width="100"
        />
        <el-table-column
          label="举报类型"
          width="100"
        >
          <template #default="{ row }">
            <el-tag
              size="small"
              :color="getTypeColor(row.type)"
              style="border:none;color:#fff"
            >
              {{ getTypeLabel(row.type) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column
          label="举报原因"
          min-width="200"
        >
          <template #default="{ row }">
            {{ row.reason }}
          </template>
        </el-table-column>
        <el-table-column
          label="被举报内容"
          min-width="200"
        >
          <template #default="{ row }">
            {{ row.target_content?.substring(0, 50) }}{{ row.target_content?.length > 50 ? '...' : '' }}
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
          label="举报时间"
          width="180"
        />
        <el-table-column
          label="操作"
          width="200"
          fixed="right"
        >
          <template #default="{ row }">
            <template v-if="row.status === 'pending'">
              <el-button
                type="success"
                link
                @click="handleReport(row, 'handled')"
              >
                处理
              </el-button>
              <el-button
                type="info"
                link
                @click="handleReport(row, 'ignored')"
              >
                忽略
              </el-button>
            </template>
            <span
              v-else
              class="handled-text"
            >已{{ getStatusLabel(row.status) }}</span>
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
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'

const loading = ref(false)
const reportList = ref([])

const filters = reactive({
  status: '',
  type: '',
})

const { pagination, handleSizeChange, handleCurrentChange, handleSearch, handleReset } = useTablePagination(fetchReports, {
  filters,
  defaultFilters: { status: '', type: '' },
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
    color: var(--m3-on-surface-variant);
    font-size: 14px;
  }
}
</style>
