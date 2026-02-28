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
        <img
          src="@/assets/logo.svg"
          alt="HeartLake"
        >
        <span
          v-if="!isCollapsed"
          class="logo-text"
        >心湖管理</span>
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
          <el-breadcrumb separator="/">
            <el-breadcrumb-item :to="{ path: '/' }">
              心湖
            </el-breadcrumb-item>
            <el-breadcrumb-item>{{ $route.meta.title }}</el-breadcrumb-item>
          </el-breadcrumb>
        </div>

        <div class="header-right">
          <!-- 实时数据 -->
          <div class="realtime-stats">
            <el-tag
              type="success"
              effect="plain"
            >
              <el-icon><User /></el-icon>
              在线: {{ realtimeStats.onlineCount }}
            </el-tag>
            <el-tag
              type="info"
              effect="plain"
            >
              今日投石: {{ realtimeStats.todayStones }}
            </el-tag>
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
              <span class="username">{{ adminInfo.nickname || '管理员' }}</span>
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
import { ref, reactive, onMounted, onUnmounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'
import websocket from '@/services/websocket'
import { useAppStore } from '@/stores'

const router = useRouter()
const appStore = useAppStore()
const isCollapsed = ref(false)

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
  { path: '/edge-ai', title: '心湖智能', icon: 'Monitor' },
]

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

/** 拉取管理员资料。 */
const fetchAdminInfo = async () => {
  try {
    const res = await api.getAdminInfo()
    if (res.data) {
      adminInfo.nickname = res.data.nickname || res.data.username || '管理员'
    }
  } catch (e: unknown) {
    console.warn('获取管理员信息失败:', (e as Error).message)
  }
}

