<template>
  <div class="main-layout">
    <div
      v-if="appStore.isGlobalLoading"
      class="global-loading-bar"
    >
      <div class="loading-progress" />
    </div>

    <div class="workspace-shell">
      <header class="workspace-shell__header">
        <button
          class="brand-mark"
          type="button"
          @click="navigateTo('/dashboard')"
        >
          <div class="brand-mark__icon">
            <img
              src="@/assets/logo.svg"
              alt="HeartLake"
            >
          </div>
          <div class="brand-mark__copy">
            <strong>心湖后台</strong>
            <span>HeartLake Admin</span>
          </div>
        </button>

        <nav
          class="top-nav"
          aria-label="主导航"
        >
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
          <div class="shell-search">
            <el-input
              v-model="navSearch"
              clearable
              :prefix-icon="Search"
              placeholder="搜索页面"
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
              <el-avatar :size="38">
                {{ adminInfo.nickname?.charAt(0) || 'A' }}
              </el-avatar>
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

const router = useRouter()
const route = useRoute()
const appStore = useAppStore()
const navSearch = ref('')

const menuItems = [
  { path: '/dashboard', title: '总览' },
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
  '/dashboard': { kicker: '湖面总览', summary: '查看旅人状态、波动趋势和需要优先处理的内容。' },
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

const realtimeStats = reactive({
  onlineCount: 0,
  todayStones: 0,
})

let statsInterval: ReturnType<typeof setInterval> | null = null

const filteredMenuItems = computed(() => {
  const keyword = navSearch.value.trim().toLowerCase()
  if (!keyword) return []

  return menuItems.filter((item) =>
    [item.title, item.path, routeNarrativeMap[item.path]?.summary].join(' ').toLowerCase().includes(keyword)
  ).slice(0, 6)
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

const handleStatsUpdate = (data: { online_count?: number; today_stones?: number }) => {
  if (data) {
    realtimeStats.onlineCount = data.online_count ?? realtimeStats.onlineCount
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
  0% { transform: translateX(-120%); }
  50% { transform: translateX(-10%); }
  100% { transform: translateX(120%); }
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
  padding: 24px;
}

.workspace-shell {
  width: min(1480px, 100%);
  margin: 0 auto;
  min-height: calc(100vh - 40px);
  padding: 22px 26px 30px;
  border-radius: 34px;
  border: 9px solid rgba(255, 255, 255, 0.98);
  background:
    radial-gradient(circle at 84% 84%, rgba(196, 240, 231, 0.4), transparent 18%),
    radial-gradient(circle at 14% 10%, rgba(194, 211, 255, 0.34), transparent 20%),
    linear-gradient(180deg, rgba(232, 241, 255, 0.98), rgba(223, 235, 255, 0.98));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 30px 70px rgba(97, 127, 179, 0.16);
  animation: shell-entrance 420ms ease-out;
}

.workspace-shell__header {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr) auto;
  gap: 22px;
  align-items: center;
  padding-bottom: 20px;
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
  width: 36px;
  height: 36px;
  display: grid;
  place-items: center;
  border-radius: 999px;
  background: rgba(117, 205, 187, 0.14);

  img {
    width: 20px;
    height: 20px;
  }
}

.brand-mark__copy {
  display: grid;
  gap: 2px;
  text-align: left;

  strong {
    color: var(--hl-ink);
    font-size: 15px;
    font-weight: 700;
  }

  span {
    color: var(--hl-ink-soft);
    font-size: 11px;
    letter-spacing: 0.08em;
  }
}

.top-nav {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
  min-width: 0;
  overflow-x: auto;
  padding: 4px 6px;
  border-radius: 999px;
  background: rgba(233, 241, 255, 0.56);
}

.top-nav__item {
  flex: 0 0 auto;
  min-height: 34px;
  padding: 0 14px;
  border: none;
  border-radius: 999px;
  background: transparent;
  color: #273149;
  font-size: 13px;
  font-weight: 500;
  cursor: pointer;
  transition: var(--m3-transition);

  &:hover {
    background: rgba(255, 255, 255, 0.48);
  }

  &.is-active {
    color: var(--hl-ink);
    background: rgba(255, 255, 255, 0.9);
    box-shadow: 0 8px 16px rgba(111, 139, 187, 0.08);
  }
}

.workspace-shell__tools {
  display: flex;
  align-items: center;
  gap: 10px;
  position: relative;
}

.shell-search {
  position: relative;
  width: min(278px, 22vw);

  :deep(.el-input__wrapper) {
    min-height: 42px;
    border-radius: 999px !important;
    background: rgba(231, 240, 253, 0.78) !important;
    border: 1px solid rgba(164, 183, 224, 0.28);
    box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.86) !important;
  }

  :deep(.el-input__inner) {
    font-size: 13px;
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
  width: 42px;
  height: 42px;
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
  min-height: 0;
  margin-top: 6px;
}

.main-content :deep(.ops-page),
.main-content :deep(.dashboard-bank),
.main-content :deep(.edge-workbench) {
  animation: page-rise 460ms ease-out both;
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
    padding: 10px;
  }

  .workspace-shell {
    min-height: calc(100vh - 20px);
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
