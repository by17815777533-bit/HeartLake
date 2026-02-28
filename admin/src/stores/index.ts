/**
 * @brief 全局状态管理模块 (Pinia setup store)
 *
 * 提供应用程序层级的全局状态管理，涵盖认证凭证(PASETO Token)、用户上下文、
 * 全局UI偏好(暗色模式)以及并发请求计数状态的维护与持久化机制。
 * 
 * 架构设计及安全策略：
 * - 认证信息隔离：敏感凭据默认定位于 SessionStorage，随标签页生命周期销毁，降低持久化存储暴露风险。
 * - 多层验证防御：在客户端构建具有 24 小时存活周期的 Token 前置检验机制，
 *   减少向后端非必要请求。最终的一致性安全裁决由后端基于 PASETO `exp` 声明处理。
 * - 防御性存取：在跨标签页隐私模式 (Privacy Mode) 等 Storage 访问受限的边缘场景中，
 *   采用妥善的异常捕获处理以保障应用不崩溃及状态回退机制的可用性。
 */
import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { UserInfo } from '@/types'

/**
 * @brief 注册应用核心级 Store 实例
 *
 * 通过组合式 API 方式定义 Pinia 状态树。对外暴露相关的响应式属性及操作函数，
 * 以便在整个 Vue 组件图及 Router / Axios 拦截器等外部作用域中调用。
 */
export const useAppStore = defineStore('app', () => {
  /**
   * @brief 全局认证令牌
   * @details 初始化期间通过沙盒机制尝试从 SessionStorage 中按需读取。
   * 以防御性编程手段（try-catch）屏蔽存储访问禁用或越权情况下的运行时异常，
   * 并将默认上下文设置为未授权状态。
   */
  const token = ref<string>((() => {
    try {
      return sessionStorage.getItem('admin_token') || ''
    } catch {
      return ''
    }
  })())

  /**
   * @brief 登录用户基础信息实体
   * @details 因 PASETO Payload 在客户端不提供标准解码手段且具有防篡改特性，
   * 用户侧身份标识及权限信息采用服务端登录响应附加的数据进行传递，并做短效期暂存。
   */
  const userInfo = ref<UserInfo | null>((() => {
    try {
      const stored = sessionStorage.getItem('admin_user_info')
      return stored ? JSON.parse(stored) : null
    } catch {
      sessionStorage.removeItem('admin_user_info')
      return null
    }
  })())

  /**
   * @brief 全局视觉主题状态
   * @details 跨回放会话保持暗色模式选择结果（持久化在 LocalStorage 范围内），
   * 默认为亮色系偏好(即 false)。
   */
  const isDark = ref<boolean>(localStorage.getItem('admin_dark_mode') === 'true')

  /**
   * @brief 异步任务栈计数器变量
   * @details 核心设计意图：维护复杂页面或并发 API 调用的聚合 Loading 状态。
   */
  const loadingCount = ref<number>(0)
  
  /**
   * @brief 顶层页面级阻塞指示器
   * @details 响应式聚合值，衍生自 `loadingCount` 以驱动视图层加载遮罩的开关动作。
   */
  const isGlobalLoading = ref<boolean>(false)

  /**
   * @brief 更迭服务端认证令牌及相关时间戳
   * 
   * 写入新的系统授权令牌并向 Session 存储中注入更新时刻锚点，
   * 该锚点用于构建前端基于时间轴的 Token 定期降维防洪机制。
   *
   * @param t 从服务端派发的 PASETO 授权口令字符串
   */
  const setToken = (t: string): void => {
    token.value = t
    sessionStorage.setItem('admin_token', t)
    sessionStorage.setItem('admin_token_ts', Date.now().toString())
  }

  /**
   * @brief 更新当前操作者的上下文信息实体
   *
   * 向内存及 Storage 同步覆盖用户相关的元数据模型。传入 Null 意味着清空当前用户态上下文。
   *
   * @param info 新的用户信息字典或空引用
   */
  const setUserInfo = (info: UserInfo | null): void => {
    userInfo.value = info
    sessionStorage.setItem('admin_user_info', JSON.stringify(info))
  }

  /**
   * @brief 强制抹除授权会话上下文
   *
   * 中断当前认证声明，清除内存及持久化缓存介质中有关当前使用者的所有认证残余及个人记录，
   * 使客户端恢复到匿名游客状态模式。
   */
  const clearToken = (): void => {
    token.value = ''
    userInfo.value = null
    sessionStorage.removeItem('admin_token')
    sessionStorage.removeItem('admin_user_info')
    sessionStorage.removeItem('admin_token_ts')
  }

  /**
   * @brief 查询现有通信令牌标量值
   *
   * 提供给请求拦截器或全局路由器守卫实时抓取当下授权凭证片段，
   * 并在 HTTP Header 中构建 `Authorization` 前缀。
   *
   * @return 返回纯文本口令字符串
   */
  const getToken = (): string => token.value

  /**
   * @brief 前端启发式身份证书验活方法
   *
   * 分析前端时间戳印记以检测逻辑过期可能（定义生存生命期为 24H），
   * 为客户端预处理 401 Unauthorized 后续状态异常提供提前规避路径。
   * 核心防线依旧处于服务层，该判定主要从提高交互鲁棒性角度出发。
   *
   * @return Token 是否在非技术过期周期内可信 (true 表示存活有效，false 表示过期或不存在)
   */
  const checkTokenValid = (): boolean => {
    if (!token.value || typeof token.value !== 'string' || token.value.length === 0) {
      return false
    }
    const ts = sessionStorage.getItem('admin_token_ts')
    if (ts && Date.now() - Number(ts) > 24 * 60 * 60 * 1000) {
      clearToken()
      return false
    }
    return true
  }

  /**
   * @brief 根据给定标识对文档级节点的类清单进行对应模式注入
   *
   * @param dark 决定是否应当添加特定的 'dark' CSS Class 
   */
  const applyDarkMode = (dark: boolean): void => {
    document.documentElement.classList.toggle('dark', dark)
  }

  /**
   * @brief 倒置系统界面对比度方案并生成长期持久化配置
   *
   * 在状态机内部转换黑白天候选项，并下发动作给 DOM 操作函数同时将配置转储于浏览器缓存系内。
   */
  const toggleDark = (): void => {
    isDark.value = !isDark.value
    applyDarkMode(isDark.value)
    localStorage.setItem('admin_dark_mode', String(isDark.value))
  }

  /**
   * @brief 初始化阶段触发的 UI 适配器
   *
   * 用以在单页应用首次解析后，执行预期的样式初始化投递。
   */
  const initDarkMode = (): void => {
    applyDarkMode(isDark.value)
  }

  /**
   * @brief 累加加载事务深度，唤醒屏幕整体沉浸式遮罩
   */
  const startLoading = (): void => {
    loadingCount.value++
    isGlobalLoading.value = true
  }

  /**
   * @brief 减免加载事务并发链深度并收起遮罩
   *
   * 当引用计数跌至零底限后复原指示器状态，安全起见加入了下限拦截处理，避免由于生命周期
   * 分散导致的负并发统计。
   */
  const stopLoading = (): void => {
    loadingCount.value = Math.max(0, loadingCount.value - 1)
    if (loadingCount.value === 0) {
      isGlobalLoading.value = false
    }
  }

  return {
    token, setToken, clearToken, getToken, checkTokenValid,
    userInfo, setUserInfo,
    isDark, toggleDark, initDarkMode,
    isGlobalLoading, startLoading, stopLoading,
  }
})
