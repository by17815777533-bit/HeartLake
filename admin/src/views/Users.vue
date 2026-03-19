<!--
  旅人关怀（用户管理）页面

  功能：
  - 按用户ID、昵称（300ms 防抖）、状态筛选用户列表
  - 用户详情弹窗展示完整信息
  - 封禁操作需输入原因，解封需二次确认
  - 输入校验：用户ID 最长 64 字符，昵称最长 50 字符
-->

<template>
  <div class="users-page ops-page">
    <OpsWorkbench>
      <template #stage>
        <OpsSurfaceCard
          eyebrow="Travelers"
          title="旅人关怀"
          :chip="`${summaryItems[1]?.value || 0} 正常`"
          tone="sky"
        >
          <div class="ops-big-metric">
            <span class="ops-big-metric__label">旅人总数</span>
            <div class="ops-big-metric__value">
              {{ summaryItems[0]?.value || 0 }}
              <small>人</small>
            </div>
            <p class="ops-big-metric__note">
              查看账号状态、活跃轨迹与基础产出，统一处理封禁、解封与个体关注。
            </p>
          </div>

          <div class="ops-soft-actions users-stage-actions">
            <el-button
              type="primary"
              @click="handleSearch"
            >
              <el-icon><Search /></el-icon>
              搜索旅人
            </el-button>
            <el-button @click="handleReset">
              <el-icon><Refresh /></el-icon>
              重置视角
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
          title="快速筛查"
          :chip="filters.status ? '定向状态' : '全量视图'"
          tone="ice"
          compact
        >
          <el-form
            :model="filters"
            aria-label="用户筛选"
            class="ops-form-grid users-filter-form"
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
          </el-form>

          <div class="ops-chip-row">
            <span class="ops-chip">
              {{ filters.nickname ? `昵称 ${filters.nickname}` : '未限制昵称' }}
            </span>
            <span class="ops-chip">
              {{ filters.userId ? `用户 ${filters.userId}` : '未指定用户' }}
            </span>
          </div>
        </OpsSurfaceCard>
      </template>

      <template #rail>
        <OpsSurfaceCard
          eyebrow="Pulse"
          title="旅人动态"
          :chip="latestActiveMeta.value"
          tone="mint"
        >
          <div class="ops-list-stack">
            <article
              v-for="item in travelerSignals"
              :key="item.label"
              class="ops-list-row"
            >
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

      <el-card
        shadow="never"
        class="table-card ops-table-card"
      >
        <div class="ops-soft-toolbar">
          <div class="users-table-copy">
            <h3>旅人列表</h3>
            <p>列表保留真实石头和纸船聚合，封禁与解封操作直接写入后台。</p>
          </div>
          <div class="ops-chip-row">
            <span class="ops-chip">
              {{ summaryItems[2]?.label }} {{ summaryItems[2]?.value }}
            </span>
            <span class="ops-chip">
              {{ summaryItems[3]?.note }}
            </span>
          </div>
        </div>

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
            label="旅人"
            min-width="190"
          >
            <template #default="{ row }">
              <div class="traveler-identity">
                <strong>{{ row.nickname || '未命名旅人' }}</strong>
                <span>@{{ row.username || row.user_id }}</span>
              </div>
            </template>
          </el-table-column>
          <el-table-column
            label="活跃轨迹"
            width="190"
          >
            <template #default="{ row }">
              <div class="activity-meta">
                <strong>{{ row.last_active_at || '暂无记录' }}</strong>
                <span>{{ getActivityNote(row.last_active_at) }}</span>
              </div>
            </template>
          </el-table-column>
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
    </OpsWorkbench>

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
import { computed, ref, reactive, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Search, Refresh } from '@element-plus/icons-vue'
import api, { isRequestCanceled } from '@/api'
import OpsWorkbench from '@/components/OpsWorkbench.vue'
import OpsSurfaceCard from '@/components/OpsSurfaceCard.vue'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import { getWorkbenchTileTone } from '@/utils/workbenchTone'
import type { User } from '@/types'

const loading = ref(false)
const users = ref<User[]>([])
const detailVisible = ref(false)
const currentUser = ref<User | null>(null)

// 搜索输入防抖，昵称输入 300ms 后自动触发搜索
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

const formatCount = (value: number) => value.toLocaleString()

const summaryItems = computed(() => {
  const activeCount = users.value.filter((item) => item.status === 'active').length
  const bannedCount = users.value.filter((item) => item.status === 'banned').length
  const stonesCount = users.value.reduce((sum, item) => sum + Number(item.stones_count || 0), 0)
  const boatsCount = users.value.reduce((sum, item) => sum + Number(item.boat_count || 0), 0)

  return [
    { label: '旅人总数', value: formatCount(Number(pagination.total || 0)), note: '当前筛选下的总账号数', tone: 'lake' as const },
    { label: '正常状态', value: formatCount(activeCount), note: '当前页可直接服务的账号', tone: 'sage' as const },
    { label: '封禁处置', value: formatCount(bannedCount), note: '当前页仍处于限制中的账号', tone: 'rose' as const },
    { label: '互动产出', value: formatCount(stonesCount + boatsCount), note: `投石 ${formatCount(stonesCount)} · 纸船 ${formatCount(boatsCount)}`, tone: 'amber' as const },
  ]
})

const formatRecentTime = (value?: string) => {
  if (!value) return { timestamp: 0, label: '暂无活跃记录' }
  const timestamp = new Date(value).getTime()
  return {
    timestamp: Number.isNaN(timestamp) ? 0 : timestamp,
    label: value,
  }
}

