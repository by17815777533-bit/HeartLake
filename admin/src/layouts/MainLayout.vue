<template>
  <div class="main-layout" :class="{ 'is-sidebar-collapsed': sidebarCollapsed }">
    <div v-if="appStore.isGlobalLoading" class="global-loading-bar">
      <div class="loading-progress" />
    </div>

    <aside class="ops-sidebar" aria-label="后台导航">
      <div class="ops-sidebar__brand">
        <button class="brand-mark" type="button" @click="navigateTo(ADMIN_HOME_PATH)">
          <span class="brand-mark__icon">
            <img src="@/assets/logo.svg" alt="HeartLake" />
          </span>
          <span class="brand-mark__copy">
            <strong>HeartLake</strong>
            <small>管理后台</small>
          </span>
        </button>

        <el-tooltip :content="sidebarCollapsed ? '展开导航' : '收起导航'" placement="right">
          <button
            class="sidebar-toggle"
            type="button"
            @click="sidebarCollapsed = !sidebarCollapsed"
          >
            <el-icon>
              <Expand v-if="sidebarCollapsed" />
              <Fold v-else />
            </el-icon>
          </button>
        </el-tooltip>
      </div>

      <nav class="ops-nav">
        <section v-for="section in menuSections" :key="section.label" class="ops-nav__section">
          <span class="ops-nav__label">{{ section.label }}</span>
          <button
            v-for="item in section.items"
            :key="item.path"
            type="button"
            class="ops-nav__item"
            :class="{ 'is-active': isActiveRoute(item.path) }"
            :title="sidebarCollapsed ? item.title : item.desc"
            @click="navigateTo(item.path)"
          >
            <el-icon>
              <component :is="item.icon" />
            </el-icon>
            <span class="ops-nav__text">
              <strong>{{ item.title }}</strong>
              <small>{{ item.desc }}</small>
            </span>
            <em v-if="item.badge" class="ops-nav__badge">{{ item.badge }}</em>
          </button>
        </section>
      </nav>

      <div class="ops-sidebar__footer">
        <div class="sidebar-status" :class="{ 'is-warning': Boolean(headerStatusWarning) }">
          <span class="sidebar-status__dot" />
          <div>
            <strong>{{ headerStatusText }}</strong>
            <small>{{ headerStatusWarning || '服务状态正常' }}</small>
          </div>
        </div>

        <div class="sidebar-metrics">
          <article>
            <span>在线</span>
            <strong>{{ Number(realtimeStats.onlineCount || 0) }}</strong>
          </article>
          <article>
            <span>今日投石</span>
            <strong>{{ Number(realtimeStats.todayStones || 0) }}</strong>
          </article>
        </div>
      </div>
    </aside>

    <section class="ops-workbench">
      <header class="ops-topbar">
        <div class="page-identity">
          <span>{{ currentSectionLabel }}</span>
          <h1>{{ currentMenuItem?.title || route.meta.title || '管理后台' }}</h1>
          <p>{{ currentMenuItem?.desc || '统一处理运营、审核、配置与审计任务。' }}</p>
        </div>

        <div class="ops-topbar__tools">
          <div class="shell-search">
            <el-input
              v-model="navSearch"
              clearable
              :prefix-icon="Search"
              placeholder="搜索功能页"
              @keyup.enter="openFirstSearchResult"
            />

            <div v-if="navSearch.trim()" class="shell-search__results">
              <button
                v-for="item in filteredMenuItems"
                :key="item.path"
                type="button"
                class="shell-search__result"
                @click="navigateTo(item.path)"
              >
                <el-icon>
                  <component :is="item.icon" />
                </el-icon>
                <span>
                  <strong>{{ item.title }}</strong>
                  <small>{{ item.desc }}</small>
                </span>
              </button>

              <div v-if="!filteredMenuItems.length" class="shell-search__empty">没有匹配的入口</div>
            </div>
          </div>

          <el-tooltip content="刷新实时统计" placement="bottom">
            <button class="header-icon-btn" type="button" @click="fetchRealtimeStats">
              <el-icon><RefreshRight /></el-icon>
            </button>
          </el-tooltip>

          <el-tooltip content="求助处理" placement="bottom">
            <button class="header-icon-btn" type="button" @click="navigateTo('/reports')">
              <el-icon><Bell /></el-icon>
            </button>
          </el-tooltip>

          <el-tooltip
            :content="appStore.isDark ? '切换亮色模式' : '切换暗色模式'"
            placement="bottom"
          >
            <button class="header-icon-btn" type="button" @click="appStore.toggleDark()">
              <el-icon>
                <Sunny v-if="appStore.isDark" />
                <Moon v-else />
              </el-icon>
            </button>
          </el-tooltip>

          <el-dropdown @command="handleCommand">
            <button class="header-account" type="button">
              <el-avatar :size="34">
                {{ adminInitial }}
              </el-avatar>
              <span>
                <strong>{{ adminInfo.nickname }}</strong>
                <small>{{ appStore.userInfo?.role || 'admin' }}</small>
              </span>
            </button>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="profile">个人设置</el-dropdown-item>
                <el-dropdown-item command="logout" divided>
                  <el-icon><SwitchButton /></el-icon>
                  退出登录
                </el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </header>

      <main class="main-content">
        <router-view v-slot="{ Component }">
          <transition name="fade" mode="out-in">
            <component :is="Component" />
          </transition>
        </router-view>
      </main>
    </section>
  </div>
