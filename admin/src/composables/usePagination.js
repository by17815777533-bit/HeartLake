import { reactive } from 'vue'

/**
 * 分页逻辑复用 composable
 * @param {Function} fetchFn - 数据获取函数，接收 { page, pageSize } 参数
 * @param {Object} options - 配置项
 * @param {number} options.defaultPageSize - 默认每页条数，默认 20
 */
export function usePagination(fetchFn, options = {}) {
  const { defaultPageSize = 20 } = options

  const pagination = reactive({
    page: 1,
    pageSize: defaultPageSize,
    total: 0,
  })

  const handleSizeChange = (size) => {
    pagination.pageSize = size
    pagination.page = 1
    fetchFn()
  }

  const handleCurrentChange = (page) => {
    pagination.page = page
    fetchFn()
  }

  const resetPage = () => {
    pagination.page = 1
  }

  return {
    pagination,
    handleSizeChange,
    handleCurrentChange,
    resetPage,
  }
}
