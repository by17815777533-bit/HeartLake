/**
 * @file views/Settings.test.ts
 * @brief Settings.vue 组件测试
 */
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia, setActivePinia } from 'pinia'
import { defineComponent, h } from 'vue'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), success: vi.fn(), warning: vi.fn() },
  ElMessageBox: { confirm: vi.fn().mockResolvedValue('confirm') },
}))

vi.mock('@/api', () => ({
  default: { getSystemConfig: vi.fn(), updateSystemConfig: vi.fn(), getEdgeAIStatus: vi.fn(), broadcastMessage: vi.fn() },
}))
vi.mock('@/utils/errorHelper', () => ({ getErrorMessage: (_e: any, f: string) => f }))

import Settings from '@/views/Settings.vue'
import api from '@/api'

const ElTabs = defineComponent({ name: 'ElTabs', props: ['modelValue'], emits: ['update:modelValue'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElTabPane = defineComponent({ name: 'ElTabPane', props: ['label', 'name'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElCard = defineComponent({ name: 'ElCard', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElForm = defineComponent({ name: 'ElForm', props: ['model', 'rules'], setup(_, { slots, expose }) { expose({ validate: () => Promise.resolve(true), resetFields: vi.fn() }); return () => h('form', slots.default?.()) } })
const ElFormItem = defineComponent({ name: 'ElFormItem', props: ['label', 'prop'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElInput = defineComponent({ name: 'ElInput', props: ['modelValue', 'type', 'rows'], emits: ['update:modelValue'], setup(p, { emit }) { return () => h('input', { value: p.modelValue, onInput: (e: any) => emit('update:modelValue', e.target.value) }) } })
const ElInputNumber = defineComponent({ name: 'ElInputNumber', props: ['modelValue'], emits: ['update:modelValue'], setup(p) { return () => h('input', { value: p.modelValue, type: 'number' }) } })
const ElSelect = defineComponent({ name: 'ElSelect', props: ['modelValue'], emits: ['update:modelValue'], setup(_, { slots }) { return () => h('select', slots.default?.()) } })
const ElOption = defineComponent({ name: 'ElOption', props: ['label', 'value'], setup(p) { return () => h('option', { value: p.value }, p.label) } })
const ElSwitch = defineComponent({ name: 'ElSwitch', props: ['modelValue'], emits: ['update:modelValue'], setup(p) { return () => h('input', { type: 'checkbox', checked: p.modelValue }) } })
const ElButton = defineComponent({ name: 'ElButton', props: ['loading', 'type'], emits: ['click'], setup(_, { slots, emit }) { return () => h('button', { onClick: () => emit('click') }, slots.default?.()) } })
const ElDivider = defineComponent({ name: 'ElDivider', setup() { return () => h('hr') } })
const ElAlert = defineComponent({ name: 'ElAlert', props: ['title', 'type'], setup(p) { return () => h('div', p.title) } })

const mountOpts = {
  global: {
    components: { ElTabs, ElTabPane, ElCard, ElForm, ElFormItem, ElInput, ElInputNumber, ElSelect, ElOption, ElSwitch, ElButton, ElDivider, ElAlert },
  },
}

describe('Settings.vue', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    setActivePinia(createPinia())
    vi.mocked(api.getSystemConfig).mockResolvedValue({
      data: { data: {
        system: { name: '心湖', description: '测试', allow_register: true, allow_anonymous: false },
        ai: { provider: 'deepseek', api_key: 'sk-test', base_url: 'https://api.deepseek.com', model: 'deepseek-chat' },
        rate: { stone_per_hour: 15, boat_per_hour: 50, message_per_minute: 60, max_content_length: 2000 },
      } },
    } as any)
  })
  it('渲染设置页面', () => {
    const wrapper = mount(Settings, mountOpts)
    expect(wrapper.find('.settings-page').exists()).toBe(true)
  })

  it('包含 Tabs 组件', () => {
    const wrapper = mount(Settings, mountOpts)
    expect(wrapper.findComponent({ name: 'ElTabs' }).exists()).toBe(true)
  })

  it('包含多个 TabPane', () => {
    const wrapper = mount(Settings, mountOpts)
    const panes = wrapper.findAllComponents({ name: 'ElTabPane' })
    expect(panes.length).toBeGreaterThanOrEqual(3)
  })

  it('包含表单', () => {
    const wrapper = mount(Settings, mountOpts)
    expect(wrapper.findComponent({ name: 'ElForm' }).exists()).toBe(true)
  })

  it('包含保存按钮', () => {
    const wrapper = mount(Settings, mountOpts)
    const buttons = wrapper.findAllComponents({ name: 'ElButton' })
    expect(buttons.length).toBeGreaterThanOrEqual(1)
  })

  it('包含开关组件', () => {
    const wrapper = mount(Settings, mountOpts)
    expect(wrapper.findComponent({ name: 'ElSwitch' }).exists()).toBe(true)
  })

  it('包含输入框', () => {
    const wrapper = mount(Settings, mountOpts)
    expect(wrapper.findComponent({ name: 'ElInput' }).exists()).toBe(true)
  })

  it('包含 AI 提供商选择', () => {
    const wrapper = mount(Settings, mountOpts)
    expect(wrapper.findComponent({ name: 'ElSelect' }).exists()).toBe(true)
  })

  it('挂载时加载配置', async () => {
    mount(Settings, mountOpts)
    await vi.dynamicImportSettled()
    expect(api.getSystemConfig).toHaveBeenCalled()
  })

  it('加载配置失败不崩溃', async () => {
    vi.mocked(api.getSystemConfig).mockRejectedValue(new Error('fail'))
    const wrapper = mount(Settings, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    expect(wrapper.find('.settings-page').exists()).toBe(true)
  })
})
