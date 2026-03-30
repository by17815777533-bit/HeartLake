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

## 当前约束

- 管理端只消费后端当前集合壳。
- Dashboard 和图表刷新失败时必须保留 stale 状态。
- 实时链只处理白名单事件类型。
- 页面失败不能静默落成空表或默认配置。

## 详细代码手册

- [../docs/13_管理端代码地图与运营链手册.md](../docs/13_管理端代码地图与运营链手册.md)
- [../docs/02_API与实时链路手册.md](../docs/02_API与实时链路手册.md)
