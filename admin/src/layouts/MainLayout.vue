<template>
  <div class="main-layout">
    <div v-if="appStore.isGlobalLoading" class="global-loading-bar">
      <div class="loading-progress" />
    </div>

    <div class="workspace-shell">
      <header class="workspace-shell__header">
        <button class="brand-mark" type="button" @click="navigateTo(ADMIN_HOME_PATH)">
          <div class="brand-mark__icon">
            <img src="@/assets/logo.svg" alt="HeartLake" />
          </div>
          <div class="brand-mark__copy">
            <strong>心湖后台</strong>
            <span>值守工作台</span>
          </div>
        </button>

        <nav class="top-nav" aria-label="主导航">
          <button
            v-for="item in menuItems"
            :key="item.path"
            type="button"
            class="top-nav__item"
            :class="{ 'is-active': route.path === item.path }"
            @click="navigateTo(item.path)"
          >
            {{ item.title }}
          </button>
        </nav>

        <div class="workspace-shell__tools">
          <div
            class="header-status"
            :class="{ 'is-warning': Boolean(headerStatusWarning) }"
            :title="headerStatusWarning || headerStatusText"
          >
            <strong>{{ headerStatusText }}</strong>
            <span v-if="headerStatusWarning">{{ headerStatusWarning }}</span>
          </div>

          <div class="shell-search">
            <el-input
              v-model="navSearch"
              clearable
              :prefix-icon="Search"
              placeholder="搜索页面"
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
                <div>
                  <strong>{{ item.title }}</strong>
                  <span>{{ routeNarrativeMap[item.path]?.summary }}</span>
                </div>
                <small>{{ item.path.replace('/', '') || 'dashboard' }}</small>
              </button>

              <div v-if="!filteredMenuItems.length" class="shell-search__empty">没有匹配的入口</div>
            </div>
          </div>

          <button
            class="header-icon-btn"
            type="button"
            aria-label="查看求助处理"
            @click="navigateTo('/reports')"
          >
            <el-icon><Bell /></el-icon>
          </button>

          <button
            class="header-icon-btn"
            type="button"
            aria-label="查看系统偏好"
            @click="navigateTo('/settings')"
          >
            <el-icon><Setting /></el-icon>
          </button>

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
              <el-avatar :size="38">
                {{ adminInfo.nickname?.charAt(0) || 'A' }}
              </el-avatar>
            </button>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="profile"> 个人设置 </el-dropdown-item>
                <el-dropdown-item command="logout" divided> 退出登录 </el-dropdown-item>
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
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, reactive, onMounted, onUnmounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Bell, Moon, Search, Setting, Sunny } from '@element-plus/icons-vue'
import api, { isRequestCanceled } from '@/api'
import websocket from '@/services/websocket'
import { useAppStore } from '@/stores'
import { ADMIN_HOME_PATH, ADMIN_LOGIN_PATH } from '@/utils/adminRoutes'
import { normalizePayloadRecord } from '@/utils/collectionPayload'

const router = useRouter()
const route = useRoute()
const appStore = useAppStore()
const navSearch = ref('')

const menuItems = [
  { path: ADMIN_HOME_PATH, title: '总览' },
  { path: '/users', title: '旅人' },
  { path: '/content', title: '内容' },
  { path: '/moderation', title: '守护' },
  { path: '/reports', title: '求助' },
  { path: '/sensitive-words', title: '词典' },
  { path: '/logs', title: '日志' },
  { path: '/settings', title: '设置' },
  { path: '/edge-ai', title: '智能辅助' },
]

const routeNarrativeMap: Record<string, { kicker: string; summary: string }> = {
  [ADMIN_HOME_PATH]: {
    kicker: '湖面总览',
    summary: '查看旅人状态、波动趋势和需要优先处理的内容。',
  },
  '/users': { kicker: '旅人关怀', summary: '筛查账户状态、活跃记录和个体产出。' },
  '/content': { kicker: '内容台账', summary: '统一查看石头与纸船，确认当前内容水位。' },
  '/moderation': { kicker: '温暖守护', summary: '把系统判断与人工复核放在同一个工作面上。' },
  '/reports': { kicker: '求助处理', summary: '优先处理待办举报，确保反馈和回执有闭环。' },
  '/sensitive-words': { kicker: '风险词典', summary: '维护风控边界，避免策略漂移和漏拦截。' },
  '/logs': { kicker: '服务留痕', summary: '查看后台动作和交接轨迹。' },
  '/settings': { kicker: '系统偏好', summary: '高权限配置、广播和模型参数统一收口。' },
  '/edge-ai': { kicker: '智能辅助', summary: '查看引擎状态、节点能力和推理健康度。' },
}

