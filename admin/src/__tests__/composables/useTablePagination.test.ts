/**
 * @file useTablePagination.test.ts
 * @brief useTablePagination composable 测试
 */
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { useTablePagination } from '@/composables/useTablePagination'
import { reactive } from 'vue'

describe('useTablePagination', () => {
  let fetchData: ReturnType<typeof vi.fn>

  beforeEach(() => {
    fetchData = vi.fn()
  })

  describe('初始状态', () => {
    it('默认 page 为 1', () => {
      const { pagination } = useTablePagination(fetchData)
      expect(pagination.page).toBe(1)
    })

    it('默认 pageSize 为 20', () => {
      const { pagination } = useTablePagination(fetchData)
      expect(pagination.pageSize).toBe(20)
    })

    it('默认 total 为 0', () => {
      const { pagination } = useTablePagination(fetchData)
      expect(pagination.total).toBe(0)
    })

    it('自定义 defaultPageSize 为 50', () => {
      const { pagination } = useTablePagination(fetchData, { defaultPageSize: 50 })
      expect(pagination.pageSize).toBe(50)
    })

    it('自定义 defaultPageSize 为 10', () => {
      const { pagination } = useTablePagination(fetchData, { defaultPageSize: 10 })
      expect(pagination.pageSize).toBe(10)
    })

    it('自定义 defaultPageSize 为 100', () => {
      const { pagination } = useTablePagination(fetchData, { defaultPageSize: 100 })
      expect(pagination.pageSize).toBe(100)
    })
  })
  describe('handleSizeChange', () => {
    it('改变 pageSize', () => {
      const { pagination, handleSizeChange } = useTablePagination(fetchData)
      handleSizeChange(50)
      expect(pagination.pageSize).toBe(50)
    })

    it('改变 pageSize 后 page 重置为 1', () => {
      const { pagination, handleCurrentChange, handleSizeChange } = useTablePagination(fetchData)
      handleCurrentChange(3)
      handleSizeChange(50)
      expect(pagination.page).toBe(1)
    })

    it('改变 pageSize 后调用 fetchData', () => {
      const { handleSizeChange } = useTablePagination(fetchData)
      handleSizeChange(50)
      expect(fetchData).toHaveBeenCalled()
    })

    it('pageSize 改为 10', () => {
      const { pagination, handleSizeChange } = useTablePagination(fetchData)
      handleSizeChange(10)
      expect(pagination.pageSize).toBe(10)
    })

    it('连续改变 pageSize 取最后值', () => {
      const { pagination, handleSizeChange } = useTablePagination(fetchData)
      handleSizeChange(10)
      handleSizeChange(50)
      handleSizeChange(100)
      expect(pagination.pageSize).toBe(100)
      expect(fetchData).toHaveBeenCalledTimes(3)
    })
  })

  describe('handleCurrentChange', () => {
    it('改变当前页码', () => {
      const { pagination, handleCurrentChange } = useTablePagination(fetchData)
      handleCurrentChange(5)
      expect(pagination.page).toBe(5)
    })

    it('改变页码后调用 fetchData', () => {
      const { handleCurrentChange } = useTablePagination(fetchData)
      handleCurrentChange(2)
      expect(fetchData).toHaveBeenCalled()
    })

    it('翻回第 1 页', () => {
      const { pagination, handleCurrentChange } = useTablePagination(fetchData)
      handleCurrentChange(3)
      handleCurrentChange(1)
      expect(pagination.page).toBe(1)
    })

    it('翻到大页码', () => {
      const { pagination, handleCurrentChange } = useTablePagination(fetchData)
      handleCurrentChange(999)
      expect(pagination.page).toBe(999)
    })

    it('连续翻页调用次数正确', () => {
      const { handleCurrentChange } = useTablePagination(fetchData)
      handleCurrentChange(2)
      handleCurrentChange(3)
      handleCurrentChange(4)
      expect(fetchData).toHaveBeenCalledTimes(3)
    })
  })
  describe('handleSearch', () => {
    it('搜索时 page 重置为 1', () => {
      const { pagination, handleCurrentChange, handleSearch } = useTablePagination(fetchData)
      handleCurrentChange(5)
      fetchData.mockClear()
      handleSearch()
      expect(pagination.page).toBe(1)
    })

    it('搜索时调用 fetchData', () => {
      const { handleSearch } = useTablePagination(fetchData)
      handleSearch()
      expect(fetchData).toHaveBeenCalled()
    })

    it('beforeSearch 返回 false 时不执行搜索', () => {
      const beforeSearch = vi.fn(() => false)
      const { handleSearch } = useTablePagination(fetchData, { beforeSearch })
      handleSearch()
      expect(beforeSearch).toHaveBeenCalled()
      expect(fetchData).not.toHaveBeenCalled()
    })

    it('beforeSearch 返回 undefined 时正常搜索', () => {
      const beforeSearch = vi.fn(() => undefined)
      const { handleSearch } = useTablePagination(fetchData, { beforeSearch })
      handleSearch()
      expect(fetchData).toHaveBeenCalled()
    })

    it('beforeSearch 返回 true 时正常搜索', () => {
      const beforeSearch = vi.fn(() => true)
      const { handleSearch } = useTablePagination(fetchData, { beforeSearch })
      handleSearch()
      expect(fetchData).toHaveBeenCalled()
    })

    it('没有 beforeSearch 时正常搜索', () => {
      const { handleSearch } = useTablePagination(fetchData, {})
      handleSearch()
      expect(fetchData).toHaveBeenCalled()
    })
  })

  describe('handleReset', () => {
    it('重置时 page 回到 1', () => {
      const { pagination, handleCurrentChange, handleReset } = useTablePagination(fetchData)
      handleCurrentChange(5)
      fetchData.mockClear()
      handleReset()
      expect(pagination.page).toBe(1)
    })

    it('重置时调用 fetchData', () => {
      const { handleReset } = useTablePagination(fetchData)
      handleReset()
      expect(fetchData).toHaveBeenCalled()
    })

    it('重置时恢复 filters 到默认值', () => {
      const filters = reactive({ keyword: '', status: '' })
      const defaultFilters = { keyword: '', status: '' }
      const { handleReset } = useTablePagination(fetchData, { filters, defaultFilters })
      filters.keyword = '测试'
      filters.status = 'active'
      handleReset()
      expect(filters.keyword).toBe('')
      expect(filters.status).toBe('')
    })

    it('没有 filters 和 defaultFilters 时仅重置页码', () => {
      const { pagination, handleCurrentChange, handleReset } = useTablePagination(fetchData)
      handleCurrentChange(3)
      fetchData.mockClear()
      handleReset()
      expect(pagination.page).toBe(1)
      expect(fetchData).toHaveBeenCalledTimes(1)
    })

    it('filters 存在但 defaultFilters 为 null 时不恢复', () => {
      const filters = reactive({ keyword: '测试' })
      const { handleReset } = useTablePagination(fetchData, { filters, defaultFilters: null })
      handleReset()
      expect(filters.keyword).toBe('测试')
    })
  })

  describe('resetPage', () => {
    it('仅重置页码不调用 fetchData', () => {
      const { pagination, handleCurrentChange, resetPage } = useTablePagination(fetchData)
      handleCurrentChange(5)
      fetchData.mockClear()
      resetPage()
      expect(pagination.page).toBe(1)
      expect(fetchData).not.toHaveBeenCalled()
    })

    it('多次 resetPage 幂等', () => {
      const { pagination, resetPage } = useTablePagination(fetchData)
      resetPage()
      resetPage()
      resetPage()
      expect(pagination.page).toBe(1)
    })
  })

  describe('边界值', () => {
    it('total 可以手动设置', () => {
      const { pagination } = useTablePagination(fetchData)
      pagination.total = 1000
      expect(pagination.total).toBe(1000)
    })

    it('total 设为 0', () => {
      const { pagination } = useTablePagination(fetchData)
      pagination.total = 0
      expect(pagination.total).toBe(0)
    })

    it('pageSize 为 1 的极端情况', () => {
      const { pagination, handleSizeChange } = useTablePagination(fetchData)
      handleSizeChange(1)
      expect(pagination.pageSize).toBe(1)
    })

    it('返回的对象包含所有方法', () => {
      const result = useTablePagination(fetchData)
      expect(result).toHaveProperty('pagination')
      expect(result).toHaveProperty('handleSizeChange')
      expect(result).toHaveProperty('handleCurrentChange')
      expect(result).toHaveProperty('handleSearch')
      expect(result).toHaveProperty('handleReset')
      expect(result).toHaveProperty('resetPage')
    })

    it('pagination 是响应式的', () => {
      const { pagination } = useTablePagination(fetchData)
      pagination.page = 10
      expect(pagination.page).toBe(10)
      pagination.total = 500
      expect(pagination.total).toBe(500)
    })
  })
})