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
  z-index: 9999;
  background: transparent;
  overflow: hidden;

  .loading-progress {
    height: 100%;
    width: 30%;
    background: var(--sky-gradient-gold, linear-gradient(90deg, #F2CC8F, #E8A87C, #E07A5F));
    border-radius: 0 2px 2px 0;
    animation: loading-slide 1.2s ease-in-out infinite;
    box-shadow: 0 0 12px rgba(242, 204, 143, 0.5);
  }
}

/* 页面过渡 */
.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

/* 主布局 */
.main-layout {
  display: flex;
  height: 100vh;
  background: var(--sky-bg-light, #FFF8F0);
}

/* ============================================
   侧边栏
   ============================================ */
.sidebar {
  background: rgba(255, 255, 255, 0.92);
  backdrop-filter: blur(20px);
  -webkit-backdrop-filter: blur(20px);
  display: flex;
  flex-direction: column;
  transition: width 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  position: relative;
  border-right: 1px solid rgba(212, 165, 90, 0.15);
  z-index: 10;

  .logo {
    height: 60px;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 0 16px;
    border-bottom: 1px solid rgba(212, 165, 90, 0.1);
    position: relative;

    img {
      width: 32px;
      height: 32px;
      animation: float-gentle 4s ease-in-out infinite;
    }

    .logo-text {
      margin-left: 12px;
      font-size: 16px;
      font-weight: 700;
      background: var(--sky-gradient-gold, linear-gradient(135deg, #F2CC8F, #E8A87C));
      -webkit-background-clip: text;
      background-clip: text;
      -webkit-text-fill-color: transparent;
      letter-spacing: 1px;
    }
  }

  .sidebar-menu {
    flex: 1;
    overflow-y: auto;
    border-right: none;
    background: transparent;
    padding: 8px 0;

    &::-webkit-scrollbar {
      width: 4px;
    }

    &::-webkit-scrollbar-thumb {
      background: rgba(212, 165, 90, 0.2);
      border-radius: 2px;
    }

    :deep(.el-menu-item) {
      margin: 2px 8px;
      border-radius: var(--sky-radius-sm, 8px);
      color: #6B5B4F;
      transition: all 0.3s ease;
      position: relative;
      height: 44px;
      line-height: 44px;

      .el-icon {
        color: #9A8A7A;
        transition: color 0.3s ease;
      }

      &:hover {
        background: rgba(242, 204, 143, 0.12);
        color: #D4A55A;

        .el-icon {
          color: #D4A55A;
        }
      }

      &.is-active {
        background: rgba(242, 204, 143, 0.15);
        color: #C4883A;
        font-weight: 600;

        &::before {
          content: '';
          position: absolute;
          left: 0;
          top: 50%;
          transform: translateY(-50%);
          width: 3px;
          height: 20px;
          background: var(--sky-gradient-gold, linear-gradient(180deg, #F2CC8F, #E8A87C));
          border-radius: 0 3px 3px 0;
        }

        .el-icon {
          color: #D4A55A;
        }
      }
    }

    /* 折叠状态 */
    :deep(.el-menu--collapse .el-menu-item) {
      margin: 2px 4px;
      padding: 0 !important;
      justify-content: center;
    }
  }

  .collapse-btn {
    height: 44px;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    color: #9A8A7A;
    border-top: 1px solid rgba(212, 165, 90, 0.1);
    transition: all 0.3s ease;

    &:hover {
      color: #D4A55A;
      background: rgba(242, 204, 143, 0.08);
    }
  }
}

/* ============================================
   顶部栏
   ============================================ */
.header {
  height: 56px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 20px;
  background: rgba(255, 255, 255, 0.88);
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
  border-bottom: 1px solid rgba(212, 165, 90, 0.1);

  .header-left {
    :deep(.el-breadcrumb) {
      .el-breadcrumb__inner {
        color: #6B5B4F;
        transition: color 0.3s ease;

        &.is-link:hover {
          color: #D4A55A;
        }
      }

      .el-breadcrumb__separator {
        color: #B8A99A;
      }
    }
  }

  .header-right {
    display: flex;
    align-items: center;
    gap: 12px;

    .realtime-stats {
      display: flex;
      gap: 8px;

      :deep(.el-tag) {
        background: rgba(242, 204, 143, 0.08);
        border: 1px solid rgba(212, 165, 90, 0.15);
        color: #9A7A4A;
        border-radius: 16px;
        font-size: 12px;

        .el-icon {
          margin-right: 2px;
        }
      }
    }

    .dark-toggle {
      border: 1px solid rgba(212, 165, 90, 0.15);
      background: transparent;
      color: #9A8A7A;
      transition: all 0.3s ease;

      &:hover {
        background: rgba(242, 204, 143, 0.12);
        color: #D4A55A;
        border-color: rgba(242, 204, 143, 0.3);
      }
    }

    .user-info {
      display: flex;
      align-items: center;
      gap: 8px;
      cursor: pointer;
      padding: 4px 12px;
      border-radius: var(--sky-radius-md, 12px);
      transition: all 0.3s ease;

      &:hover {
        background: rgba(242, 204, 143, 0.1);
      }

      :deep(.el-avatar) {
        background: var(--sky-gradient-gold, linear-gradient(135deg, #F2CC8F, #E8A87C));
        color: #fff;
        font-weight: 600;
        font-size: 14px;
      }

      .username {
        font-size: 14px;
        color: #6B5B4F;
        font-weight: 500;
      }
    }
  }
}

/* ============================================
   主内容区
   ============================================ */
.main-content {
  background: #FFF8F0;
  overflow-y: auto;
  padding: 20px;

  &::-webkit-scrollbar {
    width: 6px;
  }

  &::-webkit-scrollbar-thumb {
    background: rgba(212, 165, 90, 0.2);
    border-radius: 3px;
  }

  &::-webkit-scrollbar-track {
    background: transparent;
  }
}

/* ============================================
   暗色模式 (光遇星空主题 - 默认推荐)
   ============================================ */
html.dark {
  .main-layout {
    background: var(--sky-bg-deep, #0A0A1A);
  }

  .sidebar {
    background: rgba(10, 10, 26, 0.85);
    backdrop-filter: blur(20px);
    -webkit-backdrop-filter: blur(20px);
    border-right: 1px solid rgba(242, 204, 143, 0.1);

    .logo {
      border-bottom: 1px solid rgba(242, 204, 143, 0.08);

      img {
        animation: logo-glow 3s ease-in-out infinite, float-gentle 4s ease-in-out infinite;
      }

      .logo-text {
        background: var(--sky-gradient-gold, linear-gradient(135deg, #F2CC8F, #E8A87C));
        -webkit-background-clip: text;
        background-clip: text;
        -webkit-text-fill-color: transparent;
      }
    }

    .sidebar-menu {
      &::-webkit-scrollbar-thumb {
        background: rgba(242, 204, 143, 0.15);
      }

      :deep(.el-menu-item) {
        color: var(--sky-text-secondary, #B8A99A);

        .el-icon {
          color: var(--sky-text-muted, #7A6F63);
        }

        &:hover {
          background: rgba(242, 204, 143, 0.1);
          color: var(--sky-gold, #F2CC8F);

          .el-icon {
            color: var(--sky-gold, #F2CC8F);
          }
        }

        &.is-active {
          background: rgba(242, 204, 143, 0.12);
          color: var(--sky-gold, #F2CC8F);

          &::before {
            background: var(--sky-gradient-gold, linear-gradient(180deg, #F2CC8F, #E8A87C));
            box-shadow: 0 0 8px rgba(242, 204, 143, 0.4);
          }

          .el-icon {
            color: var(--sky-gold, #F2CC8F);
          }
        }
      }
    }

    .collapse-btn {
      color: var(--sky-text-muted, #7A6F63);
      border-top: 1px solid rgba(242, 204, 143, 0.08);

      &:hover {
        color: var(--sky-gold, #F2CC8F);
        background: rgba(242, 204, 143, 0.08);
      }
    }
  }

  .header {
    background: rgba(20, 20, 50, 0.8);
    backdrop-filter: blur(16px);
    -webkit-backdrop-filter: blur(16px);
    border-bottom: 1px solid rgba(242, 204, 143, 0.08);

    .header-left {
      :deep(.el-breadcrumb) {
        .el-breadcrumb__inner {
          color: var(--sky-text-secondary, #B8A99A);

          &.is-link:hover {
            color: var(--sky-gold, #F2CC8F);
          }
        }

        .el-breadcrumb__separator {
          color: var(--sky-text-muted, #7A6F63);
        }
      }
    }

    .header-right {
      .realtime-stats :deep(.el-tag) {
        background: rgba(242, 204, 143, 0.08);
        border: 1px solid rgba(242, 204, 143, 0.12);
        color: var(--sky-gold, #F2CC8F);
      }

      .dark-toggle {
        color: var(--sky-text-secondary, #B8A99A);
        border-color: rgba(242, 204, 143, 0.1);

        &:hover {
          background: rgba(242, 204, 143, 0.1);
          color: var(--sky-gold, #F2CC8F);
          border-color: rgba(242, 204, 143, 0.25);
          box-shadow: 0 0 12px rgba(242, 204, 143, 0.15);
        }
      }

      .user-info {
        &:hover {
          background: rgba(242, 204, 143, 0.08);
        }

        :deep(.el-avatar) {
          background: var(--sky-gradient-gold, linear-gradient(135deg, #F2CC8F, #E8A87C));
          box-shadow: 0 0 10px rgba(242, 204, 143, 0.25);
        }

        .username {
          color: var(--sky-text-secondary, #B8A99A);
        }
      }
    }
  }

  /* 星空主内容区 */
  .main-content {
    background: linear-gradient(180deg, #0A0A1A 0%, #0F0F2A 50%, #141432 100%);
    position: relative;

    /* 星星粒子层1 - 小星星 */
    &::before {
      content: '';
      position: fixed;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      pointer-events: none;
      z-index: 0;
      background-image:
        radial-gradient(1px 1px at 10% 15%, rgba(242, 204, 143, 0.6) 0%, transparent 100%),
        radial-gradient(1px 1px at 25% 35%, rgba(255, 255, 255, 0.5) 0%, transparent 100%),
        radial-gradient(1.5px 1.5px at 40% 10%, rgba(242, 204, 143, 0.7) 0%, transparent 100%),
        radial-gradient(1px 1px at 55% 45%, rgba(255, 255, 255, 0.4) 0%, transparent 100%),
        radial-gradient(1px 1px at 70% 20%, rgba(232, 168, 124, 0.5) 0%, transparent 100%),
        radial-gradient(1.5px 1.5px at 85% 55%, rgba(242, 204, 143, 0.6) 0%, transparent 100%),
        radial-gradient(1px 1px at 15% 65%, rgba(255, 255, 255, 0.3) 0%, transparent 100%),
        radial-gradient(1px 1px at 35% 80%, rgba(242, 204, 143, 0.5) 0%, transparent 100%),
        radial-gradient(1.5px 1.5px at 60% 70%, rgba(255, 255, 255, 0.4) 0%, transparent 100%),
        radial-gradient(1px 1px at 80% 85%, rgba(232, 168, 124, 0.4) 0%, transparent 100%),
        radial-gradient(1px 1px at 92% 40%, rgba(242, 204, 143, 0.5) 0%, transparent 100%),
        radial-gradient(1px 1px at 5% 90%, rgba(255, 255, 255, 0.3) 0%, transparent 100%),
        radial-gradient(1.5px 1.5px at 48% 25%, rgba(123, 104, 174, 0.4) 0%, transparent 100%),
        radial-gradient(1px 1px at 75% 60%, rgba(242, 204, 143, 0.4) 0%, transparent 100%),
        radial-gradient(1px 1px at 30% 50%, rgba(255, 255, 255, 0.35) 0%, transparent 100%);
      animation: twinkle 4s ease-in-out infinite;
    }

    /* 星星粒子层2 - 大星星，错开闪烁 */
    &::after {
      content: '';
      position: fixed;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      pointer-events: none;
      z-index: 0;
      background-image:
        radial-gradient(2px 2px at 20% 25%, rgba(242, 204, 143, 0.8) 0%, transparent 100%),
        radial-gradient(2px 2px at 50% 15%, rgba(255, 255, 255, 0.6) 0%, transparent 100%),
        radial-gradient(2.5px 2.5px at 75% 35%, rgba(242, 204, 143, 0.7) 0%, transparent 100%),
        radial-gradient(2px 2px at 90% 65%, rgba(232, 168, 124, 0.6) 0%, transparent 100%),
        radial-gradient(2px 2px at 10% 75%, rgba(123, 104, 174, 0.5) 0%, transparent 100%),
        radial-gradient(2.5px 2.5px at 45% 55%, rgba(242, 204, 143, 0.6) 0%, transparent 100%),
        radial-gradient(2px 2px at 65% 85%, rgba(255, 255, 255, 0.5) 0%, transparent 100%),
        radial-gradient(2px 2px at 35% 95%, rgba(242, 204, 143, 0.4) 0%, transparent 100%);
      animation: twinkle-slow 6s ease-in-out infinite 2s;
    }

    /* 确保内容在星星之上 */
    :deep(> *) {
      position: relative;
      z-index: 1;
    }

    &::-webkit-scrollbar-thumb {
      background: rgba(242, 204, 143, 0.15);
    }
  }

  /* 暗色模式加载条 */
  .global-loading-bar .loading-progress {
    background: var(--sky-gradient-gold, linear-gradient(90deg, #F2CC8F, #E8A87C, #E07A5F));
    box-shadow: 0 0 16px rgba(242, 204, 143, 0.6);
  }
}
</style>
