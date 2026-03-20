<!--
  风险词典（敏感词管理）页面

  功能：
  - 敏感词列表展示，支持按关键词搜索和分页
  - 新增敏感词：填写词条、风险等级（low/medium/high/critical）、替换词
  - 编辑敏感词：弹窗修改词条属性
  - 删除敏感词：二次确认后删除
  - 风险等级以不同颜色 tag 区分，便于快速识别
-->

<template>
  <div class="sensitive-words-page ops-page">
    <OpsPageHero
      eyebrow="词典"
      title="风险词典"
      :description="sensitiveHeroDescription"
      :status="sensitiveLabel"
      :chips="sensitiveHeroChips"
    >
      <template #actions>
        <el-button type="primary" @click="showAddDialog"> 添加敏感词 </el-button>
        <el-button @click="handleSearch"> 刷新词典 </el-button>
      </template>
    </OpsPageHero>

    <OpsWorkbench>
      <template #stage>
        <OpsSurfaceCard
          eyebrow="总览"
          title="词典概览"
          :chip="`${sensitiveScore} 分 ${sensitiveLabel}`"
          tone="sky"
        >
          <div class="ops-stage-shell">
            <div class="ops-big-metric">
              <span class="ops-big-metric__label">词条总量</span>
              <div class="ops-big-metric__value">
                {{ summaryItems[0]?.value || 0 }}
                <small>条</small>
              </div>
              <p class="ops-big-metric__note">
                维护敏感词与风险等级，统一管理替换策略和批量删除，保障识别规则清晰、可维护。
              </p>
            </div>

            <div class="ops-stage-aside">
              <article class="ops-stage-pod">
                <span>最新词条</span>
                <strong>{{ latestSensitiveMeta.value }}</strong>
                <small>{{ latestSensitiveMeta.note }}</small>
              </article>

              <article class="ops-stage-pod ops-stage-pod--mint">
                <span>批量准备</span>
                <strong>{{ sensitiveSignals[2]?.value || '0 条' }}</strong>
                <small>{{ sensitiveSignals[2]?.note }}</small>
              </article>
            </div>
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
          eyebrow="级别"
          title="级别分布"
          :chip="`${highRiskWordCount} 高风险`"
          tone="ice"
          compact
        >
          <OpsMiniBars :items="sensitiveVizBars" />
        </OpsSurfaceCard>
      </template>

      <template #footer>
        <OpsSurfaceCard eyebrow="建议" title="维护建议" :chip="sensitiveLabel" tone="mint">
          <div class="ops-guidance">
            <div class="ops-guidance__headline">
              <strong>{{ sensitiveGuideHeadline }}</strong>
              <span>{{ sensitiveGuideCopy }}</span>
            </div>

            <div class="ops-guidance__meta">
              <article
                v-for="item in sensitiveGuideMetrics"
                :key="item.label"
                class="ops-guidance__metric"
              >
                <span>{{ item.label }}</span>
                <strong>{{ item.value }}</strong>
              </article>
            </div>
          </div>
        </OpsSurfaceCard>
      </template>

      <template #rail>
        <OpsSurfaceCard
          eyebrow="动态"
          title="词典动态"
          :chip="latestSensitiveMeta.value"
          tone="mint"
        >
          <div class="ops-list-stack">
            <article v-for="item in sensitiveSignals" :key="item.label" class="ops-list-row">
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
        <div class="ops-soft-toolbar sensitive-table-toolbar">
          <div class="sensitive-table-copy">
            <h3>词典列表</h3>
            <p>词条、级别、替换词和批量操作都集中在这里，便于快速维护风控边界。</p>
          </div>
          <div class="ops-soft-actions">
            <el-form
              :model="filters"
              inline
              aria-label="敏感词筛选"
              class="sensitive-inline-filter"
            >
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
          <el-table-column prop="replacement" label="替换词" width="120">
            <template #default="{ row }">
              {{ row.replacement || '***' }}
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
    </OpsWorkbench>

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
            <el-option label="中 - 替换显示" value="medium" />
            <el-option label="高 - 拦截发布" value="high" />
          </el-select>
        </el-form-item>
        <el-form-item label="替换词">
          <el-input
            v-model="form.replacement"
            placeholder="默认为 ***"
            maxlength="50"
            show-word-limit
          />
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
import OpsPageHero from '@/components/OpsPageHero.vue'
import OpsWorkbench from '@/components/OpsWorkbench.vue'
import OpsSurfaceCard from '@/components/OpsSurfaceCard.vue'
import OpsMiniBars from '@/components/OpsMiniBars.vue'

import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import { getWorkbenchTileTone } from '@/utils/workbenchTone'
import type { SensitiveWord } from '@/types'

const loading = ref(false)
const submitting = ref(false)
const wordList = ref<SensitiveWord[]>([])
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
const form = reactive({ word: '', level: 'medium', replacement: '' })
// 保存编辑前的原始值，用于变更检测
const originalForm = reactive({ word: '', level: 'medium', replacement: '' })
const hasFormChanged = computed(() => {
  if (!isEdit.value) return true // 新增模式始终允许提交
  return (
    form.word !== originalForm.word ||
    form.level !== originalForm.level ||
    form.replacement !== originalForm.replacement
  )
})
const rules = {
  word: [{ required: true, message: '请输入敏感词', trigger: 'blur' }],
  level: [{ required: true, message: '请选择级别', trigger: 'change' }],
}

