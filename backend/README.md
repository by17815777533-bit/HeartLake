# 后端说明

后端是 Drogon + C++20 单体服务，统一承载 API、实时链、推荐链、AI 链和管理链。

## 入口

- API：`http://121.41.195.165/api`
- 管理 API：`http://121.41.195.165/api/admin`
- WebSocket：`ws://121.41.195.165/ws/broadcast`

## 分层结构

```text
+------------------------+
| interfaces/api         |
| HTTP / WS Controllers  |
+-----------+------------+
            |
            v
+------------------------+
| application            |
| 用例编排 / 事务边界    |
+-----------+------------+
            |
            v
+------------------------+
| domain                 |
| 规则 / 仓储接口        |
+-----------+------------+
            |
            v
+------------------------+
| infrastructure + utils |
| DB / Redis / AI / Auth |
| Realtime / Storage     |
+------------------------+
```

## 文件结构

```text
backend/
|-- src/main.cpp
|-- include/interfaces/api/
|-- src/interfaces/api/
|-- src/application/
|-- include/domain/
|-- src/domain/
|-- include/infrastructure/ai/
|-- src/infrastructure/ai/
|-- src/infrastructure/services/
|-- src/infrastructure/cache/
|-- src/infrastructure/vector/
|-- src/utils/
|-- migrations/
`-- models/
```

## 构建

```bash
cd backend
cmake -S . -B build-2c2g -DCMAKE_BUILD_TYPE=Release
cmake --build build-2c2g -j2
```

## 运行条件

- `PASETO_KEY`
- `ADMIN_PASETO_KEY`
- `DB_PASSWORD`
- `REDIS_PASSWORD`
- `PUBLIC_API_URL`
- `PUBLIC_WS_URL`
- `CORS_ALLOWED_ORIGIN`

## 启动链

### 关键文件

- `src/main.cpp`
- `include/infrastructure/ArchitectureBootstrap.h`
- `src/application/ApplicationServiceFactory.cpp`

### 启动顺序

1. 加载 `.env` 和配置文件路径。
2. 校验关键环境变量。
3. 计算线程数并应用低配机参数。
4. 配置监听地址、日志、静态目录和上传目录。
5. 注册异常处理、CORS、安全响应头、认证和链路追踪。
6. 执行 `ArchitectureBootstrap::initialize()`。
7. 初始化 RBAC、过滤器、AI、Embedding、EdgeAI、共鸣、推荐、RAG、Redis、TTL 和搜索服务。
8. 根据环境变量启动后台守护任务和 WebSocket 心跳。
9. `app.run()`。

## 目录职责

### `interfaces/api/`

- 鉴权
- 参数校验
- 响应封装
- WebSocket 接入

控制器包括：

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

### `domain/`

- `stone/`
- `user/`
- `friend/`

### `infrastructure/ai/`

- `AIService`
- `AdvancedEmbeddingEngine`
- `RecommendationEngine`
- `EmotionResonanceEngine`
- `DualMemoryRAG`
- `EdgeAIEngine`
- `SentimentAnalyzer`
- `ContentModerator`
- `EmotionPulseDetector`
- `FederatedLearner`
- `EdgeDifferentialPrivacy`
- `HNSWIndex`
- `ModelQuantizer`
- `EdgeNodeMonitor`
- `OnnxSentimentEngine`
- `SemanticCache`
- `SummaryService`

### `infrastructure/services/`

- `LakeGodGuardianService`
- `EmotionTrackingService`
- `UserFollowUpService`
- `ResonanceSearchService`
- `SafeHarborService`
- `VIPService`
- `GuardianIncentiveService`

### `utils/`

- `PasetoUtil.*`
- `RBACManager.*`
- `ResponseUtil.*`
- `RealtimeEvent.*`
- `AdminConfigStore.*`
- `EnvUtils.*`

## 装配与事件

`ArchitectureBootstrap::initialize()` 注册：

- `CacheManager`
- `EventBus`
- `MilvusClient`
- `SummaryService`
- `IStoneRepository -> StoneRepository`
- `IUserRepository -> UserRepository`
- `IFriendRepository -> FriendRepository`
- `StoneService`
- `FriendService`

事件订阅：

- `StonePublishedEvent -> StonePublishedHandler`
- `BoatSentEvent -> BoatSentHandler`

## AI 与后台任务

### 八大 Edge AI 子系统

1. `SentimentAnalyzer`
2. `ContentModerator`
3. `EmotionPulseDetector`
4. `FederatedLearner`
5. `EdgeDifferentialPrivacy`
6. `HNSWIndex`
7. `ModelQuantizer`
8. `EdgeNodeMonitor`

### 上层 AI 能力

- `AIService`：情感分析、内容审核、对话生成、嵌入向量。
- `RecommendationEngine`：协同过滤、内容推荐、探索策略。
- `EmotionResonanceEngine`：语义相似、DTW 轨迹、时间衰减、多样性奖励。
- `DualMemoryRAG`：湖神双记忆对话与情绪洞察。
- `ResonanceSearchService`：向量召回和共鸣结果拼装。
- `LakeGodGuardianService`：零互动内容巡检和自动关怀。

### EdgeAIController 能力

- `/api/edge-ai/status`
- `/api/edge-ai/metrics`
- `/api/edge-ai/analyze`
- `/api/edge-ai/moderate`
- `/api/edge-ai/emotion-pulse`
- `/api/edge-ai/federated/aggregate`
- `/api/edge-ai/privacy-budget`
- `/api/edge-ai/vector-search`
- `/api/edge-ai/vector-insert`
- `/api/edge-ai/emotion-sample`

### 其余 AI 入口

- `RecommendationController`
- `VectorSearchController`
- `GuardianController`

## 实现约束

- 控制器只做鉴权、参数校验和响应封装。
- 业务编排收敛在 application/service。
- 写链成功后才广播事件。
- 集合接口显式返回总数。
- AI、推荐、共鸣失败显式暴露。

## 性能说明

- PASETO 密钥读取已缓存。
- token 验证路径只解析一次 JSON payload。
- WebSocket 鉴权成功和错误消息使用最小 JSON 载荷。
- 现网基准见 [../docs/06_测试验证与压测手册.md](../docs/06_测试验证与压测手册.md)。

## 环境变量分组

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
