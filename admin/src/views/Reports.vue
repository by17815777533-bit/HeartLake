<!--
  求助处理（举报管理）页面

  功能：
  - 举报列表展示：举报人、目标类型（石头/纸船/用户）、原因、内容预览、状态
  - 处理操作：approve（确认违规）/ reject（驳回举报）/ dismiss（忽略）
  - 处理时可附加备注说明
  - 支持按状态筛选和分页浏览
-->

<template>
  <div class="reports-page ops-page ops-page--compact">
    <OpsDashboardDeck
      compact
      eyebrow="求助"
      title="求助处理"
      :heading-chip="`${reportResolutionScore} 分 ${reportResolutionLabel}`"
      metric-label="求助总数"
      :metric-value="summaryItems[0]?.value || '0'"
      metric-unit="条"
      :metric-description="reportsOverviewDescription"
      section-note="处置重点"
      :overview-cards="reportsOverviewCards"
      :focus-card="reportsFocusCard"
      rhythm-eyebrow="队列"
      rhythm-title="处置分布"
      :rhythm-chip="`${reportPendingCount} 待处理`"
      :rhythm-badge="`${reportHandledCount} 条已完成`"
      :rhythm-items="reportRhythmItems"
      activity-title="求助动态"
      :activity-chip="latestReportMeta.value"
      :activity-rows="reportActivityRows"
      guide-title="处置建议"
      :guide-chip="reportResolutionLabel"
      :guide-headline="reportGuideHeadline"
      :guide-copy="reportGuideCopy"
      guide-pulse-label="当前处置评分"
      :guide-pulse-value="`${reportResolutionScore} 分`"
      :guide-pulse-note="reportsGuidePulseNote"
      :guide-items="reportGuideItems"
    >
      <template #actions>
        <button type="button" class="overview-action" @click="handleSearch">刷新工单</button>
        <button type="button" class="overview-action" @click="handleReset">重置筛查</button>
      </template>
    </OpsDashboardDeck>

    <el-card shadow="never" class="table-card ops-table-card">
      <div class="ops-soft-toolbar reports-table-toolbar">
        <div class="reports-table-copy">
          <h3>求助列表</h3>
          <p>待处理与已回执共用一张工作台，处置同步写入日志。</p>
          <div class="ops-toolbar-meta">
            <span class="ops-toolbar-meta__item">待处理 {{ reportPendingCount }} 条</span>
            <span class="ops-toolbar-meta__item">已完成 {{ reportHandledCount }} 条</span>
            <span class="ops-toolbar-meta__item">处置评分 {{ reportResolutionScore }} 分</span>
          </div>
        </div>
        <el-form :model="filters" inline aria-label="举报筛选" class="reports-inline-filter">
          <el-form-item label="状态">
            <el-select v-model="filters.status" placeholder="全部" clearable>
              <el-option label="待处理" value="pending" />
              <el-option label="已处理" value="handled" />
              <el-option label="已忽略" value="ignored" />
            </el-select>
          </el-form-item>
          <el-form-item label="类型">
            <el-select v-model="filters.type" placeholder="全部" clearable>
              <el-option label="垃圾信息" value="spam" />
              <el-option label="骚扰辱骂" value="harassment" />
              <el-option label="不当内容" value="inappropriate" />
              <el-option label="暴力内容" value="violence" />
              <el-option label="其他" value="other" />
            </el-select>
          </el-form-item>
          <el-form-item>
            <el-button type="primary" @click="handleSearch"> 搜索 </el-button>
            <el-button @click="handleReset"> 重置 </el-button>
          </el-form-item>
        </el-form>
      </div>

      <el-table v-loading="loading" :data="reportList" stripe aria-label="举报列表">
        <el-table-column prop="id" label="ID" width="100" />
        <el-table-column label="举报类型" width="100">
          <template #default="{ row }">
            <el-tag size="small" :color="getTypeColor(row.type)" style="border: none; color: #fff">
              {{ getTypeLabel(row.type) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="举报原因" min-width="200">
          <template #default="{ row }">
            {{ row.reason }}
          </template>
        </el-table-column>
        <el-table-column label="被举报内容" min-width="200">
          <template #default="{ row }">
            {{ row.target_content?.substring(0, 50)
            }}{{ row.target_content?.length > 50 ? '...' : '' }}
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
              <el-button type="info" link @click="handleReport(row, 'ignored')"> 忽略 </el-button>
            </template>
            <span v-else class="handled-text">已{{ getStatusLabel(row.status) }}</span>
          </template>
        </el-table-column>
        <template #empty>
          <el-empty description="暂无举报数据" :image-size="88" />
        </template>
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
import { computed, ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import api, { isRequestCanceled } from '@/api'
import OpsDashboardDeck from '@/components/OpsDashboardDeck.vue'
import { normalizeCollectionResponse } from '@/utils/collectionPayload'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import {
  createDeckActivityRows,
  createDeckFocusCard,
  createDeckGuideItems,
  createDeckOverviewCards,
  createDeckRhythmItems,
} from '@/utils/opsDashboardDeck'
import type { Report } from '@/types'

const loading = ref(false)
const reportList = ref<Report[]>([])

const filters = reactive({
  status: '',
  type: '',
})

const {
  pagination,
  buildParams,
  handleSizeChange,
  handleCurrentChange,
  handleSearch,
  handleReset,
} = useTablePagination(fetchReports, {
  filters,
  defaultFilters: { status: '', type: '' },
})

const getTypeLabel = (type: string) => {
  const map: Record<string, string> = {
    spam: '垃圾信息',
    harassment: '骚扰辱骂',
    inappropriate: '不当内容',
    violence: '暴力内容',
    other: '其他',
  }
  return map[type] || type
}

const getTypeColor = (type: string) => {
  const map: Record<string, string> = {
    spam: '#909399',
    harassment: '#E07A5F',
    inappropriate: '#F2CC8F',
    violence: '#E6A23C',
    other: '#81B29A',
  }
  return map[type] || '#909399'
}

const getStatusType = (status: string) => {
  const map: Record<string, string> = { pending: 'warning', handled: 'success', ignored: 'info' }
  return map[status] || 'info'
}

const getStatusLabel = (status: string) => {
  const map: Record<string, string> = { pending: '待处理', handled: '已处理', ignored: '已忽略' }
  return map[status] || status
}

const formatCount = (value: number) => value.toLocaleString()

const summaryItems = computed(() => {
  const pendingCount = reportList.value.filter((item) => item.status === 'pending').length
  const handledCount = reportList.value.filter((item) => item.status === 'handled').length
  const ignoredCount = reportList.value.filter((item) => item.status === 'ignored').length

  return [
    {
      label: '求助总数',
      value: formatCount(Number(pagination.total || 0)),
      note: '当前筛选下的记录总量',
      tone: 'lake' as const,
    },
    {
      label: '待优先处理',
      value: formatCount(pendingCount),
      note: '当前页仍需人工判断的记录',
      tone: 'rose' as const,
    },
    {
      label: '已完成处理',
      value: formatCount(handledCount),
      note: '当前页已给出处置结果',
      tone: 'sage' as const,
    },
    {
      label: '已忽略',
      value: formatCount(ignoredCount),
      note: '当前页不进入进一步处理的记录',
      tone: 'amber' as const,
    },
  ]
})

const reportPendingCount = computed(
  () => reportList.value.filter((item) => item.status === 'pending').length,
)
const reportHandledCount = computed(
  () => reportList.value.filter((item) => item.status === 'handled').length,
)
const reportIgnoredCount = computed(
  () => reportList.value.filter((item) => item.status === 'ignored').length,
)

const reportVizBars = computed(() => [
  {
    label: '待处理',
    value: reportPendingCount.value,
    display: formatCount(reportPendingCount.value),
  },
  {
    label: '已处理',
    value: reportHandledCount.value,
    display: formatCount(reportHandledCount.value),
  },
  {
    label: '已忽略',
    value: reportIgnoredCount.value,
    display: formatCount(reportIgnoredCount.value),
  },
  { label: '总量', value: reportList.value.length, display: formatCount(reportList.value.length) },
])

const reportResolutionScore = computed(() => {
  const total = Math.max(reportList.value.length, 1)
  return Math.max(
    24,
    Math.min(96, Math.round(((reportHandledCount.value + reportIgnoredCount.value) / total) * 100)),
  )
})

const reportResolutionLabel = computed(() => {
  if (reportResolutionScore.value >= 80) return '高效'
  if (reportResolutionScore.value >= 55) return '处理中'
  return '积压'
})

const getReportTimeNote = (value?: string) => {
  if (!value) return '等待时间写入'
  const diffMinutes = Math.max(0, Math.floor((Date.now() - new Date(value).getTime()) / 60000))
  if (Number.isNaN(diffMinutes)) return '时间格式待确认'
  if (diffMinutes <= 60) return `${diffMinutes} 分钟前发起`
  if (diffMinutes <= 24 * 60) return `${Math.floor(diffMinutes / 60)} 小时前发起`
  return `${Math.floor(diffMinutes / (24 * 60))} 天前发起`
}

const latestReportMeta = computed(() => {
  const latestItem = reportList.value.reduce<Report | null>((latest, item) => {
    if (!latest) return item
    return new Date(item.created_at).getTime() > new Date(latest.created_at).getTime()
      ? item
      : latest
  }, null)

  if (!latestItem) {
    return {
      value: '暂无新求助',
      note: '当前筛选条件下没有新的待回看记录。',
    }
  }

  return {
    value: latestItem.reason || '最新求助',
    note: `${latestItem.created_at || '暂无时间'} · ${getReportTimeNote(latestItem.created_at)}`,
  }
})

const reportSignals = computed(() => {
  const total = reportList.value.length
  const pendingRatio = total ? Math.round((reportPendingCount.value / total) * 100) : 0
  const filterMode =
    filters.status || filters.type
      ? `${filters.status ? getStatusLabel(filters.status) : '全部状态'} / ${filters.type ? getTypeLabel(filters.type) : '全部类型'}`
      : '全量视角'

  return [
    {
      label: '当前视角',
      value: filterMode,
      note: filters.type
        ? `当前重点关注“${getTypeLabel(filters.type)}”相关工单。`
        : '默认同时查看全部求助和举报记录。',
      badge: filters.status ? getStatusLabel(filters.status) : '未限状态',
    },
    {
      label: '最新流入',
      value: latestReportMeta.value.value,
      note: latestReportMeta.value.note,
      badge: `当前页 ${formatCount(total)} 条`,
    },
    {
      label: '待处理占比',
      value: `${pendingRatio}%`,
      note: `当前页待处理 ${formatCount(reportPendingCount.value)} 条，已完成 ${formatCount(reportHandledCount.value)} 条。`,
      badge: `忽略 ${formatCount(reportIgnoredCount.value)}`,
    },
  ]
})

const reportsHeroDescription =
  '把求助、举报和回执状态收在同一张处置台面里，先判断队列温度，再决定是优先确认、回看历史，还是继续保持观察。'

const reportsHeroChips = computed(() => [
  `${summaryItems.value[0]?.value || 0} 条工单`,
  `${reportPendingCount.value} 条待处理`,
  `${reportResolutionScore.value} 分 ${reportResolutionLabel.value}`,
])

const reportGuideHeadline = computed(() => {
  if (reportPendingCount.value > reportHandledCount.value)
    return '待处理工单偏多，先清掉堆积再回看已忽略项'
  if (reportIgnoredCount.value > 0) return '当前队列不算拥挤，可以抽空复查已忽略记录是否仍然成立'
  return '处置节奏平稳，继续盯住最新流入和高频类型即可'
})

const reportGuideCopy = computed(() => {
  if (reportPendingCount.value > reportHandledCount.value) {
    return `当前页待处理 ${formatCount(reportPendingCount.value)} 条，高于已完成 ${formatCount(reportHandledCount.value)} 条，建议优先处理新流入工单，避免队列继续堆积。`
  }
  if (reportIgnoredCount.value > 0) {
    return `当前页有 ${formatCount(reportIgnoredCount.value)} 条已忽略记录，可以结合举报原因和目标类型进行抽样回看，降低误判遗留。`
  }
  return '当前队列节奏比较平稳，继续以最新流入和待处理状态为主轴即可。'
})

const reportGuideMetrics = computed(() => [
  { label: '待处理', value: `${formatCount(reportPendingCount.value)} 条` },
  { label: '已完成', value: `${formatCount(reportHandledCount.value)} 条` },
  { label: '处置评分', value: `${reportResolutionScore.value} 分` },
])

const reportsOverviewDescription = computed(() => {
  if (reportPendingCount.value > reportHandledCount.value) {
    return `汇总举报与求助记录，按状态快速筛查并完成确认、驳回或忽略，当前优先消化 ${formatCount(reportPendingCount.value)} 条待处理工单。`
  }
  return '汇总举报与求助记录，按状态快速筛查并完成确认、驳回或忽略，保证处置过程清晰可追溯。'
})

const reportsOverviewCards = computed(() => createDeckOverviewCards(summaryItems.value.slice(1, 3)))
const reportsFocusCard = computed(() =>
  createDeckFocusCard(summaryItems.value[3], '当前页已忽略但仍可抽样回看的记录数量。'),
)
const reportRhythmItems = computed(() => createDeckRhythmItems(reportVizBars.value))
const reportActivityRows = computed(() => createDeckActivityRows(reportSignals.value))
const reportsGuidePulseNote = computed(
  () =>
    `待处理占比 ${reportList.value.length ? Math.round((reportPendingCount.value / reportList.value.length) * 100) : 0}% · ${reportResolutionLabel.value}`,
)
const reportGuideItems = computed(() =>
  createDeckGuideItems([
    {
      label: '待处理',
      value: `${formatCount(reportPendingCount.value)} 条`,
      note: '先处理新流入工单，避免求助队列继续堆积。',
    },
    {
      label: '已完成',
      value: `${formatCount(reportHandledCount.value)} 条`,
      note: '完成量反映当前人工处置节奏是否跟得上流入。',
    },
    {
      label: '处置评分',
      value: `${reportResolutionScore.value} 分`,
      note: '综合待处理、已完成和已忽略比例形成当前判断。',
    },
  ]),
)

async function fetchReports() {
  loading.value = true
  try {
    const res = await api.getReports(buildParams(filters))
    const { items, total } = normalizeCollectionResponse<Report>(res.data, ['reports'])
    reportList.value = items
    pagination.total = total
  } catch (e) {
    if (isRequestCanceled(e)) return
    console.error('获取举报列表失败:', e)
    ElMessage.error(getErrorMessage(e, '获取举报列表失败'))
    reportList.value = []
    pagination.total = 0
  } finally {
    loading.value = false
  }
}

const handleReport = async (row: { id: string; status: string }, action: string) => {
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
    const ts = new Date().toLocaleTimeString('zh-CN', { hour12: false })
    ElMessage.success(`${actionText}成功 [${ts}]，操作已记录到审计日志`)
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
  .reports-filter-form {
    :deep(.el-form-item) {
      margin-bottom: 0;
    }
  }

  .reports-table-toolbar {
    display: grid;
    grid-template-columns: minmax(0, 0.86fr) minmax(0, 1.14fr);
    align-items: flex-end;
  }

  .reports-inline-filter {
    justify-content: flex-end;
  }

  .reports-table-copy {
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

  @media (max-width: 1180px) {
    .reports-table-toolbar {
      grid-template-columns: 1fr;
    }
  }

  .pagination-wrapper {
    margin-top: 10px;
    display: flex;
    justify-content: flex-end;
  }

  .handled-text {
    color: var(--m3-on-surface-variant);
    font-size: 14px;
  }
}
</style>
