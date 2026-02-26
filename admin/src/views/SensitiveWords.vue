<!--
  @file SensitiveWords.vue
  @brief 敏感词管理页面
-->

<template>
  <div class="sensitive-words-page">
    <!-- 操作栏 -->
    <el-card
      shadow="never"
      class="filter-card"
    >
      <div class="flex-between">
        <el-form
          :model="filters"
          inline
          aria-label="敏感词筛选"
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
              style="width: 120px"
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
        <el-button
          type="primary"
          @click="showAddDialog"
        >
          添加敏感词
        </el-button>
      </div>
    </el-card>

    <!-- 敏感词列表 -->
    <el-card shadow="never">
      <el-table
        v-loading="loading"
        :data="wordList"
        stripe
        aria-label="敏感词列表"
      >
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
          @click="handleSubmit"
        >
          确定
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import api from '@/api'

import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'

const loading = ref(false)
const submitting = ref(false)
const wordList = ref([])
const dialogVisible = ref(false)
const isEdit = ref(false)
const formRef = ref(null)
const currentId = ref(null)

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
const rules = {
  word: [{ required: true, message: '请输入敏感词', trigger: 'blur' }],
  level: [{ required: true, message: '请选择级别', trigger: 'change' }],
}

const getLevelType = (level: string) => ({ low: 'info', medium: 'warning', high: 'danger' }[level] || 'info')
const getLevelLabel = (level: string) => ({ low: '低', medium: '中', high: '高' }[level] || level)

async function fetchWords() {
  loading.value = true
  try {
    const res = await api.getSensitiveWords(buildParams(filters))
    wordList.value = res.data?.words || res.data?.list || []
    pagination.total = res.data?.total || 0
  } catch (e) {
    // M-7: 空 catch 块添加 console.error
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
  dialogVisible.value = true
}

const showEditDialog = (row: { id: number; word: string; level: string; replacement?: string }) => {
  isEdit.value = true
  currentId.value = row.id
  Object.assign(form, { word: row.word, level: row.level, replacement: row.replacement || '' })
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
    // M-7: 空 catch 块添加 console.error
    console.error('删除敏感词失败:', e)
    ElMessage.error(getErrorMessage(e, '删除失败'))
  }
}

onMounted(() => fetchWords())
</script>

