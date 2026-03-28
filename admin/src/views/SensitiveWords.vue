<!--
  风险词典（敏感词管理）页面

  功能：
  - 敏感词列表展示，支持按关键词搜索和分页
  - 新增敏感词：填写词条、风险等级（low/medium/high/critical）、处置策略
  - 编辑敏感词：弹窗修改词条属性
  - 删除敏感词：二次确认后删除
  - 风险等级以不同颜色 tag 区分，便于快速识别
-->

<template>
  <div class="sensitive-words-page ops-page ops-page--compact">
    <OpsDashboardDeck
      compact
      eyebrow="词典"
      title="风险词典"
      :heading-chip="sensitiveHeadingChip"
      metric-label="词条总量"
      :metric-value="summaryItems[0]?.value || '0'"
      metric-unit="条"
      :metric-description="sensitiveOverviewDescription"
      section-note="维护重点"
      :overview-cards="sensitiveOverviewCards"
      :focus-card="sensitiveFocusCard"
      rhythm-eyebrow="级别"
      rhythm-title="级别分布"
      :rhythm-chip="sensitiveRhythmChip"
      :rhythm-badge="sensitiveRhythmBadge"
      :rhythm-items="sensitiveRhythmItems"
      activity-title="词典动态"
      :activity-chip="latestSensitiveMeta.value"
      :activity-rows="sensitiveActivityRows"
      guide-title="维护建议"
      :guide-chip="sensitiveLabel"
      :guide-headline="sensitiveGuideHeadline"
      :guide-copy="sensitiveGuideCopy"
      guide-pulse-label="当前词典评分"
      :guide-pulse-value="sensitiveGuidePulseValue"
      :guide-pulse-note="sensitiveGuidePulseNote"
      :guide-items="sensitiveGuideItems"
    >
      <template #actions>
        <button type="button" class="overview-action" @click="showAddDialog">添加敏感词</button>
        <button type="button" class="overview-action" @click="handleSearch">刷新词典</button>
      </template>
    </OpsDashboardDeck>

    <el-card shadow="never" class="table-card ops-table-card">
      <div class="ops-soft-toolbar sensitive-table-toolbar">
        <div class="sensitive-table-copy">
          <h3>词典列表</h3>
          <p>词条、级别与处置策略都集中在这里维护。</p>
          <div class="ops-toolbar-meta">
            <span class="ops-toolbar-meta__item">高风险 {{ highRiskWordCount }} 条</span>
            <span class="ops-toolbar-meta__item">放行策略 {{ allowStrategyCount }} 条</span>
            <span class="ops-toolbar-meta__item">已选 {{ selectedWords.length }} 项</span>
          </div>
        </div>
        <div class="ops-soft-actions">
          <el-form :model="filters" inline aria-label="敏感词筛选" class="sensitive-inline-filter">
            <el-form-item label="关键词">
              <el-input
                v-model="filters.keyword"
                placeholder="搜索敏感词"
                clearable
                @keyup.enter="handleSearch"
              />
            </el-form-item>
            <el-form-item label="级别">
              <el-select v-model="filters.level" placeholder="全部" clearable>
                <el-option label="低" value="low" />
                <el-option label="中" value="medium" />
                <el-option label="高" value="high" />
              </el-select>
            </el-form-item>
            <el-form-item>
              <el-button type="primary" @click="handleSearch"> 搜索 </el-button>
              <el-button @click="handleReset"> 重置 </el-button>
            </el-form-item>
          </el-form>
          <el-button type="primary" @click="showAddDialog"> 添加敏感词 </el-button>
        </div>
      </div>

      <el-alert v-if="wordsError" class="ops-inline-alert" type="error" :closable="false" show-icon>
        <template #title>风险词典加载失败</template>
        <template #default>
          <div class="ops-inline-alert__body">
            <span>{{ wordsError }}</span>
            <el-button type="danger" link @click="fetchWords">重新加载</el-button>
          </div>
        </template>
      </el-alert>

      <div v-if="selectedWords.length > 0" class="batch-bar">
        <span>已选 {{ selectedWords.length }} 项</span>
        <el-popconfirm title="确定批量删除选中的敏感词？" @confirm="handleBatchDelete">
          <template #reference>
            <el-button type="danger" size="small"> 批量删除 </el-button>
          </template>
        </el-popconfirm>
      </div>

      <el-table
        ref="tableRef"
        v-loading="loading"
        :data="wordList"
        stripe
        aria-label="敏感词列表"
        @selection-change="handleSelectionChange"
      >
        <el-table-column type="selection" width="45" :selectable="canSelect" />
        <el-table-column prop="id" label="ID" width="80" />
        <el-table-column prop="word" label="敏感词" min-width="150" />
        <el-table-column label="级别" width="100">
          <template #default="{ row }">
            <el-tag :type="getLevelType(row.level)" size="small">
              {{ getLevelLabel(row.level) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="action" label="处置策略" width="120">
          <template #default="{ row }">
            {{ getActionLabel(row.action) }}
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="添加时间" width="180" />
        <el-table-column label="操作" width="150" fixed="right">
          <template #default="{ row }">
            <el-button type="primary" link @click="showEditDialog(row)"> 编辑 </el-button>
            <el-popconfirm title="确定删除此敏感词？" @confirm="handleDelete(row)">
              <template #reference>
                <el-button type="danger" link> 删除 </el-button>
              </template>
            </el-popconfirm>
          </template>
        </el-table-column>
        <template #empty>
          <el-empty :description="emptyDescription" :image-size="88" />
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

    <!-- 添加/编辑弹窗 -->
    <el-dialog
      v-model="dialogVisible"
      :title="isEdit ? '编辑敏感词' : '添加敏感词'"
      width="450px"
      aria-labelledby="sensitive-word-dialog-title"
    >
      <el-form ref="formRef" :model="form" :rules="rules" label-width="80px">
        <el-form-item label="敏感词" prop="word">
          <el-input v-model="form.word" placeholder="请输入敏感词" maxlength="50" show-word-limit />
        </el-form-item>
        <el-form-item label="级别" prop="level">
          <el-select v-model="form.level" style="width: 100%">
            <el-option label="低 - 仅记录" value="low" />
            <el-option label="中 - 审慎处理" value="medium" />
            <el-option label="高 - 拦截发布" value="high" />
          </el-select>
        </el-form-item>
        <el-form-item label="处置" prop="action">
          <el-select v-model="form.action" style="width: 100%">
            <el-option label="拦截" value="block" />
            <el-option label="放行" value="allow" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false"> 取消 </el-button>
        <el-button
          type="primary"
          :loading="submitting"
          :disabled="!hasFormChanged"
          @click="handleSubmit"
        >
          确定
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import type { FormInstance, TableInstance } from 'element-plus'
import api, { isRequestCanceled } from '@/api'
import OpsDashboardDeck from '@/components/OpsDashboardDeck.vue'

import { normalizeCollectionResponse } from '@/utils/collectionPayload'
import { normalizeAdminPayload, pickBoolean, pickNumber, pickString } from '@/utils/adminPayload'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import {
  createDeckActivityRows,
  createDeckFocusCard,
  createDeckGuideItems,
  createDeckOverviewCards,
  createDeckRhythmItems,
} from '@/utils/opsDashboardDeck'
import type { SensitiveWord } from '@/types'

type SensitiveWordRow = SensitiveWord & {
  action?: string
}

const loading = ref(false)
const submitting = ref(false)
const wordList = ref<SensitiveWordRow[]>([])
const wordsError = ref('')
const dialogVisible = ref(false)
const isEdit = ref(false)
const formRef = ref<FormInstance | null>(null)
const tableRef = ref<TableInstance | null>(null)
const currentId = ref<string | number | null>(null)

// 批量选中，最多100条
const MAX_BATCH_SIZE = 100
const selectedWords = ref<Array<{ id: string | number }>>([])

const handleSelectionChange = (rows: Array<{ id: string | number }>) => {
  if (rows.length > MAX_BATCH_SIZE) {
    ElMessage.warning(`批量操作最多选择 ${MAX_BATCH_SIZE} 条`)
    // 只保留前100条，取消多余的勾选
    const kept = rows.slice(0, MAX_BATCH_SIZE)
    selectedWords.value = kept
    // 通过 tableRef 同步表格勾选状态
    const table = tableRef.value
    if (table) {
      table.clearSelection()
      kept.forEach((row) => table.toggleRowSelection(row, true))
    }
    return
  }
  selectedWords.value = rows
}

// 已达上限时禁止勾选更多行
const canSelect = () => {
  return selectedWords.value.length < MAX_BATCH_SIZE
}

const handleBatchDelete = async () => {
  if (selectedWords.value.length === 0) return
  submitting.value = true
  try {
    await Promise.all(selectedWords.value.map((w) => api.deleteSensitiveWord(w.id)))
    ElMessage.success(`成功删除 ${selectedWords.value.length} 条敏感词`)
    selectedWords.value = []
    fetchWords()
  } catch (e) {
    ElMessage.error(getErrorMessage(e, '批量删除失败'))
  } finally {
    submitting.value = false
  }
}

const filters = reactive({ keyword: '', level: '' })
const {
  pagination,
  buildParams,
  handleSizeChange,
  handleCurrentChange,
  handleSearch,
  handleReset,
} = useTablePagination(fetchWords, {
  filters,
  defaultFilters: { keyword: '', level: '' },
  beforeSearch: () => {
    filters.keyword = filters.keyword.trim()
    if (filters.keyword.length > 50) {
      ElMessage.warning('搜索关键词过长')
      return false
    }
  },
})
const form = reactive({ word: '', level: 'medium', action: 'block' })
// 保存编辑前的原始值，用于变更检测
const originalForm = reactive({ word: '', level: 'medium', action: 'block' })
const hasFormChanged = computed(() => {
  if (!isEdit.value) return true // 新增模式始终允许提交
  return (
    form.word !== originalForm.word ||
    form.level !== originalForm.level ||
    form.action !== originalForm.action
  )
})
const rules = {
  word: [{ required: true, message: '请输入敏感词', trigger: 'blur' }],
  level: [{ required: true, message: '请选择级别', trigger: 'change' }],
  action: [{ required: true, message: '请选择处置策略', trigger: 'change' }],
}

const getLevelType = (level: string) =>
  ({ low: 'info', medium: 'warning', high: 'danger' })[level] || 'info'
const getLevelLabel = (level: string) => ({ low: '低', medium: '中', high: '高' })[level] || level
const getActionLabel = (action?: string) =>
  ({ allow: '放行', block: '拦截' })[action || 'block'] || '拦截'
const formatCount = (value: number) => value.toLocaleString()
const hasStaleWords = computed(() => !!wordsError.value && wordList.value.length > 0)
const emptyDescription = computed(() =>
  wordsError.value ? '风险词典加载失败，请重试' : '当前筛选下暂无敏感词',
)

const summaryItems = computed(() => {
  const highRiskCount = highRiskWordCount.value
  const allowCount = allowStrategyCount.value

  return [
    {
      label: '词条总量',
      value: formatCount(Number(pagination.total || 0)),
      note: '当前筛选条件下的规则总数',
      tone: 'lake' as const,
    },
    {
      label: '高风险词条',
      value: formatCount(highRiskCount),
      note: '当前页高强度拦截或重点观察词条',
      tone: 'rose' as const,
    },
    {
      label: '放行策略',
      value: formatCount(allowCount),
      note: '当前页明确标记为放行的词条',
      tone: 'amber' as const,
    },
    {
      label: '批量选中',
      value: formatCount(selectedWords.value.length),
      note: `单次最多处理 ${MAX_BATCH_SIZE} 条`,
      tone: 'sage' as const,
    },
  ]
})

const highRiskWordCount = computed(
  () => wordList.value.filter((item) => item.level === 'high' || item.level === 'critical').length,
)
const allowStrategyCount = computed(
  () => wordList.value.filter((item) => item.action === 'allow').length,
)
const mediumRiskWordCount = computed(
  () => wordList.value.filter((item) => item.level === 'medium').length,
)

const sensitiveVizBars = computed(() => [
  { label: '高', value: highRiskWordCount.value, display: formatCount(highRiskWordCount.value) },
  {
    label: '中',
    value: mediumRiskWordCount.value,
    display: formatCount(mediumRiskWordCount.value),
  },
  {
    label: '放行',
    value: allowStrategyCount.value,
    display: formatCount(allowStrategyCount.value),
  },
  {
    label: '选中',
    value: selectedWords.value.length,
    display: formatCount(selectedWords.value.length),
  },
])

const sensitiveScore = computed(() => {
  const total = Math.max(wordList.value.length, 1)
  return Math.max(
    30,
    Math.min(
      96,
      Math.round(((highRiskWordCount.value + allowStrategyCount.value) / total) * 60 + 35),
    ),
  )
})

const sensitiveLabel = computed(() => {
  if (sensitiveScore.value >= 82) return '清晰'
  if (sensitiveScore.value >= 60) return '稳定'
  return '松散'
})
const sensitiveHeadingChip = computed(() =>
  wordsError.value
    ? hasStaleWords.value
      ? '显示旧词典'
      : '加载失败'
    : `${sensitiveScore.value} 分 ${sensitiveLabel.value}`,
)
const sensitiveRhythmChip = computed(() =>
  wordsError.value
    ? hasStaleWords.value
      ? '词典未刷新'
      : '等待恢复'
    : `${highRiskWordCount.value} 高风险`,
)
const sensitiveRhythmBadge = computed(() =>
  wordsError.value
    ? hasStaleWords.value
      ? '陈旧数据'
      : '无可用数据'
    : `${formatCount(wordList.value.length)} 条词条`,
)
const sensitiveGuidePulseValue = computed(() =>
  wordsError.value ? (hasStaleWords.value ? '陈旧' : '失败') : `${sensitiveScore.value} 分`,
)

const latestSensitiveMeta = computed(() => {
  if (wordsError.value) {
    return {
      value: hasStaleWords.value ? '显示旧词典' : '加载失败',
      note: hasStaleWords.value
        ? '最近一次词典刷新失败，当前展示的是上次成功获取的结果。'
        : '当前没有可用的词典数据，请先修复加载错误后再维护。',
    }
  }

  const latestItem = wordList.value.reduce<SensitiveWord | null>((latest, item) => {
    if (!latest) return item
    return new Date(item.created_at).getTime() > new Date(latest.created_at).getTime()
      ? item
      : latest
  }, null)

  if (!latestItem) {
    return {
      value: '暂无新词条',
      note: '当前筛选条件下还没有更新的词条。',
    }
  }

  return {
    value: latestItem.word,
    note: `${getLevelLabel(latestItem.level)}级 · ${latestItem.created_at || '暂无时间'}`,
  }
})

const sensitiveSignals = computed(() => {
  const filterMode =
    filters.keyword || filters.level
      ? `${filters.keyword || '全部关键词'} / ${filters.level ? `${getLevelLabel(filters.level)}级` : '全部级别'}`
      : '全量词典'

  return [
    {
      label: '当前视角',
      value: filterMode,
      note: filters.keyword
        ? `正在按“${filters.keyword}”缩小词典范围。`
        : '默认查看全部词条和级别分布。',
      badge: `${formatCount(wordList.value.length)} 条`,
    },
    {
      label: '最新词条',
      value: latestSensitiveMeta.value.value,
      note: latestSensitiveMeta.value.note,
      badge: `高风险 ${formatCount(highRiskWordCount.value)}`,
    },
    {
      label: '批量准备',
      value: `${formatCount(selectedWords.value.length)} 条`,
      note: selectedWords.value.length
        ? `当前已选 ${formatCount(selectedWords.value.length)} 条，可直接执行批量删除。`
        : `单次最多可处理 ${MAX_BATCH_SIZE} 条词条。`,
      badge: `上限 ${MAX_BATCH_SIZE}`,
    },
  ]
})

const sensitiveGuideHeadline = computed(() => {
  if (wordsError.value) {
    return hasStaleWords.value
      ? '词典刷新失败，先确认当前陈旧结果是否还能支撑处置判断'
      : '词典未加载成功，先恢复数据链路再做维护'
  }
  if (selectedWords.value.length > 0) return '当前已经选中一批词条，先确认误删风险再执行批量操作'
  if (highRiskWordCount.value > mediumRiskWordCount.value)
    return '高风险词条偏多，优先回看处置策略和误伤边界'
  return '词典结构相对稳定，继续补充新词和细化中风险表达即可'
})

const sensitiveGuideCopy = computed(() => {
  if (wordsError.value) {
    return hasStaleWords.value
      ? `最近一次刷新失败，页面仍显示 ${formatCount(wordList.value.length)} 条上次成功获取的词条，请勿把它当作最新词典。`
      : '当前没有成功加载到任何词典数据，请先修复接口或登录态，再继续维护敏感词。'
  }
  if (selectedWords.value.length > 0) {
    return `当前已选中 ${formatCount(selectedWords.value.length)} 条词条，建议先复核级别和处置策略，再进行批量删除。`
  }
  if (highRiskWordCount.value > mediumRiskWordCount.value) {
    return `当前页高风险词条 ${formatCount(highRiskWordCount.value)} 条，多于中风险 ${formatCount(mediumRiskWordCount.value)} 条，建议优先确认是否需要更细的处置策略。`
  }
  return '当前词典没有明显堆积，适合继续补充新出现的表达并保持规则整洁。'
})

const sensitiveOverviewDescription = computed(() => {
  if (wordsError.value) {
    return hasStaleWords.value
      ? '词典刷新失败，当前卡片和列表展示的是最近一次成功拉取到的旧结果。'
      : '风险词典暂时无法加载，当前页面没有可供维护的词条数据。'
  }
  if (selectedWords.value.length > 0) {
    return `维护敏感词与风险等级，统一管理处置策略和批量删除，当前已有 ${formatCount(selectedWords.value.length)} 条词条待确认批量操作。`
  }
  return '维护敏感词与风险等级，统一管理处置策略和批量删除，保障识别规则清晰、可维护。'
})

const sensitiveOverviewCards = computed(() =>
  createDeckOverviewCards(summaryItems.value.slice(1, 3)),
)
const sensitiveFocusCard = computed(() =>
  createDeckFocusCard(summaryItems.value[3], `单次最多可处理 ${MAX_BATCH_SIZE} 条词条。`),
)
const sensitiveRhythmItems = computed(() => createDeckRhythmItems(sensitiveVizBars.value))
const sensitiveActivityRows = computed(() => createDeckActivityRows(sensitiveSignals.value))
const sensitiveGuidePulseNote = computed(() =>
  wordsError.value
    ? hasStaleWords.value
      ? '当前展示的是上次成功获取的词典结果。'
      : '当前没有可用的词典数据。'
    : `高风险 ${formatCount(highRiskWordCount.value)} 条 · ${sensitiveLabel.value}`,
)
const sensitiveGuideItems = computed(() =>
  createDeckGuideItems([
    {
      label: '高风险词条',
      value: `${formatCount(highRiskWordCount.value)} 条`,
      note: '高风险密度更高时，优先回看处置策略与误伤边界。',
    },
    {
      label: '放行策略',
      value: `${formatCount(allowStrategyCount.value)} 条`,
      note: '放行词条越少，越容易解释当前规则边界。',
    },
    {
      label: '词典评分',
      value: `${sensitiveScore.value} 分`,
      note: '综合高风险词条、处置策略和批量准备形成当前判断。',
    },
  ]),
)

const normalizeSensitiveLevel = (payload: Record<string, unknown>) => {
  const level = pickString(payload, ['level'])
  if (level) return level

  const numericLevel = pickNumber(payload, ['level'])
  if ((numericLevel ?? 0) >= 3) return 'high'
  if (numericLevel === 2) return 'medium'
  return 'low'
}

const normalizeSensitiveAction = (payload: Record<string, unknown>) => {
  const action = pickString(payload, ['action'])
  if (action) return action
  const isActive = pickBoolean(payload, ['is_active'])
  return isActive === false ? 'allow' : 'block'
}

const normalizeSensitiveWord = (item: unknown): SensitiveWordRow => {
  const payload = normalizeAdminPayload(item)
  const action = normalizeSensitiveAction(payload)

  return {
    id: pickNumber(payload, ['id']) ?? pickString(payload, ['id']) ?? '',
    word: pickString(payload, ['word']) ?? '',
    level: normalizeSensitiveLevel(payload),
    category: pickString(payload, ['category']) ?? 'general',
    action,
    replacement: getActionLabel(action),
    created_at: pickString(payload, ['created_at', 'createdAt']) ?? '',
    updated_at: pickString(payload, ['updated_at', 'updatedAt']) ?? '',
  }
}

async function fetchWords() {
  loading.value = true
  wordsError.value = ''
  try {
    const res = await api.getSensitiveWords(buildParams(filters))
    const { items, total } = normalizeCollectionResponse<unknown>(res.data, ['words'], {
      requireExplicitTotal: true,
    })
    wordList.value = items.map((item) => normalizeSensitiveWord(item))
    pagination.total = total
  } catch (e) {
    if (isRequestCanceled(e)) return
    const message = getErrorMessage(e, '获取敏感词列表失败')
    wordsError.value = message
    ElMessage.error(message)
  } finally {
    loading.value = false
  }
}

const showAddDialog = () => {
  isEdit.value = false
  currentId.value = null
  Object.assign(form, { word: '', level: 'medium', action: 'block' })
  Object.assign(originalForm, { word: '', level: 'medium', action: 'block' })
  dialogVisible.value = true
}

const showEditDialog = (row: SensitiveWordRow) => {
  isEdit.value = true
  currentId.value = row.id
  const values = { word: row.word, level: row.level, action: row.action || 'block' }
  Object.assign(form, values)
  Object.assign(originalForm, values) // 记录原始值用于变更检测
  dialogVisible.value = true
}

const handleSubmit = async () => {
  const formInstance = formRef.value
  if (!formInstance) return
  const valid = await formInstance.validate().then(
    () => true,
    () => false,
  )
  if (!valid) return
  submitting.value = true
  try {
    const payload = { word: form.word, level: form.level, action: form.action }
    if (isEdit.value) {
      await api.updateSensitiveWord(currentId.value, payload)
      ElMessage.success('更新成功')
    } else {
      await api.addSensitiveWord(payload)
      ElMessage.success('添加成功')
    }
    dialogVisible.value = false
    fetchWords()
  } catch (e) {
    ElMessage.error(getErrorMessage(e, '操作失败'))
  } finally {
    submitting.value = false
  }
}

const handleDelete = async (row: { id: number }) => {
  try {
    await api.deleteSensitiveWord(row.id)
    ElMessage.success('删除成功')
    fetchWords()
  } catch (e) {
    ElMessage.error(getErrorMessage(e, '删除失败'))
  }
}

onMounted(() => fetchWords())
</script>

<style lang="scss" scoped>
.sensitive-words-page {
  .sensitive-filter-form {
    :deep(.el-form-item) {
      margin-bottom: 0;
    }
  }

  .sensitive-table-toolbar {
    display: grid;
    grid-template-columns: minmax(0, 0.82fr) minmax(0, 1.18fr);
    align-items: flex-end;
  }

  :deep(.sensitive-table-toolbar .ops-soft-actions) {
    width: 100%;
    display: grid;
    grid-template-columns: minmax(0, 1fr) auto;
    align-items: end;
  }

  .sensitive-inline-filter {
    width: 100%;
    justify-content: flex-end;
  }

  .sensitive-table-copy {
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
    .sensitive-table-toolbar {
      grid-template-columns: 1fr;
    }

    :deep(.sensitive-table-toolbar .ops-soft-actions) {
      grid-template-columns: 1fr;
    }
  }

  .batch-bar {
    display: flex;
    align-items: center;
    gap: 12px;
    margin-bottom: 12px;
    padding: 10px 14px;
    border: 1px solid rgba(182, 122, 66, 0.14);
    background: rgba(182, 122, 66, 0.08);
    border-radius: 16px;
    font-size: 13px;
    color: var(--hl-ink-soft);
  }

  .ops-inline-alert {
    margin-bottom: 14px;
  }

  .ops-inline-alert__body {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
  }

  .pagination-wrapper {
    display: flex;
    justify-content: flex-end;
  }
}
</style>
