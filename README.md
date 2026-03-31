# HeartLake
>心湖——基于侧端ai隐私计算的匿名心理关怀系统
HeartLake 是一套匿名情绪社区系统，包含 Flutter 移动端、Vue 3 管理端和 Drogon + C++20 后端，承载内容互动、关系链、情绪链、推荐链、AI 链、后台治理和实时广播。

## 线上信息

- 公网入口：`http://121.41.195.165`
- API：`http://121.41.195.165/api`
- 管理后台：`http://121.41.195.165/admin/`
- WebSocket：`ws://121.41.195.165/ws/broadcast`
- 云端仓库目录：`/root/HeartLake`
- 部署别名：`heartlake-server`
- Android release 包：`frontend/build/app/outputs/flutter-apk/app-release.apk`
- APK SHA-256：`9654d5facf294ab1c0d21e6ce6f73728d346977994fe911a23be1de02553ac31`

## 运行架构

```text
+------------------+
| Flutter App      |
+------------------+
          |
          v
+------------------+      +------------------+
| Nginx Gateway    |----->| Vue Admin        |
| /api /ws /admin  |      +------------------+
+---------+--------+
          |
          v
+------------------+
| Drogon Backend   |
| Controllers      |
| Application      |
| Domain           |
| Infrastructure   |
+----+----+----+---+
     |    |    |
     |    |    +--------------------+
     |    |                         |
     v    v                         v
+--------+------+        +-----------------------+
| PostgreSQL    |        | AI / Recommendation   |
| content graph |        | Resonance / EdgeAI    |
| sessions      |        | DualMemoryRAG         |
+---------------+        +-----------------------+
          |
          v
+------------------+
| Redis            |
| cache / realtime |
+------------------+
```

## 仓库结构

```text
.
|-- frontend/
|   |-- lib/main.dart
|   |-- lib/router/
|   |-- lib/data/datasources/
|   |-- lib/presentation/providers/
|   |-- lib/presentation/screens/
|   |-- lib/presentation/widgets/
|   `-- lib/edge_ai/
|-- admin/
|   `-- src/
|       |-- api/
|       |-- router/
|       |-- services/
|       |-- stores/
|       |-- layouts/
|       |-- composables/
|       |-- views/
|       `-- utils/
|-- backend/
|   |-- src/main.cpp
|   |-- include/interfaces/api/
|   |-- src/interfaces/api/
|   |-- src/application/
|   |-- src/domain/
|   |-- src/infrastructure/ai/
|   |-- src/infrastructure/services/
|   |-- src/utils/
|   |-- migrations/
|   `-- models/
|-- docs/
`-- scripts/
```

## 模块能力

| 模块 | 说明 |
|---|---|
| 石头 / 涟漪 / 纸船 | 统一走集合壳、写后确认和实时事件链 |
| 好友 / 临时好友 / 守护 | 自动关系模型、消息链、灯火与守护入口 |
| 情绪日历 / 热力图 / 趋势 / 脉搏 | 消费真实情绪数据，失败显式暴露 |
| 推荐 / 高级推荐 / 共鸣搜索 | 使用高级算法和向量检索结果 |
| 湖神 / 安全港 / 咨询 / VIP | 保留显式降级语义，不伪装成功 |
| 管理后台 | Dashboard、用户、内容、审核、举报、日志、配置、广播 |

## 代码入口

### 后端

- 启动入口：`backend/src/main.cpp`
- 装配入口：`backend/include/infrastructure/ArchitectureBootstrap.h`
- 应用服务工厂：`backend/src/application/ApplicationServiceFactory.cpp`
- HTTP / WS 控制器：`backend/include/interfaces/api/` 和 `backend/src/interfaces/api/`
- AI 实现：`backend/src/infrastructure/ai/`
- 后台任务：`backend/src/infrastructure/services/`

### 移动端

- 应用入口：`frontend/lib/main.dart`
- 路由：`frontend/lib/router/app_router.dart`
- 依赖注入：`frontend/lib/di/service_locator.dart`
- 数据源：`frontend/lib/data/datasources/`
- 状态管理：`frontend/lib/presentation/providers/`
- 页面：`frontend/lib/presentation/screens/`

### 管理端

- 路由：`admin/src/router/index.ts`
- 布局：`admin/src/layouts/MainLayout.vue`
- HTTP：`admin/src/api/index.ts`
- WebSocket：`admin/src/services/websocket.ts`
- 页面：`admin/src/views/`

## 认证、加密与隐私

```text
+-------------------+      +----------------------+      +----------------------+
| 匿名登录 / 恢复   |----->| PASETO + Session     |----->| API / WebSocket      |
| recovery_key      |      | token + refresh      |      | Bearer / query token |
+---------+---------+      +----------+-----------+      +----------+-----------+
          |                           |                             |
          |                           |                             |
          v                           v                             v
+-------------------+      +----------------------+      +----------------------+
| FlutterSecure     |      | 咨询 E2E 会话        |      | RBAC / Rate Limit    |
| Storage           |      | X25519 + HKDF + GCM  |      | CORS / 审计日志      |
+---------+---------+      +----------+-----------+      +----------+-----------+
          |                           |                             |
          +---------------------------+-----------------------------+
                                      |
                                      v
                           +----------------------+
                           | 隐私设置 / 数据导出  |
                           | DP 预算 / 安全事件   |
                           +----------------------+
