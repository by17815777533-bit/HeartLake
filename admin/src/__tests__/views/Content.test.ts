/**
 * @file views/Content.test.ts
 * @brief Content.vue 组件测试
 */
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia, setActivePinia } from 'pinia'
import { defineComponent, h } from 'vue'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), success: vi.fn(), warning: vi.fn() },
  ElMessageBox: { prompt: vi.fn().mockResolvedValue({ value: '违规' }), confirm: vi.fn().mockResolvedValue('confirm') },
}))

vi.mock('@/api', () => ({
  default: { getStones: vi.fn(), getBoats: vi.fn(), deleteStone: vi.fn(), deleteBoat: vi.fn() },
}))
vi.mock('@/utils/errorHelper', () => ({ getErrorMessage: (_e: any, f: string) => f }))
vi.mock('@/composables/useTablePagination', () => ({
  useTablePagination: (_fn: any, _opts: any) => {
    const pagination = { page: 1, pageSize: 20, total: 0 }
    return { pagination, handleSizeChange: vi.fn(), handleCurrentChange: vi.fn(), handleSearch: vi.fn(), handleReset: vi.fn(), resetPage: vi.fn() }
  },
}))

import Content from '@/views/Content.vue'
import api from '@/api'

const ElCard = defineComponent({ name: 'ElCard', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElForm = defineComponent({ name: 'ElForm', setup(_, { slots }) { return () => h('form', slots.default?.()) } })
const ElFormItem = defineComponent({ name: 'ElFormItem', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElInput = defineComponent({ name: 'ElInput', props: ['modelValue'], emits: ['update:modelValue'], setup(p, { emit }) { return () => h('input', { value: p.modelValue, onInput: (e: any) => emit('update:modelValue', e.target.value) }) } })
const ElSelect = defineComponent({ name: 'ElSelect', props: ['modelValue'], emits: ['update:modelValue'], setup(_, { slots }) { return () => h('select', slots.default?.()) } })
const ElOption = defineComponent({ name: 'ElOption', props: ['label', 'value'], setup(p) { return () => h('option', { value: p.value }, p.label) } })
const ElButton = defineComponent({ name: 'ElButton', emits: ['click'], setup(_, { slots, emit }) { return () => h('button', { onClick: () => emit('click') }, slots.default?.()) } })
const ElTable = defineComponent({ name: 'ElTable', props: ['data'], setup(_, { slots }) { return () => h('table', slots.default?.()) } })
const ElTableColumn = defineComponent({ name: 'ElTableColumn', props: ['prop', 'label'], setup() { return () => h('td') } })
const ElTag = defineComponent({ name: 'ElTag', setup(_, { slots }) { return () => h('span', slots.default?.()) } })
const ElPagination = defineComponent({ name: 'ElPagination', props: ['total'], emits: ['size-change', 'current-change'], setup() { return () => h('div') } })
const ElDialog = defineComponent({ name: 'ElDialog', props: ['modelValue'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElDescriptions = defineComponent({ name: 'ElDescriptions', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElDescriptionsItem = defineComponent({ name: 'ElDescriptionsItem', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElTabs = defineComponent({ name: 'ElTabs', props: ['modelValue'], emits: ['update:modelValue'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElTabPane = defineComponent({ name: 'ElTabPane', props: ['label', 'name'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })

const mountOpts = {
  global: {
    components: { ElCard, ElForm, ElFormItem, ElInput, ElSelect, ElOption, ElButton, ElTable, ElTableColumn, ElTag, ElPagination, ElDialog, ElDescriptions, ElDescriptionsItem, ElTabs, ElTabPane },
  },
}

describe('Content.vue', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    setActivePinia(createPinia())
    vi.mocked(api.getStones).mockResolvedValue({ data: { data: { stones: [], total: 0 } } } as any)
    vi.mocked(api.getBoats).mockResolvedValue({ data: { data: { boats: [], total: 0 } } } as any)
  })
  it('渲染内容页面', () => {
    const wrapper = mount(Content, mountOpts)
    expect(wrapper.find('.content-page').exists()).toBe(true)
  })

  it('挂载时调用 getStones', async () => {
    mount(Content, mountOpts)
    await vi.dynamicImportSettled()
    expect(api.getStones).toHaveBeenCalled()
  })

  it('包含筛选表单', () => {
    const wrapper = mount(Content, mountOpts)
    expect(wrapper.findComponent({ name: 'ElForm' }).exists()).toBe(true)
  })

  it('包含内容表格', () => {
    const wrapper = mount(Content, mountOpts)
    expect(wrapper.findComponent({ name: 'ElTable' }).exists()).toBe(true)
  })

  it('包含分页', () => {
    const wrapper = mount(Content, mountOpts)
    expect(wrapper.findComponent({ name: 'ElPagination' }).exists()).toBe(true)
  })

  it('包含详情弹窗', () => {
    const wrapper = mount(Content, mountOpts)
    expect(wrapper.findComponent({ name: 'ElDialog' }).exists()).toBe(true)
  })

  it('有 aria-label 内容筛选', () => {
    const wrapper = mount(Content, mountOpts)
    expect(wrapper.find('[aria-label="内容筛选"]').exists()).toBe(true)
  })

  it('接口失败时列表为空', async () => {
    vi.mocked(api.getStones).mockRejectedValue(new Error('fail'))
    const wrapper = mount(Content, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    const table = wrapper.findComponent({ name: 'ElTable' })
    expect(table.props('data')).toEqual([])
  })

  it('返回数据正确渲染', async () => {
    vi.mocked(api.getStones).mockResolvedValue({
      data: { data: { stones: [{ stone_id: 's1', content: '测试', status: 'published', created_at: '2025-01-01' }], total: 1 } },
    } as any)
    const wrapper = mount(Content, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    const table = wrapper.findComponent({ name: 'ElTable' })
    expect(table.props('data')).toHaveLength(1)
  })

  it('包含搜索和重置按钮', () => {
    const wrapper = mount(Content, mountOpts)
    const buttons = wrapper.findAllComponents({ name: 'ElButton' })
    expect(buttons.length).toBeGreaterThanOrEqual(2)
  })

  it('包含类型筛选', () => {
    const wrapper = mount(Content, mountOpts)
    expect(wrapper.findComponent({ name: 'ElSelect' }).exists()).toBe(true)
  })
})