const adminInfo = reactive({
  nickname: appStore.userInfo?.nickname || appStore.userInfo?.username || '管理员',
})
const adminInfoError = ref<string | null>(null)

const realtimeStats = reactive({
  onlineCount: 0,
  todayStones: 0,
})
const realtimeStatsError = ref<string | null>(null)
const hasRealtimeStats = ref(false)

let statsInterval: ReturnType<typeof setInterval> | null = null

const filteredMenuItems = computed(() => {
  const keyword = navSearch.value.trim().toLowerCase()
  if (!keyword) return []

  return menuItems
    .filter((item) =>
      [item.title, item.path, routeNarrativeMap[item.path]?.summary]
        .join(' ')
        .toLowerCase()
        .includes(keyword),
    )
    .slice(0, 6)
})

const headerStatusText = computed(() => {
  if (!hasRealtimeStats.value) {
    return '实时统计待加载'
  }
  return `在线 ${Number(realtimeStats.onlineCount || 0)} · 今日投石 ${Number(realtimeStats.todayStones || 0)}`
})

const headerStatusWarning = computed(() => adminInfoError.value || realtimeStatsError.value)

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
      adminInfo.nickname = data.nickname || data.username || '管理员'
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
      realtimeStats.onlineCount = data.online_count || data.online_users || 0
      realtimeStats.todayStones = data.today_stones || 0
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
      // 取消退出
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
  top: 16px;
  left: 50%;
  width: min(380px, calc(100vw - 40px));
  height: 5px;
  transform: translateX(-50%);
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.72);
  overflow: hidden;
  z-index: 9999;
  box-shadow: 0 12px 24px rgba(76, 103, 148, 0.14);

  .loading-progress {
    width: 40%;
    height: 100%;
    border-radius: inherit;
    background: linear-gradient(90deg, #8cb2ff, #99ddd5);
    animation: loading-progress 1.4s ease-in-out infinite;
  }
}

.main-layout {
  min-height: 100vh;
  padding: 14px;
  background:
    radial-gradient(circle at 50% 0%, rgba(223, 236, 255, 0.92), transparent 44%),
    linear-gradient(180deg, #dbe8fd 0%, #d0e0f8 100%);
}

.workspace-shell {
  width: 100%;
  margin: 0 auto;
  min-height: calc(100vh - 28px);
  display: flex;
  flex-direction: column;
  overflow: hidden;
  padding: 18px 20px 20px;
  border-radius: 32px;
  border: 7px solid rgba(255, 255, 255, 0.98);
  background:
    radial-gradient(circle at 84% 80%, rgba(214, 240, 233, 0.32), transparent 18%),
    radial-gradient(circle at 14% 12%, rgba(203, 219, 255, 0.28), transparent 22%),
    linear-gradient(180deg, rgba(237, 244, 255, 0.98), rgba(229, 238, 255, 0.98));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 30px 64px rgba(97, 127, 179, 0.14);
  animation: shell-entrance 420ms ease-out;
  -webkit-font-smoothing: antialiased;
  font-family:
    'Avenir Next', 'SF Pro Text', 'PingFang SC', 'Hiragino Sans GB', 'Segoe UI', sans-serif;
}

.workspace-shell__header {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr) auto;
  gap: 14px;
  align-items: center;
  padding-bottom: 14px;
  flex: 0 0 auto;
}

.brand-mark {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  border: none;
  background: transparent;
  padding: 0;
  cursor: pointer;
}

.brand-mark__icon {
  width: 34px;
  height: 34px;
  display: grid;
  place-items: center;
  border-radius: 999px;
  background: rgba(117, 205, 187, 0.18);

  img {
    width: 22px;
    height: 22px;
  }
}

