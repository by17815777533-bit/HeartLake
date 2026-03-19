<!--
  服务记录（操作日志）页面

  功能：
  - 管理员操作审计日志列表，支持分页浏览
  - 展示操作人、操作类型、操作目标、详情、IP 地址、时间
  - 用于安全审计追踪，记录所有管理后台的关键操作
-->

<template>
  <div class="logs-page ops-page">
    <OpsPageHero
      eyebrow="服务审计"
      title="服务记录"
      description="按操作人、动作类型和时间范围回看后台处理过程，为核查、交接和安全审计提供依据。"
      status="全量留痕"
      :chips="['审计追踪', '操作对象', '时间回查']"
    />

    <OpsMetricStrip :items="summaryItems" />

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
            <el-option
              label="处理举报"
              value="handle_report"
            />
            <el-option
              label="发送广播"
              value="broadcast"
            />
            <el-option
              label="新增敏感词"
              value="sensitive_add"
            />
            <el-option
              label="更新敏感词"
              value="sensitive_update"
            />
            <el-option
              label="删除敏感词"
              value="sensitive_delete"
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
            :disabled-date="disabledDate"
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

    <el-card
      shadow="never"
      class="table-card"
    >
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
        <template #empty>
          <el-empty
            description="暂无操作日志"
            :image-size="120"
          />
        </template>
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
import { computed, ref, reactive, onMounted } from 'vue'
import api, { isRequestCanceled } from '@/api'
import { ElMessage } from 'element-plus'
import OpsPageHero from '@/components/OpsPageHero.vue'
import OpsMetricStrip from '@/components/OpsMetricStrip.vue'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import type { OperationLog } from '@/types'

const loading = ref(false)
const logList = ref<OperationLog[]>([])
const filters = reactive({ operator: '', action: '', dateRange: null as string[] | null })

// 日期范围限制最大90天，且不能选择未来日期
const MAX_DATE_RANGE_DAYS = 90
const disabledDate = (date: Date) => {
  // 不能选择未来日期
  if (date.getTime() > Date.now()) return true
  // 如果已选了起始日期，限制结束日期在90天内
  if (filters.dateRange && filters.dateRange[0]) {
    const start = new Date(filters.dateRange[0]).getTime()
    const maxEnd = start + MAX_DATE_RANGE_DAYS * 24 * 3600 * 1000
    return date.getTime() < start || date.getTime() > maxEnd
  }
  return false
}

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

const actionMap: Record<string, { label: string; type: string; icon: string }> = {
  login: { label: '登录', type: 'info', icon: '🔑' },
  ban_user: { label: '封禁用户', type: 'danger', icon: '🚫' },
  unban_user: { label: '解封用户', type: 'success', icon: '✓' },
  delete_content: { label: '删除内容', type: 'danger', icon: '🗑' },
  approve: { label: '审核通过', type: 'success', icon: '✓' },
  reject: { label: '审核拒绝', type: 'warning', icon: '✗' },
  config: { label: '修改配置', type: 'primary', icon: '⚙' },
  handle_report: { label: '处理举报', type: 'warning', icon: '📮' },
  broadcast: { label: '发送广播', type: 'primary', icon: '📡' },
  sensitive_add: { label: '新增敏感词', type: 'warning', icon: '＋' },
  sensitive_update: { label: '更新敏感词', type: 'info', icon: '✎' },
  sensitive_delete: { label: '删除敏感词', type: 'danger', icon: '－' },
  user_status: { label: '更新状态', type: 'info', icon: '◉' },
}

const getActionLabel = (action: string) => `${actionMap[action]?.icon || ''} ${actionMap[action]?.label || action}`
const getActionType = (action: string) => actionMap[action]?.type || 'info'
const formatCount = (value: number) => value.toLocaleString()

const summaryItems = computed(() => {
  const loginCount = logList.value.filter((item) => item.action === 'login').length
  const contentActions = logList.value.filter((item) => ['delete_content', 'approve', 'reject'].includes(item.action)).length
  const configCount = logList.value.filter((item) => item.action === 'config').length

  return [
    { label: '记录总量', value: formatCount(Number(pagination.total || 0)), note: '当前筛选下的审计记录总数', tone: 'lake' as const },
    { label: '登录动作', value: formatCount(loginCount), note: '当前页账号进入后台的记录', tone: 'sage' as const },
    { label: '内容处置', value: formatCount(contentActions), note: '当前页涉及审核或删除的动作', tone: 'amber' as const },
    { label: '配置改动', value: formatCount(configCount), note: '当前页涉及系统偏好调整的动作', tone: 'rose' as const },
  ]
})

async function fetchLogs() {
  loading.value = true
  try {
    const { dateRange, ...rest } = { ...filters }
    const extra: Record<string, unknown> = { ...rest }
    if (dateRange?.length === 2) {
      extra.start_date = dateRange[0]
      extra.end_date = dateRange[1]
    }
    const params = buildParams(extra)
    const res = await api.getOperationLogs(params)
    const data = res.data?.data || res.data || {}
    logList.value = data.list || []
    pagination.total = data.total || 0
  } catch (e) {
    if (isRequestCanceled(e)) return
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
  .filter-card {
    margin-bottom: 16px;
  }

  .pagination-wrapper {
    display: flex;
    justify-content: flex-end;
  }
}
</style>
