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
    <OpsDashboardDeck
      eyebrow="旅人"
      title="旅人关怀"
      :heading-chip="`${activeTravelerCount} 正常`"
      metric-label="旅人总数"
      :metric-value="summaryItems[0]?.value || '0'"
      metric-unit="位"
      :metric-description="usersOverviewDescription"
      section-note="值守重点"
      :overview-cards="travelerOverviewCards"
      :overview-highlights="travelerOverviewHighlights"
      :focus-card="travelerFocusCard"
      rhythm-title="活跃分布"
      :rhythm-chip="travelerPeakLabel"
      :rhythm-badge="`${activeTravelerCount} 活跃`"
      :rhythm-items="travelerRhythmItems"
      activity-title="旅人动态"
      :activity-chip="latestActiveMeta.value"
      :activity-rows="travelerActivityRows"
      guide-title="关怀建议"
      :guide-chip="engagementLabel"
      :guide-headline="usersGuideHeadline"
      :guide-copy="usersGuideCopy"
      guide-pulse-label="当前关怀评分"
      :guide-pulse-value="travelerGuidePulseValue"
      :guide-pulse-note="travelerGuidePulseNote"
      :guide-items="usersGuideItems"
    >
      <template #actions>
        <button type="button" class="overview-action" @click="handleSearch">搜索旅人</button>
        <button type="button" class="overview-action" @click="handleReset">重置视角</button>
      </template>
    </OpsDashboardDeck>

    <el-card shadow="never" class="table-card ops-table-card">
      <div class="ops-soft-toolbar ops-soft-toolbar--stacked users-table-toolbar">
        <div class="users-table-copy">
          <h3>旅人列表</h3>
          <p>把身份、活跃和互动产出收在同一张台面里，便于值守时快速判断该关注谁。</p>
          <div class="ops-toolbar-meta">
            <span class="ops-toolbar-meta__item">当前页 {{ users.length }} 人</span>
            <span class="ops-toolbar-meta__item">活跃 {{ activeTravelerCount }} 人</span>
            <span class="ops-toolbar-meta__item"
              >产出 {{ formatCount(totalStones + totalBoats) }} 条</span
            >
          </div>
        </div>
        <el-form :model="filters" inline aria-label="用户筛选" class="users-inline-filter">
          <el-form-item label="用户ID">
            <el-input v-model="filters.userId" placeholder="请输入用户ID" clearable />
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
            <el-select v-model="filters.status" placeholder="全部" clearable>
              <el-option label="正常" value="active" />
              <el-option label="已封禁" value="banned" />
            </el-select>
          </el-form-item>
          <el-form-item>
            <el-button type="primary" @click="handleSearch">
              <el-icon><Search /></el-icon>
              搜索
            </el-button>
            <el-button @click="handleReset">
              <el-icon><Refresh /></el-icon>
              重置
            </el-button>
          </el-form-item>
        </el-form>
      </div>

      <el-table v-loading="loading" :data="users" stripe style="width: 100%" aria-label="用户列表">
        <el-table-column label="旅人档案" min-width="286">
          <template #default="{ row }">
            <div class="traveler-identity traveler-identity--rich">
              <div class="traveler-identity__avatar">
                {{ getTravelerInitial(row) }}
              </div>
              <div class="traveler-identity__copy">
                <strong>{{ row.nickname || '未命名旅人' }}</strong>
                <span>@{{ row.username || row.user_id }}</span>
                <div class="traveler-identity__meta">
                  <em>{{ row.user_id }}</em>
                  <i :class="row.status === 'active' ? 'is-active' : 'is-banned'">
                    {{ row.status === 'active' ? '正常陪伴' : '限制中' }}
                  </i>
                </div>
              </div>
            </div>
          </template>
        </el-table-column>
        <el-table-column label="活跃轨迹" width="200">
          <template #default="{ row }">
            <div class="activity-meta">
              <strong>{{ getActivityNote(row.last_active_at) }}</strong>
              <span>{{ row.last_active_at || '暂无记录' }}</span>
            </div>
          </template>
        </el-table-column>
        <el-table-column label="互动画像" width="228">
          <template #default="{ row }">
            <div class="user-stats">
              <article class="user-stat-pill is-stone">
                <span>投石</span>
                <strong>{{ row.stones_count || 0 }}</strong>
              </article>
              <article class="user-stat-pill is-boat">
                <span>纸船</span>
                <strong>{{ row.boat_count || 0 }}</strong>
              </article>
            </div>
          </template>
        </el-table-column>
        <el-table-column label="状态" width="100">
          <template #default="{ row }">
            <el-tag :type="row.status === 'active' ? 'success' : 'danger'">
              {{ row.status === 'active' ? '正常' : '已封禁' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="入湖时间" width="188">
          <template #default="{ row }">
            <div class="registration-meta">
              <strong>{{ row.created_at || '暂无记录' }}</strong>
              <span>{{ getRegistrationNote(row.created_at) }}</span>
            </div>
          </template>
        </el-table-column>
        <el-table-column label="操作" fixed="right" width="180">
          <template #default="{ row }">
            <el-button type="primary" link @click="handleViewDetail(row)"> 详情 </el-button>
            <el-button v-if="row.status === 'active'" type="danger" link @click="handleBan(row)">
              封禁
            </el-button>
            <el-button v-else type="success" link @click="handleUnban(row)"> 解封 </el-button>
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

    <!-- 用户详情弹窗 -->
    <el-dialog
      v-model="detailVisible"
      title="用户详情"
      width="600px"
      aria-labelledby="user-detail-title"
    >
      <el-descriptions v-if="currentUser" :column="2" border>
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
import api, { isRequestCanceled } from '@/api'
import OpsDashboardDeck from '@/components/OpsDashboardDeck.vue'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import {
  createDeckActivityRows,
  createDeckFocusCard,
  createDeckGuideItems,
  createDeckOverviewHighlights,
  createDeckOverviewCards,
  createDeckRhythmItems,
} from '@/utils/opsDashboardDeck'
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
    {
      label: '旅人总数',
      value: formatCount(Number(pagination.total || 0)),
      note: '当前筛选下的总账号数',
      tone: 'lake' as const,
    },
    {
      label: '正常状态',
      value: formatCount(activeCount),
      note: '当前页可直接服务的账号',
      tone: 'sage' as const,
    },
    {
      label: '封禁处置',
      value: formatCount(bannedCount),
      note: '当前页仍处于限制中的账号',
      tone: 'rose' as const,
    },
    {
      label: '互动产出',
      value: formatCount(stonesCount + boatsCount),
      note: `投石 ${formatCount(stonesCount)} · 纸船 ${formatCount(boatsCount)}`,
      tone: 'amber' as const,
    },
  ]
})

