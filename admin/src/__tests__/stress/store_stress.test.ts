import { describe, it, expect, vi, beforeEach } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useAppStore } from '@/stores'

describe('Store Stress Tests', () => {
  let appStore: ReturnType<typeof useAppStore>

  beforeEach(() => {
    localStorage.clear()
    sessionStorage.clear()
    vi.clearAllMocks()
    setActivePinia(createPinia())
    appStore = useAppStore()
  })

  // ========== Token快速读写压测 ==========
  describe('Token快速读写', () => {
    it('连续设置token 1000次', () => {
      for (let i = 0; i < 1000; i++) {
        appStore.setToken(`token-${i}`)
      }
      expect(appStore.getToken()).toBe('token-999')
      expect(sessionStorage.getItem('admin_token')).toBe('token-999')
    })

    it('交替设置和清除token 500次', () => {
      for (let i = 0; i < 500; i++) {
        appStore.setToken(`token-${i}`)
        appStore.clearToken()
      }
      expect(appStore.getToken()).toBe('')
      expect(sessionStorage.getItem('admin_token')).toBeNull()
    })

    it('快速设置-读取-清除循环', () => {
      for (let i = 0; i < 200; i++) {
        appStore.setToken(`cycle-${i}`)
        expect(appStore.getToken()).toBe(`cycle-${i}`)
        appStore.clearToken()
        expect(appStore.getToken()).toBe('')
      }
    })

    it('并发getToken调用一致性', () => {
      appStore.setToken('consistent-token')
      const results = Array.from({ length: 1000 }, () => appStore.getToken())
      expect(new Set(results).size).toBe(1)
      expect(results[0]).toBe('consistent-token')
    })

    it('设置空字符串token', () => {
      appStore.setToken('')
      expect(appStore.getToken()).toBe('')
      expect(sessionStorage.getItem('admin_token')).toBe('')
    })

    it('设置后立即多次读取', () => {
      appStore.setToken('read-test')
      for (let i = 0; i < 500; i++) {
        expect(appStore.getToken()).toBe('read-test')
      }
    })
  })

  // ========== Token极端值 ==========
  describe('Token极端值', () => {
    it('超长token (100KB)', () => {
      const longToken = 'x'.repeat(100000)
      appStore.setToken(longToken)
      expect(appStore.getToken()).toBe(longToken)
      expect(appStore.getToken().length).toBe(100000)
    })

    it('包含特殊字符的token', () => {
      const specialTokens = [
        'token-with-spaces and tabs\t',
        'token/with/slashes',
        'token?with=query&params',
        'token#with#hash',
        'token<script>alert(1)</script>',
        'token"with"quotes',
        "token'with'single'quotes",
        'token\nwith\nnewlines',
        'token\0with\0nulls',
      ]
      for (const t of specialTokens) {
        appStore.setToken(t)
        expect(appStore.getToken()).toBe(t)
      }
    })

    it('Unicode token', () => {
      const unicodeToken = '令牌🔑密钥🗝️认证✅'
      appStore.setToken(unicodeToken)
      expect(appStore.getToken()).toBe(unicodeToken)
    })

    it('Base64编码token', () => {
      const b64Token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIn0.dozjgNryP4J3jVmNHl0w5N_XgL0n3I9PlFUP0THsR8U'
      appStore.setToken(b64Token)
      expect(appStore.getToken()).toBe(b64Token)
    })

    it('PASETO格式token', () => {
      const pasetoToken = 'v4.public.eyJkYXRhIjoidGhpcyBpcyBhIHNpZ25lZCBtZXNzYWdlIiwiZXhwIjoiMjAyMi0wMS0wMVQwMDowMDowMCswMDowMCJ9.abc123'
      appStore.setToken(pasetoToken)
      expect(appStore.getToken()).toBe(pasetoToken)
    })

    it('纯数字token', () => {
      appStore.setToken('1234567890')
      expect(appStore.getToken()).toBe('1234567890')
    })

    it('只有空格的token', () => {
      appStore.setToken('   ')
      expect(appStore.getToken()).toBe('   ')
    })
  })

  // ========== checkTokenValid压测 ==========
  describe('checkTokenValid压测', () => {
    it('有效token返回true', () => {
      appStore.setToken('valid-token')
      expect(appStore.checkTokenValid()).toBe(true)
    })

    it('空token返回false', () => {
      expect(appStore.checkTokenValid()).toBe(false)
    })

    it('清除后返回false', () => {
      appStore.setToken('some-token')
      appStore.clearToken()
      expect(appStore.checkTokenValid()).toBe(false)
    })

    it('连续调用1000次checkTokenValid', () => {
      appStore.setToken('check-token')
      for (let i = 0; i < 1000; i++) {
        expect(appStore.checkTokenValid()).toBe(true)
      }
    })

    it('过期token返回false并清除', () => {
      appStore.setToken('expired-token')
      // 手动设置过期时间戳（25小时前）
      sessionStorage.setItem('admin_token_ts', String(Date.now() - 25 * 60 * 60 * 1000))
      expect(appStore.checkTokenValid()).toBe(false)
      expect(appStore.getToken()).toBe('')
    })

    it('刚好24小时的token过期', () => {
      appStore.setToken('boundary-token')
      sessionStorage.setItem('admin_token_ts', String(Date.now() - 24 * 60 * 60 * 1000 - 1))
      expect(appStore.checkTokenValid()).toBe(false)
    })

    it('23小时59分的token未过期', () => {
      appStore.setToken('not-expired')
      sessionStorage.setItem('admin_token_ts', String(Date.now() - 23 * 60 * 60 * 1000))
      expect(appStore.checkTokenValid()).toBe(true)
    })

    it('时间戳为0的token过期', () => {
      appStore.setToken('old-token')
      sessionStorage.setItem('admin_token_ts', '0')
      expect(appStore.checkTokenValid()).toBe(false)
    })

    it('时间戳为未来时间的token有效', () => {
      appStore.setToken('future-token')
      sessionStorage.setItem('admin_token_ts', String(Date.now() + 1000000))
      expect(appStore.checkTokenValid()).toBe(true)
    })

    it('无时间戳的token有效', () => {
      appStore.setToken('no-ts-token')
      sessionStorage.removeItem('admin_token_ts')
      expect(appStore.checkTokenValid()).toBe(true)
    })
  })

  // ========== Loading计数器压测 ==========
  describe('Loading计数器压测', () => {
    it('连续startLoading 1000次', () => {
      for (let i = 0; i < 1000; i++) {
        appStore.startLoading()
      }
      expect(appStore.isGlobalLoading).toBe(true)
    })

    it('startLoading 1000次后stopLoading 1000次', () => {
      for (let i = 0; i < 1000; i++) {
        appStore.startLoading()
      }
      for (let i = 0; i < 1000; i++) {
        appStore.stopLoading()
      }
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('交替start/stop 500次', () => {
      for (let i = 0; i < 500; i++) {
        appStore.startLoading()
        appStore.stopLoading()
      }
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('stopLoading不会产生负数计数', () => {
      for (let i = 0; i < 100; i++) {
        appStore.stopLoading()
      }
      expect(appStore.isGlobalLoading).toBe(false)
      // 再start一次应该正常工作
      appStore.startLoading()
      expect(appStore.isGlobalLoading).toBe(true)
      appStore.stopLoading()
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('不对称start/stop: start多于stop', () => {
      for (let i = 0; i < 10; i++) {
        appStore.startLoading()
      }
      for (let i = 0; i < 5; i++) {
        appStore.stopLoading()
      }
      expect(appStore.isGlobalLoading).toBe(true)
      // 再stop 5次
      for (let i = 0; i < 5; i++) {
        appStore.stopLoading()
      }
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('不对称start/stop: stop多于start', () => {
      for (let i = 0; i < 3; i++) {
        appStore.startLoading()
      }
      for (let i = 0; i < 10; i++) {
        appStore.stopLoading()
      }
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('快速交替模式: SSSSTTTT', () => {
      appStore.startLoading()
      appStore.startLoading()
      appStore.startLoading()
      appStore.startLoading()
      expect(appStore.isGlobalLoading).toBe(true)
      appStore.stopLoading()
      appStore.stopLoading()
      appStore.stopLoading()
      expect(appStore.isGlobalLoading).toBe(true)
      appStore.stopLoading()
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('单次start/stop循环', () => {
      appStore.startLoading()
      expect(appStore.isGlobalLoading).toBe(true)
      appStore.stopLoading()
      expect(appStore.isGlobalLoading).toBe(false)
    })
  })

  // ========== UserInfo压测 ==========
  describe('UserInfo压测', () => {
    it('设置和读取userInfo', () => {
      const info = { id: '1', username: 'admin', role: 'admin' }
      appStore.setUserInfo(info as any)
      expect(appStore.userInfo).toEqual(info)
    })

    it('连续设置userInfo 500次', () => {
      for (let i = 0; i < 500; i++) {
        appStore.setUserInfo({ id: String(i), username: `user-${i}`, role: 'admin' } as any)
      }
      expect(appStore.userInfo?.username).toBe('user-499')
    })

    it('设置null userInfo', () => {
      appStore.setUserInfo({ id: '1', username: 'test', role: 'admin' } as any)
      appStore.setUserInfo(null)
      expect(appStore.userInfo).toBeNull()
    })

    it('大对象userInfo', () => {
      const bigInfo = {
        id: '1',
        username: 'admin',
        role: 'admin',
        metadata: Object.fromEntries(
          Array.from({ length: 1000 }, (_, i) => [`key_${i}`, `value_${i}`])
        ),
      }
      appStore.setUserInfo(bigInfo as any)
      expect(appStore.userInfo).toEqual(bigInfo)
    })

    it('特殊字符userInfo', () => {
      const info = {
        id: '1',
        username: '<script>alert("xss")</script>',
        role: '"; DROP TABLE users; --',
      }
      appStore.setUserInfo(info as any)
      expect(appStore.userInfo?.username).toContain('script')
    })

    it('Unicode userInfo', () => {
      const info = { id: '1', username: '用户🎉', role: '管理员' }
      appStore.setUserInfo(info as any)
      expect(appStore.userInfo?.username).toBe('用户🎉')
    })

    it('clearToken同时清除userInfo', () => {
      appStore.setToken('some-token')
      appStore.setUserInfo({ id: '1', username: 'admin', role: 'admin' } as any)
      appStore.clearToken()
      expect(appStore.userInfo).toBeNull()
      expect(appStore.getToken()).toBe('')
    })

    it('交替设置userInfo和null', () => {
      for (let i = 0; i < 200; i++) {
        appStore.setUserInfo({ id: String(i), username: `u${i}`, role: 'admin' } as any)
        appStore.setUserInfo(null)
      }
      expect(appStore.userInfo).toBeNull()
    })
  })

  // ========== 暗色模式压测 ==========
  describe('暗色模式压测', () => {
    it('快速切换暗色模式500次', () => {
      for (let i = 0; i < 500; i++) {
        appStore.toggleDark()
      }
      // 500次切换后应该回到初始状态（偶数次）
      expect(appStore.isDark).toBe(false)
    })

    it('奇数次切换后为暗色', () => {
      for (let i = 0; i < 501; i++) {
        appStore.toggleDark()
      }
      expect(appStore.isDark).toBe(true)
    })

    it('切换后localStorage持久化', () => {
      appStore.toggleDark()
      expect(localStorage.getItem('admin_dark_mode')).toBe('true')
      appStore.toggleDark()
      expect(localStorage.getItem('admin_dark_mode')).toBe('false')
    })

    it('从localStorage恢复暗色模式', () => {
      localStorage.setItem('admin_dark_mode', 'true')
      setActivePinia(createPinia())
      const newStore = useAppStore()
      expect(newStore.isDark).toBe(true)
    })

    it('localStorage值为非法字符串', () => {
      localStorage.setItem('admin_dark_mode', 'invalid')
      setActivePinia(createPinia())
      const newStore = useAppStore()
      expect(newStore.isDark).toBe(false)
    })

    it('initDarkMode不改变状态', () => {
      appStore.toggleDark() // true
      appStore.initDarkMode()
      expect(appStore.isDark).toBe(true)
    })
  })

  // ========== SessionStorage异常压测 ==========
  describe('SessionStorage边界', () => {
    it('sessionStorage被清空后getToken返回空', () => {
      appStore.setToken('test-token')
      sessionStorage.clear()
      // token ref仍然保持值（内存中）
      expect(appStore.getToken()).toBe('test-token')
    })

    it('sessionStorage和内存状态同步', () => {
      appStore.setToken('sync-token')
      expect(sessionStorage.getItem('admin_token')).toBe('sync-token')
      appStore.clearToken()
      expect(sessionStorage.getItem('admin_token')).toBeNull()
    })

    it('时间戳持久化验证', () => {
      appStore.setToken('ts-token')
      const ts = sessionStorage.getItem('admin_token_ts')
      expect(ts).not.toBeNull()
      expect(Number(ts)).toBeGreaterThan(0)
      expect(Number(ts)).toBeLessThanOrEqual(Date.now())
    })

    it('clearToken清除所有相关sessionStorage项', () => {
      appStore.setToken('clear-test')
      appStore.setUserInfo({ id: '1', username: 'test', role: 'admin' } as any)
      appStore.clearToken()
      expect(sessionStorage.getItem('admin_token')).toBeNull()
      expect(sessionStorage.getItem('admin_user_info')).toBeNull()
      expect(sessionStorage.getItem('admin_token_ts')).toBeNull()
    })

    it('userInfo JSON序列化/反序列化', () => {
      const info = { id: '1', username: 'test', role: 'admin', nested: { a: [1, 2, 3] } }
      appStore.setUserInfo(info as any)
      const stored = sessionStorage.getItem('admin_user_info')
      expect(JSON.parse(stored!)).toEqual(info)
    })
  })

  // ========== 多Store实例一致性 ==========
  describe('多Store实例一致性', () => {
    it('多次useAppStore返回同一实例', () => {
      const store1 = useAppStore()
      const store2 = useAppStore()
      store1.setToken('shared-token')
      expect(store2.getToken()).toBe('shared-token')
    })

    it('一个实例修改token另一个实例可见', () => {
      const store1 = useAppStore()
      const store2 = useAppStore()
      for (let i = 0; i < 100; i++) {
        store1.setToken(`token-${i}`)
        expect(store2.getToken()).toBe(`token-${i}`)
      }
    })

    it('一个实例clearToken另一个实例可见', () => {
      const store1 = useAppStore()
      const store2 = useAppStore()
      store1.setToken('to-clear')
      store2.clearToken()
      expect(store1.getToken()).toBe('')
    })

    it('loading状态跨实例一致', () => {
      const store1 = useAppStore()
      const store2 = useAppStore()
      store1.startLoading()
      expect(store2.isGlobalLoading).toBe(true)
      store2.stopLoading()
      expect(store1.isGlobalLoading).toBe(false)
    })

    it('暗色模式跨实例一致', () => {
      const store1 = useAppStore()
      const store2 = useAppStore()
      store1.toggleDark()
      expect(store2.isDark).toBe(true)
    })
  })

  // ========== 状态组合压测 ==========
  describe('状态组合压测', () => {
    it('同时操作token和loading', () => {
      for (let i = 0; i < 100; i++) {
        appStore.setToken(`token-${i}`)
        appStore.startLoading()
        appStore.stopLoading()
        appStore.clearToken()
      }
      expect(appStore.getToken()).toBe('')
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('同时操作所有状态', () => {
      for (let i = 0; i < 50; i++) {
        appStore.setToken(`token-${i}`)
        appStore.setUserInfo({ id: String(i), username: `u${i}`, role: 'admin' } as any)
        appStore.startLoading()
        appStore.toggleDark()
        appStore.stopLoading()
        appStore.clearToken()
      }
      expect(appStore.getToken()).toBe('')
      expect(appStore.userInfo).toBeNull()
      expect(appStore.isGlobalLoading).toBe(false)
      expect(appStore.isDark).toBe(false) // 50次切换=偶数
    })

    it('clearToken不影响loading状态', () => {
      appStore.startLoading()
      appStore.startLoading()
      appStore.setToken('test')
      appStore.clearToken()
      expect(appStore.isGlobalLoading).toBe(true)
      appStore.stopLoading()
      appStore.stopLoading()
      expect(appStore.isGlobalLoading).toBe(false)
    })

    it('clearToken不影响暗色模式', () => {
      appStore.toggleDark() // true
      appStore.setToken('test')
      appStore.clearToken()
      expect(appStore.isDark).toBe(true)
    })

    it('loading和暗色模式互不影响', () => {
      appStore.startLoading()
      appStore.toggleDark()
      expect(appStore.isGlobalLoading).toBe(true)
      expect(appStore.isDark).toBe(true)
      appStore.stopLoading()
      expect(appStore.isDark).toBe(true)
      appStore.toggleDark()
      expect(appStore.isGlobalLoading).toBe(false)
    })
  })

  // ========== 初始化压测 ==========
  describe('Store初始化', () => {
    it('全新store初始状态正确', () => {
      setActivePinia(createPinia())
      const fresh = useAppStore()
      expect(fresh.getToken()).toBe('')
      expect(fresh.userInfo).toBeNull()
      expect(fresh.isGlobalLoading).toBe(false)
    })

    it('带sessionStorage数据初始化', () => {
      sessionStorage.setItem('admin_token', 'persisted-token')
      sessionStorage.setItem('admin_user_info', JSON.stringify({ id: '1', username: 'saved', role: 'admin' }))
      setActivePinia(createPinia())
      const restored = useAppStore()
      expect(restored.getToken()).toBe('persisted-token')
      expect(restored.userInfo?.username).toBe('saved')
    })

    it('损坏的userInfo JSON不崩溃', () => {
      sessionStorage.setItem('admin_user_info', '{invalid json}}}')
      setActivePinia(createPinia())
      const store = useAppStore()
      expect(store.userInfo).toBeNull()
    })

    it('连续创建100个pinia实例', () => {
      for (let i = 0; i < 100; i++) {
        setActivePinia(createPinia())
        const store = useAppStore()
        store.setToken(`init-${i}`)
        expect(store.getToken()).toBe(`init-${i}`)
      }
    })

    it('sessionStorage中token为空字符串', () => {
      sessionStorage.setItem('admin_token', '')
      setActivePinia(createPinia())
      const store = useAppStore()
      expect(store.getToken()).toBe('')
    })

    it('localStorage暗色模式初始化', () => {
      localStorage.setItem('admin_dark_mode', 'true')
      setActivePinia(createPinia())
      const store = useAppStore()
      expect(store.isDark).toBe(true)
    })
  })

  // ========== 边界条件 ==========
  describe('边界条件', () => {
    it('setToken后立即checkTokenValid', () => {
      appStore.setToken('immediate-check')
      expect(appStore.checkTokenValid()).toBe(true)
    })

    it('clearToken后立即checkTokenValid', () => {
      appStore.setToken('to-clear')
      appStore.clearToken()
      expect(appStore.checkTokenValid()).toBe(false)
    })

    it('setToken覆盖旧token', () => {
      appStore.setToken('old')
      appStore.setToken('new')
      expect(appStore.getToken()).toBe('new')
      expect(sessionStorage.getItem('admin_token')).toBe('new')
    })

    it('时间戳在setToken时更新', () => {
      appStore.setToken('first')
      const ts1 = sessionStorage.getItem('admin_token_ts')
      appStore.setToken('second')
      const ts2 = sessionStorage.getItem('admin_token_ts')
      expect(Number(ts2)).toBeGreaterThanOrEqual(Number(ts1))
    })

    it('多次clearToken不报错', () => {
      expect(() => {
        for (let i = 0; i < 100; i++) {
          appStore.clearToken()
        }
      }).not.toThrow()
    })

    it('未设置token时clearToken不报错', () => {
      expect(() => appStore.clearToken()).not.toThrow()
      expect(appStore.getToken()).toBe('')
    })

    it('startLoading后多次stopLoading不产生负数', () => {
      appStore.startLoading()
      for (let i = 0; i < 50; i++) {
        appStore.stopLoading()
      }
      expect(appStore.isGlobalLoading).toBe(false)
      // 验证再次start/stop正常
      appStore.startLoading()
      expect(appStore.isGlobalLoading).toBe(true)
      appStore.stopLoading()
      expect(appStore.isGlobalLoading).toBe(false)
    })
  })
})