```

- 用户认证链由 `/api/auth/anonymous`、`/api/auth/recover`、`/api/auth/refresh` 组成。服务端返回短期访问令牌、`refresh_token`、`session_id`，首次匿名登录还会返回 `recovery_key`。
- 用户端和管理端分开使用两套 PASETO 密钥。用户 token 负载含 `sub / iss / iat / exp`，管理员 token 额外携带 `role`，后端 `PasetoUtil` 会缓存密钥并校验过期时间。
- Flutter 端把 `token`、`refresh_token`、`user_id`、`session_id` 放进 `FlutterSecureStorage`，昵称、设备号和偏好走 `SharedPreferences`。401 响应统一走 refresh 链，不再靠页面各自重登。
- 咨询链是显式端到端加密。移动端 `E2EEncryption` 和后端 `E2EEncryption` 都使用 `X25519 + HKDF-SHA256 + AES-256-GCM`；`ConsultationController` 只存 `ciphertext / iv / tag`，服务端不落明文。
- 移动端 `ApiClient` 支持可选证书 pinning，读取 `ENABLE_CERT_PINNING` 和 `CERT_SHA256_PINS`；生产启用后按证书 SHA-256 指纹校验。
- 用户 API 经过 `SecurityAuditFilter`，管理 API 经过 `AdminAuthFilter`。管理侧会继续做 `RBACManager` 的路径级权限检查；CORS 只允许 `CORS_ALLOWED_ORIGIN` 显式列出的来源。
- 限流由 `RateLimitFilter` 负责，普通 API 默认 `100` 次/分钟，AI API 默认 `20` 次/分钟；WebSocket 握手需要 query `token`，已连上后走 `auth_success / ping / pong` 保活链。
- 隐私链落在 `AccountController`、`PrivacyController`、`privacy_settings_screen.dart` 和 `EdgeAI` 隐私预算页面，覆盖资料可见性、在线状态、陌生人消息、数据导出、停用、永久删除、差分隐私统计和预算报告。

## 八大 Edge AI 子系统

`EdgeAIEngine` 是八大核心子系统的统一门面：

1. `SentimentAnalyzer`：轻量级情感分析，融合规则、词典、统计和 ONNX 路径。
2. `ContentModerator`：本地文本审核，负责敏感词和心理风险初筛。
3. `EmotionPulseDetector`：实时情绪脉搏检测，维护社区情绪快照。
4. `FederatedLearner`：联邦学习聚合器，负责模型汇总和 FedAvg。
5. `EdgeDifferentialPrivacy`：差分隐私预算和噪声注入。
6. `HNSWIndex`：本地向量近邻检索。
7. `ModelQuantizer`：INT8 量化推理。
8. `EdgeNodeMonitor`：边缘节点健康监控。

## AI 功能

- `AIService`：统一调度情感分析、内容审核、对话生成和嵌入生成。
- `AdvancedEmbeddingEngine`：生成文本嵌入。
- `RecommendationEngine`：个性化推荐和探索策略。
- `EmotionResonanceEngine`：四维共鸣评分和 DTW 情绪轨迹匹配。
- `DualMemoryRAG`：湖神双记忆系统，支持情绪洞察和陪伴对话。
- `ResonanceSearchService`：向量召回和共鸣结果组装。
- `LakeGodGuardianService`：零互动石头巡检和自动关怀。

## AI 功能落点

- 发布链：`publish_screen` 和 `ai_content_preview` 做情绪分析与提示。
- 推荐链：`discover_screen` 和 `personalized_screen` 展示个性化推荐与高级共鸣推荐。
- 情绪链：日历、热力图、趋势、脉搏页面消费 Edge AI 和推荐引擎结果。
- 湖神链：`lake_god_chat_screen` 做陪伴对话，`guardian_screen` 做情绪洞察。
- 管理链：`EdgeAI.vue` 展示引擎、脉搏、联邦、预算和配置；`Users.vue` 查看高级推荐产出。

## 性能快照

- 5 分钟混合 HTTP soak：`406572` 请求，整体 `1330.91 rps`
- `GET /api/lake/stones`：平均 `8.52ms`，`p95 16.61ms`
- `GET /api/account/info`：平均 `8.27ms`，`p95 16.32ms`
- `GET /api/recommendations/trending`：平均 `7.81ms`，`p95 15.95ms`
- `GET /api/stones/{id}/resonance`：平均 `11.33ms`，`p95 19.45ms`
- `POST /api/stones/{id}/ripples`：平均 `9.77ms`，`p95 17.77ms`
- WebSocket `120` 并发连接：`120/120` 建连成功，握手平均 `42.67ms`，`p95 51.62ms`

## 常用命令

```bash
ssh heartlake-server
curl -fsS http://121.41.195.165/api/health
curl -I -fsS http://121.41.195.165/admin/
./scripts/verify-2c2g.sh
./scripts/docker-test.sh
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
