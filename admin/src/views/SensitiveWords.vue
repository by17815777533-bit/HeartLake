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
    <OpsWorkbench>
      <template #stage>
        <OpsSurfaceCard
          eyebrow="Lexicon"
          title="风险词典"
          :chip="summaryItems[1]?.value ? `${summaryItems[1].value} 高风险` : '实时生效'"
          tone="sky"
        >
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

          <div class="ops-soft-actions sensitive-stage-actions">
            <el-button
              type="primary"
              @click="showAddDialog"
            >
              添加敏感词
            </el-button>
            <el-button @click="handleSearch">
              刷新词典
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
          eyebrow="Filter"
          title="检索条件"
          :chip="filters.level ? `${getLevelLabel(filters.level)}级` : '全部级别'"
          tone="ice"
          compact
        >
          <el-form
            :model="filters"
            aria-label="敏感词筛选"
            class="ops-form-grid sensitive-filter-form"
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
              <el-select
                v-model="filters.level"
                placeholder="全部"
                clearable
              >
                <el-option
                  label="低"
                  value="low"
                />
                <el-option
                  label="中"
                  value="medium"
                />
                <el-option
                  label="高"
                  value="high"
                />
              </el-select>
            </el-form-item>
          </el-form>
          <div class="ops-chip-row">
            <span class="ops-chip">
              {{ filters.keyword ? `关键词 ${filters.keyword}` : '未指定关键词' }}
            </span>
          </div>
        </OpsSurfaceCard>
      </template>

      <template #rail>
        <OpsSurfaceCard
          eyebrow="Batch"
          title="批量状态"
          :chip="selectedWords.length ? `${selectedWords.length} 已选` : '等待选择'"
          tone="mint"
        >
          <div class="ops-kv-grid">
            <article class="ops-kv-item">
              <span>当前选中</span>
              <strong>{{ selectedWords.length }}</strong>
            </article>
            <article class="ops-kv-item">
              <span>替换策略</span>
              <strong>{{ summaryItems[2]?.value || 0 }}</strong>
            </article>
            <article class="ops-kv-item">
              <span>批量上限</span>
              <strong>{{ MAX_BATCH_SIZE }}</strong>
            </article>
          </div>
        </OpsSurfaceCard>
      </template>

      <template #footer>
        <OpsSurfaceCard
          eyebrow="Rules"
          title="策略提示"
          :chip="filters.level ? `${getLevelLabel(filters.level)}级筛查` : '全量词典'"
          tone="plain"
          compact
        >
          <div class="ops-kv-grid">
            <article class="ops-kv-item">
              <span>高风险词条</span>
              <strong>{{ summaryItems[1]?.value || 0 }}</strong>
            </article>
            <article class="ops-kv-item">
              <span>替换策略</span>
              <strong>{{ summaryItems[2]?.value || 0 }}</strong>
            </article>
            <article class="ops-kv-item">
              <span>批量上限</span>
              <strong>{{ MAX_BATCH_SIZE }}</strong>
            </article>
          </div>
        </OpsSurfaceCard>
      </template>

      <el-card
        shadow="never"
        class="table-card ops-table-card"
      >
        <div class="ops-soft-toolbar">
          <div class="sensitive-table-copy">
            <h3>词典列表</h3>
            <p>词条、级别、替换词和批量操作都集中在这里，便于快速维护风控边界。</p>
          </div>
        </div>

        <div
          v-if="selectedWords.length > 0"
          class="batch-bar"
        >
          <span>已选 {{ selectedWords.length }} 项</span>
          <el-popconfirm
            title="确定批量删除选中的敏感词？"
            @confirm="handleBatchDelete"
          >
            <template #reference>
              <el-button
                type="danger"
                size="small"
              >
                批量删除
              </el-button>
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
          <el-table-column
            type="selection"
            width="45"
            :selectable="canSelect"
          />
          <el-table-column
            prop="id"
            label="ID"
            width="80"
          />
          <el-table-column
            prop="word"
            label="敏感词"
            min-width="150"
          />
          <el-table-column
            label="级别"
            width="100"
          >
            <template #default="{ row }">
              <el-tag
                :type="getLevelType(row.level)"
                size="small"
              >
                {{ getLevelLabel(row.level) }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column
            prop="replacement"
            label="替换词"
            width="120"
          >
            <template #default="{ row }">
              {{ row.replacement || '***' }}
            </template>
          </el-table-column>
          <el-table-column
            prop="created_at"
            label="添加时间"
            width="180"
          />
          <el-table-column
            label="操作"
            width="150"
            fixed="right"
          >
            <template #default="{ row }">
              <el-button
                type="primary"
                link
                @click="showEditDialog(row)"
              >
                编辑
              </el-button>
              <el-popconfirm
                title="确定删除此敏感词？"
                @confirm="handleDelete(row)"
              >
                <template #reference>
                  <el-button
                    type="danger"
                    link
                  >
                    删除
                  </el-button>
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
      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-width="80px"
      >
        <el-form-item
          label="敏感词"
          prop="word"
        >
          <el-input
            v-model="form.word"
            placeholder="请输入敏感词"
            maxlength="50"
            show-word-limit
          />
        </el-form-item>
        <el-form-item
          label="级别"
          prop="level"
        >
          <el-select
            v-model="form.level"
            style="width: 100%"
          >
            <el-option
              label="低 - 仅记录"
              value="low"
            />
            <el-option
              label="中 - 替换显示"
              value="medium"
            />
            <el-option
              label="高 - 拦截发布"
              value="high"
            />
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
        <el-button @click="dialogVisible = false">
          取消
        </el-button>
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
import OpsWorkbench from '@/components/OpsWorkbench.vue'
import OpsSurfaceCard from '@/components/OpsSurfaceCard.vue'

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
      kept.forEach(row => table.toggleRowSelection(row, true))
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
    await Promise.all(selectedWords.value.map(w => api.deleteSensitiveWord(w.id)))
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
const { pagination, buildParams, handleSizeChange, handleCurrentChange, handleSearch, handleReset } = useTablePagination(fetchWords, {
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
  return form.word !== originalForm.word
    || form.level !== originalForm.level
    || form.replacement !== originalForm.replacement
})
const rules = {
  word: [{ required: true, message: '请输入敏感词', trigger: 'blur' }],
  level: [{ required: true, message: '请选择级别', trigger: 'change' }],
}

const getLevelType = (level: string) => ({ low: 'info', medium: 'warning', high: 'danger' }[level] || 'info')
const getLevelLabel = (level: string) => ({ low: '低', medium: '中', high: '高' }[level] || level)
const formatCount = (value: number) => value.toLocaleString()

const summaryItems = computed(() => {
  const highRiskCount = wordList.value.filter((item) => item.level === 'high' || item.level === 'critical').length
  const replacementCount = wordList.value.filter((item) => Boolean(item.replacement)).length

  return [
    { label: '词条总量', value: formatCount(Number(pagination.total || 0)), note: '当前筛选条件下的规则总数', tone: 'lake' as const },
    { label: '高风险词条', value: formatCount(highRiskCount), note: '当前页高强度拦截或重点观察词条', tone: 'rose' as const },
    { label: '替换策略', value: formatCount(replacementCount), note: '当前页设置了替换文案的词条', tone: 'amber' as const },
    { label: '批量选中', value: formatCount(selectedWords.value.length), note: `单次最多处理 ${MAX_BATCH_SIZE} 条`, tone: 'sage' as const },
  ]
})

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
  .sensitive-stage-actions {
    margin: 22px 0 18px;
  }

  .sensitive-filter-form {
    :deep(.el-form-item) {
      margin-bottom: 0;
    }
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
