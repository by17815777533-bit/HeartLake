# 管理端说明

当前管理端是 Vue 3 + Vite 运营控制台，直接服务于现网运营、审核、配置和实时治理。

## 当前入口

- 页面：`http://121.41.195.165/admin/`
- API：`http://121.41.195.165/api`
- WebSocket：`ws://121.41.195.165/ws/broadcast`

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

## 当前环境变量

- `VITE_API_BASE_URL`
- `VITE_WS_URL`

## 当前入口链

### 关键文件

- `src/router/index.ts`
- `src/layouts/MainLayout.vue`
- `src/stores/index.ts`
- `src/api/index.ts`
- `src/services/websocket.ts`

### 当前路由与角色

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

当前角色层级：

- `viewer = 1`
- `admin = 2`
- `super_admin = 3`

## 当前状态与通信

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

## 当前页面

- Dashboard
- Users
- Content
- Moderation
- Reports
- SensitiveWords
- Logs
- Settings
- EdgeAI

## 当前组件职责

### 布局

- `MainLayout.vue`：顶部导航、搜索、管理员信息、实时摘要、warning 保留

### 组合式函数

- `useChartOptions.ts`
- `useDashboardData.ts`
- `useDashboardLoaders.ts`
- `useTablePagination.ts`

### 工具与类型

- `utils/adminPayload.ts`
- `utils/collectionPayload.ts`
- `utils/errorHelper.ts`
- `utils/chartSignals.ts`
- `utils/opsDashboardDeck.ts`
- `utils/adminRoutes.ts`
- `types/index.ts`

## 当前约束

- 管理端只消费后端当前集合壳。
- Dashboard 和图表刷新失败时必须保留 stale 状态。
- 实时链只处理白名单事件类型。
- 页面失败不能静默落成空表或默认配置。

## 相关手册

- [../docs/04_技术实现全景手册.md](../docs/04_技术实现全景手册.md)
- [../docs/02_API与实时链路手册.md](../docs/02_API与实时链路手册.md)
- [../docs/05_API接口全量清单.md](../docs/05_API接口全量清单.md)
