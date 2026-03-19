<template>
  <div class="main-layout">
    <!-- 全局加载进度条 -->
    <div
      v-if="appStore.isGlobalLoading"
      class="global-loading-bar"
    >
      <div class="loading-progress" />
    </div>

    <!-- 侧边栏 -->
    <el-aside
      :width="isCollapsed ? '64px' : '220px'"
      class="sidebar"
    >
      <div class="logo">
        <div class="logo-badge">
          <img
            src="@/assets/logo.svg"
            alt="HeartLake"
          >
        </div>
        <div
          v-if="!isCollapsed"
          class="logo-copy"
        >
          <span class="logo-text">心湖管理</span>
          <span class="logo-sub">服务后台</span>
        </div>
      </div>

      <div
        v-if="!isCollapsed"
        class="sidebar-caption"
      >
        常用入口
      </div>

      <nav aria-label="主导航">
        <el-menu
          :default-active="$route.path"
          :collapse="isCollapsed"
          :collapse-transition="false"
          router
          class="sidebar-menu"
        >
          <el-menu-item
            v-for="item in menuItems"
            :key="item.path"
            :index="item.path"
          >
            <el-icon><component :is="item.icon" /></el-icon>
            <template #title>
              {{ item.title }}
            </template>
          </el-menu-item>
        </el-menu>
      </nav>

      <section
        v-if="!isCollapsed"
        class="sidebar-insight"
      >
        <span class="sidebar-insight__eyebrow">值守提示</span>
        <strong>{{ currentRouteMeta.kicker }}</strong>
        <p>{{ currentRouteMeta.summary }}</p>
        <div class="sidebar-insight__meta">
          <span>在线 {{ realtimeStats.onlineCount }}</span>
          <span>今日投石 {{ realtimeStats.todayStones }}</span>
        </div>
      </section>

      <div
        class="collapse-btn"
        @click="isCollapsed = !isCollapsed"
      >
        <el-icon>
          <Fold v-if="!isCollapsed" />
          <Expand v-else />
        </el-icon>
      </div>
    </el-aside>

    <!-- 主内容区 -->
    <el-container class="main-container">
      <!-- 顶部栏 -->
      <el-header class="header">
        <div class="header-left">
          <span class="header-badge">在线</span>
          <div class="breadcrumb-stack">
            <el-breadcrumb separator="/">
              <el-breadcrumb-item :to="{ path: '/' }">
                心湖
              </el-breadcrumb-item>
              <el-breadcrumb-item>{{ $route.meta.title }}</el-breadcrumb-item>
            </el-breadcrumb>
            <span class="header-caption">{{ currentRouteMeta.summary }}</span>
          </div>
        </div>

        <div class="header-right">
          <div class="signal-board">
            <article class="signal-chip is-sage">
              <span>在线旅人</span>
              <strong>{{ realtimeStats.onlineCount }}</strong>
            </article>
            <article class="signal-chip is-lake">
              <span>今日投石</span>
              <strong>{{ realtimeStats.todayStones }}</strong>
            </article>
            <article class="signal-chip is-amber">
              <span>值守时刻</span>
              <strong>{{ currentTime }}</strong>
            </article>
          </div>

          <!-- 暗色模式切换 -->
          <el-tooltip
            :content="appStore.isDark ? '切换亮色模式' : '切换暗色模式'"
            placement="bottom"
          >
            <el-button
              circle
              size="small"
              class="dark-toggle"
              @click="appStore.toggleDark()"
            >
              <el-icon :size="16">
                <Sunny v-if="appStore.isDark" />
                <Moon v-else />
              </el-icon>
            </el-button>
          </el-tooltip>

          <!-- 用户信息 -->
          <el-dropdown @command="handleCommand">
            <div class="user-info">
              <el-avatar :size="32">
                {{ adminInfo.nickname?.charAt(0) || 'A' }}
              </el-avatar>
              <div class="user-copy">
                <span class="user-label">当前账号</span>
                <span class="username">{{ adminInfo.nickname || '管理员' }}</span>
              </div>
            </div>
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
      </el-header>

      <!-- 内容区 -->
      <el-main class="main-content">
        <router-view v-slot="{ Component }">
          <transition
            name="fade"
            mode="out-in"
          >
            <component :is="Component" />
          </transition>
        </router-view>
      </el-main>
    </el-container>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, reactive, onMounted, onUnmounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import api, { isRequestCanceled } from '@/api'
import websocket from '@/services/websocket'
import { useAppStore } from '@/stores'

const router = useRouter()
const route = useRoute()
const appStore = useAppStore()
const isCollapsed = ref(false)
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
    // 静默失败，不影响页面展示
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
  top: 0;
  left: 0;
  right: 0;
  height: 2px;
  background: rgba(24, 36, 47, 0.08);
  z-index: 9999;
  overflow: hidden;

  .loading-progress {
    height: 100%;
    width: 40%;
    background: linear-gradient(90deg, var(--m3-secondary), var(--m3-primary));
    animation: loading-progress 1.4s ease-in-out infinite;
  }
}

