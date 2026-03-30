# 后端说明

当前后端是 Drogon + C++20 单体服务，统一承载 API、实时链、推荐链、AI 链和管理链。

## 当前入口

- API：`http://121.41.195.165/api`
- 管理 API：`http://121.41.195.165/api/admin`
- WebSocket：`ws://121.41.195.165/ws/broadcast`

## 当前分层

```text
+----------------------+      +----------------------+      +----------------------+
| interfaces/          |----->| application/         |----->| domain/              |
| Controllers / WS     |      | 业务编排 / 事务边界  |      | 规则 / 仓储接口      |
+----------------------+      +----------+-----------+      +----------------------+
                                            |
                                            v
                               +------------------------------+
                               | infrastructure/ + utils/     |
                               | DB / Redis / AI / Vector     |
                               | Realtime / Storage / Auth    |
                               +------------------------------+
```

## 当前构建

```bash
cd backend
cmake -S . -B build-2c2g -DCMAKE_BUILD_TYPE=Release
cmake --build build-2c2g -j2
```

## 当前运行条件

- 必须提供 `PASETO_KEY`
- 必须提供 `ADMIN_PASETO_KEY`
- 必须提供 `DB_PASSWORD`
- 必须提供 `REDIS_PASSWORD`
- 推荐提供 `PUBLIC_API_URL`
- 推荐提供 `PUBLIC_WS_URL`
- 推荐显式设置 `CORS_ALLOWED_ORIGIN`

## 启动入口与顺序

### 入口文件

- `src/main.cpp`
- `include/infrastructure/ArchitectureBootstrap.h`
- `src/application/ApplicationServiceFactory.cpp`

### 当前启动顺序

1. 加载 `.env` 和配置文件路径。
2. 校验关键环境变量。
3. 计算线程数并应用低配机默认参数。
4. 配置监听地址、日志、静态资源和上传目录。
5. 注册异常处理、CORS、安全响应头、认证和链路追踪。
6. 执行 `ArchitectureBootstrap::initialize()`。
7. 初始化 RBAC、过滤器、AI、Embedding、EdgeAI、共鸣、推荐、RAG、Redis、TTL 和搜索服务。
8. 根据环境变量启动后台守护任务和 WebSocket 心跳。
9. `app.run()`。

## 当前目录职责

### `interfaces/`

- `api/`：HTTP 控制器和 WebSocket 控制器。
- 控制器只负责鉴权、参数校验、调用应用层或基础设施层、返回统一响应。

当前控制器包括：

- `AccountController`
- `AdminController`
- `AdminManagementController`
- `BroadcastWebSocketController`
- `ConsultationController`
- `EdgeAIController`
- `FriendController`
- `GuardianController`
- `HealthController`
- `InteractionController`
- `MediaController`
- `PaperBoatController`
- `PrivacyController`
- `RecommendationController`
- `ReportController`
- `SafeHarborController`
- `StoneController`
- `TempFriendController`
- `UserController`
- `VIPController`
- `VectorSearchController`

### `application/`

- `StoneApplicationService`
- `UserApplicationService`
- `InteractionApplicationService`
- `ApplicationServiceFactory`

职责是把多仓储写链、事件发送和跨模块编排收敛在一层。

### `domain/`

- `stone/`：石头仓储接口、仓储实现、领域服务。
- `user/`：用户仓储接口、仓储实现。
- `friend/`：好友仓储接口、仓储实现、领域服务。

### `infrastructure/`

- `ai/`：`AIService`、`RecommendationEngine`、`DualMemoryRAG`、`EmotionResonanceEngine`、`EdgeAIEngine`、`OnnxSentimentEngine`、`HNSWIndex`
- `cache/`：`CacheManager`、`RedisCache`
- `filters/`：`AdminAuthFilter`、`RateLimitFilter`、`SecurityAuditFilter`、`TraceMiddleware`
- `services/`：`LakeGodGuardianService`、`EmotionTrackingService`、`UserFollowUpService`、`ResonanceSearchService`、`SafeHarborService`、`VIPService`
- `vector/`：`MilvusClient`

### `utils/`

