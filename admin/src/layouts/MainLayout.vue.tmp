<!--
  @file MainLayout.vue
  @brief MainLayout 组件 - 光遇(Sky: Children of the Light)风格
  Created by 林子怡
-->

<template>
  <div class="main-layout">
    <!-- 全局加载进度条 -->
    <div v-if="appStore.isGlobalLoading" class="global-loading-bar">
      <div class="loading-progress"></div>
    </div>

    <!-- 侧边栏 -->
    <el-aside :width="isCollapsed ? '64px' : '220px'" class="sidebar">
      <div class="logo">
        <img src="@/assets/logo.svg" alt="HeartLake" />
        <span v-if="!isCollapsed" class="logo-text">心湖管理</span>
      </div>

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
          <template #title>{{ item.title }}</template>
        </el-menu-item>
      </el-menu>

      <div class="collapse-btn" @click="isCollapsed = !isCollapsed">
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
            <el-breadcrumb-item :to="{ path: '/' }">首页</el-breadcrumb-item>
            <el-breadcrumb-item>{{ $route.meta.title }}</el-breadcrumb-item>
          </el-breadcrumb>
        </div>

        <div class="header-right">
          <!-- 实时数据 -->
          <div class="realtime-stats">
            <el-tag type="success" effect="plain">
              <el-icon><User /></el-icon>
              在线: {{ realtimeStats.onlineCount }}
            </el-tag>
            <el-tag type="info" effect="plain">
              今日投石: {{ realtimeStats.todayStones }}
            </el-tag>
          </div>

          <!-- 暗色模式切换 -->
          <el-tooltip :content="appStore.isDark ? '切换亮色模式' : '切换暗色模式'" placement="bottom">
            <el-button
              circle
              size="small"
              @click="appStore.toggleDark()"
              class="dark-toggle"
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
              <el-avatar :size="32">{{ adminInfo.nickname?.charAt(0) || 'A' }}</el-avatar>
              <span class="username">{{ adminInfo.nickname || '管理员' }}</span>
            </div>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="profile">个人设置</el-dropdown-item>
                <el-dropdown-item command="logout" divided>退出登录</el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </el-header>

      <!-- 内容区 -->
      <el-main class="main-content">
        <router-view v-slot="{ Component }">
          <transition name="fade" mode="out-in">
            <component :is="Component" />
          </transition>
        </router-view>
      </el-main>
    </el-container>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, onUnmounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'
import websocket from '@/services/websocket'
import { useAppStore } from '@/stores'

const router = useRouter()
const appStore = useAppStore()
const isCollapsed = ref(false)

// 菜单项
const menuItems = [
  { path: '/dashboard', title: '数据大屏', icon: 'DataAnalysis' },
  { path: '/users', title: '用户管理', icon: 'User' },
  { path: '/content', title: '内容管理', icon: 'Document' },
  { path: '/moderation', title: '内容审核', icon: 'Check' },
  { path: '/reports', title: '举报处理', icon: 'Bell' },
  { path: '/sensitive-words', title: '敏感词', icon: 'Warning' },
  { path: '/logs', title: '操作日志', icon: 'Tickets' },
  { path: '/settings', title: '系统设置', icon: 'Setting' },
  { path: '/edge-ai', title: '边缘AI', icon: 'Monitor' },
]

// 管理员信息 — 优先使用 store 中登录时存储的用户信息
const adminInfo = reactive({
  nickname: appStore.userInfo?.nickname || appStore.userInfo?.username || '管理员',
})

// 实时统计
const realtimeStats = reactive({
  onlineCount: 0,
  todayStones: 0,
})

let statsInterval = null

// L-4: 获取管理员信息
const fetchAdminInfo = async () => {
  try {
    const res = await api.getAdminInfo()
    if (res.data) {
      adminInfo.nickname = res.data.nickname || res.data.username || '管理员'
    }
  } catch (e) {
    console.warn('获取管理员信息失败:', e.message)
  }
}

// 获取实时统计
const fetchRealtimeStats = async () => {
  try {
    const res = await api.getRealtimeStats()
    if (res.data) {
      realtimeStats.onlineCount = res.data.online_count || res.data.online_users || 0
      realtimeStats.todayStones = res.data.today_stones || 0
    }
  } catch (e) {
    // 静默失败，不影响页面展示
    console.warn('获取实时统计失败:', e.message)
  }
}