const activeTravelerCount = computed(
  () => users.value.filter((item) => item.status === 'active').length,
)
const bannedTravelerCount = computed(
  () => users.value.filter((item) => item.status === 'banned').length,
)
const totalStones = computed(() =>
  users.value.reduce((sum, item) => sum + Number(item.stones_count || 0), 0),
)
const totalBoats = computed(() =>
  users.value.reduce((sum, item) => sum + Number(item.boat_count || 0), 0),
)

const usersVizBars = computed(() => [
  {
    label: '活跃',
    value: activeTravelerCount.value,
    display: formatCount(activeTravelerCount.value),
  },
  {
    label: '封禁',
    value: bannedTravelerCount.value,
    display: formatCount(bannedTravelerCount.value),
  },
  { label: '投石', value: totalStones.value, display: formatCount(totalStones.value) },
  { label: '纸船', value: totalBoats.value, display: formatCount(totalBoats.value) },
])

const engagementScore = computed(() => {
  const activeRatio = users.value.length ? activeTravelerCount.value / users.value.length : 0
  const outputBonus = Math.min(30, (totalStones.value + totalBoats.value) / 5)
  return Math.max(28, Math.min(98, Math.round(activeRatio * 70 + outputBonus)))
})

const engagementLabel = computed(() => {
  if (engagementScore.value >= 82) return '良好'
  if (engagementScore.value >= 60) return '稳定'
  return '需关注'
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

const getRegistrationNote = (value?: string) => {
  if (!value) return '入湖时间待补充'
  const diffDays = Math.max(0, Math.floor((Date.now() - new Date(value).getTime()) / 86400000))
  if (Number.isNaN(diffDays)) return '时间格式待确认'
  if (diffDays <= 1) return '近 24 小时入湖'
  if (diffDays <= 7) return '近一周加入'
  if (diffDays <= 30) return `${diffDays} 天湖龄`
  return `${Math.max(1, Math.floor(diffDays / 30))} 个月湖龄`
}

const getTravelerInitial = (user: User) => {
  const source = (user.nickname || user.username || user.user_id || '旅').trim()
  return source.charAt(0).toUpperCase()
}

const latestActiveMeta = computed(() => {
  const latestUser = users.value.reduce<User | null>((latest, item) => {
    if (!latest) return item
    return formatRecentTime(item.last_active_at).timestamp >
      formatRecentTime(latest.last_active_at).timestamp
      ? item
      : latest
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
  const outputCount = users.value.reduce(
    (sum, item) => sum + Number(item.stones_count || 0) + Number(item.boat_count || 0),
    0,
  )
  const averageOutput = users.value.length ? (outputCount / users.value.length).toFixed(1) : '0.0'
  const filterFocus =
    filters.status === 'banned'
      ? '封禁回看'
      : filters.userId || filters.nickname
        ? '定向检索'
        : '旅人总览'

  return [
    {
      label: '当前视角',
      value: filterFocus,
      note: filters.nickname
        ? `正在按昵称“${filters.nickname}”缩小范围。`
        : filters.userId
          ? `正在定向查看用户 ${filters.userId}。`
          : '默认浏览全量旅人，优先留意异常状态与高活跃账号。',
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

const usersStageHighlights = computed(() => [
  summaryItems.value[1],
  {
    label: '最近回湖',
    value: latestActiveMeta.value.value,
    note: latestActiveMeta.value.note,
    tone: 'plain' as const,
  },
  {
    label: '互动密度',
    value: travelerSignals.value[2]?.value || '0.0 / 人',
    note: travelerSignals.value[2]?.badge || '总产出 0',
    tone: 'amber' as const,
  },
])

const usersGuideHeadline = computed(() => {
  if (bannedTravelerCount.value > 0) return '先回看限制旅人'
  if (activeTravelerCount.value > 0) return '旅人状态平稳'
  return '当前旅人侧较安静'
})

const usersGuideCopy = computed(() => {
  if (bannedTravelerCount.value > 0) {
    return `当前页仍有 ${formatCount(bannedTravelerCount.value)} 位账号处于限制中，优先结合最近活跃和产出轨迹判断是否继续限制。`
  }
  if (activeTravelerCount.value > 0) {
    return `优先留意最近回湖和互动密度偏高的账号，避免漏掉需要人工跟进的个体。`
  }
  return '当前页暂未呈现明显活跃波动，适合回看筛选条件并准备下一轮定向检索。'
})

const usersGuideMetrics = computed(() => [
  { label: '限制中账号', value: `${formatCount(bannedTravelerCount.value)} 位` },
  { label: '总互动产出', value: `${formatCount(totalStones.value + totalBoats.value)} 条` },
  { label: '关怀评分', value: `${engagementScore.value} 分` },
])

const usersOverviewDescription = computed(() => {
  if (bannedTravelerCount.value > 0) {
    return `查看账号状态、活跃轨迹与基础产出，优先处理 ${formatCount(bannedTravelerCount.value)} 位限制中账号，再回看最近回湖旅人。`
  }
  return '查看账号状态、活跃轨迹与基础产出，统一判断最近回湖旅人和高互动账号是否需要继续人工关注。'
})

const travelerOverviewCards = computed(() =>
  createDeckOverviewCards(summaryItems.value.slice(1, 3)),
)
const travelerOverviewHighlights = computed(() =>
  createDeckOverviewHighlights([
    {
      label: '最近回湖',
      value: latestActiveMeta.value.value,
      note: latestActiveMeta.value.note,
      tone: 'lake' as const,
    },
    {
      label: '互动密度',
      value: travelerSignals.value[2]?.value || '0.0 / 人',
      note: travelerSignals.value[2]?.note || '当前页暂无互动轨迹。',
      tone: 'sage' as const,
    },
  ]),
)
const travelerFocusCard = computed(() =>
  createDeckFocusCard(summaryItems.value[3], '投石与纸船合并后的当前页总产出。'),
)
const travelerRhythmItems = computed(() => createDeckRhythmItems(usersVizBars.value))
const travelerPeakLabel = computed(() => {
  const peak = [...usersVizBars.value].sort((a, b) => Number(b.value) - Number(a.value))[0]
  return peak ? `${peak.label}领先` : '暂无波动'
})
const travelerActivityRows = computed(() => createDeckActivityRows(travelerSignals.value))
const travelerGuidePulseValue = computed(() => `${engagementScore.value} 分`)
const travelerGuidePulseNote = computed(() => {
  const totalUsers = Math.max(users.value.length, 1)
  const activeRatio = Math.round((activeTravelerCount.value / totalUsers) * 100)
  return `当前页活跃占比 ${activeRatio}% · ${engagementLabel.value}`
})
const usersGuideItems = computed(() =>
  createDeckGuideItems([
    {
      label: '限制中账号',
      value: `${formatCount(bannedTravelerCount.value)} 位`,
      note: '优先回看仍处于限制中的账号是否需要继续限制。',
    },
    {
      label: '总互动产出',
      value: `${formatCount(totalStones.value + totalBoats.value)} 条`,
      note: '最近产出更高的旅人更适合优先回看。',
    },
    {
      label: '关怀评分',
      value: `${engagementScore.value} 分`,
      note: '结合活跃占比和公开表达密度形成当前判断。',
    },
  ]),
)

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

const {
  pagination,
  buildParams,
  handleSizeChange,
  handleCurrentChange,
  handleSearch,
  handleReset,
} = useTablePagination(fetchUsers, {
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
    margin-top: 20px;

    :deep(.el-button) {
      flex: 1 1 0;
      min-width: 0;
      height: 44px;
      justify-content: flex-start;
      gap: 10px;
      border-radius: 999px !important;
      padding: 0 18px 0 12px;
      box-shadow:
        inset 0 1px 0 rgba(255, 255, 255, 0.96),
        0 12px 18px rgba(113, 138, 184, 0.08);
    }

    :deep(.el-button .el-icon) {
      width: 28px;
      height: 28px;
      display: grid;
      place-items: center;
      border-radius: 50%;
      background: rgba(233, 241, 255, 0.92);
      box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.9);
    }
  }

  .users-stage-heading {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    margin-top: 18px;
    margin-bottom: 14px;

    span {
      color: #111827;
      font-size: 15px;
      font-weight: 680;
    }

    small {
      color: var(--hl-ink-soft);
      font-size: 10px;
      line-height: 1.4;
    }
  }

  .users-filter-form {
    :deep(.el-form-item) {
      margin-bottom: 0;
    }
  }

  .users-table-toolbar {
    gap: 18px;
  }

  .users-inline-filter {
    width: 100%;
    justify-content: stretch;

    :deep(.el-form-item:nth-child(1)),
    :deep(.el-form-item:nth-child(2)),
    :deep(.el-form-item:nth-child(3)),
    :deep(.el-form-item:nth-child(4)) {
      grid-column: span 3;
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

  .traveler-identity--rich {
    grid-template-columns: auto minmax(0, 1fr);
    align-items: center;
    gap: 12px;
  }

  .traveler-identity__avatar {
    width: 42px;
    height: 42px;
    display: grid;
    place-items: center;
    border-radius: 50%;
    background: linear-gradient(180deg, rgba(184, 208, 255, 0.94), rgba(171, 197, 255, 0.98));
    color: #18233a;
    font-size: 14px;
    font-weight: 800;
    box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.88);
  }

  .traveler-identity__copy {
    min-width: 0;
    display: grid;
    gap: 4px;

    span {
      font-size: 12px;
    }

    strong {
      line-height: 1.4;
    }

    em {
      color: rgba(91, 105, 136, 0.82);
      font-size: 11px;
      font-style: normal;
      font-family: var(--hl-font-mono);
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
  }

  .traveler-identity__meta {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    align-items: center;

    i {
      display: inline-flex;
      align-items: center;
      min-height: 22px;
      padding: 0 8px;
      border-radius: 999px;
      font-style: normal;
      font-size: 10px;
      font-weight: 700;
      letter-spacing: 0.02em;
      box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.88);
    }

    i.is-active {
      background: rgba(219, 245, 239, 0.96);
      color: #1f7a69;
    }

    i.is-banned {
      background: rgba(255, 231, 233, 0.94);
      color: #b44d59;
    }
  }

  .user-stats {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 8px;
  }

  .user-stat-pill {
    display: grid;
    gap: 3px;
    min-height: 62px;
    padding: 11px 12px;
    border-radius: 18px;
    background: rgba(255, 255, 255, 0.68);
    border: 1px solid rgba(149, 171, 214, 0.14);
    box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.84);

    span {
      color: var(--hl-ink-soft);
      font-size: 11px;
      line-height: 1.3;
    }

    strong {
      color: var(--hl-ink);
      font-size: 18px;
      font-weight: 760;
      letter-spacing: -0.04em;
    }
  }

  .user-stat-pill.is-stone {
    background: linear-gradient(180deg, rgba(244, 248, 255, 0.96), rgba(233, 241, 255, 0.96));
  }

  .user-stat-pill.is-boat {
    background: linear-gradient(180deg, rgba(228, 246, 241, 0.96), rgba(218, 241, 235, 0.96));
  }

  .registration-meta {
    display: grid;
    gap: 4px;

    strong {
      color: var(--hl-ink);
      font-size: 12px;
      font-weight: 700;
      line-height: 1.45;
    }

    span {
      color: var(--hl-ink-soft);
      font-size: 11px;
      line-height: 1.5;
    }
  }

  .pagination-wrapper {
    margin-top: 20px;
    display: flex;
    justify-content: flex-end;
  }
}
</style>