</template>

<script setup lang="ts">
import {
  computed,
  ref,
  reactive,
  onMounted,
  onUnmounted,
  type Component as VueComponent,
} from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  Bell,
  Cpu,
  DataLine,
  Document,
  Expand,
  Fold,
  HomeFilled,
  Management,
  Moon,
  RefreshRight,
  Search,
  Setting,
  Sunny,
  SwitchButton,
  Tickets,
  User,
  Warning,
} from '@element-plus/icons-vue'
import api, { isRequestCanceled } from '@/api'
import websocket from '@/services/websocket'
import { useAppStore } from '@/stores'
import { ADMIN_HOME_PATH, ADMIN_LOGIN_PATH } from '@/utils/adminRoutes'
import { normalizePayloadRecord } from '@/utils/collectionPayload'

interface MenuItem {
  path: string
  title: string
  desc: string
  icon: VueComponent
  badge?: string
}

interface MenuSection {
  label: string
  items: MenuItem[]
}

const router = useRouter()
const route = useRoute()
const appStore = useAppStore()
const navSearch = ref('')
const sidebarCollapsed = ref(false)

const adminInfo = reactive({
  nickname:
    appStore.userInfo?.nickname?.toString() || appStore.userInfo?.username?.toString() || '管理员',
})
const adminInfoError = ref<string | null>(null)

const realtimeStats = reactive({
  onlineCount: 0,
  todayStones: 0,
})
const realtimeStatsError = ref<string | null>(null)
const hasRealtimeStats = ref(false)

let statsInterval: ReturnType<typeof setInterval> | null = null

const menuSections = computed<MenuSection[]>(() => [
  {
    label: '运营中台',
    items: [
      {
        path: ADMIN_HOME_PATH,
        title: '运营总览',
        desc: '旅人、内容、情绪和实时处理水位',
        icon: HomeFilled,
      },
      {
        path: '/users',
        title: '旅人管理',
        desc: '账户状态、活跃轨迹和个体关怀',
        icon: User,
      },
      {
        path: '/content',
        title: '内容管理',
        desc: '石头、纸船、作者和发布状态台账',
        icon: Document,
      },
    ],
  },
  {
    label: '风控处理',
    items: [
      {
        path: '/moderation',
        title: '内容审核',
        desc: 'AI 预审、人工复核与审核历史',
        icon: Warning,
      },
      {
        path: '/reports',
        title: '求助工单',
        desc: '举报流转、处置备注和回执闭环',
        icon: Tickets,
        badge: Number(realtimeStats.todayStones || 0) > 0 ? '实时' : '',
      },
      {
        path: '/sensitive-words',
        title: '风险词典',
        desc: '词条级别、处置策略和批量维护',
        icon: Management,
      },
    ],
  },
  {
    label: '系统治理',
    items: [
      {
        path: '/edge-ai',
        title: '智能辅助',
        desc: '引擎节点、情绪脉搏和隐私预算',
        icon: Cpu,
      },
      {
        path: '/logs',
        title: '操作审计',
        desc: '管理员动作、来源 IP 和事件链路',
        icon: DataLine,
      },
      {
        path: '/settings',
        title: '系统配置',
        desc: '全站开关、AI 配置、限流和广播',
        icon: Setting,
      },
    ],
  },
])

