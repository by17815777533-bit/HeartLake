import { describe, it, expect, beforeEach, vi } from 'vitest'
import { createRouter, createWebHistory } from 'vue-router'
import { setActivePinia, createPinia } from 'pinia'
import { useAppStore } from '@/stores'

// 简化路由配置用于测试
const routes = [
  { path: '/login', component: { template: '<div>Login</div>' } },
  { path: '/dashboard', component: { template: '<div>Dashboard</div>' }, meta: { title: '数据大屏' } },
  { path: '/:pathMatch(.*)*', redirect: '/dashboard' }
]

describe('Router Guards', () => {
  let router, store

  beforeEach(() => {
    setActivePinia(createPinia())
    store = useAppStore()
    localStorage.clear()

    router = createRouter({
      history: createWebHistory(),
      routes
    })

    // 复制路由守卫逻辑
    router.beforeEach((to, from, next) => {
      if (to.path !== '/login' && !store.checkTokenValid()) {
        store.clearToken()
        next('/login')
      } else if (to.path === '/login' && store.checkTokenValid()) {
        next('/dashboard')
      } else {
        next()
      }
    })
  })

  it('should redirect to login when no token', async () => {
    await router.push('/dashboard')
    await router.isReady()
    expect(router.currentRoute.value.path).toBe('/login')
  })

  it('should allow access with valid token', async () => {
    store.setToken('valid-token-123')
    await router.push('/dashboard')
    await router.isReady()
    expect(router.currentRoute.value.path).toBe('/dashboard')
  })

  it('should redirect logged-in user from login to dashboard', async () => {
    store.setToken('valid-token-123')
    await router.push('/login')
    await router.isReady()
    expect(router.currentRoute.value.path).toBe('/dashboard')
  })

  it('should handle 404 routes', async () => {
    store.setToken('valid-token-123')
    await router.push('/nonexistent')
    await router.isReady()
    expect(router.currentRoute.value.path).toBe('/dashboard')
  })
})