const getActivityNote = (value?: string) => {
  if (!value) return '尚未留下活跃轨迹'
  const diffMinutes = Math.max(0, Math.floor((Date.now() - new Date(value).getTime()) / 60000))
  if (Number.isNaN(diffMinutes)) return '时间格式待确认'
  if (diffMinutes <= 10) return '刚刚回到湖面'
  if (diffMinutes <= 60) return `${diffMinutes} 分钟内活跃`
  if (diffMinutes <= 24 * 60) return `${Math.floor(diffMinutes / 60)} 小时内活跃`
  return `${Math.floor(diffMinutes / (24 * 60))} 天前活跃`
}

const latestActiveMeta = computed(() => {
  const latestUser = users.value.reduce<User | null>((latest, item) => {
    if (!latest) return item
    return formatRecentTime(item.last_active_at).timestamp > formatRecentTime(latest.last_active_at).timestamp ? item : latest
  }, null)

  if (!latestUser) {
    return {
      value: '暂无旅人回湖',
      note: '当前页还没有活跃记录可供判断。',
    }
  }

  return {
    value: latestUser.nickname || latestUser.username || latestUser.user_id,
    note: `${latestUser.last_active_at || '暂无时间'} · ${getActivityNote(latestUser.last_active_at)}`,
  }
})

const travelerSignals = computed(() => {
  const activeCount = users.value.filter((item) => item.status === 'active').length
  const outputCount = users.value.reduce((sum, item) => sum + Number(item.stones_count || 0) + Number(item.boat_count || 0), 0)
  const averageOutput = users.value.length ? (outputCount / users.value.length).toFixed(1) : '0.0'
  const filterFocus = filters.status === 'banned'
    ? '封禁回看'
    : (filters.userId || filters.nickname ? '定向检索' : '旅人总览')

  return [
    {
      label: '当前视角',
      value: filterFocus,
      note: filters.nickname
        ? `正在按昵称“${filters.nickname}”缩小范围。`
        : (filters.userId ? `正在定向查看用户 ${filters.userId}。` : '默认浏览全量旅人，优先留意异常状态与高活跃账号。'),
      badge: filters.status ? `状态 ${filters.status}` : '全部状态',
      tone: 'lake' as const,
    },
    {
      label: '最近回湖',
      value: latestActiveMeta.value.value,
      note: latestActiveMeta.value.note,
      badge: `${formatCount(activeCount)} 人正常`,
      tone: 'sage' as const,
    },
    {
      label: '互动密度',
      value: `${averageOutput} / 人`,
      note: `当前页平均每位旅人留下 ${averageOutput} 条公开或漂流表达。`,
      badge: `总产出 ${formatCount(outputCount)}`,
      tone: 'amber' as const,
    },
  ]
})

/**
 * 拉取用户列表，兼容后端两种响应格式：
 * - { data: { users, total } }
 * - { users, total }
 */
const fetchUsers = async () => {
  loading.value = true
  try {
    // 构建搜索参数，将 nickname 作为 search 传递给后端
    const extra: Record<string, unknown> = {}
    if (filters.userId) extra.userId = filters.userId
    if (filters.nickname) extra.search = filters.nickname
    if (filters.status) extra.status = filters.status
    const params = buildParams(extra)

    const res = await api.getUsers(params)
    // 兼容后端两种响应格式: {data: {users, total}} 或 {users, total}
    const resData = res.data?.data || res.data || {}
    users.value = resData.users || []
    pagination.total = resData.total || 0
  } catch (e) {
    if (isRequestCanceled(e)) return
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

/** 查看用户详情 */
const handleViewDetail = (row: User) => {
  currentUser.value = row
  detailVisible.value = true
}

/** 封禁用户，弹窗输入原因后调用后端接口 */
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
    // 统一错误处理
    ElMessage.error(getErrorMessage(e, '封禁失败'))
  }
}

/** 解封用户，二次确认后调用后端接口 */
const handleUnban = async (row: User) => {
  try {
    await ElMessageBox.confirm('确定要解封该用户吗？', '解封用户', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning',
    })
  } catch {
    return
  }

  try {
    await api.unbanUser(row.user_id)
    ElMessage.success('解封成功')
    fetchUsers()
  } catch (e) {
    // 统一错误处理
    ElMessage.error(getErrorMessage(e, '解封失败'))
  }
}

onMounted(() => {
  fetchUsers()
})
</script>

<style lang="scss" scoped>
.users-page {
  .users-stage-actions {
    margin: 22px 0 18px;
  }

  .users-filter-form {
    :deep(.el-form-item) {
      margin-bottom: 0;
    }
  }

  .users-table-copy {
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

  .traveler-identity,
  .activity-meta {
    display: grid;
    gap: 6px;

    strong {
      color: var(--hl-ink);
      font-size: 14px;
      font-weight: 700;
    }

    span {
      color: var(--hl-ink-soft);
      font-size: 12px;
      line-height: 1.5;
    }
  }

  .user-stats {
    display: flex;
    gap: 16px;

    .stat-item {
      display: flex;
      align-items: center;
      min-height: 30px;
      padding: 0 10px;
      border-radius: 999px;
      background: rgba(255, 255, 255, 0.58);
      border: 1px solid rgba(115, 141, 151, 0.12);
      font-size: 12px;
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
