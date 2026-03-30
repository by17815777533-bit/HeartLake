# HeartLake

HeartLake 是一套运行中的匿名情绪社区系统。当前项目由 Flutter 移动端、Vue 3 管理端、Drogon + C++20 后端组成，统一承载内容互动、关系链、情绪链、推荐链、AI 链、后台治理和实时广播。

## 当前线上事实

- 公网入口：`http://121.41.195.165`
- API：`http://121.41.195.165/api`
- 管理后台：`http://121.41.195.165/admin/`
- WebSocket：`ws://121.41.195.165/ws/broadcast`
- 云端仓库目录：`/root/HeartLake`
- 部署别名：`heartlake-server`
- 移动端 release 包：`frontend/build/app/outputs/flutter-apk/app-release.apk`
- 当前 APK SHA-256：`9654d5facf294ab1c0d21e6ce6f73728d346977994fe911a23be1de02553ac31`

## 当前架构

```text
+---------------------------+      +-----------------------------+
| Flutter 移动端            |----->|                             |
| Vue 3 管理端              |      | Nginx Gateway               |
| curl / SSH / 压测脚本     |----->| /api /admin /ws /healthz    |
+---------------------------+      +-------------+---------------+
                                                  |
                                                  v
                                 +-------------------------------------------+
                                 | Drogon 后端                               |
                                 | Controllers -> Application -> Domain      |
                                 | -> Infrastructure / Realtime / AI         |
                                 +----------+---------------+----------------+
                                            |               |
                         +------------------+               +------------------+
                         |                                                     |
                         v                                                     v
              +-----------------------+                           +-----------------------+
              | PostgreSQL 16         |                           | Redis 7               |
              | 业务数据 / 统计 / 会话 |                           | 缓存 / 实时辅助       |
              +-----------------------+                           +-----------------------+
                                            |
                                            v
                                 +-------------------------------------------+
                                 | 高级算法链                                |
                                 | Recommendation / Resonance / EdgeAI       |
                                 | ONNX / HNSW / LakeGod / Guardian          |
                                 +-------------------------------------------+
```

## 当前能力

| 模块 | 当前状态 |
|---|---|
| 石头 / 涟漪 / 纸船 | 统一走集合壳、写后确认和实时事件链 |
| 好友 / 临时好友 / 守护 | 只保留当前自动关系模型和可用消息链 |
| 情绪日历 / 热力图 / 趋势 / 脉搏 | 只消费真实情绪数据，失败显式暴露 |
| 推荐 / 高级推荐 / 共鸣搜索 | 直接使用当前高级算法和向量检索结果 |
| 湖神 / 安全港 / 咨询 / VIP | 保留显式降级语义，不伪装成功 |
| 管理后台 | Dashboard、用户、内容、审核、举报、日志、配置、广播可用 |

## 仓库结构

- `frontend/`：Flutter 移动端，包含启动入口、路由、数据源、Provider、页面和组件
- `admin/`：Vue 3 管理端，包含路由、布局、Pinia 状态、HTTP 基座、WebSocket 和运营页面
- `backend/`：Drogon + C++20 后端，包含控制器、应用服务、领域层、基础设施、迁移和模型资源
- `docs/`：现行手册、接口说明、压测结果、部署和上线清单
- `scripts/`：部署、校验、环境生成、联调、压测辅助脚本
- `datasets/`：离线参考数据说明

## 当前关键实现

### 后端

- 入口：`backend/src/main.cpp`
- 装配：`backend/include/infrastructure/ArchitectureBootstrap.h`
- 应用服务工厂：`backend/src/application/ApplicationServiceFactory.cpp`
- 控制器统一承载账号、内容、关系、AI、管理和 WebSocket 入口
- 基础设施层统一承载 PostgreSQL、Redis、AI、向量检索、后台任务和实时事件

### 移动端

- 入口：`frontend/lib/main.dart`
- 路由：`frontend/lib/router/app_router.dart`
- 依赖注入：`frontend/lib/di/service_locator.dart`
- Provider：`user`、`stone`、`friend`、`notification`、`edge_ai`
- 数据源统一放在 `frontend/lib/data/datasources/`

### 管理端

- 路由：`admin/src/router/index.ts`
- 主布局：`admin/src/layouts/MainLayout.vue`
- HTTP：`admin/src/api/index.ts`
- WebSocket：`admin/src/services/websocket.ts`
- 页面：Dashboard、Users、Content、Moderation、Reports、SensitiveWords、Logs、Settings、EdgeAI

## 当前性能快照

最近一次云端压测直接在服务器本机执行，结果如下：

- 5 分钟混合 HTTP soak：`406572` 请求，整体 `1330.91 rps`
- `GET /api/lake/stones`：平均 `8.52ms`，`p95 16.61ms`
- `GET /api/account/info`：平均 `8.27ms`，`p95 16.32ms`
- `GET /api/recommendations/trending`：平均 `7.81ms`，`p95 15.95ms`
- `GET /api/stones/{id}/resonance`：平均 `11.33ms`，`p95 19.45ms`
- `POST /api/stones/{id}/ripples`：平均 `9.77ms`，`p95 17.77ms`
- WebSocket `120` 并发连接：`120/120` 建连成功，握手平均 `42.67ms`，`p95 51.62ms`
- 压测窗口内 PostgreSQL 没有出现超过 `20ms` 的慢 SQL

## 常用命令

```bash
# 云端登录与健康检查
ssh heartlake-server
curl -fsS http://121.41.195.165/api/health
curl -I -fsS http://121.41.195.165/admin/

# 本地全量校验
./scripts/verify-2c2g.sh
./scripts/docker-test.sh

# 云端部署
./scripts/docker-up.sh server-lite
./scripts/docker-up.sh server-lite-backend
./scripts/docker-up.sh server-lite-admin
```

## 文档入口

- [文档索引](docs/README.md)
- [技术实现全景手册](docs/04_技术实现全景手册.md)
- [API 与实时链路手册](docs/02_API与实时链路手册.md)
- [API 接口全量清单](docs/05_API接口全量清单.md)
- [测试验证与压测手册](docs/06_测试验证与压测手册.md)
- [上线检查清单](docs/10_上线检查清单.md)
- [Ubuntu 2C2G 部署手册](docs/deploy-ubuntu-2c2g.md)
- [后端说明](backend/README.md)
- [移动端说明](frontend/README.md)
- [管理端说明](admin/README.md)
- [数据迁移说明](backend/migrations/README.md)
- [模型资源说明](backend/models/README.md)
