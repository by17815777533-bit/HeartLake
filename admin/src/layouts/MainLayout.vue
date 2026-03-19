<template>
  <div class="main-layout">
    <div
      v-if="appStore.isGlobalLoading"
      class="global-loading-bar"
    >
      <div class="loading-progress" />
    </div>

    <div class="main-shell">
      <aside class="shell-rail">
        <button
          class="shell-rail__brand"
          type="button"
          @click="navigateTo('/dashboard')"
        >
          <div class="brand-orb">
            <img
              src="@/assets/logo.svg"
              alt="HeartLake"
            >
          </div>
          <div class="brand-copy">
            <strong>心湖</strong>
            <span>Admin</span>
          </div>
        </button>

        <nav
          class="rail-nav"
          aria-label="主导航"
        >
          <el-tooltip
            v-for="item in menuItems"
            :key="item.path"
            :content="item.title"
            placement="right"
          >
            <button
              class="rail-nav__button"
              :class="{ 'is-active': route.path === item.path }"
              type="button"
              @click="navigateTo(item.path)"
            >
              <el-icon><component :is="item.icon" /></el-icon>
            </button>
          </el-tooltip>
        </nav>

        <div class="shell-rail__footer">
          <span>{{ currentTime }}</span>
          <strong>{{ realtimeStats.onlineCount }} 在线</strong>
          <small>今日投石 {{ realtimeStats.todayStones }}</small>
        </div>
      </aside>

      <section class="shell-workspace">
        <header class="shell-header">
          <div class="shell-header__intro">
            <span class="shell-header__eyebrow">{{ currentRouteMeta.kicker }}</span>
            <h1>{{ shellGreeting }}，{{ adminInfo.nickname || '管理员' }}</h1>
            <p>{{ currentDateLabel }} · {{ currentRouteMeta.summary }}</p>
          </div>

          <div class="shell-header__tools">
            <div class="shell-search">
              <el-input
                v-model="navSearch"
                clearable
                :prefix-icon="Search"
                placeholder="搜索页面..."
                @keyup.enter="openFirstSearchResult"
              />

              <div
                v-if="navSearch.trim()"
                class="shell-search__results"
              >
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

                <div
                  v-if="!filteredMenuItems.length"
                  class="shell-search__empty"
                >
                  没有匹配的入口
                </div>
              </div>
            </div>

            <button
              class="header-icon-btn"
              type="button"
              aria-label="打开求助处理"
              @click="navigateTo('/reports')"
            >
              <el-icon><Bell /></el-icon>
            </button>

            <el-tooltip
              :content="appStore.isDark ? '切换亮色模式' : '切换暗色模式'"
              placement="bottom"
            >
              <button
                class="header-icon-btn"
                type="button"
                @click="appStore.toggleDark()"
              >
                <el-icon>
                  <Sunny v-if="appStore.isDark" />
                  <Moon v-else />
                </el-icon>
              </button>
            </el-tooltip>

            <el-dropdown @command="handleCommand">
              <button
                class="header-account"
                type="button"
              >
                <el-avatar :size="44">
                  {{ adminInfo.nickname?.charAt(0) || 'A' }}
                </el-avatar>
                <div class="header-account__copy">
                  <span>当前账号</span>
                  <strong>{{ adminInfo.nickname || '管理员' }}</strong>
                </div>
              </button>
              <template #dropdown>
                <el-dropdown-menu>
                  <el-dropdown-item command="profile">
                    个人设置
                  </el-dropdown-item>
                  <el-dropdown-item
                    command="logout"
                    divided
                  >
                    退出登录
                  </el-dropdown-item>
                </el-dropdown-menu>
              </template>
            </el-dropdown>
          </div>
        </header>

        <main class="main-content">
          <router-view v-slot="{ Component }">
            <transition
              name="fade"
              mode="out-in"
            >
              <component :is="Component" />
            </transition>
          </router-view>
        </main>
      </section>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, reactive, onMounted, onUnmounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Bell, Moon, Search, Sunny } from '@element-plus/icons-vue'
import api, { isRequestCanceled } from '@/api'
import websocket from '@/services/websocket'
import { useAppStore } from '@/stores'