.brand-mark__copy {
  display: grid;
  gap: 2px;
  text-align: left;

  strong {
    color: var(--hl-ink);
    font-size: 14px;
    font-weight: 700;
  }

  span {
    color: var(--hl-ink-soft);
    font-size: 10px;
    letter-spacing: 0.08em;
  }
}

.top-nav {
  display: flex;
  align-items: center;
  justify-content: flex-start;
  gap: 6px;
  min-width: 0;
  overflow-x: auto;
  padding: 0 2px;
  background: transparent;
}

.top-nav__item {
  flex: 0 0 auto;
  min-height: 30px;
  padding: 0 9px;
  border: none;
  border-radius: 14px;
  background: transparent;
  color: #273149;
  font-size: 12px;
  font-weight: 650;
  cursor: pointer;
  transition: var(--m3-transition);

  &:hover {
    color: var(--hl-ink);
  }

  &.is-active {
    color: var(--hl-ink);
    background: rgba(255, 255, 255, 0.62);
    box-shadow: 0 6px 12px rgba(111, 139, 187, 0.08);
  }
}

.workspace-shell__tools {
  display: flex;
  align-items: center;
  gap: 7px;
  position: relative;
}

.header-status {
  display: grid;
  gap: 2px;
  min-width: 168px;
  padding: 9px 12px;
  border-radius: 18px;
  border: 1px solid rgba(132, 160, 202, 0.18);
  background: rgba(255, 255, 255, 0.72);
  color: #4f6670;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.86);

  strong {
    font-size: 12px;
    font-weight: 700;
    line-height: 1.2;
  }

  span {
    font-size: 11px;
    line-height: 1.3;
    color: #8a5a2b;
  }

  &.is-warning {
    border-color: rgba(198, 138, 73, 0.3);
    background: rgba(255, 244, 230, 0.86);
  }
}

.shell-search {
  position: relative;
  width: min(222px, 17vw);

  :deep(.el-input__wrapper) {
    min-height: 38px;
    border-radius: 999px !important;
    background: rgba(240, 246, 255, 0.9) !important;
    border: 1px solid rgba(164, 183, 224, 0.24);
    box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.86) !important;
  }

  :deep(.el-input__inner) {
    font-size: 12px;
  }
}

.shell-search__results {
  position: absolute;
  top: calc(100% + 10px);
  left: 0;
  right: 0;
  padding: 10px;
  border-radius: 22px;
  background: rgba(248, 252, 255, 0.96);
  border: 1px solid rgba(137, 165, 207, 0.14);
  box-shadow: 0 24px 50px rgba(96, 121, 169, 0.18);
  z-index: 20;
}

.shell-search__result {
  width: 100%;
  display: flex;
  justify-content: space-between;
  gap: 12px;
  padding: 12px 14px;
  border: none;
  border-radius: 16px;
  background: transparent;
  color: inherit;
  text-align: left;
  cursor: pointer;
  transition: var(--m3-transition);

  strong {
    display: block;
    color: var(--hl-ink);
    font-size: 13px;
  }

  span,
  small {
    color: var(--hl-ink-soft);
    font-size: 12px;
    line-height: 1.5;
  }

  &:hover {
    background: rgba(233, 242, 255, 0.86);
  }
}

.shell-search__empty {
  padding: 12px 14px;
  color: var(--hl-ink-soft);
  font-size: 13px;
}

.header-icon-btn,
.header-account {
  flex: 0 0 auto;
  width: 38px;
  height: 38px;
  display: grid;
  place-items: center;
  border: none;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.82);
  color: var(--hl-ink);
  cursor: pointer;
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 10px 18px rgba(112, 137, 183, 0.08);
  transition: var(--m3-transition);

  &:hover {
    transform: translateY(-1px);
    background: rgba(255, 255, 255, 0.96);
  }
}

