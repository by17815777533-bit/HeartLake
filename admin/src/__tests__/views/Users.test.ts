/**
 * Users.vue 组件测试
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
  default: { getUsers: vi.fn(), banUser: vi.fn(), unbanUser: vi.fn() },
  isRequestCanceled: vi.fn().mockReturnValue(false),
}))
vi.mock('@/utils/errorHelper', () => ({
  getErrorMessage: (_e: any, f: string) => f,
}))
vi.mock('@/composables/useTablePagination', () => ({
  useTablePagination: (_fn: any, _opts: any) => {
    const pagination = { page: 1, pageSize: 20, total: 0 }
    return {
      pagination,
      buildParams: (extra?: Record<string, unknown>) => ({ page: pagination.page, page_size: pagination.pageSize, ...extra }),
      handleSizeChange: vi.fn(),
      handleCurrentChange: vi.fn(),
      handleSearch: vi.fn(),
      handleReset: vi.fn(),
      resetPage: vi.fn(),
    }
  },
}))
vi.mock('@element-plus/icons-vue', () => ({
  Search: defineComponent({ name: 'Search', render() { return h('i') } }),
  Refresh: defineComponent({ name: 'Refresh', render() { return h('i') } }),
}))

import Users from '@/views/Users.vue'
import api from '@/api'

const ElCard = defineComponent({ name: 'ElCard', setup(_, { slots }) { return () => h('div', { class: 'el-card' }, slots.default?.()) } })
const ElForm = defineComponent({ name: 'ElForm', setup(_, { slots }) { return () => h('form', slots.default?.()) } })
const ElFormItem = defineComponent({ name: 'ElFormItem', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElInput = defineComponent({ name: 'ElInput', props: ['modelValue'], emits: ['update:modelValue'], setup(p, { emit }) { return () => h('input', { value: p.modelValue, onInput: (e: any) => emit('update:modelValue', e.target.value) }) } })
const ElSelect = defineComponent({ name: 'ElSelect', props: ['modelValue'], emits: ['update:modelValue'], setup(_, { slots }) { return () => h('select', slots.default?.()) } })
const ElOption = defineComponent({ name: 'ElOption', props: ['label', 'value'], setup(p) { return () => h('option', { value: p.value }, p.label) } })
const ElButton = defineComponent({ name: 'ElButton', emits: ['click'], setup(_, { slots, emit }) { return () => h('button', { onClick: () => emit('click') }, slots.default?.()) } })
const ElTable = defineComponent({ name: 'ElTable', props: ['data'], setup(_, { slots }) { return () => h('table', slots.default?.()) } })
const ElTableColumn = defineComponent({ name: 'ElTableColumn', props: ['prop', 'label'], setup() { return () => h('td') } })
const ElTag = defineComponent({ name: 'ElTag', setup(_, { slots }) { return () => h('span', slots.default?.()) } })
const ElPagination = defineComponent({ name: 'ElPagination', props: ['total', 'currentPage', 'pageSize'], emits: ['update:currentPage', 'update:pageSize', 'size-change', 'current-change'], setup() { return () => h('div') } })
const ElDialog = defineComponent({ name: 'ElDialog', props: ['modelValue'], setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElDescriptions = defineComponent({ name: 'ElDescriptions', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElDescriptionsItem = defineComponent({ name: 'ElDescriptionsItem', setup(_, { slots }) { return () => h('div', slots.default?.()) } })
const ElIcon = defineComponent({ name: 'ElIcon', setup(_, { slots }) { return () => h('i', slots.default?.()) } })

const mountOpts = {
  global: {
    components: { ElCard, ElForm, ElFormItem, ElInput, ElSelect, ElOption, ElButton, ElTable, ElTableColumn, ElTag, ElPagination, ElDialog, ElDescriptions, ElDescriptionsItem, ElIcon },
  },
}

describe('Users.vue', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    setActivePinia(createPinia())
    vi.mocked(api.getUsers).mockResolvedValue({ data: { data: { users: [], total: 0 } } } as any)
  })

  it('渲染用户页面', () => {
    const wrapper = mount(Users, mountOpts)
    expect(wrapper.find('.users-page').exists()).toBe(true)
  })

  it('挂载时调用 getUsers', async () => {
    mount(Users, mountOpts)
    await vi.dynamicImportSettled()
    expect(api.getUsers).toHaveBeenCalled()
  })

  it('包含筛选卡片', () => {
    const wrapper = mount(Users, mountOpts)
    expect(wrapper.findComponent({ name: 'ElCard' }).exists()).toBe(true)
  })

  it('包含用户表格', () => {
    const wrapper = mount(Users, mountOpts)
    expect(wrapper.findComponent({ name: 'ElTable' }).exists()).toBe(true)
  })

  it('包含分页组件', () => {
    const wrapper = mount(Users, mountOpts)
    expect(wrapper.findComponent({ name: 'ElPagination' }).exists()).toBe(true)
  })

  it('getUsers 失败时用户列表为空', async () => {
    vi.mocked(api.getUsers).mockRejectedValue(new Error('fail'))
    const wrapper = mount(Users, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    const table = wrapper.findComponent({ name: 'ElTable' })
    expect(table.props('data')).toEqual([])
  })

  it('getUsers 返回用户数据', async () => {
    vi.mocked(api.getUsers).mockResolvedValue({
      data: { data: { users: [{ user_id: 'u1', nickname: '测试', status: 'active' }], total: 1 } },
    } as any)
    const wrapper = mount(Users, mountOpts)
    await vi.dynamicImportSettled()
    await new Promise(r => setTimeout(r, 10))
    const table = wrapper.findComponent({ name: 'ElTable' })
    expect(table.props('data')).toHaveLength(1)
  })

  it('包含搜索按钮', () => {
    const wrapper = mount(Users, mountOpts)
    const buttons = wrapper.findAllComponents({ name: 'ElButton' })
    expect(buttons.length).toBeGreaterThanOrEqual(2)
  })

  it('包含状态筛选', () => {
    const wrapper = mount(Users, mountOpts)
    expect(wrapper.findComponent({ name: 'ElSelect' }).exists()).toBe(true)
  })

  it('包含详情弹窗', () => {
    const wrapper = mount(Users, mountOpts)
    expect(wrapper.findComponent({ name: 'ElDialog' }).exists()).toBe(true)
  })

  it('有 aria-label 用户筛选', () => {
    const wrapper = mount(Users, mountOpts)
    expect(wrapper.find('[aria-label="用户筛选"]').exists()).toBe(true)
  })

  it('有 aria-label 用户列表', () => {
    const wrapper = mount(Users, mountOpts)
    expect(wrapper.find('[aria-label="用户列表"]').exists()).toBe(true)
  })
})
