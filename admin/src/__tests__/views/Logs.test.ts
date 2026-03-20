/**
 * Logs.vue 组件测试
 */
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia, setActivePinia } from 'pinia'
import { defineComponent, h } from 'vue'

vi.mock('element-plus', () => ({
  ElMessage: { error: vi.fn(), success: vi.fn(), warning: vi.fn() },
}))

vi.mock('@/api', () => ({
  default: { getOperationLogs: vi.fn() },
  isRequestCanceled: vi.fn().mockReturnValue(false),
}))
vi.mock('@/utils/errorHelper', () => ({ getErrorMessage: (_e: any, f: string) => f }))
vi.mock('@/composables/useTablePagination', () => ({
  useTablePagination: (_fn: any, _opts: any) => {
    const pagination = { page: 1, pageSize: 20, total: 0 }
    return { pagination, buildParams: (extra?: Record<string, unknown>) => ({ page: pagination.page, page_size: pagination.pageSize, ...extra }), handleSizeChange: vi.fn(), handleCurrentChange: vi.fn(), handleSearch: vi.fn(), handleReset: vi.fn(), resetPage: vi.fn() }
  },
}))

import Logs from '@/views/Logs.vue'
import api from '@/api'
import { ElMessage } from 'element-plus'

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
const ElDatePicker = defineComponent({ name: 'ElDatePicker', props: ['modelValue'], emits: ['update:modelValue'], setup() { return () => h('div') } })

const mountOpts = {
  global: {
    components: { ElCard, ElForm, ElFormItem, ElInput, ElSelect, ElOption, ElButton, ElTable, ElTableColumn, ElTag, ElPagination, ElDatePicker },
  },
}

describe('Logs.vue', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    setActivePinia(createPinia())
    vi.mocked(api.getOperationLogs).mockResolvedValue({ data: { list: [], total: 0 } } as any)
  })

  it('渲染日志页面', () => {
    const wrapper = mount(Logs, mountOpts)
    expect(wrapper.find('.logs-page').exists()).toBe(true)
  })
  it('挂载时调用 getOperationLogs', async () => {
    mount(Logs, mountOpts)
    await vi.dynamicImportSettled()
    expect(api.getOperationLogs).toHaveBeenCalled()
  })

  it('包含筛选卡片', () => {
    const wrapper = mount(Logs, mountOpts)
    expect(wrapper.findComponent({ name: 'ElCard' }).exists()).toBe(true)
  })

  it('包含日志表格', () => {
    const wrapper = mount(Logs, mountOpts)
    expect(wrapper.findComponent({ name: 'ElTable' }).exists()).toBe(true)
  })

  it('包含分页', () => {
    const wrapper = mount(Logs, mountOpts)
    expect(wrapper.findComponent({ name: 'ElPagination' }).exists()).toBe(true)
  })

  it('包含操作类型筛选', () => {
    const wrapper = mount(Logs, mountOpts)
    expect(wrapper.findComponent({ name: 'ElSelect' }).exists()).toBe(true)
  })

  it('包含时间范围选择器', () => {
    const wrapper = mount(Logs, mountOpts)
    expect(wrapper.findComponent({ name: 'ElDatePicker' }).exists()).toBe(true)
  })

  it('接口失败时列表为空', async () => {
    vi.mocked(api.getOperationLogs).mockRejectedValue(new Error('fail'))
    const wrapper = mount(Logs, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    const table = wrapper.findComponent({ name: 'ElTable' })
    expect(table.props('data')).toEqual([])
  })

  it('返回日志数据', async () => {
    vi.mocked(api.getOperationLogs).mockResolvedValue({ data: { list: [{ id: 1, operator: 'admin', action: 'login', created_at: '2025-01-01' }], total: 1 } } as any)
    const wrapper = mount(Logs, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    const table = wrapper.findComponent({ name: 'ElTable' })
    expect(table.props('data')).toHaveLength(1)
  })

  it('失败时显示错误消息', async () => {
    vi.mocked(api.getOperationLogs).mockRejectedValue(new Error('fail'))
    mount(Logs, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    expect(ElMessage.error).toHaveBeenCalled()
  })

  it('包含搜索和重置按钮', () => {
    const wrapper = mount(Logs, mountOpts)
    const buttons = wrapper.findAllComponents({ name: 'ElButton' })
    expect(buttons.length).toBeGreaterThanOrEqual(2)
  })
})