// 处理下拉菜单命令
const handleCommand = async (command) => {
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

// WebSocket 实时数据更新
const handleStatsUpdate = (data) => {
  if (data) {
    realtimeStats.onlineCount = data.online_count ?? realtimeStats.onlineCount
    realtimeStats.todayStones = data.today_stones ?? realtimeStats.todayStones
  }
}

// L-5: 页面可见性检测 — 不可见时暂停轮询，可见时恢复
const handleVisibilityChange = () => {
  if (document.hidden) {
    // 页面不可见，暂停轮询
    if (statsInterval) {
      clearInterval(statsInterval)
      statsInterval = null
    }
  } else {
    // 页面恢复可见，立即刷新并重启轮询
    fetchRealtimeStats()
    if (!statsInterval) {
      statsInterval = setInterval(fetchRealtimeStats, 30000)
    }
  }
}

onMounted(() => {
  // L-4: 获取管理员信息
  fetchAdminInfo()
  fetchRealtimeStats()

  // 每30秒更新一次实时数据
  statsInterval = setInterval(fetchRealtimeStats, 30000)

  // 连接WebSocket并监听实时更新
  websocket.connect()
  websocket.on('stats_update', handleStatsUpdate)

  // L-5: 监听页面可见性变化
  document.addEventListener('visibilitychange', handleVisibilityChange)
})

onUnmounted(() => {
  if (statsInterval) {
    clearInterval(statsInterval)
    statsInterval = null
  }
  websocket.off('stats_update', handleStatsUpdate)
  // L-5: 移除可见性监听
  document.removeEventListener('visibilitychange', handleVisibilityChange)
})
</script>

<style lang="scss" scoped>
/* ============================================
   Material Design 3 主题
   ============================================ */

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
  background: var(--m3-surface);
}

/* 侧边栏 */
.sidebar {
  background: var(--m3-surface-container-low);
  border-right: 1px solid var(--m3-outline-variant);
  display: flex;
  flex-direction: column;
  transition: var(--m3-transition);
  position: relative;
  z-index: 10;
  box-shadow: var(--m3-elevation-1);

  .logo {
    height: 64px;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 12px;
    padding: 0 20px;
    border-bottom: 1px solid var(--m3-outline-variant);
    background: var(--m3-surface-container);

    img {
      width: 32px;
      height: 32px;
    }

    .logo-text {
      font-size: 18px;
      font-weight: 600;
      color: var(--m3-primary);
      letter-spacing: 0.5px;
    }
  }

  .sidebar-menu {
    flex: 1;
    border: none;
    background: transparent;
    padding: 12px 8px;
    overflow-y: auto;

    &::-webkit-scrollbar {
      width: 4px;
    }

    &::-webkit-scrollbar-thumb {
      background: var(--m3-outline-variant);
      border-radius: var(--m3-shape-sm);
    }

    :deep(.el-menu-item) {
      margin: 4px 0;
      border-radius: var(--m3-shape-md);
      color: var(--m3-on-surface-variant);
      transition: var(--m3-transition);

      &:hover {
        background: var(--m3-surface-container-highest);
        color: var(--m3-on-surface);
      }

      &.is-active {
        background: var(--m3-secondary-container);
        color: var(--m3-on-secondary-container);
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
    color: var(--m3-on-surface-variant);
    border-top: 1px solid var(--m3-outline-variant);
    background: var(--m3-surface-container);
    transition: var(--m3-transition);

    &:hover {
      background: var(--m3-surface-container-high);
      color: var(--m3-on-surface);
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
  background: var(--m3-surface);
  border-bottom: 1px solid var(--m3-outline-variant);
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 24px;
  box-shadow: var(--m3-elevation-1);
  position: relative;
  z-index: 5;

  .header-left {
    :deep(.el-breadcrumb) {
      .el-breadcrumb__item {
        .el-breadcrumb__inner {
          color: var(--m3-on-surface-variant);
          transition: var(--m3-transition);

          &:hover {
            color: var(--m3-primary);
          }

          &.is-link {
            font-weight: 500;
          }
        }

        &:last-child .el-breadcrumb__inner {
          color: var(--m3-on-surface);
        }
      }

      .el-breadcrumb__separator {
        color: var(--m3-outline);
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
        background: var(--m3-surface-container-high);
        border-color: var(--m3-outline-variant);
        color: var(--m3-on-surface);
        font-weight: 500;

        .el-icon {
          margin-right: 4px;
        }
      }
    }

    .dark-toggle {
      background: var(--m3-surface-container-high);
      border-color: var(--m3-outline-variant);
      color: var(--m3-on-surface);

      &:hover {
        background: var(--m3-surface-container-highest);
        border-color: var(--m3-outline);
      }
    }

    .user-info {
      display: flex;
      align-items: center;
      gap: 12px;
      padding: 8px 16px;
      border-radius: var(--m3-shape-lg);
      background: var(--m3-surface-container-high);
      cursor: pointer;
      transition: var(--m3-transition);

      &:hover {
        background: var(--m3-surface-container-highest);
      }

      :deep(.el-avatar) {
        border: 2px solid var(--m3-outline-variant);
      }

      .username {
        color: var(--m3-on-surface);
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
  background: var(--m3-surface-dim);

  &::-webkit-scrollbar {
    width: 8px;
  }

  &::-webkit-scrollbar-track {
    background: var(--m3-surface-container);
    border-radius: var(--m3-shape-sm);
  }

  &::-webkit-scrollbar-thumb {
    background: var(--m3-outline-variant);
    border-radius: var(--m3-shape-sm);
    transition: var(--m3-transition);

    &:hover {
      background: var(--m3-outline);
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
