/**
 * 表格分页、搜索、重置与 loading 状态的通用 composable。
 *
 * 使用方式：
 * ```ts
 * const { pagination, tableLoading, handleSearch, buildParams } = useTablePagination(fetchData, {
 *   filters: myFilters,
 *   defaultFilters: { status: '' },
 *   beforeSearch: () => { if (invalid) return false },
 * })
 * ```
 *
 * 设计要点：
 * - pagination 是 reactive 对象，可直接绑定 el-pagination 的 v-model
 * - buildParams 自动将 camelCase 的 pageSize 转为后端 snake_case 的 page_size
 * - handleReset 会用 defaultFilters 覆盖 filters 并回到第一页
 * - beforeSearch 钩子返回 false 可阻止本次搜索（用于前端校验）
 */

import { reactive, ref } from 'vue'
import type { TablePaginationOptions } from '@/types'

/**
 * @param fetchData - 数据拉取函数，分页/搜索/重置后自动调用
 * @param options - 可选配置：默认页大小、筛选条件、搜索前校验
 * @returns 分页状态、loading 标志、事件处理函数、参数构建器
 */
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

  // 表格 loading 状态，供调用方直接绑定 v-loading
  const tableLoading = ref(false)

  /** 切换每页条数时回到第一页，避免越界 */
  const handleSizeChange = (size: number) => {
    pagination.pageSize = size
    pagination.page = 1
    fetchData()
  }

  /** 翻页 */
  const handleCurrentChange = (page: number) => {
    pagination.page = page
    fetchData()
  }

  /** 搜索：先执行 beforeSearch 校验，通过后回到第一页并拉取数据 */
  const handleSearch = () => {
    if (beforeSearch && beforeSearch() === false) return
    pagination.page = 1
    fetchData()
  }

  /** 重置筛选条件为初始值并重新拉取 */
  const handleReset = () => {
    if (filters && defaultFilters) {
      Object.assign(filters, defaultFilters)
    }
    pagination.page = 1
    fetchData()
  }

  /** 仅重置页码（不触发请求），用于外部手动控制场景 */
  const resetPage = () => {
    pagination.page = 1
  }

  /**
   * 构建分页请求参数。
   * 内部 camelCase (pageSize) 发送给后端时转为 snake_case (page_size)，
   * 调用方直接 buildParams(extraFilters) 即可。
   * @param extra - 额外筛选参数，空值会被自动过滤
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