.main-layout {
  display: flex;
  height: 100vh;
  width: 100vw;
  overflow: hidden;
  position: relative;
  background: transparent;

  &::before {
    content: '';
    position: absolute;
    inset: 0;
    pointer-events: none;
    background:
      radial-gradient(520px 280px at 0% 0%, rgba(182, 122, 66, 0.12), transparent 72%),
      radial-gradient(620px 360px at 100% 100%, rgba(17, 62, 74, 0.12), transparent 74%);
  }
}

.sidebar {
  background:
    linear-gradient(180deg, rgba(11, 20, 26, 0.97), rgba(8, 16, 21, 0.99)),
    var(--hl-paper-deep);
  border-right: 1px solid rgba(255, 255, 255, 0.06);
  display: flex;
  flex-direction: column;
  transition: var(--m3-transition);
  position: relative;
  z-index: 10;
  box-shadow: 20px 0 36px rgba(0, 0, 0, 0.18);

  .logo {
    min-height: 80px;
    display: flex;
    align-items: center;
    gap: 14px;
    padding: 18px 18px 16px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.08);

    .logo-badge {
      width: 40px;
      height: 40px;
      display: grid;
      place-items: center;
      border-radius: 14px;
      background: linear-gradient(135deg, rgba(182, 122, 66, 0.18), rgba(132, 184, 199, 0.18));
      border: 1px solid rgba(255, 255, 255, 0.12);

      img {
        width: 24px;
        height: 24px;
      }
    }

    .logo-copy {
      display: flex;
      flex-direction: column;
      min-width: 0;
    }

    .logo-text {
      font-family: var(--hl-font-display);
      font-size: 18px;
      font-weight: 600;
      color: #f6eee2;
      letter-spacing: 0.06em;
    }

    .logo-sub {
      font-family: var(--hl-font-mono);
      font-size: 11px;
      letter-spacing: 0.16em;
      text-transform: uppercase;
      color: rgba(255, 255, 255, 0.52);
    }
  }

  .sidebar-caption {
    padding: 14px 20px 4px;
    font-family: var(--hl-font-mono);
    font-size: 11px;
    letter-spacing: 0.14em;
    text-transform: uppercase;
    color: rgba(255, 255, 255, 0.36);
  }

  nav[aria-label="主导航"] {
    flex: 1;
    overflow-y: auto;
  }

  .sidebar-menu {
    border: none;
    background: transparent;
    padding: 10px 10px 14px;

    &::-webkit-scrollbar {
      width: 4px;
    }

    &::-webkit-scrollbar-thumb {
      background: rgba(255, 255, 255, 0.12);
      border-radius: var(--m3-shape-sm);
    }

    :deep(.el-menu-item) {
      height: 46px;
      margin: 6px 0;
      border-radius: 14px;
      color: rgba(255, 255, 255, 0.72);
      border: 1px solid transparent;
      transition: var(--m3-transition);
      font-weight: 500;

      &:hover {
        background: rgba(255, 255, 255, 0.06);
        color: #fff8f1;
        border-color: rgba(255, 255, 255, 0.08);
      }

      &.is-active {
        background: linear-gradient(135deg, rgba(132, 184, 199, 0.18), rgba(182, 122, 66, 0.16));
        color: #fdf8f0;
        font-weight: 600;
        border-color: rgba(132, 184, 199, 0.18);
        box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.06);
      }

      .el-icon {
        color: inherit;
        font-size: 16px;
      }
    }
  }

  .sidebar-insight {
    margin: 0 14px 14px;
    padding: 16px 16px 14px;
    border-radius: 22px;
    border: 1px solid rgba(255, 255, 255, 0.1);
    background:
      radial-gradient(circle at top right, rgba(208, 161, 98, 0.18), transparent 42%),
      linear-gradient(180deg, rgba(255, 255, 255, 0.05), rgba(255, 255, 255, 0.02));
    box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.06);

    &__eyebrow {
      display: inline-flex;
      min-height: 26px;
      align-items: center;
      padding: 0 10px;
      border-radius: 999px;
      background: rgba(255, 255, 255, 0.08);
      color: rgba(255, 255, 255, 0.6);
      font-family: var(--hl-font-mono);
      font-size: 10px;
      letter-spacing: 0.14em;
      text-transform: uppercase;
    }

    strong {
      display: block;
      margin-top: 14px;
      color: #f8f2ea;
      font-family: var(--hl-font-display);
      font-size: 24px;
      letter-spacing: 0.04em;
    }

    p {
      margin-top: 10px;
      color: rgba(255, 255, 255, 0.66);
      line-height: 1.75;
      font-size: 13px;
    }

    &__meta {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      margin-top: 14px;

      span {
        display: inline-flex;
        align-items: center;
        min-height: 28px;
        padding: 0 10px;
        border-radius: 999px;
        background: rgba(255, 255, 255, 0.06);
        color: rgba(255, 255, 255, 0.78);
        font-size: 12px;
      }
    }
  }

  .collapse-btn {
    height: 54px;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    color: rgba(255, 255, 255, 0.7);
    border-top: 1px solid rgba(255, 255, 255, 0.08);
    background: rgba(255, 255, 255, 0.02);
    transition: var(--m3-transition);

    &:hover {
      background: rgba(255, 255, 255, 0.06);
      color: #fff8f1;
    }
  }
}

