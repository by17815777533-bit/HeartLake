<!--
  温暖守护（内容审核）页面

  功能：
  - 双 Tab 切换：待审核队列 / 审核历史
  - 待审核列表展示 AI 预审分数和触发原因，支持通过/拒绝操作
  - 拒绝操作需输入原因（>=5字），通过操作直接执行
  - 审核历史支持分页浏览
  - AI 风险分以进度条可视化，颜色随分值变化（绿→橙→红）
-->

<template>
  <div class="moderation-page ops-page ops-page--compact">
    <OpsDashboardDeck
      compact
      eyebrow="守护"
      title="温暖守护"
      :heading-chip="`${moderationScore} 分 ${moderationLabel}`"
      metric-label="待复核队列"
      :metric-value="summaryItems[0]?.value || '0'"
      metric-unit="条"
      :metric-description="moderationOverviewDescription"
      section-note="守护重点"
      :overview-cards="moderationOverviewCards"
      :focus-card="moderationFocusCard"
      rhythm-eyebrow="风险"
      rhythm-title="风险分布"
      :rhythm-chip="`${highRiskPendingCount} 高危`"
      :rhythm-badge="`${formatCount(Number(pagination.total || 0))} 条待复核`"
      :rhythm-items="moderationRhythmItems"
      activity-eyebrow="情报"
      activity-title="风险情报"
      :activity-chip="moderationSignals[0]?.value || '秩序平稳'"
      :activity-rows="moderationActivityRows"
      guide-title="复核建议"
      :guide-chip="moderationLabel"
      :guide-headline="moderationGuideHeadline"
      :guide-copy="moderationGuideCopy"
      guide-pulse-label="当前守护评分"
      :guide-pulse-value="`${moderationScore} 分`"
      :guide-pulse-note="moderationGuidePulseNote"
      :guide-items="moderationGuideItems"
    >
      <template #actions>
        <button type="button" class="overview-action" @click="fetchPending">刷新队列</button>
        <button type="button" class="overview-action" @click="fetchHistory">回看历史</button>
      </template>
    </OpsDashboardDeck>

    <el-card shadow="never" class="table-card ops-table-card">
      <div class="ops-soft-toolbar">
        <div class="moderation-table-copy">
          <h3>审核工作区</h3>
          <p>待审与历史共用一张工作台，便于快速回看决策。</p>
          <div class="ops-toolbar-meta">
            <span class="ops-toolbar-meta__item"
              >待复核 {{ formatCount(Number(pagination.total || 0)) }} 条</span
            >
            <span class="ops-toolbar-meta__item">高风险 {{ highRiskPendingCount }} 条</span>
            <span class="ops-toolbar-meta__item">守护评分 {{ moderationScore }} 分</span>
          </div>
        </div>
      </div>

      <el-tabs v-model="activeTab" @tab-change="handleTabChange">
        <el-tab-pane label="待审核" name="pending">
          <el-table v-loading="loading" :data="pendingList" stripe aria-label="待审核内容列表">
            <el-table-column prop="moderation_id" label="ID" width="80" />
            <el-table-column label="类型" width="80">
              <template #default="{ row }">
                <el-tag :type="row.content_type === 'stone' ? 'primary' : 'success'" size="small">
                  {{ row.content_type === 'stone' ? '石头' : '纸船' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="内容" min-width="250">
              <template #default="{ row }">
                <p class="content-preview">
                  {{ row.content?.substring(0, 80) }}{{ row.content?.length > 80 ? '...' : '' }}
                </p>
              </template>
            </el-table-column>
            <el-table-column label="触发原因" width="150">
              <template #default="{ row }">
                <el-tag type="warning" size="small">
                  {{ row.ai_reason || '系统识别' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="风险分" width="180">
              <template #default="{ row }">
                <div class="risk-meter">
                  <div class="risk-meter__meta">
                    <strong>{{ getRiskLabel(row.ai_score) }}</strong>
                    <span>{{ getRiskPercent(row.ai_score) }}%</span>
                  </div>
                  <el-progress
                    :percentage="getRiskPercent(row.ai_score)"
                    :show-text="false"
                    :stroke-width="8"
                    :color="getRiskColor(row.ai_score)"
                  />
                </div>
              </template>
            </el-table-column>
            <el-table-column prop="created_at" label="提交时间" width="170" />
            <el-table-column label="操作" width="180" fixed="right">
              <template #default="{ row }">
                <el-button type="success" link @click="handleApprove(row)"> 通过 </el-button>
                <el-button type="danger" link @click="handleReject(row)"> 拒绝 </el-button>
                <el-button type="primary" link @click="viewDetail(row)"> 详情 </el-button>
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

        <el-tab-pane label="审核历史" name="history">
          <div class="moderation-history-filter">
            <el-form :model="historyFilters" inline aria-label="审核历史筛选">
              <el-form-item label="结果">
                <el-select v-model="historyFilters.result" placeholder="全部" clearable>
                  <el-option label="通过" value="approved" />
                  <el-option label="拒绝" value="rejected" />
                </el-select>
              </el-form-item>
              <el-form-item>
                <el-button type="primary" @click="fetchHistory"> 搜索 </el-button>
              </el-form-item>
            </el-form>
          </div>

          <el-table v-loading="historyLoading" :data="historyList" stripe aria-label="审核历史列表">
            <el-table-column prop="moderation_id" label="ID" width="80" />
            <el-table-column label="内容摘要" min-width="200">
              <template #default="{ row }">
                {{ row.content?.substring(0, 50) }}{{ row.content?.length > 50 ? '...' : '' }}
              </template>
            </el-table-column>
            <el-table-column label="审核结果" width="100">
              <template #default="{ row }">
                <el-tag :type="row.result === 'approved' ? 'success' : 'danger'" size="small">
                  {{ row.result === 'approved' ? '通过' : '拒绝' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="moderator" label="操作人" width="100" />
            <el-table-column prop="moderated_at" label="处理时间" width="170" />
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
    </el-card>

    <!-- 详情弹窗 -->
    <el-dialog
      v-model="detailVisible"
      title="内容详情"
      width="600px"
      aria-labelledby="moderation-detail-title"
    >
      <div v-if="currentItem">
        <el-descriptions :column="2" border>
          <el-descriptions-item label="ID">
            {{ currentItem.moderation_id || currentItem.content_id }}
          </el-descriptions-item>
          <el-descriptions-item label="类型">
            {{ currentItem.content_type === 'stone' ? '石头' : '纸船' }}
          </el-descriptions-item>
          <el-descriptions-item label="触发原因" :span="2">
            {{ currentItem.ai_reason || '自动检测' }}
          </el-descriptions-item>
          <el-descriptions-item label="风险评级">
            {{ getRiskLabel(currentItem.ai_score) }}
          </el-descriptions-item>
          <el-descriptions-item label="风险分">
            {{ getRiskPercent(currentItem.ai_score) }}%
          </el-descriptions-item>
          <el-descriptions-item label="内容" :span="2">
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
import type { ModerationItem } from '@/types'

const activeTab = ref('pending')
const loading = ref(false)
const historyLoading = ref(false)
const pendingList = ref<ModerationItem[]>([])
const historyList = ref<ModerationItem[]>([])
const detailVisible = ref(false)
const currentItem = ref<ModerationItem | null>(null)

const historyFilters = reactive({ result: '' })

const formatCount = (value: number) => value.toLocaleString()

const {
  pagination,
  buildParams: buildPendingParams,
  handleSizeChange: handlePendingSizeChange,
  handleCurrentChange: handlePendingCurrentChange,
} = useTablePagination(fetchPending)
const {
  pagination: historyPagination,
  buildParams: buildHistoryParams,
  handleSizeChange: handleHistorySizeChange,
  handleCurrentChange: handleHistoryCurrentChange,
} = useTablePagination(fetchHistory, {
  filters: historyFilters,
  defaultFilters: { result: '' },
})

const summaryItems = computed(() => {
  return [
    {
      label: '待复核队列',
      value: formatCount(Number(pagination.total || 0)),
      note: '当前仍等待人工复核的内容',
      tone: 'rose' as const,
    },
    {
      label: '高风险提示',
      value: formatCount(pendingPageStats.value.highRiskCount),
      note: '当前页风险分较高的条目',
      tone: 'amber' as const,
    },
    {
      label: '历史通过',
      value: formatCount(historyPageStats.value.approvedCount),
      note: '当前页审核历史中的通过条目',
      tone: 'sage' as const,
    },
    {
      label: '历史拒绝',
      value: formatCount(historyPageStats.value.rejectedCount),
      note: '当前页审核历史中的拦截条目',
      tone: 'lake' as const,
    },
  ]
})

const getRiskPercent = (score?: number) =>
  Math.min(100, Math.max(0, Math.round(Number(score || 0) * 100)))

const getRiskLabel = (score?: number) => {
  const percent = getRiskPercent(score)
  if (percent >= 85) return '高危'
  if (percent >= 60) return '重点复核'
  if (percent >= 30) return '需要留意'
  return '低危'
}

const getRiskColor = (score?: number) => {
  const percent = getRiskPercent(score)
  if (percent >= 85) return '#a5483e'
  if (percent >= 60) return '#b07622'
  if (percent >= 30) return '#8c8f52'
  return '#49735a'
}

const pendingPageStats = computed(() => {
  let highRiskCount = 0
  let mediumRiskCount = 0
  let lowRiskCount = 0
  const reasonCounter = new Map<string, number>()

  pendingList.value.forEach((item) => {
    const percent = getRiskPercent(item.ai_score)
    if (percent >= 85) highRiskCount += 1
    else if (percent >= 60) mediumRiskCount += 1
    else lowRiskCount += 1

    const reason = item.ai_reason || '系统识别'
    reasonCounter.set(reason, (reasonCounter.get(reason) || 0) + 1)
  })

  let dominantReason = '系统识别'
  let dominantReasonCount = 0
  reasonCounter.forEach((count, reason) => {
    if (count >= dominantReasonCount) {
      dominantReason = reason
      dominantReasonCount = count
    }
  })

  return {
    highRiskCount,
    mediumRiskCount,
    lowRiskCount,
    dominantReason,
    dominantReasonCount,
  }
})

const historyPageStats = computed(() => {
  let approvedCount = 0
  let rejectedCount = 0

  historyList.value.forEach((item) => {
    if ((item as ModerationItem & { result?: string }).result === 'approved') approvedCount += 1
    if ((item as ModerationItem & { result?: string }).result === 'rejected') rejectedCount += 1
  })

  return { approvedCount, rejectedCount }
})

const highRiskPendingCount = computed(() => pendingPageStats.value.highRiskCount)
const mediumRiskPendingCount = computed(() => pendingPageStats.value.mediumRiskCount)
const lowRiskPendingCount = computed(() => pendingPageStats.value.lowRiskCount)

const moderationVizBars = computed(() => [
  {
    label: '高危',
    value: highRiskPendingCount.value,
    display: formatCount(highRiskPendingCount.value),
  },
  {
    label: '中危',
    value: mediumRiskPendingCount.value,
    display: formatCount(mediumRiskPendingCount.value),
  },
  {
    label: '低危',
    value: lowRiskPendingCount.value,
    display: formatCount(lowRiskPendingCount.value),
  },
  {
    label: '历史',
    value: historyList.value.length,
    display: formatCount(historyList.value.length),
  },
])

const moderationScore = computed(() => {
  const reviewedCount = historyList.value.length
  const approvalRate = reviewedCount ? historyPageStats.value.approvedCount / reviewedCount : 0
  return Math.max(
    28,
    Math.min(96, Math.round(68 + approvalRate * 22 - highRiskPendingCount.value * 4)),
  )
})

const moderationLabel = computed(() => {
  if (moderationScore.value >= 80) return '稳健'
  if (moderationScore.value >= 58) return '复核中'
  return '紧张'
})

const dominantReason = computed(() => {
  return {
    reason: pendingPageStats.value.dominantReason,
    count: pendingPageStats.value.dominantReasonCount,
  }
})

const moderationSignals = computed(() => {
  const highRiskCount = pendingPageStats.value.highRiskCount
  const reviewedCount = historyList.value.length
  const approvedCount = historyPageStats.value.approvedCount
  const approvalRate = reviewedCount
    ? `${Math.round((approvedCount / reviewedCount) * 100)}%`
    : '0%'

  return [
    {
      label: '队列温度',
      value: highRiskCount > 0 ? '需要优先干预' : '秩序平稳',
      note:
        highRiskCount > 0
          ? `当前仍有 ${formatCount(highRiskCount)} 条高危内容等待人工复核。`
          : '当前页暂未出现高危条目，可以更关注边界判断与误伤风险。',
      badge: `待复核 ${formatCount(Number(pagination.total || 0))}`,
      tone: 'rose' as const,
    },
    {
      label: '主要触发',
      value: dominantReason.value.reason,
      note:
        dominantReason.value.count > 0
          ? `${formatCount(dominantReason.value.count)} 条内容触发了同类原因。`
          : '当前没有待审核内容，触发原因将随新队列更新。',
      badge: activeTab.value === 'pending' ? '待审核视角' : '历史视角',
      tone: 'amber' as const,
    },
    {
      label: '人工倾向',
      value: approvalRate,
      note: reviewedCount
        ? `当前历史页共回看 ${formatCount(reviewedCount)} 条记录。`
        : '切到审核历史后会显示这一页的人审通过占比。',
      badge: `通过 ${formatCount(approvedCount)}`,
      tone: 'sage' as const,
    },
  ]
})

const moderationGuideHeadline = computed(() => {
  if (highRiskPendingCount.value > 0) return '高危条目仍在队列里，先处理边界最清晰的一批'
  if (historyList.value.length > 0) return '队列相对平稳，可以抽时间回看历史通过率和误伤边界'
  return '当前队列较轻，保持巡看即可，重点等待新的触发原因出现'
})

const moderationGuideCopy = computed(() => {
  if (highRiskPendingCount.value > 0) {
    return `当前页仍有 ${formatCount(highRiskPendingCount.value)} 条高危内容等待人工复核，建议先处理风险分最高且触发原因最集中的条目。`
  }
  if (historyList.value.length > 0) {
    return `当前已回看 ${formatCount(historyList.value.length)} 条历史记录，可以顺手检查通过与拒绝比例是否仍符合当前边界标准。`
  }
  return '当前没有明显堆积，保持对新入队内容的持续巡看即可。'
})

const moderationOverviewDescription = computed(() => {
  if (highRiskPendingCount.value > 0) {
    return `查看待审核队列与审核历史，结合触发原因完成人工复核，当前优先处理 ${formatCount(highRiskPendingCount.value)} 条高危内容。`
  }
  return '查看待审核队列与审核历史，结合触发原因完成人工复核，保证社区秩序与表达边界。'
})

const moderationOverviewCards = computed(() =>
  createDeckOverviewCards(summaryItems.value.slice(1, 3)),
)
const moderationFocusCard = computed(() =>
  createDeckFocusCard(summaryItems.value[3], '当前历史视角中被人工拒绝拦截的条目数量。'),
)
const moderationRhythmItems = computed(() => createDeckRhythmItems(moderationVizBars.value))
const moderationActivityRows = computed(() => createDeckActivityRows(moderationSignals.value))
const moderationGuidePulseNote = computed(
  () => `高危 ${formatCount(highRiskPendingCount.value)} 条 · ${moderationLabel.value}`,
)
const moderationGuideItems = computed(() =>
  createDeckGuideItems([
    {
      label: '高危队列',
      value: `${formatCount(highRiskPendingCount.value)} 条`,
      note: '先处理边界最清晰且风险分最高的一批内容。',
    },
    {
      label: '历史回看',
      value: `${formatCount(historyList.value.length)} 条`,
      note: '历史越充分，越容易校准当前人审标准。',
    },
    {
      label: '守护评分',
      value: `${moderationScore.value} 分`,
      note: '结合高危密度和历史通过率形成当前判断。',
    },
  ]),
)

async function fetchPending() {
  loading.value = true
  try {
    const res = await api.getPendingModeration(buildPendingParams())
    const { items, total } = normalizeCollectionResponse<ModerationItem>(res.data, ['pending'])
    pendingList.value = items
    pagination.total = total
  } catch (e) {
    if (isRequestCanceled(e)) return
    console.error('获取待审核列表失败:', e)
    ElMessage.error(getErrorMessage(e, '获取待审核列表失败'))
    pendingList.value = []
    pagination.total = 0
  } finally {
    loading.value = false
  }
}

async function fetchHistory() {
  historyLoading.value = true
  try {
    const res = await api.getModerationHistory(buildHistoryParams(historyFilters))
    const { items, total } = normalizeCollectionResponse<ModerationItem>(res.data, ['history'])
    historyList.value = items
    historyPagination.total = total
  } catch (e) {
    if (isRequestCanceled(e)) return
    console.error('获取审核历史失败:', e)
    ElMessage.error(getErrorMessage(e, '获取审核历史失败'))
    historyList.value = []
    historyPagination.total = 0
  } finally {
    historyLoading.value = false
  }
}

const handleTabChange = (tab: string) => {
  if (tab === 'pending') {
    fetchPending()
    return
  }
  fetchHistory()
}

// 审核通过添加二次确认
const handleApprove = async (row: ModerationItem) => {
  try {
    await ElMessageBox.confirm('确定通过该内容的审核吗？', '审核确认', {
      confirmButtonText: '确定通过',
      cancelButtonText: '取消',
      type: 'info',
    })
  } catch {
    return
  }
  // 乐观更新：先从本地列表移除，再异步请求后端
  const itemId = row.moderation_id || row.content_id
  const removedIndex = pendingList.value.findIndex(
    (item) => (item.moderation_id || item.content_id) === itemId,
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
    confirmButtonText: '确定',
    cancelButtonText: '取消',
    inputPattern: /\S+/,
    inputErrorMessage: '请输入原因',
  }).catch(() => ({ value: null }))
  if (!reason) return
  // 乐观更新：先从本地列表移除，再异步请求后端
  const itemId = row.moderation_id || row.content_id
  const removedIndex = pendingList.value.findIndex(
    (item) => (item.moderation_id || item.content_id) === itemId,
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

const viewDetail = (row: ModerationItem) => {
  currentItem.value = row
  detailVisible.value = true
}

onMounted(() => fetchPending())
</script>

<style lang="scss" scoped>
.moderation-page {
  .moderation-table-copy {
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

  .moderation-history-filter {
    margin-bottom: 10px;
  }

  .content-preview {
    margin: 0;
    color: var(--hl-ink);
    line-height: 1.7;
  }

  .risk-meter {
    display: grid;
    gap: 8px;
  }

  .risk-meter__meta {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 10px;

    strong {
      color: var(--hl-ink);
      font-size: 12px;
      font-weight: 700;
    }

    span {
      color: var(--hl-ink-soft);
      font-size: 12px;
    }
  }

  .pagination-wrapper {
    margin-top: 10px;
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
