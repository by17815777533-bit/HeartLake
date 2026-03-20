<!--
  服务记录（操作日志）页面

  功能：
  - 管理员操作审计日志列表，支持分页浏览
  - 展示操作人、操作类型、操作目标、详情、IP 地址、时间
  - 用于安全审计追踪，记录所有管理后台的关键操作
-->

<template>
  <div class="logs-page ops-page ops-page--compact">
    <OpsDashboardDeck
      compact
      eyebrow="审计"
      title="服务记录"
      :heading-chip="`${auditScore} 分 ${auditLabel}`"
      metric-label="记录总量"
      :metric-value="summaryItems[0]?.value || '0'"
      metric-unit="条"
      :metric-description="logsOverviewDescription"
      section-note="核查重点"
      :overview-cards="logsOverviewCards"
      :focus-card="logsFocusCard"
      rhythm-eyebrow="节律"
      rhythm-title="动作频次"
      :rhythm-chip="`${loginCount} 次登录`"
      :rhythm-badge="`${formatCount(logList.length)} 条审计`"
      :rhythm-items="logRhythmItems"
      activity-title="审计动态"
      :activity-chip="latestLogMeta.value"
      :activity-rows="logActivityRows"
      guide-title="核查建议"
      :guide-chip="auditLabel"
      :guide-headline="logsGuideHeadline"
      :guide-copy="logsGuideCopy"
      guide-pulse-label="当前审计评分"
      :guide-pulse-value="`${auditScore} / 100`"
      :guide-pulse-note="logsGuidePulseNote"
      :guide-items="logsGuideItems"
    >
      <template #actions>
        <button type="button" class="overview-action" @click="handleSearch">刷新审计</button>
        <button type="button" class="overview-action" @click="handleReset">清空筛选</button>
      </template>
    </OpsDashboardDeck>

    <el-card shadow="never" class="table-card ops-table-card">
      <div class="ops-soft-toolbar logs-table-toolbar">
        <div class="logs-table-copy">
          <h3>审计列表</h3>
          <p>按操作人、对象和结果回看后台动作链路。</p>
          <div class="ops-toolbar-meta">
            <span class="ops-toolbar-meta__item">当前页 {{ logList.length }} 条</span>
            <span class="ops-toolbar-meta__item">登录 {{ loginCount }} 次</span>
            <span class="ops-toolbar-meta__item">完整度 {{ auditScore }} / 100</span>
          </div>
        </div>
        <el-form :model="filters" inline aria-label="日志筛选" class="logs-inline-filter">
          <el-form-item label="操作人">
            <el-input v-model="filters.operator" placeholder="管理员账号" clearable />
          </el-form-item>
          <el-form-item label="操作类型">
            <el-select v-model="filters.action" placeholder="全部" clearable>
              <el-option label="登录" value="login" />
              <el-option label="封禁用户" value="ban_user" />
              <el-option label="解封用户" value="unban_user" />
              <el-option label="删除内容" value="delete_content" />
              <el-option label="审核通过" value="approve" />
              <el-option label="审核拒绝" value="reject" />
              <el-option label="修改配置" value="config" />
              <el-option label="处理举报" value="handle_report" />
              <el-option label="发送广播" value="broadcast" />
              <el-option label="新增敏感词" value="sensitive_add" />
              <el-option label="更新敏感词" value="sensitive_update" />
              <el-option label="删除敏感词" value="sensitive_delete" />
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
            />
          </el-form-item>
          <el-form-item>
            <el-button type="primary" @click="handleSearch"> 搜索 </el-button>
            <el-button @click="handleReset"> 重置 </el-button>
          </el-form-item>
        </el-form>
      </div>

      <el-table v-loading="loading" :data="logList" stripe aria-label="操作日志列表">
        <el-table-column prop="id" label="编号" width="88" />
        <el-table-column label="操作人" min-width="180">
          <template #default="{ row }">
            <div class="log-operator">
              <strong>{{ getLogOperator(row) }}</strong>
              <span>{{ getOperatorMeta(row) }}</span>
            </div>
          </template>
        </el-table-column>
        <el-table-column label="操作类型" width="120">
          <template #default="{ row }">
            <el-tag :type="getActionType(row.action)" size="small">
              {{ getActionLabel(row.action) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作对象" min-width="170">
          <template #default="{ row }">
            <div class="log-target">
              <strong>{{ getLogTarget(row) }}</strong>
              <span>{{ getActionLabel(row.action) }}链路</span>
            </div>
          </template>
        </el-table-column>
        <el-table-column label="详情" min-width="260">
          <template #default="{ row }">
            <div class="log-detail">
              <strong>{{ getLogDetail(row) }}</strong>
              <span>{{ getActionNote(row.action) }}</span>
            </div>
          </template>
        </el-table-column>
        <el-table-column label="操作时间" width="188">
          <template #default="{ row }">
            <div class="log-time">
              <strong>{{ row.created_at || '暂无时间' }}</strong>
              <span>{{ getTimeNote(row.created_at) }}</span>
            </div>
          </template>
        </el-table-column>
        <template #empty>
          <el-empty description="暂无操作日志" :image-size="88" />
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
import OpsDashboardDeck from '@/components/OpsDashboardDeck.vue'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import {
  createDeckActivityRows,
  createDeckFocusCard,
  createDeckGuideItems,
  createDeckOverviewCards,
  createDeckRhythmItems,
} from '@/utils/opsDashboardDeck'
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

const {
  pagination,
  buildParams,
  handleSizeChange,
  handleCurrentChange,
  handleSearch,
  handleReset,
} = useTablePagination(fetchLogs, {
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

const actionMap: Record<string, { label: string; type: string; note: string }> = {
  login: { label: '登录', type: 'info', note: '后台进入或会话恢复已被记录。' },
  ban_user: { label: '封禁用户', type: 'danger', note: '账号状态被切换到限制中。' },
  unban_user: { label: '解封用户', type: 'success', note: '账号已恢复为正常可用状态。' },
  delete_content: { label: '删除内容', type: 'danger', note: '内容已从公开流向中移除。' },
  approve: { label: '审核通过', type: 'success', note: '审核队列中的内容被放行。' },
  reject: { label: '审核拒绝', type: 'warning', note: '审核结果为拦截或退回。' },
  config: { label: '修改配置', type: 'primary', note: '系统偏好或阈值发生了调整。' },
  handle_report: { label: '处理举报', type: 'warning', note: '求助或举报单已进入处置闭环。' },
  broadcast: { label: '发送广播', type: 'primary', note: '面向全站的消息已发出。' },
  sensitive_add: { label: '新增敏感词', type: 'warning', note: '风控词典增加了新的拦截项。' },
  sensitive_update: { label: '更新敏感词', type: 'info', note: '风控词典中的策略被更新。' },
  sensitive_delete: { label: '删除敏感词', type: 'danger', note: '已有风险词已从词典中移除。' },
  user_status: { label: '更新状态', type: 'info', note: '旅人状态被人工重新标记。' },
}

const getActionLabel = (action: string) => actionMap[action]?.label || action
const getActionType = (action: string) => actionMap[action]?.type || 'info'
const getActionNote = (action: string) => actionMap[action]?.note || '后台动作已写入审计链路。'
const formatCount = (value: number) => value.toLocaleString()

const getLogOperator = (row: Partial<OperationLog> & Record<string, unknown>) =>
  String(row.operator || row.admin_id || '系统')

const getOperatorMeta = (row: Partial<OperationLog> & Record<string, unknown>) => {
  const ip = row.ip?.toString().trim()
  return ip ? `来源 ${ip}` : '后台操作员'
}

const getLogTarget = (row: Partial<OperationLog> & Record<string, unknown>) => {
  const target = row.target?.toString().trim()
  if (target) return target

  const targetType = row.target_type?.toString().trim()
  const targetId = row.target_id?.toString().trim()
  if (!targetType) return '系统范围'
  return targetId ? `${targetType} · ${targetId}` : targetType
}

const getLogDetail = (row: Partial<OperationLog> & Record<string, unknown>) => {
  const detail = row.detail?.toString().trim() || row.details?.toString().trim()
  return detail || '未提供额外详情'
}

const getTimeNote = (value?: string) => {
  if (!value) return '等待写入时间'
  const diffMinutes = Math.max(0, Math.floor((Date.now() - new Date(value).getTime()) / 60000))
  if (Number.isNaN(diffMinutes)) return '时间格式待确认'
  if (diffMinutes <= 5) return '刚刚写入'
  if (diffMinutes <= 60) return `${diffMinutes} 分钟前`
  if (diffMinutes <= 24 * 60) return `${Math.floor(diffMinutes / 60)} 小时前`
  return `${Math.floor(diffMinutes / (24 * 60))} 天前`
}

const loginCount = computed(() => logList.value.filter((item) => item.action === 'login').length)
const contentActionCount = computed(
  () =>
    logList.value.filter((item) => ['delete_content', 'approve', 'reject'].includes(item.action))
      .length,
)
const configActionCount = computed(
  () => logList.value.filter((item) => item.action === 'config').length,
)

const summaryItems = computed(() => {
  const contentActions = contentActionCount.value
  const configCount = configActionCount.value

  return [
    {
      label: '记录总量',
      value: formatCount(Number(pagination.total || 0)),
      note: '当前筛选下的审计记录总数',
      tone: 'lake' as const,
    },
    {
      label: '登录动作',
      value: formatCount(loginCount.value),
      note: '当前页账号进入后台的记录',
      tone: 'sage' as const,
    },
    {
      label: '内容处置',
      value: formatCount(contentActions),
      note: '当前页涉及审核或删除的动作',
      tone: 'amber' as const,
    },
    {
      label: '配置改动',
      value: formatCount(configCount),
      note: '当前页涉及系统偏好调整的动作',
      tone: 'rose' as const,
    },
  ]
})

const logVizBars = computed(() => [
  { label: '登录', value: loginCount.value, display: formatCount(loginCount.value) },
  {
    label: '内容',
    value: contentActionCount.value,
    display: formatCount(contentActionCount.value),
  },
  { label: '配置', value: configActionCount.value, display: formatCount(configActionCount.value) },
  { label: '总量', value: logList.value.length, display: formatCount(logList.value.length) },
])

const auditScore = computed(() => {
  const total = Math.max(logList.value.length, 1)
  const structured = loginCount.value + contentActionCount.value + configActionCount.value
  return Math.max(32, Math.min(96, Math.round((structured / total) * 100)))
})

const auditLabel = computed(() => {
  if (auditScore.value >= 80) return '完整'
  if (auditScore.value >= 58) return '可追'
  return '稀疏'
})

const latestLogMeta = computed(() => {
  const latestItem = logList.value.reduce<OperationLog | null>((latest, item) => {
    if (!latest) return item
    return new Date(item.created_at).getTime() > new Date(latest.created_at).getTime()
      ? item
      : latest
  }, null)

  if (!latestItem) {
    return {
      value: '暂无新记录',
      note: '当前筛选条件下还没有新的审计写入。',
    }
  }

  return {
    value: getActionLabel(latestItem.action),
    note: `${getLogOperator(latestItem)} · ${latestItem.created_at || '暂无时间'} · ${getTimeNote(latestItem.created_at)}`,
  }
})

const logSignals = computed(() => {
  const filterMode =
    filters.operator || filters.action
      ? `${filters.operator || '全部操作人'} / ${filters.action ? getActionLabel(filters.action) : '全部动作'}`
      : '全量留痕'

  return [
    {
      label: '当前视角',
      value: filterMode,
      note:
        filters.dateRange?.length === 2
          ? `已限定 ${filters.dateRange[0]} 至 ${filters.dateRange[1]}`
          : '默认查看当前条件下的全部审计写入。',
      badge: `${formatCount(logList.value.length)} 条`,
    },
    {
      label: '最近动作',
      value: latestLogMeta.value.value,
      note: latestLogMeta.value.note,
      badge: `${formatCount(loginCount.value)} 次登录`,
    },
    {
      label: '链路完整度',
      value: `${auditScore.value} / 100`,
      note: `内容处置 ${formatCount(contentActionCount.value)} 次，配置改动 ${formatCount(configActionCount.value)} 次。`,
      badge: auditLabel.value,
    },
  ]
})

const logsHeroDescription =
  '把后台登录、内容处置和配置改动收在同一条审计链路里，先判断最近动作和筛选范围，再决定是否继续深查。'

const logsHeroChips = computed(() => [
  `${summaryItems.value[0]?.value || 0} 条记录`,
  `${loginCount.value} 次登录`,
  `${auditScore.value} 分 ${auditLabel.value}`,
])

const logsGuideHeadline = computed(() => {
  if (configActionCount.value > 0) return '最近存在配置改动，先核对参数变更是否符合预期'
  if (contentActionCount.value > 0) return '内容处置链路活跃，建议回看动作细节和时间顺序'
  return '当前以常规留痕为主，继续盯住登录和最新写入即可'
})

const logsGuideCopy = computed(() => {
  if (configActionCount.value > 0) {
    return `当前页包含 ${formatCount(configActionCount.value)} 次配置改动，建议优先核查改动时间、操作人和对应的后续动作是否匹配。`
  }
  if (contentActionCount.value > 0) {
    return `当前页包含 ${formatCount(contentActionCount.value)} 次内容处置动作，可以按时间顺序回看审核、删除和举报处理之间的关联。`
  }
  return '当前页以基础登录留痕为主，没有明显异常链路，可以保持常规巡看。'
})

const logsGuideMetrics = computed(() => [
  { label: '登录动作', value: `${formatCount(loginCount.value)} 次` },
  { label: '内容处置', value: `${formatCount(contentActionCount.value)} 次` },
  { label: '审计评分', value: `${auditScore.value} 分` },
])

const logsOverviewDescription = computed(() => {
  if (configActionCount.value > 0) {
    return `按操作人、动作类型和时间范围回看后台处理过程，当前优先核查 ${formatCount(configActionCount.value)} 次配置改动及其后续链路。`
  }
  return '按操作人、动作类型和时间范围回看后台处理过程，为核查、交接和安全审计提供依据。'
})

const logsOverviewCards = computed(() => createDeckOverviewCards(summaryItems.value.slice(1, 3)))
const logsFocusCard = computed(() =>
  createDeckFocusCard(summaryItems.value[3], '当前页涉及系统偏好或关键参数的配置改动次数。'),
)
const logRhythmItems = computed(() => createDeckRhythmItems(logVizBars.value))
const logActivityRows = computed(() => createDeckActivityRows(logSignals.value))
const logsGuidePulseNote = computed(
  () => `内容处置 ${formatCount(contentActionCount.value)} 次 · ${auditLabel.value}`,
)
const logsGuideItems = computed(() =>
  createDeckGuideItems([
    {
      label: '登录动作',
      value: `${formatCount(loginCount.value)} 次`,
      note: '登录记录用于判断后台值守与会话恢复是否正常。',
    },
    {
      label: '内容处置',
      value: `${formatCount(contentActionCount.value)} 次`,
      note: '处置链路越密集，越需要按时间顺序回看动作关联。',
    },
    {
      label: '审计评分',
      value: `${auditScore.value} 分`,
      note: '综合关键动作的结构化程度形成当前评分。',
    },
  ]),
)

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
  .logs-filter-form {
    :deep(.el-form-item) {
      margin-bottom: 0;
    }
  }

  .logs-table-toolbar {
    display: grid;
    grid-template-columns: minmax(0, 0.84fr) minmax(0, 1.16fr);
    gap: 14px;
  }

  .logs-inline-filter {
    width: 100%;
    flex: none;
    justify-content: stretch;

    :deep(.el-form-item:nth-child(1)),
    :deep(.el-form-item:nth-child(2)) {
      grid-column: span 3;
    }

    :deep(.el-form-item:nth-child(3)) {
      grid-column: span 4;
    }

    :deep(.el-form-item:nth-child(4)) {
      grid-column: span 2;
    }
  }

  .logs-table-copy {
    h3 {
      color: var(--hl-ink);
      font-size: 22px;
      font-weight: 700;
      letter-spacing: -0.03em;
    }

    p {
      margin-top: 4px;
      color: var(--hl-ink-soft);
      font-size: 12px;
      line-height: 1.55;
    }
  }

  @media (max-width: 1200px) {
    .logs-table-toolbar {
      grid-template-columns: 1fr;
    }
  }

  .log-operator,
  .log-target,
  .log-detail,
  .log-time {
    display: grid;
    gap: 4px;

    strong {
      color: var(--hl-ink);
      font-size: 13px;
      font-weight: 700;
      line-height: 1.45;
    }

    span {
      color: var(--hl-ink-soft);
      font-size: 11px;
      line-height: 1.55;
    }
  }

  .log-target strong {
    font-family: var(--hl-font-mono);
    font-size: 12px;
  }

  .log-time strong {
    font-size: 12px;
  }

  .pagination-wrapper {
    display: flex;
    justify-content: flex-end;
  }
}
</style>
