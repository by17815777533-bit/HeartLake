/**
 * 路由配置与全局导航守卫
 *
 * 路由表采用懒加载（动态 import），首屏只加载 Login 或 MainLayout，
 * 其余页面按需拉取，配合 Vite 的代码分割将初始 JS 控制在 ~80KB。
 *
 * 权限模型：三级角色层级 viewer < admin < super_admin，
 * 路由 meta.requiredRole 声明最低角色要求，守卫中做数值比较。
 */
import { createRouter, createWebHistory } from 'vue-router'

/** 扩展 vue-router 的 RouteMeta，增加认证和角色字段 */
declare module 'vue-router' {
  interface RouteMeta {
    title?: string
    requiresAuth?: boolean
    requiredRole?: string
  }
}
import { useAppStore } from '@/stores'
import { clearPendingRequests } from "@/api"

const router = createRouter({
  history: createWebHistory(),
  routes: [
    { path: '/login', component: () => import('@/views/Login.vue') },
    {
      path: '/',
      component: () => import('@/layouts/MainLayout.vue'),
      redirect: '/dashboard',
      children: [
        { path: 'dashboard', component: () => import('@/views/Dashboard.vue'), meta: { title: '湖面总览', requiresAuth: true } },
        { path: 'users', component: () => import('@/views/Users.vue'), meta: { title: '旅人关怀', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'content', component: () => import('@/views/Content.vue'), meta: { title: '石头与纸船', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'moderation', component: () => import('@/views/Moderation.vue'), meta: { title: '温暖守护', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'reports', component: () => import('@/views/Reports.vue'), meta: { title: '求助处理', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'sensitive-words', component: () => import('@/views/SensitiveWords.vue'), meta: { title: '风险词典', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'logs', component: () => import('@/views/Logs.vue'), meta: { title: '服务记录', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'settings', component: () => import('@/views/Settings.vue'), meta: { title: '系统偏好', requiresAuth: true, requiredRole: 'super_admin' } },
        { path: 'edge-ai', component: () => import('@/views/EdgeAI.vue'), meta: { title: '智能辅助', requiresAuth: true, requiredRole: 'admin' } },
      ]
    },
    // 403 权限不足页面
    { path: '/forbidden', component: () => import('@/views/Forbidden.vue'), meta: { title: '权限不足' } },
    // 404 catch-all：未匹配路由显示 404 页面
    {
      path: '/:pathMatch(.*)*',
      name: 'NotFound',
      component: () => import('@/views/NotFound.vue'),
      meta: { title: '页面不存在' },
    }
  ]
})

/**
 * 角色权限层级映射，数值越大权限越高。
 * 新增角色时在此追加即可，守卫逻辑无需改动。
 */
const ROLE_HIERARCHY: Record<string, number> = {
  viewer: 1,
  admin: 2,
  super_admin: 3,
}

/**
 * 判断用户角色是否满足路由要求的最低权限
 * @param userRole - 当前用户角色（可能为 undefined）
 * @param requiredRole - 路由 meta 中声明的最低角色
 * @returns 用户权限 >= 要求权限时返回 true
 */
function hasRequiredRole(userRole: string | undefined, requiredRole: string): boolean {
  return (ROLE_HIERARCHY[userRole || ''] ?? 0) >= (ROLE_HIERARCHY[requiredRole] ?? 0)
}

/**
 * 全局前置守卫，每次路由跳转时依次检查：
 * 1. 取消上一页遗留的 pending 请求（防止竞态）
 * 2. token 有效性 → 无效则跳登录
 * 3. 角色权限 → 不足则跳 403
 * 4. 已登录用户访问 /login → 重定向到首页
 */
router.beforeEach((to, from, next) => {
  clearPendingRequests()
  const appStore = useAppStore()

  if (to.meta.requiresAuth && !appStore.checkTokenValid()) {
    // token 无效或过期，清除并跳转登录
    appStore.clearToken()
    next('/login')
  } else if (to.meta.requiredRole && !hasRequiredRole(appStore.userInfo?.role, to.meta.requiredRole)) {
    // 角色权限不足，跳转403
    next('/forbidden')
  } else if (to.path === '/login' && appStore.checkTokenValid()) {
    // 已登录访问登录页，跳转首页
    next('/dashboard')
  } else {
    next()
  }
})

export default router
