<!--
  内容管理页面 -- 石头与纸船的查看、筛选、删除

  功能：
  - 按类型（石头/纸船）、状态（已发布/待审核/已删除）、关键词筛选
  - 关键词输入 300ms 防抖，减少无效请求
  - 删除操作强制输入原因（>=5字），根据内容类型调用不同 API
  - 内容详情弹窗展示完整文本
-->

<template>
  <div class="content-page ops-page">
    <OpsWorkbench>
      <template #stage>
        <OpsSurfaceCard
          eyebrow="Content"
          title="石头与纸船"
          :chip="contentSignals[0]?.value || '混合巡检'"
          tone="sky"
        >
          <div class="ops-big-metric">
            <span class="ops-big-metric__label">内容总量</span>
            <div class="ops-big-metric__value">
              {{ summaryItems[0]?.value || 0 }}
              <small>条</small>
            </div>
            <p class="ops-big-metric__note">
              统一查看石头与纸船的状态、文案与作者信息，必要时直接处置并保留操作原因。
            </p>
          </div>

          <div class="ops-soft-actions content-stage-actions">
            <el-button
              type="primary"
              @click="handleSearch"
            >
              搜索内容
            </el-button>
            <el-button @click="handleReset">
              重置视图
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
          eyebrow="Flow"
          title="内容流向"
          :chip="`${contentFlowScore}% 待确认占比`"
          tone="ice"
          compact
        >
          <OpsMiniBars :items="contentVizBars" />
        </OpsSurfaceCard>
      </template>

      <template #rail>
        <OpsSurfaceCard
          eyebrow="Watch"
          title="内容动态"
          :chip="latestContentMeta.value"
          tone="mint"
        >
          <div class="ops-list-stack">
            <article
              v-for="item in contentSignals"
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

      <template #footer>
        <OpsSurfaceCard
          eyebrow="Score"
          title="巡检评分"
          :chip="`${contentHealthScore} / 100`"
          tone="plain"
          compact
        >
          <OpsGaugeMeter
            :value="contentHealthScore"
            :max="100"
            :label="contentHealthLabel"
          />
        </OpsSurfaceCard>
      </template>

      <el-card
        shadow="never"
        class="table-card ops-table-card"
      >
        <div class="ops-soft-toolbar content-table-toolbar">
          <div class="content-table-copy">
            <h3>内容列表</h3>
            <p>石头和纸船在同一张工作台上巡检，删除会强制要求理由并保留处置痕迹。</p>
          </div>
          <el-form
            :model="filters"
            inline
            aria-label="内容筛选"
            class="content-inline-filter"
          >
            <el-form-item label="类型">
              <el-select
                v-model="filters.type"
                placeholder="全部"
                clearable
              >
                <el-option
                  label="石头"
                  value="stone"
                />
                <el-option
                  label="纸船"
                  value="boat"
                />
              </el-select>
            </el-form-item>
            <el-form-item label="状态">
              <el-select
                v-model="filters.status"
                placeholder="全部"
                clearable
              >
                <el-option
                  label="已发布"
                  value="published"
                />
                <el-option
                  label="待审核"
                  value="pending"
                />
                <el-option
                  label="已删除"
                  value="deleted"
                />
              </el-select>
            </el-form-item>
            <el-form-item label="关键词">
              <el-input
                v-model="filters.keyword"
                placeholder="搜索内容"
                clearable
                @input="onKeywordInput"
              />
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
          :data="contentList"
          stripe
          aria-label="内容列表"
        >
          <el-table-column
            prop="id"
            label="ID"
            width="100"
          />
          <el-table-column
            label="类型"
            width="80"
          >
            <template #default="{ row }">
              <el-tag
                :type="row.type === 'stone' ? 'primary' : 'success'"
                size="small"
              >
                {{ row.type === 'stone' ? '石头' : '纸船' }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column
            label="内容"
            min-width="300"
          >
            <template #default="{ row }">
              <div class="content-copy">
                <p class="content-text">
                  {{ row.content?.substring(0, 100) }}{{ row.content?.length > 100 ? '...' : '' }}
                </p>
                <span class="content-note">
                  {{ filters.keyword ? `当前关键词：${filters.keyword}` : '未指定关键词，展示最新内容切片。' }}
                </span>
              </div>
            </template>
          </el-table-column>
          <el-table-column
            label="作者"
            width="120"
          >
            <template #default="{ row }">
              <div class="content-author">
                <strong>{{ row.user?.nickname || '匿名' }}</strong>
                <span>{{ row.type === 'stone' ? '公开投石' : '纸船漂流' }}</span>
              </div>
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
            label="发布时间"
            width="180"
          >
            <template #default="{ row }">
              <div class="content-time">
                <strong>{{ row.created_at || '暂无时间' }}</strong>
                <span>{{ getTimelineNote(row.created_at) }}</span>
              </div>
            </template>
          </el-table-column>
          <el-table-column
            label="操作"
            width="150"
            fixed="right"
          >
            <template #default="{ row }">
              <el-button
                type="primary"
                link
                @click="viewContent(row)"
              >
                查看
              </el-button>
              <el-button
                type="danger"
                link
                @click="deleteContent(row)"
              >
                删除
              </el-button>
            </template>
          </el-table-column>
          <template #empty>
            <el-empty
              description="暂无内容数据"
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

    <!-- 内容详情弹窗 -->
    <el-dialog
      v-model="detailVisible"
      title="内容详情"
      width="600px"
      aria-labelledby="content-detail-title"
    >
      <div v-if="currentContent">
        <el-descriptions
          :column="2"
          border
        >
          <el-descriptions-item label="ID">
            {{ currentContent.id }}
          </el-descriptions-item>
          <el-descriptions-item label="类型">
            {{ currentContent.type === 'stone' ? '石头' : '纸船' }}
          </el-descriptions-item>
          <el-descriptions-item label="作者">
            {{ currentContent.user?.nickname || '匿名' }}
          </el-descriptions-item>
          <el-descriptions-item label="状态">
            {{ getStatusLabel(currentContent.status) }}
          </el-descriptions-item>
          <el-descriptions-item
            label="发布时间"
            :span="2"
          >
            {{ currentContent.created_at }}
          </el-descriptions-item>
          <el-descriptions-item
            label="内容"
            :span="2"
          >
            <div class="detail-content">
              {{ currentContent.content }}
            </div>
          </el-descriptions-item>
        </el-descriptions>
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, reactive, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import api, { isRequestCanceled } from '@/api'
import OpsWorkbench from '@/components/OpsWorkbench.vue'
import OpsSurfaceCard from '@/components/OpsSurfaceCard.vue'
import OpsMiniBars from '@/components/OpsMiniBars.vue'
import OpsGaugeMeter from '@/components/OpsGaugeMeter.vue'
import { getErrorMessage } from '@/utils/errorHelper'
import { useTablePagination } from '@/composables/useTablePagination'
import { getWorkbenchTileTone } from '@/utils/workbenchTone'
import type { ContentItem } from '@/types'

const loading = ref(false)
const contentList = ref<ContentItem[]>([])
const detailVisible = ref(false)
const currentContent = ref<ContentItem | null>(null)

// 搜索关键词输入防抖，避免每次按键都触发请求
let keywordDebounceTimer: ReturnType<typeof setTimeout> | null = null
const onKeywordInput = () => {
  if (keywordDebounceTimer) clearTimeout(keywordDebounceTimer)
  keywordDebounceTimer = setTimeout(() => {
    handleSearch()
  }, 300)
}
onUnmounted(() => {
  if (keywordDebounceTimer) clearTimeout(keywordDebounceTimer)
})

const filters = reactive({
  type: '',
  status: '',
  keyword: '',
})

const formatCount = (value: number) => value.toLocaleString()

const summaryItems = computed(() => {
  const stoneCount = contentList.value.filter((item) => item.type === 'stone').length
  const boatCount = contentList.value.filter((item) => item.type === 'boat').length
  const pendingCount = contentList.value.filter((item) => item.status === 'pending').length

  return [
    { label: '内容总量', value: formatCount(Number(pagination.total || 0)), note: '当前筛选下的内容总数', tone: 'lake' as const },
    { label: '当前页石头', value: formatCount(stoneCount), note: '公开心声内容', tone: 'amber' as const },
    { label: '当前页纸船', value: formatCount(boatCount), note: '一对一漂流内容', tone: 'sage' as const },
    { label: '待确认内容', value: formatCount(pendingCount), note: '当前页需要继续观察的条目', tone: 'rose' as const },
  ]
})

const contentStoneCount = computed(() => contentList.value.filter((item) => item.type === 'stone').length)
const contentBoatCount = computed(() => contentList.value.filter((item) => item.type === 'boat').length)
const contentPendingCount = computed(() => contentList.value.filter((item) => item.status === 'pending').length)

const contentVizBars = computed(() => [
  { label: '石头', value: contentStoneCount.value, display: formatCount(contentStoneCount.value) },
  { label: '纸船', value: contentBoatCount.value, display: formatCount(contentBoatCount.value) },
  { label: '待审', value: contentPendingCount.value, display: formatCount(contentPendingCount.value) },
  { label: '总量', value: contentList.value.length, display: formatCount(contentList.value.length) },
])

const contentFlowScore = computed(() => {
  const total = Math.max(contentList.value.length, 1)
  return Math.round((contentPendingCount.value / total) * 100)
})

const contentHealthScore = computed(() => Math.max(30, Math.min(95, 92 - contentFlowScore.value)))
const contentHealthLabel = computed(() => {
  if (contentHealthScore.value >= 82) return '平稳'
  if (contentHealthScore.value >= 60) return '可控'
  return '积压'
})

const getTimelineNote = (value?: string) => {
  if (!value) return '等待时间写入'
  const diffMinutes = Math.max(0, Math.floor((Date.now() - new Date(value).getTime()) / 60000))
  if (Number.isNaN(diffMinutes)) return '时间格式待确认'
  if (diffMinutes <= 60) return `${diffMinutes} 分钟前入湖`
  if (diffMinutes <= 24 * 60) return `${Math.floor(diffMinutes / 60)} 小时前发布`
  return `${Math.floor(diffMinutes / (24 * 60))} 天前发布`
}

const latestContentMeta = computed(() => {
  const latestItem = contentList.value.reduce<ContentItem | null>((latest, item) => {
    if (!latest) return item
    return new Date(item.created_at).getTime() > new Date(latest.created_at).getTime() ? item : latest
  }, null)

  if (!latestItem) {
    return {
      value: '暂无内容更新',
      note: '当前筛选条件下还没有最新条目。',
    }
  }

  return {
    value: latestItem.type === 'stone' ? '最新石头' : '最新纸船',
    note: `${latestItem.created_at || '暂无时间'} · ${getTimelineNote(latestItem.created_at)}`,
  }
})

const contentSignals = computed(() => {
  const totalCount = contentList.value.length
  const pendingCount = contentList.value.filter((item) => item.status === 'pending').length
  const boatCount = contentList.value.filter((item) => item.type === 'boat').length
  const watchMode = filters.type === 'stone'
    ? '石头巡检'
    : (filters.type === 'boat' ? '纸船巡检' : '混合巡检')

  return [
    {
      label: '巡检模式',
      value: watchMode,
      note: filters.status ? `当前只查看“${getStatusLabel(filters.status)}”状态内容。` : '默认同时查看石头与纸船的最新流入。',
      badge: filters.keyword ? '关键词筛查' : '全量浏览',
      tone: 'lake' as const,
    },
    {
      label: '最新入湖',
      value: latestContentMeta.value.value,
      note: latestContentMeta.value.note,
      badge: `当前页 ${formatCount(totalCount)} 条`,
      tone: 'sage' as const,
    },
    {
      label: '待确认占比',
      value: totalCount ? `${Math.round((pendingCount / totalCount) * 100)}%` : '0%',
      note: `当前页待确认 ${formatCount(pendingCount)} 条，纸船 ${formatCount(boatCount)} 条。`,
      badge: `纸船 ${formatCount(boatCount)}`,
      tone: 'amber' as const,
    },
  ]
})

const { pagination, buildParams, handleSizeChange, handleCurrentChange, handleSearch, handleReset } = useTablePagination(fetchContent, {
  filters,
  defaultFilters: { type: '', status: '', keyword: '' },
  beforeSearch: () => {
    filters.keyword = filters.keyword.trim()
    if (filters.keyword.length > 100) {
      ElMessage.warning('搜索关键词过长，请精简后重试')
      return false
    }
  },
})

/** 内容状态 → el-tag type 映射 */
const getStatusType = (status: string) => {
  const map: Record<string, string> = { published: 'success', pending: 'warning', deleted: 'danger' }
  return map[status] || 'info'
}

/** 内容状态 → 中文标签映射 */
const getStatusLabel = (status: string) => {
  const map: Record<string, string> = { published: '已发布', pending: '待审核', deleted: '已删除' }
  return map[status] || status
}

/**
 * 拉取内容列表，兼容后端 stones/boats/list 三种响应字段名。
 * 统一映射为前端 ContentItem 结构。
 */
async function fetchContent() {
  loading.value = true
  try {
    const params = buildParams(filters)
    const currentPage = Number(params.page || 1)
    const pageSize = Number(params.page_size || 20)

    if (filters.type === 'boat') {
      const res = await api.getBoats(params)
      const resData = res.data?.data || res.data || {}
      const list = resData.list || []
      contentList.value = list.map(item => ({
        id: item.boat_id || item.id,
        type: 'boat',
        content: item.content,
        user: { nickname: item.author_nickname || item.nickname },
        status: item.status === 'active' ? 'published' : item.status,
        created_at: item.created_at,
      }))
      pagination.total = resData.total || 0
      return
    }

    if (filters.type === 'stone') {
      const res = await api.getStones(params)
      const resData = res.data?.data || res.data || {}
      const list = resData.list || []
      contentList.value = list.map(item => ({
        id: item.stone_id || item.id,
        type: 'stone',
        content: item.content,
        user: { nickname: item.author_nickname || item.nickname },
        status: item.status,
        created_at: item.created_at,
      }))
      pagination.total = resData.total || 0
      return
    }

    // 后端当前未提供统一 contents 路由，默认页在前端合并石头和纸船。
    const mergedPageSize = currentPage * pageSize
    const sharedFilters = {
      status: filters.status,
      keyword: filters.keyword,
    }
    const [stonesRes, boatsRes] = await Promise.all([
      api.getStones({ page: 1, page_size: mergedPageSize, ...sharedFilters }),
      api.getBoats({ page: 1, page_size: mergedPageSize, ...sharedFilters }),
    ])
    const stonesData = stonesRes.data?.data || stonesRes.data || {}
    const boatsData = boatsRes.data?.data || boatsRes.data || {}
    const mergedList = [
      ...(stonesData.list || []).map(item => ({
        id: item.stone_id || item.id,
        type: 'stone',
        content: item.content,
        user: { nickname: item.author_nickname || item.nickname },
        status: item.status,
        created_at: item.created_at,
      })),
      ...(boatsData.list || []).map(item => ({
        id: item.boat_id || item.id,
        type: 'boat',
        content: item.content,
        user: { nickname: item.author_nickname || item.nickname },
        status: item.status === 'active' ? 'published' : item.status,
        created_at: item.created_at,
      })),
    ]

    mergedList.sort((a, b) => String(b.created_at).localeCompare(String(a.created_at)))
    const start = (currentPage - 1) * pageSize
    contentList.value = mergedList.slice(start, start + pageSize)
    pagination.total = Number(stonesData.total || 0) + Number(boatsData.total || 0)
  } catch (e) {
    if (isRequestCanceled(e)) return
    console.error('获取内容列表失败:', e)
    ElMessage.error(getErrorMessage(e, '获取内容列表失败'))
    contentList.value = []
    pagination.total = 0
  } finally {
    loading.value = false
  }
}

/** 打开内容详情弹窗 */
const viewContent = (row: { id: string; type: string; content: string; user?: { nickname: string }; status: string; created_at: string }) => {
  currentContent.value = row
  detailVisible.value = true
}

/**
 * 删除内容，需输入原因（>=5字）。
 * 根据内容类型分别调用 deleteStone / deleteBoat 接口。
 */
const deleteContent = async (row: { id: string; type: string }) => {
  const { value: reason } = await ElMessageBox.prompt('请输入删除原因', '删除内容', {
    confirmButtonText: '确定',
    cancelButtonText: '取消',
    inputPattern: /.{5,}/,
    inputErrorMessage: '删除原因至少需要5个字符',
  }).catch(() => ({ value: null }))

  if (!reason) return

  try {
    if (row.type === 'stone') {
      await api.deleteStone(row.id, reason)
    } else {
      await api.deleteBoat(row.id, reason)
    }
    ElMessage.success('删除成功')
    fetchContent()
  } catch (e) {
    console.error('删除内容失败:', e)
    ElMessage.error(getErrorMessage(e, '删除失败'))
  }
}

onMounted(() => {
  fetchContent()
})
</script>

<style lang="scss" scoped>
.content-page {
  .content-stage-actions {
    margin: 22px 0 18px;
  }

  .content-filter-form {
    :deep(.el-form-item) {
      margin-bottom: 0;
    }
  }

  .content-table-toolbar {
    align-items: flex-start;
  }

  .content-inline-filter {
    justify-content: flex-end;
  }

  .content-table-copy {
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

  .content-copy,
  .content-author,
  .content-time {
    display: grid;
    gap: 6px;
  }

  .content-text {
    margin: 0;
    color: var(--hl-ink);
    line-height: 1.7;
  }

  .content-note,
  .content-author span,
  .content-time span {
    color: var(--hl-ink-soft);
    font-size: 12px;
    line-height: 1.5;
  }

  .content-author strong,
  .content-time strong {
    color: var(--hl-ink);
    font-size: 13px;
    font-weight: 700;
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
  }
}
</style>