const flatMenuItems = computed(() => menuSections.value.flatMap((section) => section.items))

const currentMenuItem = computed(() => flatMenuItems.value.find((item) => isActiveRoute(item.path)))

const currentSectionLabel = computed(() => {
  const section = menuSections.value.find((group) =>
    group.items.some((item) => isActiveRoute(item.path)),
  )
  return section?.label || '后台工作台'
})

const filteredMenuItems = computed(() => {
  const keyword = navSearch.value.trim().toLowerCase()
  if (!keyword) return []

  return flatMenuItems.value
    .filter((item) => [item.title, item.path, item.desc].join(' ').toLowerCase().includes(keyword))
    .slice(0, 8)
})

const headerStatusText = computed(() => {
  if (!hasRealtimeStats.value) {
    return '统计待加载'
  }
  return `在线 ${Number(realtimeStats.onlineCount || 0)} · 今日投石 ${Number(realtimeStats.todayStones || 0)}`
})

const headerStatusWarning = computed(() => adminInfoError.value || realtimeStatsError.value)

const adminInitial = computed(() => adminInfo.nickname?.charAt(0).toUpperCase() || 'A')

const isActiveRoute = (path: string) => {
  if (path === ADMIN_HOME_PATH) {
    return route.path === ADMIN_HOME_PATH || route.path === '/'
  }
  return route.path === path || route.path.startsWith(`${path}/`)
}

const navigateTo = (path: string) => {
  navSearch.value = ''
  if (route.path !== path) {
    router.push(path)
  }
}

const openFirstSearchResult = () => {
  const first = filteredMenuItems.value[0]
  if (first) {
    navigateTo(first.path)
  }
}

const fetchAdminInfo = async () => {
  try {
    const res = await api.getAdminInfo()
    const data = normalizePayloadRecord(res.data)
    if (data) {
      adminInfo.nickname = data.nickname?.toString() || data.username?.toString() || '管理员'
      adminInfoError.value = null
    }
  } catch (e: unknown) {
    if (isRequestCanceled(e)) return
    adminInfoError.value = e instanceof Error && e.message ? e.message : '管理员信息未刷新'
  }
}

const fetchRealtimeStats = async () => {
  try {
    const res = await api.getRealtimeStats({ source: 'layout' })
    const data = normalizePayloadRecord(res.data)
    if (data) {
      realtimeStats.onlineCount = Number(data.online_count || data.online_users || 0)
      realtimeStats.todayStones = Number(data.today_stones || 0)
      hasRealtimeStats.value = true
      realtimeStatsError.value = null
    }
  } catch (e: unknown) {
    if (isRequestCanceled(e)) return
    realtimeStatsError.value = e instanceof Error && e.message ? e.message : '实时统计未刷新'
  }
}

const handleCommand = async (command: string) => {
  if (command === 'logout') {
    try {
      await ElMessageBox.confirm('确定要退出登录吗？', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      })
      websocket.disconnect()
      appStore.clearToken()
      router.push(ADMIN_LOGIN_PATH)
      ElMessage.success('已退出登录')
    } catch {
      // 用户取消退出
    }
  } else if (command === 'profile') {
    router.push('/settings')
  }
}

const handleStatsUpdate = (data: {
  online_count?: number
  online_users?: number
  today_stones?: number
}) => {
  if (data) {
    realtimeStats.onlineCount = data.online_count ?? data.online_users ?? realtimeStats.onlineCount
    realtimeStats.todayStones = data.today_stones ?? realtimeStats.todayStones
    hasRealtimeStats.value = true
  }
}

