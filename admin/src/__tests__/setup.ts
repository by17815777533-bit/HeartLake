import { config } from '@vue/test-utils'
import { defineComponent, h } from 'vue'
import { vi } from 'vitest'

// Vitest setup: 提供 localStorage / sessionStorage mock（jsdom 环境下可能不完整）

function createStorageMock(): Storage {
  const store: Record<string, string> = {}
  return {
    getItem: (key: string): string | null => store[key] ?? null,
    setItem: (key: string, value: string): void => { store[key] = String(value) },
    removeItem: (key: string): void => { delete store[key] },
    clear: (): void => { Object.keys(store).forEach(k => delete store[k]) },
    get length(): number { return Object.keys(store).length },
    key: (i: number): string | null => Object.keys(store)[i] ?? null,
  }
}

Object.defineProperty(globalThis, 'localStorage', { value: createStorageMock(), writable: true })
Object.defineProperty(globalThis, 'sessionStorage', { value: createStorageMock(), writable: true })

const createPassthroughStub = (name: string, tag = 'div') =>
  defineComponent({
    name,
    setup(_, { slots }) {
      return () => h(tag, { 'data-test-stub': name }, slots.default?.())
    },
  })

const createIconStub = (name: string) =>
  defineComponent({
    name,
    setup() {
      return () => h('i', { class: `stub-icon stub-icon--${name.toLowerCase()}` })
    },
  })

config.global.components = {
  Search: createIconStub('Search'),
  Refresh: createIconStub('Refresh'),
  RefreshRight: createIconStub('RefreshRight'),
  Download: createIconStub('Download'),
}

config.global.stubs = {
  teleport: true,
  transition: false,
  'el-empty': createPassthroughStub('ElEmpty'),
  'el-progress': createPassthroughStub('ElProgress'),
}

config.global.directives = {
  loading: {
    mounted() {},
    updated() {},
  },
}

const shouldSilenceConsole = (args: unknown[]) => {
  const text = args
    .map((arg) => {
      if (typeof arg === 'string') return arg
      if (arg instanceof Error) return arg.message
      return String(arg)
    })
    .join(' ')

  return (
    text.includes('Failed to resolve component') ||
    text.includes('Failed to resolve directive') ||
    text.includes('legacy JS API is deprecated') ||
    /(?:加载|获取|登录|删除|配置保存|广播发送).*(?:失败|错误)/.test(text)
  )
}

const originalConsoleWarn = console.warn.bind(console)
const originalConsoleError = console.error.bind(console)

vi.spyOn(console, 'warn').mockImplementation((...args) => {
  if (!shouldSilenceConsole(args)) {
    originalConsoleWarn(...args)
  }
})

vi.spyOn(console, 'error').mockImplementation((...args) => {
  if (!shouldSilenceConsole(args)) {
    originalConsoleError(...args)
  }
})
