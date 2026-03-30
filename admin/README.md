# 管理端说明

管理端是 Vue 3 + Vite 运营控制台，服务于运营、审核、配置和实时治理。

## 入口

- 页面：`http://121.41.195.165/admin/`
- API：`http://121.41.195.165/api`
- WebSocket：`ws://121.41.195.165/ws/broadcast`

## 文件结构

```text
admin/src/
|-- api/
|-- components/
|-- composables/
|-- layouts/
|-- router/
|-- services/
|-- stores/
|-- styles/
|-- types/
|-- utils/
|-- views/
|   |-- dashboard/
|   `-- edge-ai/
`-- __tests__/
```

## 本地开发

```bash
cd admin
npm install
npm run dev
```

## 构建与检查

```bash
cd admin
npm run lint
npm run build
```

## 环境变量

- `VITE_API_BASE_URL`
- `VITE_WS_URL`

## 入口链

### 关键文件

- `src/router/index.ts`
- `src/layouts/MainLayout.vue`
- `src/stores/index.ts`
- `src/api/index.ts`
- `src/services/websocket.ts`

### 路由与角色

| 路径 | 页面 | 最低角色 |
|---|---|---|
| `/login` | `Login.vue` | 公开 |
| `/dashboard` | `Dashboard.vue` | viewer |
| `/users` | `Users.vue` | admin |
| `/content` | `Content.vue` | admin |
| `/moderation` | `Moderation.vue` | admin |
| `/reports` | `Reports.vue` | admin |
| `/sensitive-words` | `SensitiveWords.vue` | admin |
| `/logs` | `Logs.vue` | admin |
| `/settings` | `Settings.vue` | super_admin |
| `/edge-ai` | `EdgeAI.vue` | admin |
| `/forbidden` | `Forbidden.vue` | 公开 |
| `/:pathMatch(.*)*` | `NotFound.vue` | 公开 |

角色层级：

- `viewer = 1`
- `admin = 2`
- `super_admin = 3`

## 状态与通信

### `stores/index.ts`

- `token`
- `userInfo`
- `isDark`
- `loadingCount`
- `isGlobalLoading`

### `api/index.ts`

- `VITE_API_BASE_URL || '/api'`
- Bearer token 注入
- 请求去重
- 全局 loading 聚合
- `code != 0/200` 视为失败
- 401 和业务认证错误统一登出
- GET 对 `408/429/500/502/503/504` 自动重试

### `services/websocket.ts`

- query `token` 鉴权
- `30s` 心跳
- 最大 `10` 次重连
- 未知消息转为 `socket_error`

## 认证与安全

### 登录与会话

- `Login.vue` 负责管理员登录入口，非 HTTPS 会明确告警。
- 连续登录失败 `3` 次后，客户端会进入 `30` 秒冷却；后端仍会继续做 429 限流。
- `stores/index.ts` 把管理员 token 和管理员信息放进 `sessionStorage`，页面关闭后会话随标签页生命周期结束。
- 管理端不在浏览器里解码 PASETO，只保存登录响应里的管理员信息；真正的过期和权限判断由后端完成。

### HTTP 与 WebSocket

- `api/index.ts` 统一注入管理员 Bearer token，401 和业务认证错误会直接清会话并回登录页。
- `services/websocket.ts` 握手时通过 query `token` 鉴权，消息类型走白名单，未知类型会被上抛成 `socket_error`。
- 实时连接有 `30s` 心跳和最多 `10` 次重连，不让运营页静默掉线。

### 权限与留痕

- 路由按 `viewer / admin / super_admin` 分级。
- 后端 `AdminAuthFilter` 会结合 `RBACManager` 做路径级权限检查。
- `Logs.vue`、`Reports.vue`、`Moderation.vue`、`Settings.vue` 对应后台审计、处置和高权限配置链。

## 页面与职责

- `Dashboard.vue`：平台统计和图表
- `Users.vue`：用户列表、详情、封禁和高级推荐查看
- `Content.vue`：内容水位和治理动作
- `Moderation.vue`：审核队列和审核结果
- `Reports.vue`：举报和求助处理
- `SensitiveWords.vue`：敏感词维护
- `Logs.vue`：管理员和系统日志
- `Settings.vue`：高权限配置管理
- `EdgeAI.vue`：边缘 AI 状态和节点信息

## 组合式函数与工具

- `useChartOptions.ts`
- `useDashboardData.ts`
- `useDashboardLoaders.ts`
- `useTablePagination.ts`
- `utils/adminPayload.ts`
- `utils/collectionPayload.ts`
- `utils/errorHelper.ts`
- `utils/chartSignals.ts`
- `utils/opsDashboardDeck.ts`
- `utils/adminRoutes.ts`
- `types/index.ts`

## AI 与运营能力

### `EdgeAI.vue`

- 引擎状态总览
- 性能指标
- 情绪脉搏曲线
- 联邦学习状态
- 隐私预算
- 配置摘要和聚合触发

### `Users.vue`

- 查看指定用户的高级推荐结果
- 核对参考石头、算法标识和链路产出

### `Dashboard`

- 读取情绪脉搏、隐私预算、共鸣统计等 AI 指标

### `types/index.ts`

- `EdgeAISubsystem`
- `EdgeAIStatus`
- `EdgeAIMetrics`

## 约束

- 只消费后端集合壳。
- Dashboard 和图表刷新失败时保留 stale 状态。
- 实时链只处理白名单事件类型。
- 页面失败不能静默落成空表或默认配置。
- token 不做客户端解码和长期持久化。

## 相关手册

- [../docs/04_技术实现全景手册.md](../docs/04_技术实现全景手册.md)
- [../docs/02_API与实时链路手册.md](../docs/02_API与实时链路手册.md)
- [../docs/05_API接口全量清单.md](../docs/05_API接口全量清单.md)