const router = useRouter()
const route = useRoute()
const appStore = useAppStore()
const navSearch = ref('')
const currentTime = ref(new Date().toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit', hour12: false }))

/**
 * 主导航菜单。
 * 当前版本管理员权限一致，按固定顺序展示关键业务入口。
 */
const menuItems = [
  { path: '/dashboard', title: '湖面总览', icon: 'DataAnalysis' },
  { path: '/users', title: '旅人关怀', icon: 'User' },
  { path: '/content', title: '石头与纸船', icon: 'Document' },
  { path: '/moderation', title: '温暖守护', icon: 'Check' },
  { path: '/reports', title: '求助处理', icon: 'Bell' },
  { path: '/sensitive-words', title: '风险词典', icon: 'Warning' },
  { path: '/logs', title: '服务记录', icon: 'Tickets' },
  { path: '/settings', title: '系统偏好', icon: 'Setting' },
  { path: '/edge-ai', title: '智能辅助', icon: 'Monitor' },
]

const routeNarrativeMap: Record<string, { kicker: string; summary: string }> = {
  '/dashboard': { kicker: '湖面总览', summary: '先看整体态势，再决定今天的优先处理顺序。' },
  '/users': { kicker: '旅人关怀', summary: '关注账号状态、活跃痕迹与互动规模，判断是否需要介入。' },
  '/content': { kicker: '内容台账', summary: '统一查阅石头和纸船，快速确认需要保留或下线的内容。' },
  '/moderation': { kicker: '人工复核', summary: '把系统判定和人工判断合并，避免误伤也避免漏放。' },
  '/reports': { kicker: '求助处理', summary: '优先梳理待处理举报，让用户反馈有回执、有结果。' },
  '/sensitive-words': { kicker: '风险策略', summary: '用明确规则维护社区边界，让拦截策略足够稳定。' },
  '/logs': { kicker: '服务留痕', summary: '后台动作需要可回放、可交接、可核查。' },
  '/settings': { kicker: '系统偏好', summary: '高权限配置只放在一个入口，减少误改和分散维护。' },
  '/edge-ai': { kicker: '智能辅助', summary: '查看模型状态、脉搏变化和端侧能力的实时表现。' },
}

/** 管理员展示信息。 */
const adminInfo = reactive({
  nickname: appStore.userInfo?.nickname || appStore.userInfo?.username || '管理员',
})

/** 顶部实时统计。 */
const realtimeStats = reactive({
  onlineCount: 0,
  todayStones: 0,
})

let statsInterval: ReturnType<typeof setInterval> | null = null
let clockInterval: ReturnType<typeof setInterval> | null = null

const currentRouteMeta = computed(() => routeNarrativeMap[route.path] || {
  kicker: route.meta.title?.toString() || '管理台',
  summary: '查看当前模块的处理进度与数据变化。',
})

const shellGreeting = computed(() => {
  const hour = new Date().getHours()
  if (hour < 6) return '夜班值守'
  if (hour < 11) return '早安'
  if (hour < 14) return '中午好'
  if (hour < 18) return '下午好'
  return '晚上好'
})

const currentDateLabel = computed(() => new Date().toLocaleDateString('zh-CN', {
  month: 'long',
  day: 'numeric',
  weekday: 'long',
}))

const filteredMenuItems = computed(() => {
  const keyword = navSearch.value.trim().toLowerCase()
  if (!keyword) return []

  return menuItems.filter((item) => {
    const routeMeta = routeNarrativeMap[item.path]
    return [
      item.title,
      item.path,
      routeMeta?.kicker,
      routeMeta?.summary,
    ].join(' ').toLowerCase().includes(keyword)
  }).slice(0, 5)
})

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

/** 拉取管理员资料。 */
const fetchAdminInfo = async () => {
  try {
    const res = await api.getAdminInfo()
    const data = res.data?.data || res.data
    if (data) {
      adminInfo.nickname = data.nickname || data.username || '管理员'
    }
  } catch (e: unknown) {
    if (isRequestCanceled(e)) return
    console.warn('获取管理员信息失败:', (e as Error).message)
  }
}

/** 拉取实时统计。 */
const fetchRealtimeStats = async () => {
  try {
    const res = await api.getRealtimeStats()
    const data = res.data?.data || res.data
    if (data) {
      realtimeStats.onlineCount = data.online_count || data.online_users || 0
      realtimeStats.todayStones = data.today_stones || 0
    }
  } catch (e: unknown) {
    if (isRequestCanceled(e)) return
    console.warn('获取实时统计失败:', (e as Error).message)
  }
}

/** 处理用户下拉菜单行为。 */
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
      router.push('/login')
      ElMessage.success('已退出登录')
    } catch {
      // 取消退出
    }
  } else if (command === 'profile') {
    router.push('/settings')
  }
}

/** 处理 WebSocket 推送的统计更新。 */
const handleStatsUpdate = (data: { online_count?: number; today_stones?: number }) => {
  if (data) {
    realtimeStats.onlineCount = data.online_count ?? realtimeStats.onlineCount
    realtimeStats.todayStones = data.today_stones ?? realtimeStats.todayStones
  }
}

/** 页面可见性切换时管理轮询，避免后台页无效请求。 */
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

  clockInterval = setInterval(() => {
    currentTime.value = new Date().toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit', hour12: false })
  }, 60000)
})

