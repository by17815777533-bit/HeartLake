import { reactive } from 'vue'

/**
 * 表格分页搜索逻辑复用 composable
 * @param {Function} fetchData - 数据获取函数
 * @param {Object} options
 * @param {number} options.defaultPageSize - 默认每页条数，默认 20
 * @param {Object} options.filters - 筛选条件 reactive 对象
 * @param {Object} options.defaultFilters - 筛选条件默认值（用于重置）
 * @param {Function} options.beforeSearch - 搜索前校验/预处理，返回 false 则中止搜索
 */
export function useTablePagination(fetchData, options = {}) {
  const { defaultPageSize = 20, filters = null, defaultFilters = null, beforeSearch = null } = options

  const pagination = reactive({
    page: 1,
    pageSize: defaultPageSize,
    total: 0,
  })

  const handleSizeChange = (size) => {
    pagination.pageSize = size
    pagination.page = 1
    fetchData()
  }

  const handleCurrentChange = (page) => {
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
