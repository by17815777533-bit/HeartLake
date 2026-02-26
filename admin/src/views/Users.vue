<!--
  @file Users.vue
  @brief Users 组件
  Created by 林子怡
-->

<template>
  <div class="users-page">
    <!-- 搜索筛选 -->
    <el-card
      shadow="never"
      class="filter-card"
    >
      <el-form
        :model="filters"
        inline
        aria-label="用户筛选"
      >
        <el-form-item label="用户ID">
          <el-input
            v-model="filters.userId"
            placeholder="请输入用户ID"
            clearable
          />
        </el-form-item>
        <el-form-item label="昵称">
          <el-input
            v-model="filters.nickname"
            placeholder="请输入昵称"
            clearable
            @input="onSearchInput"
          />
        </el-form-item>
        <el-form-item label="状态">
          <el-select
            v-model="filters.status"
            placeholder="全部"
            clearable
          >
            <el-option
              label="正常"
              value="active"
            />
            <el-option
              label="已封禁"
              value="banned"
            />
          </el-select>
        </el-form-item>
        <el-form-item>
          <el-button
            type="primary"
            @click="handleSearch"
          >
            <el-icon><Search /></el-icon>
            搜索
          </el-button>
          <el-button @click="handleReset">
            <el-icon><Refresh /></el-icon>
            重置
          </el-button>
        </el-form-item>
      </el-form>
    </el-card>

    <!-- 用户列表 -->
    <el-card shadow="never">
      <el-table
        v-loading="loading"
        :data="users"
        stripe
        style="width: 100%"
        aria-label="用户列表"
      >
        <el-table-column
          prop="user_id"
          label="用户ID"
          width="180"
        />
        <el-table-column
          prop="nickname"
          label="昵称"
          width="150"
        />
        <el-table-column
          prop="username"
          label="账号"
          width="120"
        />
        <el-table-column
          label="统计"
          width="220"
        >
          <template #default="{ row }">
            <div class="user-stats">
              <span class="stat-item"><i class="stat-dot stone" />投石 {{ row.stones_count || 0 }}</span>
              <span class="stat-item"><i class="stat-dot boat" />纸船 {{ row.boat_count || 0 }}</span>
            </div>
          </template>
        </el-table-column>
        <el-table-column
          label="状态"
          width="100"
        >
          <template #default="{ row }">
            <el-tag :type="row.status === 'active' ? 'success' : 'danger'">
              {{ row.status === 'active' ? '正常' : '已封禁' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column
          prop="created_at"
          label="注册时间"
          width="180"
        />
        <el-table-column
          prop="last_active_at"
          label="最后活跃"
          width="180"
        />
        <el-table-column
          label="操作"
          fixed="right"
          width="180"
        >
          <template #default="{ row }">
            <el-button
              type="primary"
              link
              @click="handleViewDetail(row)"
            >
              详情
            </el-button>
            <el-button
              v-if="row.status === 'active'"
              type="danger"
              link
              @click="handleBan(row)"
            >
              封禁
            </el-button>
            <el-button
              v-else
              type="success"
              link
              @click="handleUnban(row)"
            >
              解封
            </el-button>
          </template>
        </el-table-column>
      </el-table>

      <!-- 分页 -->
      <div class="pagination-wrapper">
        <el-pagination
          v-model:current-page="pagination.page"
          v-model:page-size="pagination.pageSize"
          :page-sizes="[10, 20, 50, 100]"
          :total="pagination.total"
          layout="total, sizes, prev, pager, next, jumper"
          @size-change="handleSizeChange"
          @current-change="handleCurrentChange"
        />
      </div>
    </el-card>

    <!-- 用户详情弹窗 -->
    <el-dialog
      v-model="detailVisible"
      title="用户详情"
      width="600px"
      aria-labelledby="user-detail-title"
    >
      <el-descriptions
        v-if="currentUser"
        :column="2"
        border
      >
        <el-descriptions-item label="用户ID">
          {{ currentUser.user_id }}
        </el-descriptions-item>
        <el-descriptions-item label="昵称">
          {{ currentUser.nickname }}
        </el-descriptions-item>
        <el-descriptions-item label="账号">
          {{ currentUser.username }}
        </el-descriptions-item>
        <el-descriptions-item label="状态">
          <el-tag :type="currentUser.status === 'active' ? 'success' : 'danger'">
            {{ currentUser.status === 'active' ? '正常' : '已封禁' }}
          </el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="投石数">
          {{ currentUser.stones_count }}
        </el-descriptions-item>
        <el-descriptions-item label="纸船数">
          {{ currentUser.boat_count }}
        </el-descriptions-item>
        <el-descriptions-item label="注册时间">
          {{ currentUser.created_at }}
        </el-descriptions-item>
        <el-descriptions-item label="最后活跃">
          {{ currentUser.last_active_at }}
        </el-descriptions-item>
      </el-descriptions>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Search, Refresh } from '@element-plus/icons-vue'
import api from '@/api'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import type { User } from '@/types'

const loading = ref(false)
const users = ref<User[]>([])
const detailVisible = ref(false)
const currentUser = ref<User | null>(null)

// L-11: 搜索输入防抖，避免频繁请求
let searchDebounceTimer: ReturnType<typeof setTimeout> | null = null
const onSearchInput = () => {
  if (searchDebounceTimer) clearTimeout(searchDebounceTimer)
  searchDebounceTimer = setTimeout(() => {
    handleSearch()
  }, 300)
}
onUnmounted(() => {
  if (searchDebounceTimer) clearTimeout(searchDebounceTimer)
})

const defaultFilters = {
  userId: '',
  nickname: '',
  status: '',
}

const filters = reactive({ ...defaultFilters })

// 获取用户列表
const fetchUsers = async () => {
  loading.value = true
  try {
    // M-8: 构建搜索参数，将 nickname 作为 search 传递给后端
    const extra: Record<string, unknown> = {}
    if (filters.userId) extra.user_id = filters.userId
    if (filters.nickname) extra.search = filters.nickname
    if (filters.status) extra.status = filters.status
    const params = buildParams(extra)

    const res = await api.getUsers(params)
    // 兼容后端两种响应格式: {data: {users, total}} 或 {users, total}
    const resData = res.data?.data || res.data || {}
    users.value = resData.users || []
    pagination.total = resData.total || 0
  } catch (e) {
    console.error('获取用户列表失败:', e)
    ElMessage.error(getErrorMessage(e, '获取用户列表失败'))
    users.value = []
    pagination.total = 0
  } finally {
    loading.value = false
  }
}

const { pagination, buildParams, handleSizeChange, handleCurrentChange, handleSearch, handleReset } = useTablePagination(fetchUsers, {
  filters,
  defaultFilters,
  beforeSearch: () => {
    filters.userId = filters.userId.trim()
    filters.nickname = filters.nickname.trim()
    if (filters.userId.length > 64) {
      ElMessage.warning('用户ID过长，请检查输入')
      return false
    }
    if (filters.nickname.length > 50) {
      ElMessage.warning('昵称过长，请检查输入')
      return false
    }
  },
})

// 查看详情
const handleViewDetail = (row: User) => {
  currentUser.value = row
  detailVisible.value = true
}

// 封禁用户
const handleBan = async (row: User) => {
  const { value: reason } = await ElMessageBox.prompt('请输入封禁原因', '封禁用户', {
    confirmButtonText: '确定',
    cancelButtonText: '取消',
    inputPattern: /\S+/,
    inputErrorMessage: '请输入封禁原因',
  }).catch(() => ({ value: null }))

  if (!reason) return

  try {
    await api.banUser(row.user_id, { reason })
    ElMessage.success('封禁成功')
    fetchUsers()
  } catch (e) {
    // H-3: 使用统一错误处理
    ElMessage.error(getErrorMessage(e, '封禁失败'))
  }
}

// 解封用户
const handleUnban = async (row: User) => {
  try {
    await ElMessageBox.confirm('确定要解封该用户吗？', '解封用户', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning',
    })
  } catch {
    return // 用户取消
  }

  try {
    await api.unbanUser(row.user_id)
    ElMessage.success('解封成功')
    fetchUsers()
  } catch (e) {
    // H-3: 使用统一错误处理
    ElMessage.error(getErrorMessage(e, '解封失败'))
  }
}

onMounted(() => {
  fetchUsers()
})
</script>

<style lang="scss" scoped>
.users-page {
  .filter-card {
    margin-bottom: 16px;
  }

  .user-stats {
    display: flex;
    gap: 16px;

    .stat-item {
      display: flex;
      align-items: center;
      font-size: 13px;
      color: var(--m3-on-surface-variant);

      .stat-dot {
        width: 8px;
        height: 8px;
        border-radius: 50%;
        margin-right: 6px;

        &.stone {
          background: var(--m3-error);
        }

        &.boat {
          background: var(--m3-success);
        }
      }
    }
  }

  .pagination-wrapper {
    margin-top: 20px;
    display: flex;
    justify-content: flex-end;
  }
}
</style>
