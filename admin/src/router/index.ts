import { createRouter, createWebHistory } from 'vue-router'

declare module 'vue-router' {
  interface RouteMeta {
    title?: string
    requiresAuth?: boolean
    requiredRole?: string
  }
}
import { useAppStore } from '@/stores'
import { cancelAllRequests } from '@/api'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    { path: '/login', component: () => import('@/views/Login.vue') },
    {
      path: '/',
      component: () => import('@/layouts/MainLayout.vue'),
      redirect: '/dashboard',
      children: [
        { path: 'dashboard', component: () => import('@/views/Dashboard.vue'), meta: { title: '数据大屏', requiresAuth: true } },
        { path: 'users', component: () => import('@/views/Users.vue'), meta: { title: '用户管理', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'content', component: () => import('@/views/Content.vue'), meta: { title: '内容管理', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'moderation', component: () => import('@/views/Moderation.vue'), meta: { title: '内容审核', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'reports', component: () => import('@/views/Reports.vue'), meta: { title: '举报处理', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'sensitive-words', component: () => import('@/views/SensitiveWords.vue'), meta: { title: '敏感词', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'logs', component: () => import('@/views/Logs.vue'), meta: { title: '操作日志', requiresAuth: true, requiredRole: 'admin' } },
        { path: 'settings', component: () => import('@/views/Settings.vue'), meta: { title: '系统设置', requiresAuth: true, requiredRole: 'super_admin' } },
        { path: 'edge-ai', component: () => import('@/views/EdgeAI.vue'), meta: { title: '边缘AI', requiresAuth: true, requiredRole: 'admin' } },
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

// 角色权限层级：super_admin > admin > viewer
const ROLE_HIERARCHY: Record<string, number> = {
  viewer: 1,
  admin: 2,
  super_admin: 3,
}

function hasRequiredRole(userRole: string | undefined, requiredRole: string): boolean {
  return (ROLE_HIERARCHY[userRole || ''] ?? 0) >= (ROLE_HIERARCHY[requiredRole] ?? 0)
}

// G-1: 路由守卫 - token 有效性 + 角色权限检查
router.beforeEach((to, from, next) => {
  cancelAllRequests()
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