onUnmounted(() => {
  if (statsInterval) {
    clearInterval(statsInterval)
    statsInterval = null
  }
  websocket.off('stats_update', handleStatsUpdate)
  document.removeEventListener('visibilitychange', handleVisibilityChange)
  if (clockInterval) {
    clearInterval(clockInterval)
    clockInterval = null
  }
})
</script>

<style lang="scss" scoped>
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
  top: 18px;
  left: 50%;
  width: min(420px, calc(100vw - 48px));
  height: 4px;
  transform: translateX(-50%);
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.56);
  box-shadow: 0 12px 28px rgba(83, 71, 56, 0.08);
  z-index: 9999;
  overflow: hidden;

  .loading-progress {
    height: 100%;
    width: 42%;
    border-radius: inherit;
    background: linear-gradient(90deg, rgba(207, 191, 167, 0.8), rgba(102, 127, 112, 0.92));
    animation: loading-progress 1.4s ease-in-out infinite;
  }
}

.main-layout {
  min-height: 100vh;
  padding: 22px;
}

.main-shell {
  min-height: calc(100vh - 44px);
  display: grid;
  grid-template-columns: 92px minmax(0, 1fr);
  gap: 22px;
  padding: 20px;
  border-radius: 36px;
  background:
    linear-gradient(180deg, rgba(235, 229, 220, 0.9), rgba(227, 220, 209, 0.96));
  border: 1px solid rgba(121, 110, 95, 0.1);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.78),
    0 34px 72px rgba(83, 71, 56, 0.1);
}

.shell-rail {
  display: flex;
  flex-direction: column;
  gap: 18px;
  padding: 14px 10px;
  border-radius: 30px;
  background: rgba(255, 255, 255, 0.64);
  border: 1px solid rgba(121, 110, 95, 0.08);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.9),
    0 20px 42px rgba(83, 71, 56, 0.08);
}

.shell-rail__brand {
  display: grid;
  place-items: center;
  gap: 8px;
  padding: 8px 0 4px;
  border: none;
  background: transparent;
  cursor: pointer;
}

.brand-orb {
  width: 54px;
  height: 54px;
  display: grid;
  place-items: center;
  border-radius: 18px;
  background:
    radial-gradient(circle at 30% 30%, rgba(164, 189, 172, 0.32), transparent 54%),
    linear-gradient(160deg, rgba(255, 255, 255, 0.92), rgba(239, 232, 223, 0.98));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 16px 26px rgba(83, 71, 56, 0.08);

  img {
    width: 28px;
    height: 28px;
  }
}

.brand-copy {
  display: grid;
  gap: 2px;
  text-align: center;

  strong {
    color: var(--hl-ink);
    font-size: 12px;
    font-weight: 700;
    letter-spacing: 0.08em;
  }

  span {
    color: var(--hl-ink-soft);
    font-size: 10px;
    letter-spacing: 0.16em;
    text-transform: uppercase;
  }
}

.rail-nav {
  display: flex;
  flex: 1;
  flex-direction: column;
  gap: 10px;
  align-items: center;
  padding-top: 8px;
}

.rail-nav__button {
  width: 56px;
  height: 56px;
  display: grid;
  place-items: center;
  border: none;
  border-radius: 20px;
  background: rgba(247, 243, 237, 0.96);
  color: var(--hl-ink-soft);
  cursor: pointer;
  transition: var(--m3-transition);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.9),
    0 10px 20px rgba(83, 71, 56, 0.05);

  .el-icon {
    font-size: 19px;
  }

  &:hover {
    transform: translateY(-2px);
    color: var(--hl-ink);
    box-shadow:
      inset 0 1px 0 rgba(255, 255, 255, 0.96),
      0 16px 28px rgba(83, 71, 56, 0.08);
  }

  &.is-active {
    background: linear-gradient(180deg, #252322, #151413);
    color: #fff8f0;
    box-shadow: 0 20px 34px rgba(37, 35, 34, 0.22);
  }
}

.shell-rail__footer {
  display: grid;
  gap: 4px;
  padding: 14px 10px 6px;
  border-top: 1px solid rgba(121, 110, 95, 0.08);
  text-align: center;

  span,
  small {
    color: var(--hl-ink-soft);
    font-size: 11px;
  }

  strong {
    color: var(--hl-ink);
    font-size: 14px;
    font-weight: 700;
  }
}

.shell-workspace {
  min-width: 0;
  display: flex;
  flex-direction: column;
}

.shell-header {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 20px;
  align-items: start;
  padding: 10px 10px 24px;
}

