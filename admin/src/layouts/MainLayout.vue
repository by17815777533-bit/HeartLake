<!--
  @file MainLayout.vue
  @brief MainLayout 组件 - 暗色模式 + 全局加载指示器
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
/* 全局加载进度条 */
.global-loading-bar {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  height: 3px;
  z-index: 9999;
  background: transparent;
  overflow: hidden;

  .loading-progress {
    height: 100%;
    width: 30%;
    background: linear-gradient(90deg, #4285F4, #1A73E8);
    border-radius: 0 2px 2px 0;
    animation: loading-slide 1.2s ease-in-out infinite;
  }
}

@keyframes loading-slide {
  0% { transform: translateX(-100%); width: 30%; }
  50% { width: 60%; }
  100% { transform: translateX(400%); width: 30%; }
}

.main-layout {
  display: flex;
  height: 100vh;
  background: #f1f3f4;
}

.sidebar {
  background: #fff;
  display: flex;
  flex-direction: column;
  transition: width 0.3s;
  position: relative;
  border-right: 1px solid #e0e0e0;

  .logo {
    height: 60px;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 0 16px;
    border-bottom: 1px solid #e0e0e0;

    img {
      width: 32px;
      height: 32px;
    }

    .logo-text {
      margin-left: 12px;
      font-size: 18px;
      font-weight: 500;
      color: #1A73E8;
      white-space: nowrap;
    }
  }

  .sidebar-menu {
    flex: 1;
    border-right: none;
    background: transparent;

    :deep(.el-menu-item) {
      color: #5f6368;
      margin: 4px 8px;
      border-radius: 24px;

      &:hover {
        background: #e8f0fe;
        color: #1A73E8;
      }

      &.is-active {
        background: #e8f0fe;
        color: #1A73E8;
        font-weight: 500;
      }
    }
  }

  .collapse-btn {
    position: absolute;
    bottom: 20px;
    left: 50%;
    transform: translateX(-50%);
    width: 32px;
    height: 32px;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    color: #5f6368;
    border-radius: 50%;

    &:hover {
      background: #e8f0fe;
      color: #1A73E8;
    }
  }
}

.main-container {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.header {
  background: #fff;
  box-shadow: 0 1px 2px rgba(60, 64, 67, 0.1);
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 24px;

  .header-left {
    display: flex;
    align-items: center;
  }

  .header-right {
    display: flex;
    align-items: center;
    gap: 16px;

    .realtime-stats {
      display: flex;
      gap: 12px;

      :deep(.el-tag) {
        border-radius: 16px;
        border: none;
        background: #e8f0fe;
        color: #1A73E8;
      }
    }

    .dark-toggle {
      border: none;
      background: transparent;
      color: #5f6368;

      &:hover {
        background: #f1f3f4;
        color: #1A73E8;
      }
    }

    .user-info {
      display: flex;
      align-items: center;
      gap: 8px;
      cursor: pointer;
      padding: 4px 8px;
      border-radius: 20px;

      &:hover {
        background: #f1f3f4;
      }

      .username {
        font-size: 14px;
        color: #5f6368;
      }
    }
  }
}

.main-content {
  flex: 1;
  padding: 20px;
  overflow: auto;
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.2s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

/* 暗色模式适配 */
:global(html.dark) {
  .main-layout {
    background: #1a1a2e;
  }

  .sidebar {
    background: #16213e;
    border-right-color: #2a2a4a;

    .logo {
      border-bottom-color: #2a2a4a;

      .logo-text {
        color: #64b5f6;
      }
    }

    .sidebar-menu {
      :deep(.el-menu-item) {
        color: #b0b0c0;

        &:hover {
          background: #1a2744;
          color: #64b5f6;
        }

        &.is-active {
          background: #1a2744;
          color: #64b5f6;
        }
      }
    }

    .collapse-btn {
      color: #b0b0c0;

      &:hover {
        background: #1a2744;
        color: #64b5f6;
      }
    }
  }

  .header {
    background: #16213e;
    box-shadow: 0 1px 2px rgba(0, 0, 0, 0.3);

    .header-right {
      .realtime-stats :deep(.el-tag) {
        background: #1a2744;
        color: #64b5f6;
      }

      .dark-toggle {
        color: #b0b0c0;

        &:hover {
          background: #1a2744;
          color: #ffd54f;
        }
      }

      .user-info {
        &:hover {
          background: #1a2744;
        }

        .username {
          color: #b0b0c0;
        }
      }
    }
  }

  .main-content {
    background: #1a1a2e;
  }

  .global-loading-bar .loading-progress {
    background: linear-gradient(90deg, #64b5f6, #42a5f5);
  }
}
</style>
