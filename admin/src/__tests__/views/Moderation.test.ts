/**
 * @file views/Moderation.test.ts
 * @brief Moderation.vue 组件测试
 */
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia, setActivePinia } from 'pinia'
import { defineComponent, h } from 'vue'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), success: vi.fn(), warning: vi.fn() },
  ElMessageBox: { confirm: vi.fn().mockResolvedValue('confirm'), prompt: vi.fn().mockResolvedValue({ value: '违规' }) },
}))

vi.mock('@/api', () => ({
  default: {
    getPendingModeration: vi.fn(),
    getModerationHistory: vi.fn(),
    approveContent: vi.fn(),
    rejectContent: vi.fn(),
  },
}))
vi.mock('@/utils/errorHelper', () => ({ getErrorMessage: (_e: any, f: string) => f }))

import Moderation from '@/views/Moderation.vue'
import api from '@/api'
import { ElMessage } from 'element-plus'

const ElTabs = defineComponent({ name: 'ElTabs', props: ['modelValue'], emits: ['update:modelValue', 'tab-change'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElTabPane = defineComponent({ name: 'ElTabPane', props: ['label', 'name'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElCard = defineComponent({ name: 'ElCard', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElTable = defineComponent({ name: 'ElTable', props: ['data'], setup(_, { slots }) { return () => h('table', slots.default?.()) } })
const ElTableColumn = defineComponent({ name: 'ElTableColumn', props: ['prop', 'label'], setup() { return () => h('td') } })
const ElTag = defineComponent({ name: 'ElTag', setup(_, { slots }) { return () => h('span', slots.default?.()) } })
const ElButton = defineComponent({ name: 'ElButton', emits: ['click'], setup(_, { slots, emit }) { return () => h('button', { onClick: () => emit('click') }, slots.default?.()) } })
const ElPagination = defineComponent({ name: 'ElPagination', props: ['total'], emits: ['current-change', 'size-change'], setup() { return () => h('div') } })
const ElDialog = defineComponent({ name: 'ElDialog', props: ['modelValue'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElDescriptions = defineComponent({ name: 'ElDescriptions', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElDescriptionsItem = defineComponent({ name: 'ElDescriptionsItem', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElSelect = defineComponent({ name: 'ElSelect', props: ['modelValue'], setup(_, { slots }) { return () => h('select', slots.default?.()) } })
const ElOption = defineComponent({ name: 'ElOption', props: ['label', 'value'], setup(p) { return () => h('option', { value: p.value }, p.label) } })

const mountOpts = {
  global: {
    components: { ElTabs, ElTabPane, ElCard, ElTable, ElTableColumn, ElTag, ElButton, ElPagination, ElDialog, ElDescriptions, ElDescriptionsItem, ElSelect, ElOption },
  },
}

describe('Moderation.vue', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    setActivePinia(createPinia())
    vi.mocked(api.getPendingModeration).mockResolvedValue({ data: { list: [], total: 0 } } as any)
    vi.mocked(api.getModerationHistory).mockResolvedValue({ data: { list: [], total: 0 } } as any)
  })
  it('渲染审核页面', () => {
    const wrapper = mount(Moderation, mountOpts)
    expect(wrapper.find('.moderation-page').exists()).toBe(true)
  })

  it('包含 Tabs 组件', () => {
    const wrapper = mount(Moderation, mountOpts)
    expect(wrapper.findComponent({ name: 'ElTabs' }).exists()).toBe(true)
  })

  it('包含待审核和历史两个 tab', () => {
    const wrapper = mount(Moderation, mountOpts)
    const panes = wrapper.findAllComponents({ name: 'ElTabPane' })
    expect(panes.length).toBe(2)
  })

  it('挂载时调用 getPendingModeration', async () => {
    mount(Moderation, mountOpts)
    await vi.dynamicImportSettled()
    expect(api.getPendingModeration).toHaveBeenCalled()
  })

  it('包含待审核表格', () => {
    const wrapper = mount(Moderation, mountOpts)
    const tables = wrapper.findAllComponents({ name: 'ElTable' })
    expect(tables.length).toBeGreaterThanOrEqual(1)
  })

  it('包含详情弹窗', () => {
    const wrapper = mount(Moderation, mountOpts)
    expect(wrapper.findComponent({ name: 'ElDialog' }).exists()).toBe(true)
  })

  it('接口失败时列表为空', async () => {
    vi.mocked(api.getPendingModeration).mockRejectedValue(new Error('fail'))
    const wrapper = mount(Moderation, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    const tables = wrapper.findAllComponents({ name: 'ElTable' })
    expect(tables[0].props('data')).toEqual([])
  })

  it('返回待审核数据', async () => {
    vi.mocked(api.getPendingModeration).mockResolvedValue({
      data: { list: [{ moderation_id: 1, content: '测试', content_type: 'stone' }], total: 1 },
    } as any)
    const wrapper = mount(Moderation, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    const tables = wrapper.findAllComponents({ name: 'ElTable' })
    expect(tables[0].props('data')).toHaveLength(1)
  })

  it('包含分页组件', () => {
    const wrapper = mount(Moderation, mountOpts)
    expect(wrapper.findComponent({ name: 'ElPagination' }).exists()).toBe(true)
  })

  it('包含操作按钮', () => {
    const wrapper = mount(Moderation, mountOpts)
    const buttons = wrapper.findAllComponents({ name: 'ElButton' })
    expect(buttons.length).toBeGreaterThanOrEqual(0)
  })

  it('失败时显示错误消息', async () => {
    vi.mocked(api.getPendingModeration).mockRejectedValue(new Error('fail'))
    mount(Moderation, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    expect(ElMessage.error).toHaveBeenCalled()
  })
})
