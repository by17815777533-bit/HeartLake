<!--
  @file SensitiveWords.vue
  @brief 敏感词管理页面
-->

<template>
  <div class="sensitive-words-page">
    <!-- 操作栏 -->
    <el-card shadow="never" class="filter-card">
      <div class="flex-between">
        <el-form :model="filters" inline>
          <el-form-item label="关键词">
            <el-input v-model="filters.keyword" placeholder="搜索敏感词" clearable @keyup.enter="handleSearch" />
          </el-form-item>
          <el-form-item label="级别">
            <el-select v-model="filters.level" placeholder="全部" clearable style="width: 120px">
              <el-option label="低" value="low" />
              <el-option label="中" value="medium" />
              <el-option label="高" value="high" />
            </el-select>
          </el-form-item>
          <el-form-item>
            <el-button type="primary" @click="handleSearch">搜索</el-button>
            <el-button @click="handleReset">重置</el-button>
          </el-form-item>
        </el-form>
        <el-button type="primary" @click="showAddDialog">添加敏感词</el-button>
      </div>
    </el-card>

    <!-- 敏感词列表 -->
    <el-card shadow="never">
      <el-table v-loading="loading" :data="wordList" stripe>
        <el-table-column prop="id" label="ID" width="80" />
        <el-table-column prop="word" label="敏感词" min-width="150" />
        <el-table-column label="级别" width="100">
          <template #default="{ row }">
            <el-tag :type="getLevelType(row.level)" size="small">{{ getLevelLabel(row.level) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="replacement" label="替换词" width="120">
          <template #default="{ row }">{{ row.replacement || '***' }}</template>
        </el-table-column>
        <el-table-column prop="created_at" label="添加时间" width="180" />
        <el-table-column label="操作" width="150" fixed="right">
          <template #default="{ row }">
            <el-button type="primary" link @click="showEditDialog(row)">编辑</el-button>
            <el-popconfirm title="确定删除此敏感词？" @confirm="handleDelete(row)">
              <template #reference>
                <el-button type="danger" link>删除</el-button>
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
          @current-change="fetchWords"
        />
      </div>
    </el-card>

    <!-- 添加/编辑弹窗 -->
    <el-dialog v-model="dialogVisible" :title="isEdit ? '编辑敏感词' : '添加敏感词'" width="450px">
      <el-form ref="formRef" :model="form" :rules="rules" label-width="80px">
        <el-form-item label="敏感词" prop="word">
          <el-input v-model="form.word" placeholder="请输入敏感词" />
        </el-form-item>
        <el-form-item label="级别" prop="level">
          <el-select v-model="form.level" style="width: 100%">
            <el-option label="低 - 仅记录" value="low" />
            <el-option label="中 - 替换显示" value="medium" />
            <el-option label="高 - 拦截发布" value="high" />
          </el-select>
        </el-form-item>
        <el-form-item label="替换词">
          <el-input v-model="form.replacement" placeholder="默认为 ***" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleSubmit" :loading="submitting">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import api from '@/api'

import { getErrorMessage } from '@/utils/errorHelper'

const loading = ref(false)
const submitting = ref(false)
const wordList = ref([])
const dialogVisible = ref(false)
const isEdit = ref(false)
const formRef = ref(null)
const currentId = ref(null)

const filters = reactive({ keyword: '', level: '' })
const pagination = reactive({ page: 1, pageSize: 20, total: 0 })
const form = reactive({ word: '', level: 'medium', replacement: '' })
const rules = {
  word: [{ required: true, message: '请输入敏感词', trigger: 'blur' }],
  level: [{ required: true, message: '请选择级别', trigger: 'change' }],
}

const getLevelType = (level) => ({ low: 'info', medium: 'warning', high: 'danger' }[level] || 'info')
const getLevelLabel = (level) => ({ low: '低', medium: '中', high: '高' }[level] || level)

const fetchWords = async () => {
  loading.value = true
  try {
    const res = await api.getSensitiveWords({ page: pagination.page, page_size: pagination.pageSize, ...filters })
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

const handleSizeChange = () => { pagination.page = 1; fetchWords() }
const handleSearch = () => {
  filters.keyword = filters.keyword.trim()
  if (filters.keyword.length > 50) {
    ElMessage.warning('搜索关键词过长')
    return
  }
  pagination.page = 1
  fetchWords()
}
const handleReset = () => { Object.assign(filters, { keyword: '', level: '' }); pagination.page = 1; fetchWords() }

const showAddDialog = () => {
  isEdit.value = false
  currentId.value = null
  Object.assign(form, { word: '', level: 'medium', replacement: '' })
  dialogVisible.value = true
}

const showEditDialog = (row) => {
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

const handleDelete = async (row) => {
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

  // 卡片毛玻璃
  :deep(.el-card) {
    background: rgba(26, 26, 62, 0.6);
    backdrop-filter: blur(16px);
    border: 1px solid rgba(242, 204, 143, 0.08);
    border-radius: 16px;
  }

  // 表单标签
  :deep(.el-form-item__label) {
    color: #B8A99A;
  }

  // 输入框 & 选择器
  :deep(.el-input__wrapper),
  :deep(.el-select__wrapper) {
    background: rgba(20, 20, 50, 0.6);
    border: 1px solid rgba(242, 204, 143, 0.12);
    border-radius: 8px;
    box-shadow: none;

    &:hover,
    &.is-focus {
      border-color: rgba(242, 204, 143, 0.3);
      box-shadow: 0 0 0 1px rgba(242, 204, 143, 0.1);
    }
  }

  :deep(.el-input__inner) {
    color: #F0E6D3;

    &::placeholder {
      color: #7A6F63;
    }
  }

  // 主按钮金色渐变
  :deep(.el-button--primary) {
    background: linear-gradient(135deg, #F2CC8F, #E8A87C);
    border: none;
    color: #1a1a3e;
    font-weight: 600;
    border-radius: 8px;

    &:hover {
      background: linear-gradient(135deg, #f5d9a8, #edb78f);
    }
  }

  // 默认按钮
  :deep(.el-button--default) {
    background: rgba(242, 204, 143, 0.08);
    border: 1px solid rgba(242, 204, 143, 0.15);
    color: #B8A99A;
    border-radius: 8px;

    &:hover {
      border-color: rgba(242, 204, 143, 0.3);
      color: #F2CC8F;
    }
  }

  // Link 按钮
  :deep(.el-button.is-link) {
    &.el-button--primary {
      color: #F2CC8F;

      &:hover {
        color: #f5d9a8;
      }
    }

    &.el-button--danger {
      color: #E07A5F;

      &:hover {
        color: #e99a85;
      }
    }
  }

  // 表格暗色主题
  :deep(.el-table) {
    background: transparent;
    --el-table-bg-color: transparent;
    --el-table-tr-bg-color: transparent;
    --el-table-header-bg-color: rgba(26, 26, 62, 0.5);
    --el-table-row-hover-bg-color: rgba(242, 204, 143, 0.06);
    --el-table-border-color: rgba(242, 204, 143, 0.06);

    th {
      color: #B8A99A !important;
    }

    td {
      color: #F0E6D3;
      border-bottom-color: rgba(242, 204, 143, 0.06);
    }

    .el-table__row--striped td.el-table__cell {
      background: rgba(242, 204, 143, 0.02);
    }
  }

  // Tag 样式
  :deep(.el-tag) {
    border: none;

    &--info {
      background: rgba(184, 169, 154, 0.15);
      color: #B8A99A;
    }

    &--warning {
      background: rgba(242, 204, 143, 0.15);
      color: #F2CC8F;
    }

    &--danger {
      background: rgba(224, 122, 95, 0.15);
      color: #E07A5F;
    }
  }

  // Loading 遮罩
  :deep(.el-loading-mask) {
    background: rgba(20, 20, 50, 0.6);
  }

  // 分页
  :deep(.el-pagination) {
    --el-pagination-bg-color: transparent;
    --el-pagination-hover-color: #F2CC8F;
    --el-pagination-text-color: #B8A99A;

    .el-pager li {
      background: transparent;
      color: #B8A99A;

      &.is-active {
        background: linear-gradient(135deg, #F2CC8F, #E8A87C);
        color: #1a1a3e;
        border-radius: 6px;
        font-weight: 600;
      }

      &:hover:not(.is-active) {
        color: #F2CC8F;
      }
    }

    button {
      background: transparent;
      color: #B8A99A;

      &:hover {
        color: #F2CC8F;
      }
    }

    .el-pagination__sizes .el-select__wrapper {
      background: rgba(20, 20, 50, 0.6);
      border: 1px solid rgba(242, 204, 143, 0.12);
    }

    .el-pagination__total {
      color: #7A6F63;
    }
  }

  // 弹窗毛玻璃
  :deep(.el-dialog) {
    background: rgba(26, 26, 62, 0.95);
    backdrop-filter: blur(20px);
    border: 1px solid rgba(242, 204, 143, 0.1);
    border-radius: 16px;

    .el-dialog__header {
      border-bottom: 1px solid rgba(242, 204, 143, 0.06);

      .el-dialog__title {
        color: #F0E6D3;
      }

      .el-dialog__headerbtn .el-dialog__close {
        color: #7A6F63;

        &:hover {
          color: #F2CC8F;
        }
      }
    }

    .el-dialog__body {
      padding-top: 20px;
    }

    .el-dialog__footer {
      border-top: 1px solid rgba(242, 204, 143, 0.06);
    }
  }

  // Popconfirm
  :deep(.el-popconfirm) {
    .el-popconfirm__main {
      color: #F0E6D3;
    }
  }

  // 表单验证错误
  :deep(.el-form-item__error) {
    color: #E07A5F;
  }
}
</style>
