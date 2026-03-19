<!--
  求助处理（举报管理）页面

  功能：
  - 举报列表展示：举报人、目标类型（石头/纸船/用户）、原因、内容预览、状态
  - 处理操作：approve（确认违规）/ reject（驳回举报）/ dismiss（忽略）
  - 处理时可附加备注说明
  - 支持按状态筛选和分页浏览
-->

<template>
  <div class="reports-page ops-page">
    <OpsWorkbench>
      <template #stage>
        <OpsSurfaceCard
          eyebrow="Reports"
          title="求助处理"
          :chip="summaryItems[1]?.value ? `${summaryItems[1].value} 待处理` : '优先处置'"
          tone="sky"
        >
          <div class="ops-big-metric">
            <span class="ops-big-metric__label">求助总数</span>
            <div class="ops-big-metric__value">
              {{ summaryItems[0]?.value || 0 }}
              <small>条</small>
            </div>
            <p class="ops-big-metric__note">
              汇总举报与求助记录，按状态快速筛查并完成确认、驳回或忽略，保证处置过程清晰可追溯。
            </p>
          </div>

          <div class="ops-soft-actions reports-stage-actions">
            <el-button
              type="primary"
              @click="handleSearch"
            >
              刷新工单
            </el-button>
            <el-button @click="handleReset">
              重置筛查
            </el-button>
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
          eyebrow="Queue"
          title="处置分布"
          :chip="`${reportPendingCount} 待处理`"
          tone="ice"
          compact
        >
          <OpsMiniBars :items="reportVizBars" />
        </OpsSurfaceCard>
      </template>

      <template #rail>
        <OpsSurfaceCard
          eyebrow="Status"
          title="处置状态"
          :chip="summaryItems[1]?.value ? `${summaryItems[1].value} 等待` : '空闲'"
          tone="mint"
        >
          <div class="ops-kv-grid">
            <article
              v-for="item in summaryItems.slice(1)"
              :key="item.label"
              class="ops-kv-item"
            >
              <span>{{ item.label }}</span>
              <strong>{{ item.value }}</strong>
            </article>
          </div>
        </OpsSurfaceCard>
      </template>

      <template #footer>
        <OpsSurfaceCard
          eyebrow="Score"
          title="回执效率"
          :chip="`${reportResolutionScore} / 100`"
          tone="plain"
          compact
        >
          <OpsGaugeMeter
            :value="reportResolutionScore"
            :max="100"
            :label="reportResolutionLabel"
          />
        </OpsSurfaceCard>
      </template>

      <el-card
        shadow="never"
        class="table-card ops-table-card"
      >
        <div class="ops-soft-toolbar reports-table-toolbar">
          <div class="reports-table-copy">
            <h3>求助列表</h3>
            <p>待处理项和已回执项放在同一面板中回看，处置后会直接写入后台日志。</p>
          </div>
          <el-form
            :model="filters"
            inline
            aria-label="举报筛选"
            class="reports-inline-filter"
          >
            <el-form-item label="状态">
              <el-select
                v-model="filters.status"
                placeholder="全部"
                clearable
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
        </div>

        <el-table
          v-loading="loading"
          :data="reportList"
          stripe
          aria-label="举报列表"
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
          <template #empty>
            <el-empty
              description="暂无举报数据"
              :image-size="120"
            />
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
    </OpsWorkbench>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import api, { isRequestCanceled } from '@/api'
import OpsWorkbench from '@/components/OpsWorkbench.vue'
import OpsSurfaceCard from '@/components/OpsSurfaceCard.vue'
import OpsMiniBars from '@/components/OpsMiniBars.vue'
import OpsGaugeMeter from '@/components/OpsGaugeMeter.vue'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import { getWorkbenchTileTone } from '@/utils/workbenchTone'
import type { Report } from '@/types'

const loading = ref(false)
const reportList = ref<Report[]>([])

const filters = reactive({
  status: '',
  type: '',
})

const { pagination, buildParams, handleSizeChange, handleCurrentChange, handleSearch, handleReset } = useTablePagination(fetchReports, {
  filters,
  defaultFilters: { status: '', type: '' },
})

const getTypeLabel = (type: string) => {
  const map: Record<string, string> = { spam: '垃圾信息', harassment: '骚扰辱骂', inappropriate: '不当内容', violence: '暴力内容', other: '其他' }
  return map[type] || type
}

const getTypeColor = (type: string) => {
  const map: Record<string, string> = { spam: '#909399', harassment: '#E07A5F', inappropriate: '#F2CC8F', violence: '#E6A23C', other: '#81B29A' }
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
    { label: '求助总数', value: formatCount(Number(pagination.total || 0)), note: '当前筛选下的记录总量', tone: 'lake' as const },
    { label: '待优先处理', value: formatCount(pendingCount), note: '当前页仍需人工判断的记录', tone: 'rose' as const },
    { label: '已完成处理', value: formatCount(handledCount), note: '当前页已给出处置结果', tone: 'sage' as const },
    { label: '已忽略', value: formatCount(ignoredCount), note: '当前页不进入进一步处理的记录', tone: 'amber' as const },
  ]
})

const reportPendingCount = computed(() => reportList.value.filter((item) => item.status === 'pending').length)
const reportHandledCount = computed(() => reportList.value.filter((item) => item.status === 'handled').length)
const reportIgnoredCount = computed(() => reportList.value.filter((item) => item.status === 'ignored').length)

const reportVizBars = computed(() => [
  { label: '待处理', value: reportPendingCount.value, display: formatCount(reportPendingCount.value) },
  { label: '已处理', value: reportHandledCount.value, display: formatCount(reportHandledCount.value) },
  { label: '已忽略', value: reportIgnoredCount.value, display: formatCount(reportIgnoredCount.value) },
  { label: '总量', value: reportList.value.length, display: formatCount(reportList.value.length) },
])

const reportResolutionScore = computed(() => {
  const total = Math.max(reportList.value.length, 1)
  return Math.max(24, Math.min(96, Math.round(((reportHandledCount.value + reportIgnoredCount.value) / total) * 100)))
})

const reportResolutionLabel = computed(() => {
  if (reportResolutionScore.value >= 80) return '高效'
  if (reportResolutionScore.value >= 55) return '处理中'
  return '积压'
})

async function fetchReports() {
  loading.value = true
  try {
    const res = await api.getReports(buildParams(filters))
    const data = res.data?.data || res.data || {}
    reportList.value = data.list || []
    pagination.total = data.total || 0
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
  .reports-stage-actions {
    margin: 22px 0 18px;
  }

  .reports-filter-form {
    :deep(.el-form-item) {
      margin-bottom: 0;
    }
  }

  .reports-table-toolbar {
    align-items: flex-start;
  }

  .reports-inline-filter {
    justify-content: flex-end;
  }

  .reports-table-copy {
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