const handleVisibilityChange = () => {
  if (document.hidden) {
    if (statsInterval) {
      clearInterval(statsInterval)
      statsInterval = null
    }
  } else {
    fetchRealtimeStats()
    if (!statsInterval) {
      statsInterval = setInterval(fetchRealtimeStats, 30000)
    }
  }
}

onMounted(() => {
  fetchAdminInfo()
  fetchRealtimeStats()

  statsInterval = setInterval(fetchRealtimeStats, 30000)
  websocket.connect()
  websocket.on('stats_update', handleStatsUpdate)
  document.addEventListener('visibilitychange', handleVisibilityChange)
})

onUnmounted(() => {
  if (statsInterval) {
    clearInterval(statsInterval)
    statsInterval = null
  }
  websocket.off('stats_update', handleStatsUpdate)
  document.removeEventListener('visibilitychange', handleVisibilityChange)
})
</script>

<style scoped lang="scss">
@keyframes loading-progress {
  0% {
    transform: translateX(-120%);
  }
  50% {
    transform: translateX(-10%);
  }
  100% {
    transform: translateX(120%);
  }
}

.global-loading-bar {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  height: 3px;
  overflow: hidden;
  z-index: 9999;
  background: rgba(37, 99, 235, 0.08);

  .loading-progress {
    width: 36%;
    height: 100%;
    background: linear-gradient(90deg, #2563eb, #10b981, #f59e0b);
    animation: loading-progress 1.25s ease-in-out infinite;
  }
}

.main-layout {
  --sidebar-width: 252px;
  min-height: 100vh;
  display: grid;
  grid-template-columns: var(--sidebar-width) minmax(0, 1fr);
  background: #f3f6fa;
  color: #111827;
  font-family:
    'Avenir Next', 'SF Pro Text', 'PingFang SC', 'Hiragino Sans GB', 'Segoe UI', sans-serif;
  font-variant-numeric: tabular-nums;
}

.main-layout.is-sidebar-collapsed {
  --sidebar-width: 64px;
}

.ops-sidebar {
  min-height: 100vh;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr) auto;
  border-right: 1px solid #d7dde6;
  background: #ffffff;
  color: #334155;
}

.ops-sidebar__brand {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  min-height: 60px;
  padding: 10px 10px 10px 14px;
  border-bottom: 1px solid #e2e8f0;
}

.brand-mark {
  min-width: 0;
  display: inline-flex;
  align-items: center;
  gap: 12px;
  border: none;
  background: transparent;
  color: inherit;
  cursor: pointer;
}

.brand-mark__icon {
  width: 32px;
  height: 32px;
  display: grid;
  place-items: center;
  border-radius: 8px;
  background: #eff6ff;
  border: 1px solid #dbeafe;

  img {
    width: 21px;
    height: 21px;
  }
}

.brand-mark__copy {
  display: grid;
  gap: 2px;
  min-width: 0;
  text-align: left;

  strong {
    color: #111827;
    font-size: 14px;
    font-weight: 760;
    letter-spacing: 0;
  }

  small {
    color: #64748b;
    font-size: 11px;
    letter-spacing: 0;
  }
}

.sidebar-toggle,
.header-icon-btn {
  display: grid;
  place-items: center;
  border: 1px solid transparent;
  background: transparent;
  cursor: pointer;
}

.sidebar-toggle {
  flex: 0 0 auto;
  width: 30px;
  height: 30px;
  border-radius: 6px;
  color: #475569;

  &:hover {
    background: #eef2f7;
    color: #2563eb;
  }
}

.ops-nav {
  min-height: 0;
  overflow-y: auto;
  padding: 10px 8px;
}

.ops-nav__section + .ops-nav__section {
  margin-top: 10px;
}

.ops-nav__label {
  display: block;
  padding: 0 8px 6px;
  color: #94a3b8;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0;
}