- `PasetoUtil.*`
- `RBACManager.*`
- `ResponseUtil.*`
- `RealtimeEvent.*`
- `AdminConfigStore.*`
- `EnvUtils.*`

## 当前装配与事件

`ArchitectureBootstrap::initialize()` 当前注册：

- `CacheManager`
- `EventBus`
- `MilvusClient`
- `SummaryService`
- `IStoneRepository -> StoneRepository`
- `IUserRepository -> UserRepository`
- `IFriendRepository -> FriendRepository`
- `StoneService`
- `FriendService`

当前事件订阅：

- `StonePublishedEvent -> StonePublishedHandler`
- `BoatSentEvent -> BoatSentHandler`

## 当前 AI 与后台任务

当前进程内显式初始化或启动：

- `AIService`
- `AdvancedEmbeddingEngine`
- `EdgeAIEngine`
- `EmotionResonanceEngine`
- `RecommendationEngine`
- `DualMemoryRAG`
- `ResonanceSearchService`
- `FriendshipTTLEngine`
- `LakeGodGuardianService`
- `EmotionTrackingService`
- `UserFollowUpService`

当前关键开关：

- `AI_PROVIDER`
- `AI_BASE_URL`
- `AI_MODEL`
- `EDGE_AI_ONNX_ENABLED`
- `ENABLE_LAKE_GOD_GUARDIAN`
- `ENABLE_EMOTION_TRACKING`
- `ENABLE_USER_FOLLOWUP`
- `ENABLE_WS_HEARTBEAT`

## 当前实现约束

- 控制器只做鉴权、参数校验和响应封装。
- 业务编排收敛在 application/service。
- 写链必须显式确认后再广播事件。
- 集合接口必须显式返回总数。
- AI、推荐、共鸣失败必须显式暴露，不返回假空结果。

## 当前性能说明

- PASETO 密钥读取已缓存。
- token 验证路径按当前实现只解析一次 JSON payload。
- WebSocket 鉴权成功和错误消息使用最小 JSON 载荷。
- 现网基准见 [../docs/06_测试验证与压测手册.md](../docs/06_测试验证与压测手册.md)。

## 当前环境变量分组

### 服务监听

- `SERVER_HOST`
- `SERVER_PORT`
- `SERVER_THREADS`
- `LOG_LEVEL`
- `HEARTLAKE_CONFIG_PATH`
- `HEARTLAKE_DOCUMENT_ROOT`
- `HEARTLAKE_UPLOAD_PATH`

### 数据库与缓存

- `DB_HOST`
- `DB_PORT`
- `DB_NAME`
- `DB_USER`
- `DB_PASSWORD`
- `DB_POOL_SIZE`
- `REDIS_HOST`
- `REDIS_PORT`
- `REDIS_PASSWORD`
- `REDIS_DB`

### 认证与公开入口

- `PASETO_KEY`
- `ADMIN_USERNAME`
- `ADMIN_PASSWORD`
- `ADMIN_PASETO_KEY`
- `ADMIN_PASSWORD_HASH`
- `CORS_ALLOWED_ORIGIN`
- `PUBLIC_API_URL`
- `PUBLIC_WS_URL`
- `PUBLIC_QUIC_URL`

### AI 与后台任务

- `AI_PROVIDER`
- `AI_API_KEY`
- `AI_BASE_URL`
- `AI_MODEL`
- `EDGE_AI_MODEL_PATH`
- `EDGE_AI_VOCAB_PATH`
- `ENABLE_LAKE_GOD_GUARDIAN`
- `ENABLE_EMOTION_TRACKING`
- `ENABLE_USER_FOLLOWUP`
- `ENABLE_WS_HEARTBEAT`

## 相关手册

- [../docs/04_技术实现全景手册.md](../docs/04_技术实现全景手册.md)
- [../docs/05_API接口全量清单.md](../docs/05_API接口全量清单.md)
- [../docs/06_测试验证与压测手册.md](../docs/06_测试验证与压测手册.md)
- [../backend/migrations/README.md](../backend/migrations/README.md)
- [../backend/models/README.md](../backend/models/README.md)
