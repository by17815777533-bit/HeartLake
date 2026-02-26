/**
 * @file views/SensitiveWords.test.ts
 * @brief SensitiveWords.vue 组件测试
 */
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia, setActivePinia } from 'pinia'
import { defineComponent, h } from 'vue'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), success: vi.fn(), warning: vi.fn() },
}))

vi.mock('@/api', () => ({
  default: { getSensitiveWords: vi.fn(), addSensitiveWord: vi.fn(), updateSensitiveWord: vi.fn(), deleteSensitiveWord: vi.fn() },
}))
vi.mock('@/utils/errorHelper', () => ({ getErrorMessage: (_e: any, f: string) => f }))
vi.mock('@/composables/useTablePagination', () => ({
  useTablePagination: (_fn: any, _opts: any) => {
    const pagination = { page: 1, pageSize: 20, total: 0 }
    return { pagination, buildParams: (extra?: Record<string, unknown>) => ({ page: pagination.page, page_size: pagination.pageSize, ...extra }), handleSizeChange: vi.fn(), handleCurrentChange: vi.fn(), handleSearch: vi.fn(), handleReset: vi.fn(), resetPage: vi.fn() }
  },
}))

import SensitiveWords from '@/views/SensitiveWords.vue'
import api from '@/api'
import { ElMessage } from 'element-plus'

const ElCard = defineComponent({ name: 'ElCard', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElForm = defineComponent({ name: 'ElForm', setup(_, { slots, expose }) { expose({ validate: () => Promise.resolve(true) }); return () => h('form', slots.default?.()) } })
const ElFormItem = defineComponent({ name: 'ElFormItem', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElInput = defineComponent({ name: 'ElInput', props: ['modelValue'], emits: ['update:modelValue', 'keyup'], setup(p, { emit }) { return () => h('input', { value: p.modelValue, onInput: (e: any) => emit('update:modelValue', e.target.value) }) } })
const ElSelect = defineComponent({ name: 'ElSelect', props: ['modelValue'], emits: ['update:modelValue'], setup(_, { slots }) { return () => h('select', slots.default?.()) } })
const ElOption = defineComponent({ name: 'ElOption', props: ['label', 'value'], setup(p) { return () => h('option', { value: p.value }, p.label) } })
const ElButton = defineComponent({ name: 'ElButton', emits: ['click'], setup(_, { slots, emit }) { return () => h('button', { onClick: () => emit('click') }, slots.default?.()) } })
const ElTable = defineComponent({ name: 'ElTable', props: ['data'], setup(_, { slots }) { return () => h('table', slots.default?.()) } })
const ElTableColumn = defineComponent({ name: 'ElTableColumn', props: ['prop', 'label'], setup() { return () => h('td') } })
const ElTag = defineComponent({ name: 'ElTag', setup(_, { slots }) { return () => h('span', slots.default?.()) } })
const ElPagination = defineComponent({ name: 'ElPagination', props: ['total'], emits: ['size-change', 'current-change'], setup() { return () => h('div') } })
const ElDialog = defineComponent({ name: 'ElDialog', props: ['modelValue', 'title'], emits: ['update:modelValue'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElPopconfirm = defineComponent({ name: 'ElPopconfirm', props: ['title'], emits: ['confirm'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })

const mountOpts = {
  global: {
    components: { ElCard, ElForm, ElFormItem, ElInput, ElSelect, ElOption, ElButton, ElTable, ElTableColumn, ElTag, ElPagination, ElDialog, ElPopconfirm },
  },
}

describe('SensitiveWords.vue', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    setActivePinia(createPinia())
    vi.mocked(api.getSensitiveWords).mockResolvedValue({ data: { words: [], total: 0 } } as any)
  })
  it('渲染敏感词页面', () => {
    const wrapper = mount(SensitiveWords, mountOpts)
    expect(wrapper.find('.sensitive-words-page').exists()).toBe(true)
  })

  it('挂载时调用 getSensitiveWords', async () => {
    mount(SensitiveWords, mountOpts)
    await vi.dynamicImportSettled()
    expect(api.getSensitiveWords).toHaveBeenCalled()
  })

  it('包含筛选卡片', () => {
    const wrapper = mount(SensitiveWords, mountOpts)
    expect(wrapper.findComponent({ name: 'ElCard' }).exists()).toBe(true)
  })

  it('包含敏感词表格', () => {
    const wrapper = mount(SensitiveWords, mountOpts)
    expect(wrapper.findComponent({ name: 'ElTable' }).exists()).toBe(true)
  })

  it('包含分页', () => {
    const wrapper = mount(SensitiveWords, mountOpts)
    expect(wrapper.findComponent({ name: 'ElPagination' }).exists()).toBe(true)
  })

  it('包含添加按钮', () => {
    const wrapper = mount(SensitiveWords, mountOpts)
    const buttons = wrapper.findAllComponents({ name: 'ElButton' })
    expect(buttons.length).toBeGreaterThanOrEqual(1)
  })

  it('包含编辑弹窗', () => {
    const wrapper = mount(SensitiveWords, mountOpts)
    expect(wrapper.findComponent({ name: 'ElDialog' }).exists()).toBe(true)
  })

  it('接口失败时列表为空', async () => {
    vi.mocked(api.getSensitiveWords).mockRejectedValue(new Error('fail'))
    const wrapper = mount(SensitiveWords, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    const table = wrapper.findComponent({ name: 'ElTable' })
    expect(table.props('data')).toEqual([])
  })

  it('返回敏感词数据', async () => {
    vi.mocked(api.getSensitiveWords).mockResolvedValue({ data: { words: [{ id: 1, word: '测试', level: 'high' }], total: 1 } } as any)
    const wrapper = mount(SensitiveWords, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    const table = wrapper.findComponent({ name: 'ElTable' })
    expect(table.props('data')).toHaveLength(1)
  })

  it('包含级别筛选', () => {
    const wrapper = mount(SensitiveWords, mountOpts)
    expect(wrapper.findComponent({ name: 'ElSelect' }).exists()).toBe(true)
  })

  it('失败时显示错误消息', async () => {
    vi.mocked(api.getSensitiveWords).mockRejectedValue(new Error('fail'))
    mount(SensitiveWords, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    expect(ElMessage.error).toHaveBeenCalled()
  })
})