.ops-nav__item {
  width: 100%;
  min-height: 42px;
  display: grid;
  grid-template-columns: 30px minmax(0, 1fr) auto;
  align-items: center;
  gap: 8px;
  padding: 6px 8px;
  border: none;
  border-radius: 6px;
  background: transparent;
  color: #334155;
  text-align: left;
  cursor: pointer;
  transition:
    background 0.16s ease,
    color 0.16s ease;

  .el-icon {
    width: 30px;
    height: 30px;
    border-radius: 6px;
    background: #f1f5f9;
    color: #64748b;
    font-size: 16px;
  }

  &:hover {
    background: #f1f5f9;
    color: #0f172a;

    .el-icon {
      color: #2563eb;
      background: #eaf2ff;
    }
  }

  &.is-active {
    background: #eaf2ff;
    color: #1d4ed8;
    box-shadow: inset 3px 0 0 #2563eb;

    .el-icon {
      background: #dbeafe;
      color: #1d4ed8;
    }
  }
}

.ops-nav__text {
  min-width: 0;
  display: grid;
  gap: 2px;

  strong,
  small {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    letter-spacing: 0;
  }

  strong {
    font-size: 13px;
    font-weight: 720;
  }

  small {
    color: currentColor;
    opacity: 0.68;
    font-size: 10px;
  }
}

.ops-nav__badge {
  min-width: 30px;
  min-height: 20px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 4px;
  background: #dbeafe;
  color: currentColor;
  font-size: 10px;
  font-style: normal;
  font-weight: 700;
}

.ops-sidebar__footer {
  display: grid;
  gap: 8px;
  padding: 10px 12px;
  border-top: 1px solid #e2e8f0;
  background: #f8fafc;
}