const getLevelType = (level: string) =>
  ({ low: 'info', medium: 'warning', high: 'danger' })[level] || 'info'
const getLevelLabel = (level: string) => ({ low: '低', medium: '中', high: '高' })[level] || level
const formatCount = (value: number) => value.toLocaleString()

const summaryItems = computed(() => {
  const highRiskCount = highRiskWordCount.value
  const replacementCount = replacementWordCount.value

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
      label: '替换策略',
      value: formatCount(replacementCount),
      note: '当前页设置了替换文案的词条',
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
const replacementWordCount = computed(
  () => wordList.value.filter((item) => Boolean(item.replacement)).length,
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
    label: '替换',
    value: replacementWordCount.value,
    display: formatCount(replacementWordCount.value),
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
      Math.round(((highRiskWordCount.value + replacementWordCount.value) / total) * 60 + 35),
    ),
  )
})

const sensitiveLabel = computed(() => {
  if (sensitiveScore.value >= 82) return '清晰'
  if (sensitiveScore.value >= 60) return '稳定'
  return '松散'
})

const latestSensitiveMeta = computed(() => {
  const latestItem = wordList.value.reduce<SensitiveWord | null>((latest, item) => {
    if (!latest) return item
    return new Date(item.created_at).getTime() > new Date(latest.created_at).getTime() ? item : latest
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
      note: filters.keyword ? `正在按“${filters.keyword}”缩小词典范围。` : '默认查看全部词条和级别分布。',
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

const sensitiveHeroDescription =
  '把风险词、替换策略和批量操作集中到同一张维护台面里，先判断级别密度，再决定是新增、调整，还是批量清理。'

const sensitiveHeroChips = computed(() => [
  `${summaryItems.value[0]?.value || 0} 条词条`,
  `${highRiskWordCount.value} 条高风险`,
  `${sensitiveScore.value} 分 ${sensitiveLabel.value}`,
])

const sensitiveGuideHeadline = computed(() => {
  if (selectedWords.value.length > 0) return '当前已经选中一批词条，先确认误删风险再执行批量操作'
  if (highRiskWordCount.value > mediumRiskWordCount.value) return '高风险词条偏多，优先回看替换策略和误伤边界'
  return '词典结构相对稳定，继续补充新词和细化中风险表达即可'
})

const sensitiveGuideCopy = computed(() => {
  if (selectedWords.value.length > 0) {
    return `当前已选中 ${formatCount(selectedWords.value.length)} 条词条，建议先复核级别和替换词，再进行批量删除。`
  }
  if (highRiskWordCount.value > mediumRiskWordCount.value) {
    return `当前页高风险词条 ${formatCount(highRiskWordCount.value)} 条，多于中风险 ${formatCount(mediumRiskWordCount.value)} 条，建议优先确认是否需要更细的替换策略。`
  }
  return '当前词典没有明显堆积，适合继续补充新出现的表达并保持规则整洁。'
})

const sensitiveGuideMetrics = computed(() => [
  { label: '高风险词条', value: `${formatCount(highRiskWordCount.value)} 条` },
  { label: '替换策略', value: `${formatCount(replacementWordCount.value)} 条` },
  { label: '词典评分', value: `${sensitiveScore.value} 分` },
])

async function fetchWords() {
  loading.value = true
  try {
    const res = await api.getSensitiveWords(buildParams(filters))
    const data = res.data?.data || res.data || {}
    wordList.value = data.words || data.list || []
    pagination.total = data.total || 0
  } catch (e) {
    if (isRequestCanceled(e)) return
    console.error('获取敏感词列表失败:', e)
    ElMessage.error(getErrorMessage(e, '获取敏感词列表失败'))
    wordList.value = []
    pagination.total = 0
  } finally {
    loading.value = false
  }
}

const showAddDialog = () => {
  isEdit.value = false
  currentId.value = null
  Object.assign(form, { word: '', level: 'medium', replacement: '' })
  Object.assign(originalForm, { word: '', level: 'medium', replacement: '' })
  dialogVisible.value = true
}

const showEditDialog = (row: { id: number; word: string; level: string; replacement?: string }) => {
  isEdit.value = true
  currentId.value = row.id
  const values = { word: row.word, level: row.level, replacement: row.replacement || '' }
  Object.assign(form, values)
  Object.assign(originalForm, values) // 记录原始值用于变更检测
  dialogVisible.value = true
}

const handleSubmit = async () => {
  const valid = await formRef.value?.validate().catch(() => false)
  if (!valid) return
  submitting.value = true
  try {
    if (isEdit.value) {
      await api.updateSensitiveWord(currentId.value, form)
      ElMessage.success('更新成功')
    } else {
      await api.addSensitiveWord(form)
      ElMessage.success('添加成功')
    }
    dialogVisible.value = false
    fetchWords()
  } catch (e) {
    console.error('敏感词操作失败:', e)
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
    console.error('删除敏感词失败:', e)
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
    align-items: flex-start;
  }

  .sensitive-inline-filter {
    justify-content: flex-end;
  }

  .sensitive-table-copy {
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

  .pagination-wrapper {
    display: flex;
    justify-content: flex-end;
  }
}
</style>
