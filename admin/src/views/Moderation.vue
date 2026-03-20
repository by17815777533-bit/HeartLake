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
  <div class="moderation-page ops-page">
    <OpsWorkbench>
      <template #stage>
        <OpsSurfaceCard
          eyebrow="守护"
          title="温暖守护"
          :chip="activeTab === 'pending' ? '待审核视角' : '历史视角'"
          tone="sky"
        >
          <div class="ops-big-metric">
            <span class="ops-big-metric__label">待复核队列</span>
            <div class="ops-big-metric__value">
              {{ summaryItems[0]?.value || 0 }}
              <small>条</small>
            </div>
            <p class="ops-big-metric__note">
              查看待审核队列与审核历史，结合触发原因完成人工复核，保证社区秩序与表达边界。
            </p>
          </div>

          <div class="ops-soft-actions moderation-stage-actions">
            <el-button type="primary" @click="fetchPending"> 刷新队列 </el-button>
            <el-button @click="fetchHistory"> 回看历史 </el-button>
          </div>

          <div class="ops-mini-grid">
            <article
              v-for="item in summaryItems.slice(1)"
              :key="item.label"
              class="ops-mini-tile"
              :class="getWorkbenchTileTone(item.tone)"
            >
              <span>{{ item.label }}</span>
              <strong>{{ item.value }}</strong>
              <small>{{ item.note }}</small>
            </article>
          </div>
        </OpsSurfaceCard>
      </template>

      <template #support>
        <OpsSurfaceCard
          eyebrow="风险"
          title="风险分布"
          :chip="`${highRiskPendingCount} 高危`"
          tone="ice"
          compact
        >
          <OpsMiniBars :items="moderationVizBars" />
        </OpsSurfaceCard>
      </template>

      <template #rail>
        <OpsSurfaceCard
          eyebrow="情报"
          title="风险情报"
          :chip="moderationSignals[0]?.value || '秩序平稳'"
          tone="mint"
        >
          <div class="ops-list-stack">
            <article v-for="item in moderationSignals" :key="item.label" class="ops-list-row">
              <div class="ops-list-row__badge">
                {{ item.label.slice(0, 2) }}
              </div>
              <div class="ops-list-row__copy">
                <strong>{{ item.value }}</strong>
                <span>{{ item.note }}</span>
              </div>
              <div class="ops-list-row__value">
                {{ item.badge }}
              </div>
            </article>
          </div>
        </OpsSurfaceCard>
      </template>

      <el-card shadow="never" class="table-card ops-table-card">
        <div class="ops-soft-toolbar">
          <div class="moderation-table-copy">
            <h3>审核工作区</h3>
            <p>待审核和审核历史共用同一张工作台，便于切换标准和回看人审决策。</p>
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

            <el-table
              v-loading="historyLoading"
              :data="historyList"
              stripe
              aria-label="审核历史列表"
            >
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
    </OpsWorkbench>

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
import OpsWorkbench from '@/components/OpsWorkbench.vue'
import OpsSurfaceCard from '@/components/OpsSurfaceCard.vue'
import OpsMiniBars from '@/components/OpsMiniBars.vue'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import { getWorkbenchTileTone } from '@/utils/workbenchTone'
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
  const highRiskCount = pendingList.value.filter((item) => Number(item.ai_score || 0) >= 0.7).length
  const approvedCount = historyList.value.filter(
    (item) => (item as ModerationItem & { result?: string }).result === 'approved',
  ).length
  const rejectedCount = historyList.value.filter(
    (item) => (item as ModerationItem & { result?: string }).result === 'rejected',
  ).length

  return [
    {
      label: '待复核队列',
      value: formatCount(Number(pagination.total || 0)),
      note: '当前仍等待人工复核的内容',
      tone: 'rose' as const,
    },
    {
      label: '高风险提示',
      value: formatCount(highRiskCount),
      note: '当前页风险分较高的条目',
      tone: 'amber' as const,
    },
    {
      label: '历史通过',
      value: formatCount(approvedCount),
      note: '当前页审核历史中的通过条目',
      tone: 'sage' as const,
    },
    {
      label: '历史拒绝',
      value: formatCount(rejectedCount),
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

const highRiskPendingCount = computed(
  () => pendingList.value.filter((item) => getRiskPercent(item.ai_score) >= 85).length,
)
const mediumRiskPendingCount = computed(
  () =>
    pendingList.value.filter((item) => {
      const percent = getRiskPercent(item.ai_score)
      return percent >= 60 && percent < 85
    }).length,
)
const lowRiskPendingCount = computed(
  () => pendingList.value.filter((item) => getRiskPercent(item.ai_score) < 60).length,
)

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
  const approvedCount = historyList.value.filter(
    (item) => (item as ModerationItem & { result?: string }).result === 'approved',
  ).length
  const approvalRate = reviewedCount ? approvedCount / reviewedCount : 0
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
  const counter = new Map<string, number>()
  pendingList.value.forEach((item) => {
    const reason = item.ai_reason || '系统识别'
    counter.set(reason, (counter.get(reason) || 0) + 1)
  })

  const [reason, count] = [...counter.entries()].sort((a, b) => b[1] - a[1])[0] || ['系统识别', 0]
  return { reason, count }
})

const moderationSignals = computed(() => {
  const highRiskCount = pendingList.value.filter(
    (item) => getRiskPercent(item.ai_score) >= 85,
  ).length
  const reviewedCount = historyList.value.length
  const approvedCount = historyList.value.filter(
    (item) => (item as ModerationItem & { result?: string }).result === 'approved',
  ).length
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

async function fetchPending() {
  loading.value = true
  try {
    const res = await api.getPendingModeration(buildPendingParams())
    const data = res.data?.data || res.data || {}
    pendingList.value = data.list || []
    pagination.total = data.total || 0
  } catch (e) {
    if (isRequestCanceled(e)) return
    console.error('获取待审核列表失败:', e)
    ElMessage.error(getErrorMessage(e, '获取待审核列表失败'))
    pendingList.value = []
  } finally {
    loading.value = false
  }
}

async function fetchHistory() {
  historyLoading.value = true
  try {
    const res = await api.getModerationHistory(buildHistoryParams(historyFilters))
    const data = res.data?.data || res.data || {}
    historyList.value = data.list || []
    historyPagination.total = data.total || 0
  } catch (e) {
    if (isRequestCanceled(e)) return
    console.error('获取审核历史失败:', e)
    ElMessage.error(getErrorMessage(e, '获取审核历史失败'))
    historyList.value = []
  } finally {
    historyLoading.value = false
  }
}

const handleTabChange = (tab: string) => {
  tab === 'pending' ? fetchPending() : fetchHistory()
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
  .moderation-stage-actions {
    margin: 22px 0 18px;
  }

  .moderation-table-copy {
    h3 {
      color: var(--hl-ink);
      font-size: 24px;
      font-weight: 700;
      letter-spacing: -0.03em;
    }

    p {
      margin-top: 8px;
      color: var(--hl-ink-soft);
      font-size: 13px;
      line-height: 1.7;
    }
  }

  .moderation-history-filter {
    margin-bottom: 16px;
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
    margin-top: 20px;
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
