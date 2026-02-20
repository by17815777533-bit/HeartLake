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
    ElMessage.error('获取操作日志失败，请刷新重试')
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
  .filter-card { margin-bottom: 20px; border-radius: 12px; }
  .pagination-wrapper { margin-top: 20px; display: flex; justify-content: flex-end; }
}
</style>