/** 拉取实时统计。 */
const fetchRealtimeStats = async () => {
  try {
    const res = await api.getRealtimeStats()
    if (res.data) {
      realtimeStats.onlineCount = res.data.online_count || res.data.online_users || 0
      realtimeStats.todayStones = res.data.today_stones || 0
    }
  } catch (e: unknown) {
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

<style lang="scss" scoped>
@keyframes loading-progress {
  0% {
    width: 0%;
    margin-left: 0%;
  }
  50% {
    width: 60%;
    margin-left: 20%;
  }
  100% {
    width: 0%;
    margin-left: 100%;
  }
}

/* 全局加载进度条 */
.global-loading-bar {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  height: 3px;
  background: var(--m3-surface-variant);
  z-index: 9999;
  overflow: hidden;

  .loading-progress {
    height: 100%;
    background: var(--m3-primary);
    animation: loading-progress 1.5s ease-in-out infinite;
  }
}

/* 主布局 */
.main-layout {
  display: flex;
  height: 100vh;
  width: 100vw;
  overflow: hidden;
  position: relative;
  background: linear-gradient(180deg, #fff8e8 0%, #fffdf7 58%, #f4f9ff 100%);

  &::before {
    content: '';
    position: absolute;
    inset: 0;
    pointer-events: none;
    background:
      radial-gradient(600px 280px at 20% -10%, rgba(255, 217, 142, 0.28), transparent 70%),
      radial-gradient(700px 300px at 90% 110%, rgba(144, 205, 255, 0.25), transparent 72%);
  }
}

/* 侧边栏 */
.sidebar {
  background: rgba(255, 253, 246, 0.96);
  border-right: 1px solid #f3d9ad;
  display: flex;
  flex-direction: column;
  transition: var(--m3-transition);
  position: relative;
  z-index: 10;
  box-shadow: 0 10px 28px rgba(162, 130, 58, 0.12);

  .logo {
    height: 64px;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 12px;
    padding: 0 20px;
    border-bottom: 1px solid #f3e4c7;
    background: rgba(255, 252, 242, 0.95);

    img {
      width: 32px;
      height: 32px;
    }

    .logo-text {
      font-size: 18px;
      font-weight: 600;
      color: #a76f00;
      letter-spacing: 0.5px;
    }
  }

  nav[aria-label="主导航"] {
    flex: 1;
    overflow-y: auto;
  }

  .sidebar-menu {
    border: none;
    background: transparent;
    padding: 12px 8px;

    &::-webkit-scrollbar {
      width: 4px;
    }

    &::-webkit-scrollbar-thumb {
      background: #e8d2a9;
      border-radius: var(--m3-shape-sm);
    }

    :deep(.el-menu-item) {
      margin: 4px 0;
      border-radius: var(--m3-shape-md);
      color: #67563f;
      transition: var(--m3-transition);

      &:hover {
        background: #fff1d6;
        color: #4f3f2c;
      }

      &.is-active {
        background: linear-gradient(135deg, #ffe3a4 0%, #ffd18f 100%);
        color: #4f3a20;
        font-weight: 500;
      }

      .el-icon {
        color: inherit;
      }
    }
  }

  .collapse-btn {
    height: 48px;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    color: #67563f;
    border-top: 1px solid #f3e4c7;
    background: rgba(255, 251, 239, 0.98);
    transition: var(--m3-transition);

    &:hover {
      background: #ffefcf;
      color: #4f3f2c;
    }
  }
}

/* 主容器 */
.main-container {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  position: relative;
  z-index: 1;
}

/* 顶部栏 */
.header {
  height: 64px;
  background: rgba(255, 254, 249, 0.94);
  border-bottom: 1px solid #eed9b3;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 24px;
  box-shadow: 0 10px 18px rgba(162, 130, 58, 0.08);
  position: relative;
  z-index: 5;

  .header-left {
    :deep(.el-breadcrumb) {
      .el-breadcrumb__item {
        .el-breadcrumb__inner {
          color: #725f45;
          transition: var(--m3-transition);

          &:hover {
            color: #a76f00;
          }

          &.is-link {
            font-weight: 500;
          }
        }

        &:last-child .el-breadcrumb__inner {
          color: #3f3326;
        }
      }

      .el-breadcrumb__separator {
        color: #b8a281;
      }
    }
  }

  .header-right {
    display: flex;
    align-items: center;
    gap: 16px;

    .realtime-stats {
      display: flex;
      gap: 12px;

      :deep(.el-tag) {
        background: #fff4dc;
        border-color: #efd6ab;
        color: #4d3f2f;
        font-weight: 500;

        .el-icon {
          margin-right: 4px;
        }
      }
    }

    .dark-toggle {
      background: #fff4dc;
      border-color: #efd6ab;
      color: #4d3f2f;

      &:hover {
        background: #ffe8bb;
        border-color: #ddb47a;
      }
    }

    .user-info {
      display: flex;
      align-items: center;
      gap: 12px;
      padding: 8px 16px;
      border-radius: var(--m3-shape-lg);
      background: #fff4dc;
      cursor: pointer;
      transition: var(--m3-transition);

      &:hover {
        background: #ffe8bb;
      }

      :deep(.el-avatar) {
        border: 2px solid #edd4aa;
      }

      .username {
        color: #3f3326;
        font-weight: 500;
        font-size: 14px;
      }
    }
  }
}

/* 内容区域 */
.main-content {
  flex: 1;
  padding: 24px;
  overflow-y: auto;
  position: relative;
  background: linear-gradient(180deg, rgba(255, 250, 240, 0.9) 0%, rgba(246, 252, 255, 0.95) 100%);

  &::-webkit-scrollbar {
    width: 8px;
  }

  &::-webkit-scrollbar-track {
    background: #fff4df;
    border-radius: var(--m3-shape-sm);
  }

  &::-webkit-scrollbar-thumb {
    background: #e5c998;
    border-radius: var(--m3-shape-sm);
    transition: var(--m3-transition);

    &:hover {
      background: #d0a768;
    }
  }
}

/* 页面切换动画 */
.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.2s var(--m3-transition);
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>
