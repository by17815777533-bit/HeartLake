import { createRouter, createWebHistory } from 'vue-router'
import { useAppStore } from '@/stores'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    { path: '/login', component: () => import('@/views/Login.vue') },
    {
      path: '/',
      component: () => import('@/layouts/MainLayout.vue'),
      redirect: '/dashboard',
      children: [
        { path: 'dashboard', component: () => import('@/views/Dashboard.vue'), meta: { title: '数据大屏' } },
        { path: 'users', component: () => import('@/views/Users.vue'), meta: { title: '用户管理' } },
        { path: 'content', component: () => import('@/views/Content.vue'), meta: { title: '内容管理' } },
        { path: 'moderation', component: () => import('@/views/Moderation.vue'), meta: { title: '内容审核' } },
        { path: 'reports', component: () => import('@/views/Reports.vue'), meta: { title: '举报处理' } },
        { path: 'sensitive-words', component: () => import('@/views/SensitiveWords.vue'), meta: { title: '敏感词' } },
        { path: 'logs', component: () => import('@/views/Logs.vue'), meta: { title: '操作日志' } },
        { path: 'settings', component: () => import('@/views/Settings.vue'), meta: { title: '系统设置' } },
        { path: 'edge-ai', component: () => import('@/views/EdgeAI.vue'), meta: { title: '边缘AI' } },
      ]
    },
    // L-3: 404 catch-all 路由，重定向到 dashboard
    { path: '/:pathMatch(.*)*', redirect: '/dashboard' }
  ]
})

// H-1: 路由守卫 - 添加 token 有效性检查
router.beforeEach((to, from, next) => {
  const appStore = useAppStore()

  if (to.path !== '/login' && !appStore.checkTokenValid()) {
    // token 无效或过期，清除并跳转登录
    appStore.clearToken()
    next('/login')
  } else if (to.path === '/login' && appStore.checkTokenValid()) {
    // 已登录访问登录页，跳转首页
    next('/dashboard')
  } else {
    next()
  }
})

export default router