.sidebar-status {
  display: grid;
  grid-template-columns: 10px minmax(0, 1fr);
  gap: 10px;
  align-items: flex-start;
  padding: 8px;
  border: 1px solid #e2e8f0;
  border-radius: 6px;
  background: #ffffff;

  strong,
  small {
    display: block;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  strong {
    color: #111827;
    font-size: 12px;
    font-weight: 720;
  }

  small {
    margin-top: 2px;
    color: #64748b;
    font-size: 11px;
  }

  &.is-warning .sidebar-status__dot {
    background: #f59e0b;
  }
}

.sidebar-status__dot {
  width: 8px;
  height: 8px;
  margin-top: 5px;
  border-radius: 999px;
  background: #22c55e;
}

.sidebar-metrics {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;

  article {
    min-width: 0;
    padding: 8px;
    border: 1px solid #e2e8f0;
    border-radius: 6px;
    background: #ffffff;
  }

  span,
  strong {
    display: block;
  }

  span {
    color: #64748b;
    font-size: 10px;
  }

  strong {
    margin-top: 4px;
    color: #111827;
    font-size: 16px;
    font-weight: 780;
    letter-spacing: 0;
  }
}

.ops-workbench {
  min-width: 0;
  min-height: 100vh;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
}

.ops-topbar {
  min-height: 58px;
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 14px;
  align-items: center;
  padding: 9px 18px;
  border-bottom: 1px solid #d8dee8;
  background: #ffffff;
}

.page-identity {
  min-width: 0;

  span {
    display: block;
    color: #64748b;
    font-size: 12px;
    font-weight: 700;
  }

  h1 {
    margin: 2px 0 1px;
    color: #111827;
    font-size: 18px;
    font-weight: 760;
    line-height: 1.2;
    letter-spacing: 0;
  }

  p {
    max-width: 54rem;
    overflow: hidden;
    color: #6b7280;
    font-size: 11px;
    line-height: 1.4;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
}

.ops-topbar__tools {
  display: flex;
  align-items: center;
  gap: 8px;
  min-width: 0;
}

.shell-search {
  position: relative;
  width: 244px;

  :deep(.el-input__wrapper) {
    min-height: 34px;
    border-radius: 6px !important;
    background: #f8fafc !important;
    border: 1px solid #d8dee8;
    box-shadow: none !important;
  }
}

.shell-search__results {
  position: absolute;
  top: calc(100% + 8px);
  left: 0;
  right: 0;
  z-index: 40;
  padding: 6px;
  border: 1px solid #d8dee8;
  border-radius: 8px;
  background: #ffffff;
  box-shadow: 0 18px 40px rgba(15, 23, 42, 0.14);
}

.shell-search__result {
  width: 100%;
  display: grid;
  grid-template-columns: 32px minmax(0, 1fr);
  gap: 8px;
  align-items: center;
  padding: 8px;
  border: none;
  border-radius: 6px;
  background: transparent;
  color: #111827;
  text-align: left;
  cursor: pointer;

  .el-icon {
    color: #2563eb;
    font-size: 17px;
  }

  strong,
  small {
    display: block;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  strong {
    font-size: 13px;
    font-weight: 720;
  }

  small {
    color: #64748b;
    font-size: 11px;
  }

  &:hover {
    background: #f1f5f9;
  }
}

.shell-search__empty {
  padding: 12px;
  color: #64748b;
  font-size: 13px;
}

.header-icon-btn {
  width: 34px;
  height: 34px;
  border-radius: 6px;
  border-color: #d8dee8;
  background: #ffffff;
  color: #334155;

  &:hover {
    border-color: #2563eb;
    color: #2563eb;
  }
}

.header-account {
  height: 34px;
  display: inline-flex;
  align-items: center;
  gap: 9px;
  padding: 0 10px 0 3px;
  border: 1px solid #d8dee8;
  border-radius: 6px;
  background: #ffffff;
  color: #111827;
  cursor: pointer;

  :deep(.el-avatar) {
    background: #2563eb;
    color: #ffffff;
    font-weight: 720;
  }

  span {
    display: grid;
    gap: 1px;
    text-align: left;
  }

  strong {
    font-size: 12px;
    font-weight: 720;
  }

  small {
    color: #64748b;
    font-size: 10px;
  }
}

.main-content {
  min-width: 0;
  min-height: 0;
  overflow: auto;
  padding: 14px 16px 18px;
}

.main-content :deep(.ops-page),
.main-content :deep(.dashboard-bank),
.main-content :deep(.edge-workbench) {
  animation: page-rise 260ms ease-out both;
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.16s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

@keyframes page-rise {
  from {
    opacity: 0;
    transform: translateY(6px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.main-layout.is-sidebar-collapsed {
  .ops-sidebar__brand {
    padding: 10px;
    justify-content: center;
  }

  .brand-mark__copy,
  .ops-nav__label,
  .ops-nav__text,
  .ops-nav__badge,
  .ops-sidebar__footer,
  .sidebar-toggle {
    display: none;
  }

  .brand-mark {
    justify-content: center;
  }

  .ops-nav {
    padding: 10px;
  }

  .ops-nav__item {
    grid-template-columns: 30px;
    justify-content: center;
    padding: 6px;
  }
}

@media (max-width: 1180px) {
  .main-layout {
    --sidebar-width: 236px;
  }

  .shell-search {
    width: 220px;
  }

  .header-account span {
    display: none;
  }
}

@media (max-width: 900px) {
  .main-layout {
    grid-template-columns: 1fr;
  }

  .ops-sidebar {
    position: sticky;
    top: 0;
    z-index: 30;
    min-height: auto;
    grid-template-rows: auto;
  }

  .ops-sidebar__brand,
  .ops-sidebar__footer {
    display: none;
  }

  .ops-nav {
    display: flex;
    gap: 8px;
    padding: 10px;
    overflow-x: auto;
  }

  .ops-nav__section {
    display: contents;
  }

  .ops-nav__label,
  .ops-nav__text small,
  .ops-nav__badge {
    display: none;
  }

  .ops-nav__item {
    width: auto;
    min-width: 96px;
    grid-template-columns: 28px auto;
    min-height: 42px;
  }

  .ops-nav__text {
    display: block;
  }

  .ops-topbar {
    grid-template-columns: 1fr;
  }

  .ops-topbar__tools {
    justify-content: space-between;
  }

  .shell-search {
    flex: 1 1 auto;
    width: auto;
  }
}
</style>
