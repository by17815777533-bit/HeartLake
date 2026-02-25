/**
 * @file views/Login.test.ts
 * @brief Login.vue 组件测试
 */
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia, setActivePinia } from 'pinia'
import { defineComponent, h } from 'vue'

const mockPush = vi.fn()

vi.mock('vue-router', () => ({
  useRouter: () => ({ push: mockPush }),
  useRoute: () => ({ path: '/login' }),
}))

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), success: vi.fn(), warning: vi.fn() },
}))

vi.mock('@/api', () => ({ default: { login: vi.fn() } }))
vi.mock('@/stores', () => ({
  useAppStore: () => ({ setToken: vi.fn(), setUserInfo: vi.fn() }),
}))
vi.mock('@/utils/errorHelper', () => ({
  getErrorMessage: (_e: any, fallback: string) => fallback || '操作失败',
}))

import Login from '@/views/Login.vue'
import api from '@/api'
import { ElMessage } from 'element-plus'

const ElForm = defineComponent({
  name: 'ElForm',
  props: ['model', 'rules'],
  setup(_, { slots, expose }) {
    expose({ validate: () => Promise.resolve(true), resetFields: vi.fn() })
    return () => h('form', slots.default?.())
  },
})
const ElFormItem = defineComponent({ name: 'ElFormItem', props: ['prop'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElInput = defineComponent({
  name: 'ElInput',
  props: ['modelValue', 'type', 'placeholder', 'size', 'showPassword'],
  emits: ['update:modelValue', 'keyup'],
  setup(props, { emit }) {
    return () => h('input', {
      value: props.modelValue,
      onInput: (e: any) => emit('update:modelValue', e.target.value),
    })
  },
})
const ElButton = defineComponent({
  name: 'ElButton',
  props: ['loading', 'type', 'size'],
  emits: ['click'],
  setup(props, { slots, emit }) {
    return () => h('button', { onClick: () => emit('click'), disabled: props.loading }, slots.default?.())
  },
})

const mountOpts = { global: { components: { ElForm, ElFormItem, ElInput, ElButton } } }

describe('Login.vue', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    setActivePinia(createPinia())
  })

  it('渲染登录页面', () => {
    const wrapper = mount(Login, mountOpts)
    expect(wrapper.find('.login-page').exists()).toBe(true)
  })

  it('包含标题文字', () => {
    const wrapper = mount(Login, mountOpts)
    expect(wrapper.text()).toContain('心湖管理后台')
  })

  it('包含副标题', () => {
    const wrapper = mount(Login, mountOpts)
    expect(wrapper.text()).toContain('HeartLake Admin Console')
  })

  it('包含版权信息', () => {
    const wrapper = mount(Login, mountOpts)
    expect(wrapper.text()).toContain('2026 HeartLake')
  })

  it('包含登录表单', () => {
    const wrapper = mount(Login, mountOpts)
    expect(wrapper.findComponent({ name: 'ElForm' }).exists()).toBe(true)
  })

  it('包含用户名和密码输入框', () => {
    const wrapper = mount(Login, mountOpts)
    const inputs = wrapper.findAllComponents({ name: 'ElInput' })
    expect(inputs.length).toBeGreaterThanOrEqual(2)
  })

  it('包含登录按钮', () => {
    const wrapper = mount(Login, mountOpts)
    expect(wrapper.findComponent({ name: 'ElButton' }).exists()).toBe(true)
  })

  it('登录按钮初始不 loading', () => {
    const wrapper = mount(Login, mountOpts)
    const btn = wrapper.findComponent({ name: 'ElButton' })
    expect(btn.props('loading')).toBeFalsy()
  })

  it('登录成功调用 api.login', async () => {
    vi.mocked(api.login).mockResolvedValue({ data: { data: { token: 'tk', user: { username: 'admin' } } } } as any)
    const wrapper = mount(Login, mountOpts)
    await wrapper.findComponent({ name: 'ElButton' }).trigger('click')
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    expect(api.login).toHaveBeenCalled()
  })

  it('登录失败显示错误', async () => {
    vi.mocked(api.login).mockRejectedValue(new Error('密码错误'))
    const wrapper = mount(Login, mountOpts)
    await wrapper.findComponent({ name: 'ElButton' }).trigger('click')
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    expect(ElMessage.error).toHaveBeenCalled()
  })

  it('表单有 aria-label', () => {
    const wrapper = mount(Login, mountOpts)
    const form = wrapper.find('[aria-label="管理员登录"]')
    expect(form.exists()).toBe(true)
  })

  it('logo 图片存在', () => {
    const wrapper = mount(Login, mountOpts)
    expect(wrapper.find('.logo-icon').exists()).toBe(true)
  })
})
