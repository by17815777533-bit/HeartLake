/**
 * 表格分页 + 搜索 + 重置 + loading 状态 composable
 */

import { reactive, ref } from 'vue'
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

  // L-24: 集成 loading 状态，供调用方直接使用
  const tableLoading = ref(false)

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

  /**
   * M-17: 统一构建分页请求参数
   * 内部使用 camelCase (pageSize)，发送给后端统一转为 snake_case (page_size)
   * 调用方无需再手动拼 page_size，直接 buildParams(extraFilters) 即可
   */
  const buildParams = (extra?: Record<string, unknown>) => {
    const params: Record<string, unknown> = {
      page: pagination.page,
      page_size: pagination.pageSize,
    }
    if (extra) {
      Object.entries(extra).forEach(([k, v]) => {
        if (v !== '' && v !== null && v !== undefined) {
          params[k] = v
        }
      })
    }
    return params
  }

  return {
    pagination,
    tableLoading,
    handleSizeChange,
    handleCurrentChange,
    handleSearch,
    handleReset,
    resetPage,
    buildParams,
  }
}