.main-container {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  position: relative;
  z-index: 1;
}

.header {
  height: 72px;
  background: rgba(247, 250, 251, 0.82);
  border-bottom: 1px solid var(--hl-line);
  backdrop-filter: blur(14px);
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 26px;
  box-shadow: 0 18px 36px rgba(24, 36, 47, 0.06);
  position: relative;
  z-index: 5;

  .header-left {
    display: flex;
    align-items: center;
    gap: 14px;

    .header-badge {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      min-width: 56px;
      height: 28px;
      padding: 0 10px;
      border-radius: 999px;
      border: 1px solid rgba(17, 62, 74, 0.16);
      background: rgba(17, 62, 74, 0.08);
      color: var(--m3-primary);
      font-family: var(--hl-font-mono);
      font-size: 11px;
      letter-spacing: 0.14em;
      text-transform: uppercase;
    }

    .breadcrumb-stack {
      display: flex;
      flex-direction: column;
      gap: 4px;
    }

    .header-caption {
      font-size: 12px;
      color: var(--hl-ink-soft);
      letter-spacing: 0.04em;
    }

    :deep(.el-breadcrumb) {
      .el-breadcrumb__item {
        .el-breadcrumb__inner {
          color: var(--hl-ink-soft);
          transition: var(--m3-transition);

          &:hover {
            color: var(--m3-secondary);
          }

          &.is-link {
            font-weight: 600;
          }
        }

        &:last-child .el-breadcrumb__inner {
          color: var(--hl-ink);
        }
      }

      .el-breadcrumb__separator {
        color: rgba(24, 36, 47, 0.28);
      }
    }
  }

  .header-right {
    display: flex;
    align-items: center;
    gap: 16px;

    .signal-board {
      display: flex;
      gap: 12px;
    }

    .signal-chip {
      min-width: 108px;
      padding: 10px 14px;
      border-radius: 18px;
      border: 1px solid rgba(24, 36, 47, 0.08);
      background: rgba(255, 255, 255, 0.64);
      box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.42);

      span {
        display: block;
        font-size: 11px;
        letter-spacing: 0.1em;
        text-transform: uppercase;
        color: var(--hl-ink-soft);
      }

      strong {
        display: block;
        margin-top: 6px;
        font-family: var(--hl-font-display);
        font-size: 24px;
        line-height: 1;
        color: var(--hl-ink);
      }

      &.is-sage strong {
        color: #3f7257;
      }

      &.is-lake strong {
        color: #123f4b;
      }

      &.is-amber strong {
        color: #a56d37;
      }
    }

    .dark-toggle {
      background: rgba(255, 255, 255, 0.64);
      border-color: rgba(24, 36, 47, 0.1);
      color: var(--hl-ink);

      &:hover {
        background: rgba(17, 62, 74, 0.08);
        border-color: rgba(17, 62, 74, 0.18);
      }
    }

    .user-info {
      display: flex;
      align-items: center;
      gap: 12px;
      padding: 8px 14px;
      border-radius: 16px;
      background: rgba(255, 255, 255, 0.6);
      border: 1px solid rgba(24, 36, 47, 0.08);
      cursor: pointer;
      transition: var(--m3-transition);

      &:hover {
        background: rgba(255, 255, 255, 0.86);
      }

      :deep(.el-avatar) {
        background: linear-gradient(135deg, var(--m3-primary), var(--m3-secondary));
        border: none;
        color: #fffaf3;
        font-weight: 700;
      }

      .user-copy {
        display: flex;
        flex-direction: column;
        min-width: 0;
      }

      .user-label {
        font-size: 11px;
        line-height: 1;
        text-transform: uppercase;
        letter-spacing: 0.12em;
        color: rgba(24, 36, 47, 0.48);
      }

      .username {
        color: var(--hl-ink);
        font-weight: 600;
        font-size: 14px;
      }
    }
  }
}

.main-content {
  flex: 1;
  padding: 28px;
  overflow-y: auto;
  position: relative;
  background:
    linear-gradient(180deg, rgba(248, 251, 252, 0.78) 0%, rgba(234, 242, 245, 0.86) 100%);

  &::-webkit-scrollbar {
    width: 8px;
  }

  &::-webkit-scrollbar-track {
    background: rgba(24, 36, 47, 0.05);
    border-radius: var(--m3-shape-sm);
  }

  &::-webkit-scrollbar-thumb {
    background: rgba(17, 62, 74, 0.28);
    border-radius: var(--m3-shape-sm);
    transition: var(--m3-transition);

    &:hover {
      background: rgba(17, 62, 74, 0.48);
    }
  }
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

@media (max-width: 960px) {
  .header {
    padding: 0 16px;

    .header-left .header-caption,
    .header-right .signal-board {
      display: none;
    }

    .header-right {
      gap: 10px;
    }

    .user-info {
      padding: 6px 10px;

      .user-copy {
        display: none;
      }
    }
  }

  .main-content {
    padding: 18px;
  }
}
</style>
