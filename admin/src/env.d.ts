/**
 * Vite 环境类型声明
 *
 * 为 TypeScript 编译器提供 Vite 特有的类型支持：
 * - .vue 单文件组件的模块声明，使 import 语句获得正确的类型推导
 * - import.meta.env 环境变量的类型约束，限定项目实际使用的 VITE_ 前缀变量
 */
/// <reference types="vite/client" />

/** .vue 单文件组件模块声明，使 TypeScript 能正确解析 import */
declare module '*.vue' {
  import type { DefineComponent } from 'vue'
  const component: DefineComponent<Record<string, unknown>, Record<string, unknown>, unknown>
  export default component
}

/** Vite 注入的环境变量类型约束 */
interface ImportMetaEnv {
  /** 后端 API 基础路径，未配置时 fallback 到 /api */
  readonly VITE_API_BASE_URL?: string
}

/** 扩展 ImportMeta 接口，注入类型安全的环境变量访问 */
interface ImportMeta {
  readonly env: ImportMetaEnv
}