.header-account {
  overflow: hidden;

  :deep(.el-avatar) {
    background: linear-gradient(180deg, #8db2ff, #7ea0ef);
    color: #ffffff;
    font-weight: 700;
    text-transform: uppercase;
  }
}

.main-content {
  flex: 1 1 auto;
  min-height: 0;
  margin-top: 0;
  overflow: auto;
  padding-right: 2px;
}

.main-content :deep(.ops-page),
.main-content :deep(.dashboard-bank),
.main-content :deep(.edge-workbench) {
  animation: page-rise 460ms ease-out both;
}

@media (max-width: 1180px) and (min-width: 961px) {
  .main-layout {
    padding: 14px;
  }

  .workspace-shell {
    width: 100%;
    min-height: calc(100vh - 28px);
    padding: 16px 16px 18px;
    border-width: 6px;
    border-radius: 28px;
  }

  .workspace-shell__header {
    grid-template-columns: auto minmax(0, 1fr) auto;
    gap: 10px;
    padding-bottom: 12px;
  }

  .brand-mark {
    gap: 8px;
  }

  .brand-mark__icon {
    width: 30px;
    height: 30px;

    img {
      width: 18px;
      height: 18px;
    }
  }

  .brand-mark__copy {
    strong {
      font-size: 13px;
    }

    span {
      display: none;
    }
  }

  .top-nav {
    gap: 4px;
  }

  .top-nav__item {
    min-height: 28px;
    padding: 0 8px;
    font-size: 11px;
  }

  .workspace-shell__tools {
    gap: 6px;
  }

  .shell-search {
    width: min(178px, 19vw);

    :deep(.el-input__wrapper) {
      min-height: 34px;
    }

    :deep(.el-input__inner) {
      font-size: 11px;
    }
  }

  .header-icon-btn,
  .header-account {
    width: 34px;
    height: 34px;
  }
}

.fade-enter-active,
.fade-leave-active {
  transition:
    opacity 0.18s ease,
    transform 0.18s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
  transform: translateY(8px);
}

@media (max-height: 820px) and (min-width: 1121px) {
  .main-layout {
    padding: 12px;
  }

  .workspace-shell {
    width: 100%;
    min-height: calc(100vh - 24px);
    padding: 16px 18px 18px;
    border-radius: 30px;
  }

  .workspace-shell__header {
    gap: 14px;
    padding-bottom: 12px;
  }

  .brand-mark__icon {
    width: 32px;
    height: 32px;

    img {
      width: 20px;
      height: 20px;
    }
  }

  .brand-mark__copy strong {
    font-size: 14px;
  }

  .top-nav {
    gap: 6px;
  }

  .top-nav__item {
    min-height: 28px;
    padding: 0 9px;
    font-size: 12px;
  }

  .shell-search {
    width: min(204px, 16vw);

    :deep(.el-input__wrapper) {
      min-height: 36px;
    }

    :deep(.el-input__inner) {
      font-size: 12px;
    }
  }

  .header-icon-btn,
  .header-account {
    width: 36px;
    height: 36px;
  }
}

@media (max-height: 740px) and (min-width: 1181px) {
  .main-layout {
    padding: 10px;
  }

  .workspace-shell {
    min-height: calc(100vh - 20px);
    padding: 14px 16px 16px;
    border-width: 6px;
    border-radius: 28px;
  }

  .workspace-shell__header {
    gap: 12px;
    padding-bottom: 10px;
  }

  .brand-mark__icon {
    width: 30px;
    height: 30px;

    img {
      width: 18px;
      height: 18px;
    }
  }

  .brand-mark__copy strong {
    font-size: 13px;
  }

  .brand-mark__copy span,
  .top-nav__item {
    font-size: 11px;
  }

  .shell-search {
    width: min(194px, 15vw);

    :deep(.el-input__wrapper) {
      min-height: 34px;
    }

    :deep(.el-input__inner) {
      font-size: 11px;
    }
  }

  .header-icon-btn,
  .header-account {
    width: 34px;
    height: 34px;
  }
}

@media (max-width: 960px) {
  .workspace-shell__header {
    grid-template-columns: 1fr;
  }

  .workspace-shell__tools {
    justify-content: flex-start;
    flex-wrap: wrap;
  }

  .top-nav {
    justify-content: flex-start;
  }

  .shell-search {
    width: min(100%, 320px);
  }
}

@media (max-width: 760px) {
  .main-layout {
    padding: 12px;
  }

  .workspace-shell {
    min-height: calc(100vh - 24px);
    padding: 16px;
    border-width: 6px;
    border-radius: 28px;
  }
}

@keyframes shell-entrance {
  from {
    opacity: 0;
    transform: translateY(10px) scale(0.992);
  }
  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}

@keyframes page-rise {
  from {
    opacity: 0;
    transform: translateY(12px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}
</style>
