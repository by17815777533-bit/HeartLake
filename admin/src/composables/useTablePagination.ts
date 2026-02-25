/**
 * @file useTablePagination.js
 * @brief 表格分页 + 搜索 + 重置 composable
 */

import { reactive } from 'vue'
import type { TablePaginationOptions } from '@/types'

export function useTablePagination(fetchData: () => void, options: TablePaginationOptions = {}) {
  const {
    defaultPageSize = 20,
    filters = null,
    defaultFilters = null,
    beforeSearch = null,
  } = options

  const pagination = reactive({
    page: 1,
    pageSize: defaultPageSize,
    total: 0,
  })

  const handleSizeChange = (size: number) => {
    pagination.pageSize = size
    pagination.page = 1
    fetchData()
  }

  const handleCurrentChange = (page: number) => {
    pagination.page = page
    fetchData()
  }

  const handleSearch = () => {
    if (beforeSearch && beforeSearch() === false) return
    pagination.page = 1
    fetchData()
  }

  const handleReset = () => {
    if (filters && defaultFilters) {
      Object.assign(filters, defaultFilters)
    }
    pagination.page = 1
    fetchData()
  }

  const resetPage = () => {
    pagination.page = 1
  }

  return {
    pagination,
    handleSizeChange,
    handleCurrentChange,
    handleSearch,
    handleReset,
    resetPage,
  }
}