<style lang="scss" scoped>
.sensitive-words-page {
  padding: 20px;

  .filter-card {
    margin-bottom: 16px;
  }

  .flex-between {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .pagination-wrapper {
    margin-top: 20px;
    display: flex;
    justify-content: flex-end;
  }

  // ── 光遇主题：毛玻璃卡片 ──
  :deep(.el-card) {
    background: rgba(26, 26, 62, 0.6);
    backdrop-filter: blur(16px);
    border: 1px solid rgba(242, 204, 143, 0.12);
    border-radius: 16px;
    color: #F0E6D3;

    .el-card__body {
      color: #F0E6D3;
    }
  }

  // ── 表单标签 ──
  :deep(.el-form-item__label) {
    color: #B8A99A;
  }

  // ── 输入框 ──
  :deep(.el-input) {
    .el-input__wrapper {
      background: rgba(20, 20, 50, 0.6);
      border: 1px solid rgba(242, 204, 143, 0.15);
      border-radius: 8px;
      box-shadow: none;

      .el-input__inner {
        color: #F0E6D3;

        &::placeholder {
          color: #7A6F63;
        }
      }

      &:hover {
        border-color: rgba(242, 204, 143, 0.3);
      }

      &.is-focus {
        border-color: #F2CC8F;
        box-shadow: 0 0 8px rgba(242, 204, 143, 0.15);
      }
    }
  }

  // ── 下拉选择器 ──
  :deep(.el-select) {
    .el-select__wrapper {
      background: rgba(20, 20, 50, 0.6);
      border: 1px solid rgba(242, 204, 143, 0.15);
      border-radius: 8px;
      box-shadow: none;
      color: #F0E6D3;

      &:hover {
        border-color: rgba(242, 204, 143, 0.3);
      }

      &.is-focused {
        border-color: #F2CC8F;
        box-shadow: 0 0 8px rgba(242, 204, 143, 0.15);
      }

      .el-select__selected-item {
        color: #F0E6D3;
      }

      .el-select__placeholder {
        color: #7A6F63;
      }

      .el-select__suffix {
        color: #B8A99A;
      }
    }
  }

  :deep(.el-select__popper) {
    background: rgba(26, 26, 62, 0.95);
    backdrop-filter: blur(16px);
    border: 1px solid rgba(242, 204, 143, 0.12);
    border-radius: 12px;

    .el-select-dropdown__item {
      color: #B8A99A;

      &.is-hovering {
        background: rgba(242, 204, 143, 0.08);
        color: #F2CC8F;
      }

      &.is-selected {
        color: #F2CC8F;
        font-weight: 600;
      }
    }
  }

  // ── 按钮 ──
  :deep(.el-button--primary:not(.is-link)) {
    background: linear-gradient(135deg, #F2CC8F, #E8A87C);
    border: none;
    color: #141432;
    font-weight: 600;
    border-radius: 8px;

    &:hover {
      background: linear-gradient(135deg, #E8A87C, #E07A5F);
      box-shadow: 0 4px 12px rgba(242, 204, 143, 0.3);
    }
  }

  :deep(.el-button:not(.el-button--primary):not(.el-button--success):not(.el-button--danger):not(.el-button--warning):not(.el-button--info):not(.is-link)) {
    background: rgba(20, 20, 50, 0.6);
    border: 1px solid rgba(242, 204, 143, 0.2);
    color: #F0E6D3;
    border-radius: 8px;

    &:hover {
      border-color: #F2CC8F;
      color: #F2CC8F;
    }
  }

  // ── 链接按钮 ──
  :deep(.el-button.is-link) {
    &.el-button--primary {
      color: #F2CC8F;

      &:hover {
        color: #E8A87C;
      }
    }

    &.el-button--danger {
      color: #E07A5F;

      &:hover {
        color: #d4654a;
      }
    }
  }

  // ── 表格 ──
  :deep(.el-table) {
    background: transparent;
    color: #F0E6D3;
    --el-table-border-color: rgba(242, 204, 143, 0.08);
    --el-table-header-bg-color: rgba(20, 20, 50, 0.5);
    --el-table-header-text-color: #B8A99A;
    --el-table-row-hover-bg-color: rgba(242, 204, 143, 0.06);
    --el-table-bg-color: transparent;
    --el-table-tr-bg-color: transparent;
    --el-table-text-color: #F0E6D3;

    .el-table__header-wrapper th {
      background: rgba(20, 20, 50, 0.5);
      color: #B8A99A;
      font-weight: 600;
      border-bottom: 1px solid rgba(242, 204, 143, 0.1);
    }

    .el-table__body-wrapper {
      tr {
        background: transparent;

        td {
          border-bottom: 1px solid rgba(242, 204, 143, 0.05);
        }
      }

      .el-table__row--striped td {
        background: rgba(20, 20, 50, 0.3);
      }
    }

    .el-table__empty-block {
      background: transparent;
      color: #7A6F63;
    }
  }

  // ── 标签 ──
  :deep(.el-tag) {
    border: none;
    border-radius: 6px;

    &.el-tag--info {
      background: rgba(184, 169, 154, 0.15);
      color: #B8A99A;
    }

    &.el-tag--danger {
      background: rgba(224, 122, 95, 0.15);
      color: #E07A5F;
    }

    &.el-tag--success {
      background: rgba(129, 178, 154, 0.15);
      color: #81B29A;
    }

    &.el-tag--warning {
      background: rgba(232, 168, 124, 0.15);
      color: #E8A87C;
    }

    &.el-tag--primary {
      background: rgba(123, 104, 174, 0.15);
      color: #7B68AE;
    }
  }

  // ── 分页 ──
  :deep(.el-pagination) {
    --el-pagination-bg-color: transparent;
    --el-pagination-text-color: #B8A99A;
    --el-pagination-button-disabled-bg-color: transparent;

    .el-pager li {
      background: transparent;
      color: #B8A99A;
      border-radius: 6px;

      &.is-active {
        background: linear-gradient(135deg, #F2CC8F, #E8A87C);
        color: #141432;
        font-weight: 600;
      }

      &:hover:not(.is-active) {
        color: #F2CC8F;
      }
    }

    .btn-prev,
    .btn-next {
      background: transparent;
      color: #B8A99A;

      &:hover {
        color: #F2CC8F;
      }
    }

    .el-pagination__total {
      color: #7A6F63;
    }

    .el-pagination__sizes .el-select {
      .el-select__wrapper {
        background: rgba(20, 20, 50, 0.6);
        border: 1px solid rgba(242, 204, 143, 0.15);
      }
    }
  }

  // ── 弹窗（添加/编辑敏感词） ──
  :deep(.el-dialog) {
    background: rgba(26, 26, 62, 0.95);
    backdrop-filter: blur(20px);
    border: 1px solid rgba(242, 204, 143, 0.12);
    border-radius: 16px;

    .el-dialog__header {
      color: #F0E6D3;

      .el-dialog__title {
        color: #F0E6D3;
      }

      .el-dialog__headerbtn .el-dialog__close {
        color: #B8A99A;

        &:hover {
          color: #F2CC8F;
        }
      }
    }

    .el-dialog__body {
      color: #F0E6D3;
    }

    .el-dialog__footer {
      border-top: 1px solid rgba(242, 204, 143, 0.08);
    }
  }

  // ── 气泡确认框 ──
  :deep(.el-popconfirm) {
    background: rgba(26, 26, 62, 0.95);
    border: 1px solid rgba(242, 204, 143, 0.12);
  }

  // ── Loading 遮罩 ──
  :deep(.el-loading-mask) {
    background: rgba(20, 20, 50, 0.7);
    backdrop-filter: blur(4px);
  }
}
</style>