.shell-header__intro {
  min-width: 0;

  h1 {
    margin: 12px 0 8px;
    color: var(--hl-ink);
    font-size: clamp(30px, 4vw, 42px);
    line-height: 1.05;
    font-weight: 700;
    letter-spacing: -0.03em;
  }

  p {
    color: var(--hl-ink-soft);
    font-size: 15px;
    line-height: 1.8;
  }
}

.shell-header__eyebrow {
  display: inline-flex;
  align-items: center;
  min-height: 30px;
  padding: 0 14px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.64);
  color: var(--hl-ink-soft);
  font-size: 11px;
  letter-spacing: 0.16em;
  text-transform: uppercase;
}

.shell-header__tools {
  position: relative;
  display: flex;
  align-items: center;
  gap: 12px;
}

.shell-search {
  position: relative;
  width: min(360px, 34vw);

  :deep(.el-input__wrapper) {
    min-height: 54px;
    padding-inline: 16px;
    border-radius: 999px !important;
    background: rgba(255, 255, 255, 0.88) !important;
    box-shadow:
      inset 0 1px 0 rgba(255, 255, 255, 0.96),
      0 14px 28px rgba(83, 71, 56, 0.05) !important;
  }
}

.shell-search__results {
  position: absolute;
  top: calc(100% + 10px);
  left: 0;
  right: 0;
  padding: 10px;
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.94);
  border: 1px solid rgba(121, 110, 95, 0.08);
  box-shadow: 0 24px 42px rgba(83, 71, 56, 0.12);
  backdrop-filter: blur(14px);
  z-index: 20;
}

.shell-search__result {
  width: 100%;
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 14px;
  padding: 12px 14px;
  border: none;
  border-radius: 18px;
  background: transparent;
  color: inherit;
  text-align: left;
  cursor: pointer;
  transition: var(--m3-transition);

  strong {
    display: block;
    color: var(--hl-ink);
    font-size: 14px;
    font-weight: 700;
  }

  span,
  small {
    color: var(--hl-ink-soft);
    font-size: 12px;
    line-height: 1.6;
  }

  &:hover {
    background: rgba(242, 237, 231, 0.96);
  }
}

.shell-search__empty {
  padding: 16px 14px;
  color: var(--hl-ink-soft);
  font-size: 13px;
}

.header-icon-btn,
.header-account {
  flex: 0 0 auto;
  border: none;
  cursor: pointer;
  transition: var(--m3-transition);
}

.header-icon-btn {
  width: 52px;
  height: 52px;
  display: grid;
  place-items: center;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.82);
  color: var(--hl-ink);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 14px 28px rgba(83, 71, 56, 0.06);

  .el-icon {
    font-size: 18px;
  }

  &:hover {
    transform: translateY(-1px);
    background: rgba(248, 244, 238, 0.98);
  }
}

.header-account {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 6px 8px 6px 6px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.82);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 14px 28px rgba(83, 71, 56, 0.06);

  &:hover {
    transform: translateY(-1px);
    background: rgba(248, 244, 238, 0.98);
  }

  :deep(.el-avatar) {
    background: linear-gradient(180deg, #70877a, #52655b);
    color: #fff8f0;
    font-weight: 700;
    box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.3);
  }
}

.header-account__copy {
  display: grid;
  gap: 3px;
  text-align: left;

  span {
    color: var(--hl-ink-soft);
    font-size: 11px;
    line-height: 1;
  }

  strong {
    color: var(--hl-ink);
    font-size: 14px;
    line-height: 1.1;
  }
}

.main-content {
  flex: 1;
  overflow-y: auto;
  padding: 0 10px 10px;
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.18s ease, transform 0.18s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
  transform: translateY(8px);
}

@media (max-width: 1120px) {
  .shell-header {
    grid-template-columns: 1fr;
  }

  .shell-header__tools {
    flex-wrap: wrap;
  }

  .shell-search {
    width: min(100%, 420px);
  }
}

@media (max-width: 900px) {
  .main-layout {
    padding: 12px;
  }

  .main-shell {
    grid-template-columns: 1fr;
    padding: 16px;
  }

  .shell-rail {
    flex-direction: row;
    align-items: center;
    padding: 12px;
  }

  .shell-rail__brand {
    grid-auto-flow: column;
    padding: 0;
  }

  .rail-nav {
    flex-direction: row;
    justify-content: flex-start;
    overflow-x: auto;
    padding-top: 0;
  }

  .shell-rail__footer {
    display: none;
  }

  .main-content {
    padding: 0 2px 4px;
  }
}

@media (max-width: 640px) {
  .shell-header__intro h1 {
    font-size: 28px;
  }

  .header-account__copy {
    display: none;
  }

  .shell-header__tools {
    gap: 10px;
  }
}
</style>
