# HeartLake 面向汇报者项目全景介绍

截至 `2026-04-01`，本文基于当前仓库可见代码、配置、脚本与文档整理，目标不是写“开发说明”，而是给汇报者、答辩者、演示者、交接者提供一份可以直接拿来介绍项目的超详细底稿。

适用对象：

- 课程/毕设/项目答辩汇报者
- 向老师、领导、客户、投资人、合作方介绍项目的人
- 新接手项目、需要快速形成全局认知的人
- 需要把“业务价值 + 技术深度 + 工程完整度”一次讲清楚的人

建议配合阅读：

- [仓库总 README](../README.md)
- [文档索引](../docs/README.md)
- [技术实现全景手册](../docs/04_技术实现全景手册.md)
- [API 与实时链路手册](../docs/02_API与实时链路手册.md)
- [API 接口全量清单](../docs/05_API接口全量清单.md)
- [测试验证与压测手册](../docs/06_测试验证与压测手册.md)

---

## 1. 项目一句话定义

HeartLake 是一套围绕“匿名情绪表达、共鸣互动、关系建立、心理关怀与平台治理”构建的完整系统，包含：

- `Flutter` 移动端
- `Vue 3 + Vite` 管理端
- `Drogon + C++20` 后端
- `PostgreSQL + Redis` 数据与缓存
- `AI / Edge AI / 共鸣推荐 / 双记忆 RAG` 智能能力
- `WebSocket` 实时广播链
- 可选的 `Rust + QUIC` 实时网关子项目

如果要用更偏汇报的口吻概括，这个项目可以定义为：

> 一个强调匿名、安全、共鸣、陪伴和治理闭环的情绪社区平台。

---

## 2. 项目核心定位

### 2.1 项目要解决什么问题

很多人在面对情绪压力、孤独感、焦虑或短期心理波动时，并不愿意立刻进入严肃的心理咨询流程，也不愿意在熟人社交网络里直接暴露自己。HeartLake 试图解决的是这样一个中间层问题：

- 如何让用户低门槛表达情绪
- 如何让表达之后能得到回应而不是石沉大海
- 如何让回应不是简单点赞，而是更有情绪理解和共鸣匹配
- 如何在需要的时候提供更严肃的安全港、咨询和治理支持
- 如何在匿名环境里仍然兼顾安全、恢复、隐私和平台秩序

### 2.2 项目不是简单的“发帖社区”

它的核心不是传统内容社区的流量分发，而是把以下链路连成闭环：

- 表达链：投石
- 互动链：涟漪、纸船、通知、连接消息
- 关系链：好友、临时好友、守护
- 情绪链：日历、热力图、趋势、脉搏
- AI 链：分析、推荐、共鸣、湖神、关怀
- 安全链：匿名身份、恢复密钥、隐私设置、E2E 咨询
- 治理链：管理后台、审核、举报、敏感词、日志、广播

### 2.3 项目最大的特点

从汇报视角看，HeartLake 的差异化亮点主要有八个：

1. 匿名登录不是一次性匿名，而是带恢复密钥和 refresh session 的可持续匿名身份体系。
2. 内容互动不是单一点赞，而是“石头、涟漪、纸船、关系、守护”的多层互动模型。
3. 情绪数据不是装饰性展示，而是真实进入日历、热力图、趋势与脉搏链路。
4. AI 不只是聊天，而是覆盖情绪分析、审核、推荐、共鸣、洞察、守护、RAG 陪伴。
5. 咨询链采用显式端到端加密，服务端只存密文，不存明文。
6. 管理后台不是简单 CRUD，而是运营、审核、风控、日志、Edge AI 观察台。
7. 系统支持低配机部署，`server-lite` 模式面向 `2C2G` 单机环境做过收敛。
8. 仓库里不仅有业务代码，还有迁移、脚本、模型、数据集、部署和验证体系，工程完整度较高。

---

## 3. 适合汇报时怎么开场

### 3.1 30 秒版本

HeartLake 是一个匿名情绪社区平台，用户可以用匿名身份投递“石头”表达心情，其他人用“涟漪”或“纸船”回应，系统再结合推荐、共鸣搜索、情绪趋势、湖神陪伴和安全港资源，形成从表达、互动到关怀的闭环。同时，平台具备后台治理、实时广播、端到端加密咨询和低配机部署能力，是一套从产品到工程都较完整的系统。

### 3.2 3 分钟版本

这个项目解决的是“用户想表达情绪、想被理解、但又不愿暴露真实身份”的场景。前台是 Flutter App，用户可以匿名登录、保存恢复密钥、投石、接收涟漪与纸船、查看情绪日历和趋势、与湖神聊天、进入安全港或咨询流程。后台是 Vue 3 管理端，用来做用户治理、内容审核、举报处理、风险观察和 Edge AI 状态管理。后端采用 Drogon + C++20 单体架构，结合 PostgreSQL、Redis、WebSocket、推荐引擎、共鸣引擎、DualMemoryRAG 和八大 Edge AI 子系统，既保证功能深度，也兼顾性能和低资源部署。

### 3.3 10 分钟版本的推荐结构

建议按下面顺序讲：

1. 先讲项目定位：为什么要做匿名情绪社区。
2. 再讲产品闭环：表达、互动、关系、关怀、治理。
3. 再讲系统组成：移动端、后台、后端、数据库、缓存、实时链。
4. 然后讲 AI 深度：情绪分析、推荐、共鸣、湖神、Edge AI、隐私。
5. 再讲安全性：匿名恢复、PASETO、RBAC、E2E 咨询、证书 pinning。
6. 最后讲工程性：部署、脚本、压测、性能、风险与边界。

---

## 4. 当前线上与交付信息

仓库中明确记录的当前线上事实如下：

- 公网入口：`http://121.41.195.165`
- API：`http://121.41.195.165/api`
- 管理后台：`http://121.41.195.165/admin/`
- WebSocket：`ws://121.41.195.165/ws/broadcast`
- 云端仓库目录：`/root/HeartLake`
- 部署别名：`heartlake-server`
- Android Release 包：`frontend/build/app/outputs/flutter-apk/app-release.apk`
- 当前 APK SHA-256：`9654d5facf294ab1c0d21e6ce6f73728d346977994fe911a23be1de02553ac31`

汇报时可以把这一部分作为“项目已经可运行、可部署、可访问”的证据，而不只是停留在静态设计层。

---

## 5. 面向外部介绍时可以怎么定义用户角色

### 5.1 普通用户

普通用户是匿名旅人，核心动作包括：

- 匿名登录
- 保存恢复密钥
- 发布石头
- 浏览湖面内容
- 发送涟漪和纸船
- 建立连接、好友或临时好友关系
- 查看通知和个人互动
- 查看情绪日历、热力图、趋势
- 接收推荐和共鸣结果
- 使用湖神聊天、安全港与咨询

### 5.2 平台运营/审核人员

管理端用户分为三层角色：

- `viewer`
- `admin`
- `super_admin`

他们的主要职责包括：

- 看平台总览和运行状态
- 管理用户和内容
- 审核、举报、敏感词维护
- 查看风险事件和日志
- 调整系统配置
- 查看 Edge AI、情绪脉搏、隐私预算等运营指标

### 5.3 系统内部服务角色

系统内部还有若干“非人角色”的关键模块：

- 推荐引擎
- 共鸣引擎
- Edge AI 引擎
- DualMemoryRAG
- 湖神守护服务
- 情绪追踪服务
- TTL 关系维护服务
- WebSocket 广播中心
- 可选 QUIC 实时网关

---

## 6. 三端一体的系统组成

### 6.1 总体结构

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

### 6.2 每一端在项目中的职责

| 端 | 技术栈 | 主要职责 |
|---|---|---|
| 移动端 | Flutter | 面向最终用户的匿名情绪表达与互动界面 |
| 管理端 | Vue 3 + Vite | 面向运营和治理的后台控制台 |
| 后端 | Drogon + C++20 | 统一承载 API、实时链、推荐链、AI 链和管理链 |
| 网关 | Nginx | 统一入口，转发 `/api`、`/ws`、`/admin` |
| 存储层 | PostgreSQL + Redis | 结构化数据、会话、缓存、实时消息 |

### 6.3 关键入口文件索引

后端关键入口：

- `backend/src/main.cpp`
- `backend/include/infrastructure/ArchitectureBootstrap.h`
- `backend/src/application/ApplicationServiceFactory.cpp`

移动端关键入口：

- `frontend/lib/main.dart`
- `frontend/lib/router/app_router.dart`
- `frontend/lib/utils/app_config.dart`
- `frontend/lib/di/service_locator.dart`

管理端关键入口：

- `admin/src/router/index.ts`
- `admin/src/layouts/MainLayout.vue`
- `admin/src/stores/index.ts`
- `admin/src/api/index.ts`
- `admin/src/services/websocket.ts`

---

## 7. 从产品视角看，HeartLake 的完整能力图

### 7.1 用户端能力矩阵

| 能力域 | 代表功能 | 价值 |
|---|---|---|
| 身份体系 | 匿名登录、恢复密钥、refresh session | 降低使用门槛，同时保留账号延续性 |
| 内容表达 | 投石、查看湖面、石头详情、删除 | 提供情绪表达出口 |
| 互动反馈 | 涟漪、纸船、通知、连接消息 | 让表达得到回应 |
| 关系建立 | 好友、临时好友、守护 | 从短暂互动走向更稳定关系 |
| 情绪认知 | 日历、热力图、趋势、脉搏 | 帮助用户理解自己的情绪轨迹 |
| AI 支持 | 情绪分析、推荐、共鸣、湖神 | 提高内容匹配与陪伴感 |
| 安全与支持 | 安全港、咨询、举报、隐私设置 | 把社区能力延伸到支持性场景 |
| 增值能力 | VIP 状态、咨询预约、AI 评论频率 | 提供增值服务承接点 |

### 7.2 管理端能力矩阵

| 模块 | 页面 | 作用 |
|---|---|---|
| 平台总览 | `Dashboard.vue` | 看整体运行状态、统计与趋势 |
| 用户治理 | `Users.vue` | 查看用户、详情、状态、推荐结果 |
| 内容治理 | `Content.vue` | 管理内容水位与处置动作 |
| 审核链 | `Moderation.vue` | 查看待审核内容和审核历史 |
| 举报链 | `Reports.vue` | 处理举报与求助事件 |
| 风险词典 | `SensitiveWords.vue` | 维护敏感词与审核规则 |
| 日志链 | `Logs.vue` | 查看管理员与系统日志 |
| 配置链 | `Settings.vue` | 高权限配置管理 |
| 智能观察台 | `EdgeAI.vue` | 查看 Edge AI 状态、指标、预算和配置 |

---

## 8. 前台产品如何被组织起来

### 8.1 首页主导航

`frontend/lib/presentation/screens/home_screen.dart` 定义了底部五个主 Tab：

- 观湖
- 共鸣
- 投石
- 好友
- 倒影

它们共同构成用户主旅程：

1. 先看湖面内容
2. 再去发现与共鸣
3. 自己发布石头
4. 建立关系
5. 回到个人与情绪画像

### 8.2 移动端全部页面清单

当前 `frontend/lib/presentation/screens/` 下页面如下：

- `auth_screen.dart`
- `consultation_screen.dart`
- `discover_screen.dart`
- `emotion_calendar_screen.dart`
- `emotion_heatmap_screen.dart`
- `emotion_trends_screen.dart`
- `friend_chat_screen.dart`
- `friends_screen.dart`
- `guardian_screen.dart`
- `help_screen.dart`
- `home_screen.dart`
- `lake_feed_screen.dart`
- `lake_god_chat_screen.dart`
- `lake_screen.dart`
- `my_boats_screen.dart`
- `my_ripples_screen.dart`
- `my_stones_screen.dart`
- `notification_screen.dart`
- `onboarding_screen.dart`
- `personalized_screen.dart`
- `privacy_settings_screen.dart`
- `profile_screen.dart`
- `publish_screen.dart`
- `received_boats_screen.dart`
- `safe_harbor_screen.dart`
- `splash_screen.dart`
- `stone_detail_screen.dart`
- `temp_friends_screen.dart`
- `user_detail_screen.dart`
- `vip_screen.dart`

### 8.3 路由清单

公开路由：

- `/`
- `/onboarding`
- `/auth`

登录后路由：

- `/home`
- `/notifications`
- `/stone-detail`
- `/profile`
- `/my-stones`
- `/my-boats`
- `/my-ripples`
- `/received-boats`
- `/emotion-heatmap`
- `/emotion-calendar`
- `/emotion-trends`
- `/guardian`
- `/safe-harbor`
- `/consultation`
- `/light`
- `/vip`（兼容旧地址）
- `/help`
- `/privacy-settings`
- `/lake-god-chat`
- `/personalized`
- `/friends`
- `/friend-chat`
- `/temp-friends`
- `/user-detail`
- `/discover`
- `/lake-feed`

### 8.4 路由守卫逻辑

移动端并不是所有页面都默认公开，`app_router.dart` 会通过：

- `StorageUtil.getToken()`
- `StorageUtil.getUserId()`

来决定页面是否放行。这意味着项目不是“假登录”，而是从路由层就显式区分了公开页面和登录后页面。

---

## 9. 后端是这个项目的中枢

### 9.1 后端技术定义

后端是一个 `Drogon + C++20` 单体服务，但它并不简单。它统一承载：

- 业务 API
- 管理 API
- WebSocket 实时链
- 推荐链
- 共鸣链
- Edge AI
- 湖神与陪伴能力
- 隐私与安全链

### 9.2 后端分层结构

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

### 9.3 启动顺序

后端 `backend/src/main.cpp` 的启动顺序大致是：

1. 自动加载 `.env`。
2. 校验关键安全配置，例如 `PASETO_KEY`、数据库密码等。
3. 根据机器核数和场景自适应线程数。
4. 配置监听、日志、静态目录和上传目录。
5. 注册异常处理、CORS、安全响应头、认证与链路追踪。
6. 调用 `ArchitectureBootstrap::initialize()` 完成分层装配。
7. 初始化 RBAC、内容过滤、AI、Embedding、EdgeAI、共鸣、推荐、RAG、Redis 等子系统。
8. 根据环境变量决定是否启动后台守护任务。
9. `app.run()` 对外提供服务。

### 9.4 这一设计说明了什么

从汇报角度讲，这种启动链意味着项目具备以下特征：

- 有明确的依赖初始化顺序
- 有环境感知和低配机优化意识
- 有安全默认项而不是裸奔启动
- 有清晰的服务装配边界
- AI 能力不是外挂脚本，而是主服务的一等公民

---

## 10. 后端目录与职责拆解

### 10.1 核心目录树

```text
backend/
|-- src/main.cpp
|-- include/interfaces/api/
|-- src/interfaces/api/
|-- src/application/
|-- include/domain/
|-- src/domain/
|-- src/infrastructure/ai/
|-- src/infrastructure/services/
|-- src/infrastructure/cache/
|-- src/infrastructure/vector/
|-- src/utils/
|-- migrations/
|-- models/
`-- quic_gateway/
```

### 10.2 控制器清单

当前 `backend/src/interfaces/api/` 下控制器如下：

- `AccountController.cpp`
- `AdminController.cpp`
- `AdminManagementController.cpp`
- `BroadcastWebSocketController.cpp`
- `ConsultationController.cpp`
- `EdgeAIController.cpp`
- `FriendController.cpp`
- `GuardianController.cpp`
- `HealthController.cpp`
- `InteractionController.cpp`
- `MediaController.cpp`
- `PaperBoatController.cpp`
- `PrivacyController.cpp`
- `RecommendationController.cpp`
- `ReportController.cpp`
- `SafeHarborController.cpp`
- `StoneController.cpp`
- `TempFriendController.cpp`
- `UserController.cpp`
- `VIPController.cpp`
- `VectorSearchController.cpp`

### 10.3 应用层

当前应用服务工厂装配的核心应用服务包括：

- `StoneApplicationService`
- `UserApplicationService`
- `InteractionApplicationService`

这些服务承担的是“跨仓储业务编排”和“事件触发”职责，而不是把所有逻辑堆进 Controller。

### 10.4 领域层

当前领域侧主要围绕三大域：

- `stone`
- `user`
- `friend`

其中包括：

- 仓储接口
- 仓储实现
- 领域服务
- 业务规则

### 10.5 基础设施层

基础设施层承载了项目最重的能力，包含：

- 数据库访问
- Redis 缓存
- AI 推理
- 向量检索
- 后台任务
- 实时广播
- 安全与权限工具

---

## 11. 后端最值得讲的不是 CRUD，而是“多条业务链同时存在”

### 11.1 身份与账号链

负责匿名身份、恢复密钥、refresh session、账号信息、设备管理、登录日志、数据导出、停用和永久删除。

核心控制器：

- `UserController`
- `AccountController`
- `PrivacyController`

### 11.2 内容链

负责：

- 发石头
- 看湖面
- 石头详情
- 删除石头
- 湖面天气
- 共鸣分析

核心控制器：

- `StoneController`
- `InteractionController`
- `PaperBoatController`

### 11.3 关系链

负责：

- 好友申请
- 好友私信
- 临时好友
- 连接消息
- 守护与关怀

核心控制器：

- `FriendController`
- `TempFriendController`
- `GuardianController`

### 11.4 关怀链

负责：

- 安全港资源
- 咨询会话
- E2E 加密消息
- VIP 检查与预约
- 举报与求助

核心控制器：

- `SafeHarborController`
- `ConsultationController`
- `VIPController`
- `ReportController`

### 11.5 AI 与推荐链

负责：

- 情感分析
- 审核
- 推荐
- 高级推荐
- 共鸣搜索
- 向量检索
- 湖神聊天

核心控制器：

- `EdgeAIController`
- `RecommendationController`
- `VectorSearchController`
- `GuardianController`

### 11.6 实时与后台治理链

负责：

- WebSocket 接入
- 广播事件
- 管理登录
- 管理查询
- 风险和日志能力

核心控制器：

- `BroadcastWebSocketController`
- `AdminController`
- `AdminManagementController`

---

## 12. AI 是这个项目的深水区

### 12.1 后端 AI 模块清单

当前 `backend/src/infrastructure/ai/` 下模块如下：

- `AIService.cpp`
- `AdvancedEmbeddingEngine.cpp`
- `ContentModerator.cpp`
- `DualMemoryRAG.cpp`
- `EdgeAIEngine.cpp`
- `EdgeDifferentialPrivacy.cpp`
- `EdgeNodeMonitor.cpp`
- `EmotionPulseDetector.cpp`
- `EmotionResonanceEngine.cpp`
- `FederatedLearner.cpp`
- `HNSWIndex.cpp`
- `ModelQuantizer.cpp`
- `OnnxSentimentEngine.cpp`
- `RecommendationEngine.cpp`
- `SemanticCache.cpp`
- `SentimentAnalyzer.cpp`
- `SummaryService.cpp`

### 12.2 八大 Edge AI 子系统

`EdgeAIEngine` 在当前实现中是八大子系统的统一门面：

1. `SentimentAnalyzer`
2. `ContentModerator`
3. `EmotionPulseDetector`
4. `FederatedLearner`
5. `EdgeDifferentialPrivacy`
6. `HNSWIndex`
7. `ModelQuantizer`
8. `EdgeNodeMonitor`

### 12.3 每个子系统对应的意义

| 子系统 | 作用 | 汇报关键词 |
|---|---|---|
| SentimentAnalyzer | 情绪识别 | 情绪理解 |
| ContentModerator | 内容审核 | 风险控制 |
| EmotionPulseDetector | 社区情绪快照 | 群体情绪感知 |
| FederatedLearner | 联邦聚合 | 隐私友好训练 |
| EdgeDifferentialPrivacy | 隐私预算与加噪 | 数据保护 |
| HNSWIndex | 向量近邻检索 | 快速召回 |
| ModelQuantizer | INT8 量化 | 低资源推理 |
| EdgeNodeMonitor | 节点监控 | 稳定性治理 |

### 12.4 上层 AI 能力

除了八大 Edge AI 子系统之外，系统还有这些上层智能能力：

- `AIService`
- `AdvancedEmbeddingEngine`
- `RecommendationEngine`
- `EmotionResonanceEngine`
- `DualMemoryRAG`
- `ResonanceSearchService`
- `LakeGodGuardianService`
- `EmotionTrackingService`

### 12.5 从汇报视角如何解释这些能力

可以把它们理解成三层：

- 第 1 层是基础能力：情绪分析、审核、向量、摘要、量化、监控。
- 第 2 层是智能引擎：推荐、共鸣、RAG、搜索、脉搏。
- 第 3 层是产品落地：发帖前分析、推荐页面、共鸣结果、湖神聊天、守护洞察、后台 EdgeAI 看板。

---

## 13. 移动端不只是壳，它也有自己的架构与智能能力

### 13.1 移动端技术栈

`frontend/pubspec.yaml` 显示当前主要依赖包括：

- `flutter`
- `dio`
- `provider`
- `shared_preferences`
- `flutter_secure_storage`
- `web_socket_channel`
- `cryptography`
- `crypto`
- `get_it`
- `go_router`
- `uuid`
- `image_picker`
- `url_launcher`

### 13.2 移动端启动顺序

`frontend/lib/main.dart` 的核心启动逻辑包括：

1. `runZonedGuarded` 捕获全局异常。
2. 注册 `FlutterError.onError` 和 `platformDispatcher.onError`。
3. 配置错误页，避免白屏。
4. 初始化依赖注入容器。
5. 初始化应用配置。
6. 启动缓存自动清理。
7. 配置 `ApiClient` 的 refresh 与 401 恢复链。
8. 用 `MultiProvider` 挂载主题、用户、通知、石头、好友和 EdgeAI 状态。

### 13.3 移动端依赖注入清单

当前 `service_locator.dart` 注册的服务包括：

- `AuthService`
- `UserService`
- `AccountService`
- `StoneService`
- `InteractionService`
- `FriendService`
- `TempFriendService`
- `NotificationService`
- `GuardianService`
- `ReportService`
- `EdgeAIService`
- `AIRecommendationService`
- `RecommendationService`
- `LakeGodService`
- `ConsultationService`
- `PsychSupportService`
- `VIPService`

### 13.4 Provider 状态链

当前 Provider 主要包括：

- `ThemeProvider`
- `UserProvider`
- `NotificationProvider`
- `StoneProvider`
- `FriendProvider`
- `EdgeAIProvider`

### 13.5 数据源清单

当前 `frontend/lib/data/datasources/` 下数据源如下：

- `account_service.dart`
- `ai_recommendation_service.dart`
- `api_client.dart`
- `auth_service.dart`
- `base_service.dart`
- `cache_service.dart`
- `consultation_service.dart`
- `edge_ai_service.dart`
- `friend_service.dart`
- `guardian_service.dart`
- `interaction_service.dart`
- `lake_god_service.dart`
- `notification_service.dart`
- `psych_support_service.dart`
- `recommendation_response_parser.dart`
- `recommendation_service.dart`
- `report_service.dart`
- `social_payload_normalizer.dart`
- `stone_service.dart`
- `temp_friend_service.dart`
- `user_service.dart`
- `vip_service.dart`
- `websocket_manager.dart`

### 13.6 移动端本地 Edge AI

移动端仓库中还有一组本地 AI 组件：

- `frontend/lib/edge_ai/emotion_classifier.dart`
- `frontend/lib/edge_ai/local_dp.dart`

这里的设计值得专门一讲：

- 本地情绪分类器默认是启发式规则引擎式实现
- 提供 `LocalDPClassifier`
- 在设备端加入 `Laplace` 噪声
- 强调“远端优先、本地降级、带本地差分隐私保护”

`EdgeAIProvider` 的注释明确表述了：

- 优先调用后端情感分析
- 后端弃权或置信度低时走本地降级
- 两路结果可做融合

这意味着项目的移动端并不是完全被动消费服务端结果，而是具备端侧智能兜底意识。

---

## 14. 管理端体现的是“平台治理能力”

### 14.1 管理端技术栈

`admin/package.json` 显示主要技术包括：

- `vue`
- `vue-router`
- `pinia`
- `axios`
- `element-plus`
- `echarts`
- `vue-echarts`
- `dayjs`
- `typescript`
- `vite`

### 14.2 管理端页面清单

当前 `admin/src/views/` 下页面如下：

- `Content.vue`
- `Dashboard.vue`
- `EdgeAI.vue`
- `Forbidden.vue`
- `Login.vue`
- `Logs.vue`
- `Moderation.vue`
- `NotFound.vue`
- `Reports.vue`
- `SensitiveWords.vue`
- `Settings.vue`
- `Users.vue`

### 14.3 路由与角色权限

| 路径 | 页面 | 最低角色 |
|---|---|---|
| `/login` | `Login.vue` | 公开 |
| `/dashboard` | `Dashboard.vue` | `viewer` |
| `/users` | `Users.vue` | `admin` |
| `/content` | `Content.vue` | `admin` |
| `/moderation` | `Moderation.vue` | `admin` |
| `/reports` | `Reports.vue` | `admin` |
| `/sensitive-words` | `SensitiveWords.vue` | `admin` |
| `/logs` | `Logs.vue` | `admin` |
| `/settings` | `Settings.vue` | `super_admin` |
| `/edge-ai` | `EdgeAI.vue` | `admin` |
| `/forbidden` | `Forbidden.vue` | 公开 |
| `/:pathMatch(.*)*` | `NotFound.vue` | 公开 |

角色层级：

- `viewer = 1`
- `admin = 2`
- `super_admin = 3`

### 14.4 管理端做了哪些工程处理

`admin/src/api/index.ts` 与 `admin/src/services/websocket.ts` 所体现的能力包括：

- Bearer token 注入
- 请求去重
- 全局 loading 聚合
- 401 统一清会话
- GET 请求自动重试
- WebSocket query token 鉴权
- 30 秒心跳
- 最多 10 次重连
- 消息白名单处理

### 14.5 `Dashboard.vue` 值得怎么讲

从代码看，Dashboard 并不是只堆几个数字，它展示了：

- 湖面总览
- 当前累计旅人数
- 待处置事项
- 在线人数
- 情绪温度
- 湖面节律
- 旅人波动曲线
- 湖面健康度
- 守湖建议
- 常用快捷入口

也就是说，后台首页是“运营驾驶舱”，不是纯管理台列表页。

---

## 15. 身份、安全、隐私，是项目可信度的关键

### 15.1 匿名登录不是弱认证

系统用户认证链由以下接口组成：

- `POST /api/auth/anonymous`
- `POST /api/auth/recover`
- `POST /api/auth/refresh`

返回关键字段包括：

- `token`
- `refresh_token`
- `session_id`
- `recovery_key`（首次匿名登录）

这套设计的意义在于：

- 保持匿名体验
- 同时避免用户一旦卸载或换设备就彻底丢号
- 为后续设备管理、登录日志、会话清理提供基础

### 15.2 Token 与权限体系

系统使用两套 PASETO 密钥：

- 用户侧：`PASETO_KEY`
- 管理侧：`ADMIN_PASETO_KEY`

过滤链：

- 用户 API：`SecurityAuditFilter`
- 管理 API：`AdminAuthFilter`
- 管理权限：`RBACManager`

### 15.3 客户端本地安全

移动端本地存储策略是：

- 敏感数据进 `FlutterSecureStorage`
- 普通偏好进 `SharedPreferences`

典型敏感字段：

- `token`
- `refresh_token`
- `user_id`
- `session_id`

### 15.4 证书 pinning

移动端 `ApiClient` 支持证书 pinning，配置项包括：

- `ENABLE_CERT_PINNING`
- `CERT_SHA256_PINS`

这意味着项目在安全上考虑过中间人攻击场景，而不是只停留在“能请求通就行”。

### 15.5 端到端加密咨询链

咨询链采用：

- `X25519`
- `HKDF-SHA256`
- `AES-256-GCM`

核心接口：

- `POST /api/consultation/session`
- `POST /api/consultation/key-exchange`
- `POST /api/consultation/message`
- `GET /api/consultation/messages/{sessionId}`

后端只保存：

- `ciphertext`
- `iv`
- `tag`

这意味着服务端不直接落咨询明文。对于一个情绪/心理支持平台，这是一条非常值得汇报时强调的能力线。

### 15.6 隐私设置与差分隐私

隐私设置的真实字段包括：

- `profile_visibility`
- `show_online_status`
- `allow_message_from_stranger`

差分隐私相关接口包括：

- `GET /api/lake/privacy-stats`
- `GET /api/lake/privacy-report`
- `GET /api/edge-ai/privacy-budget`
- `POST /api/edge-ai/privacy/add-noise`
- `POST /api/edge-ai/privacy/reset`

---

## 16. 实时链路是 HeartLake 的体验核心

### 16.1 当前主实时方案

当前主部署中的实时方案是：

- `WebSocket`
- 路径：`/ws/broadcast`
- 通过 query `token` 鉴权
- 握手成功消息：`auth_success`
- 心跳：`ping / pong`

### 16.2 服务端广播事件

当前文档中明确列出的业务事件包括：

- `new_stone`
- `stone_deleted`
- `ripple_update`
- `ripple_deleted`
- `boat_update`
- `boat_deleted`
- `new_notification`
- `new_friend_message`
- `friend_removed`
- `temp_friend_expired`
- `new_report`
- `new_moderation`
- `broadcast`
- `stats_update`

### 16.3 实时链的重要意义

这条链使得以下体验具备即时性：

- 湖面更新
- 互动数变化
- 新通知到达
- 好友消息同步
- 后台统计和运营告警更新

### 16.4 可选 QUIC 网关子项目

仓库中还包含独立的 Rust 子项目：

- `backend/quic_gateway/`

它基于：

- `quinn`
- `tokio`
- `rustls`
- `redis pub/sub`

其职责是：

- QUIC 接入
- TLS 终结
- PASETO 鉴权
- Redis 订阅后扇出实时事件

它支持三种分发范围：

- `broadcast`
- `user`
- `room`

从当前仓库部署情况看：

- 主路径仍然是 `Nginx + WebSocket`
- `docker-compose.yml` 未把 QUIC 网关纳入默认服务
- `server-lite` 默认 `REALTIME_QUIC_ENABLED=false`

因此汇报时可以这样表述：

> 当前线上主链路是 WebSocket，仓库中额外预留了可选 QUIC 实时网关方案，体现了系统对下一阶段实时性能与传输能力扩展的准备。

---

## 17. 数据库不是随便建几张表，而是按业务域铺开的

### 17.1 当前迁移总数

正式迁移文件共有 `21` 个：

- `001_users.sql`
- `002_stones.sql`
- `003_social.sql`
- `004_interactions.sql`
- `005_ai_system.sql`
- `006_guardian.sql`
- `007_admin.sql`
- `008_notifications.sql`
- `009_consultation.sql`
- `010_data_export.sql`
- `011_performance_schema_hardening.sql`
- `012_users_username_compat.sql`
- `013_schema_fixes.sql`
- `014_stones_embedding_vector.sql`
- `015_runtime_schema_hotfix.sql`
- `016_admin_runtime_tables.sql`
- `017_account_schema_alignment.sql`
- `018_user_identity_contract.sql`
- `019_feature_contract_completion.sql`
- `020_query_path_index_tuning.sql`
- `021_auth_refresh_sessions.sql`

### 17.2 核心数据域

#### 用户与身份

- `users`
- `user_similarity`
- `user_sessions`
- `login_logs`
- `security_events`
- `user_privacy_settings`
- `user_blocks`
- `data_export_tasks`

#### 内容与互动

- `stones`
- `stone_embeddings`
- `ripples`
- `paper_boats`
- `notifications`

#### 关系与消息

- `friends`
- `temp_friends`
- `friend_messages`
- `connections`
- `connection_messages`

#### AI、情绪与推荐

- `lake_god_messages`
- `user_emotion_history`
- `user_interaction_history`
- `emotion_tracking`
- `resonance_points`
- `edge_ai_inference_logs`
- `federated_learning_rounds`
- `differential_privacy_budget`
- `community_emotion_snapshots`
- `edge_nodes`
- `vector_index_metadata`

#### 管理、关怀与增值

- `admin_users`
- `sensitive_words`
- `operation_logs`
- `reports`
- `moderation_logs`
- `broadcast_messages`
- `consultation_sessions`
- `consultation_messages`
- `vip_privileges`
- `vip_privilege_usage`
- `high_risk_events`
- `admin_interventions`

### 17.3 这一层怎么对外解释

这套表设计说明项目已经从单一内容站点扩展为多域系统，至少覆盖：

- 身份域
- 社交域
- 内容域
- 情绪域
- 推荐域
- 安全域
- 管理域
- 增值服务域

---

## 18. 运行时模型与离线数据集要区分开讲

### 18.1 运行时模型资源

`backend/models/` 当前用于线上/运行期模型资源说明，主要包括：

- `sentiment_zh.onnx`
- `vocab.txt`
- `sentiment_domain_lexicon.tsv`

仓库中还带有第三方 ONNX Runtime 资源包：

- `backend/third_party/onnxruntime-linux-x64-1.22.0.tgz`

当前本地文件大小可见：

- `sentiment_zh.onnx` 约 `99M`
- `vocab.txt` 约 `107K`
- `sentiment_domain_lexicon.tsv` 约 `45K`

### 18.2 离线数据集目录

`datasets/` 当前只用于离线实验和参考，不参与线上运行。当前包含：

- `chinese_sentiment_simplified/`
- `online_shopping_10_cats_simplified/`
- `waimai_10k_simplified/`

这些目录各自包含：

- `train.csv`
- `dev.csv`
- `test.csv`

### 18.3 如何在汇报中正确描述

建议明确区分：

- `backend/models/` 是运行时模型与词表资源
- `datasets/` 是离线实验与研究资料

这样可以体现项目既有“线上可用的工程实现”，也有“模型与数据支撑的技术背景”，但不会让人误解数据集直接参与线上服务。

---

## 19. 系统配置与环境变量非常完整

### 19.1 后端关键环境变量

后端环境变量大致分为几类：

- 服务监听：`SERVER_HOST`、`SERVER_PORT`、`SERVER_THREADS`、`LOG_LEVEL`
- 文档与上传：`HEARTLAKE_CONFIG_PATH`、`HEARTLAKE_DOCUMENT_ROOT`、`HEARTLAKE_UPLOAD_PATH`
- 公开入口与跨域：`CORS_ALLOWED_ORIGIN`、`PUBLIC_API_URL`、`PUBLIC_WS_URL`、`PUBLIC_QUIC_URL`
- PostgreSQL：`DB_HOST`、`DB_PORT`、`DB_NAME`、`DB_USER`、`DB_PASSWORD`、`DB_POOL_SIZE`
- Redis：`REDIS_HOST`、`REDIS_PORT`、`REDIS_PASSWORD`、`REDIS_DB`
- 认证：`PASETO_KEY`、`ADMIN_USERNAME`、`ADMIN_PASETO_KEY`、`ADMIN_PASSWORD_HASH`
- AI：`AI_PROVIDER`、`AI_API_KEY`、`AI_BASE_URL`、`AI_MODEL`
- Edge AI：`EDGE_AI_ENABLED`、`EDGE_AI_ONNX_ENABLED`、`EDGE_AI_MODEL_PATH`、`EDGE_AI_VOCAB_PATH`、`EDGE_AI_SENTIMENT_LEXICON_PATH`
- 后台任务：`ENABLE_LAKE_GOD_GUARDIAN`、`ENABLE_EMOTION_TRACKING`、`ENABLE_USER_FOLLOWUP`、`ENABLE_WS_HEARTBEAT`

### 19.2 移动端关键环境变量

移动端主要读取：

- `PUBLIC_ORIGIN`
- `API_BASE_URL`
- `WS_URL`
- `PRODUCTION_PUBLIC_ORIGIN`
- `DEV_API_URL`
- `ANDROID_API_HOST`
- `IOS_API_HOST`
- `ENABLE_CERT_PINNING`
- `CERT_SHA256_PINS`

### 19.3 管理端关键环境变量

- `VITE_API_BASE_URL`
- `VITE_WS_URL`

### 19.4 这能说明什么

说明项目不是把地址和密钥硬编码进代码，而是具备：

- 环境切换能力
- 部署适配能力
- 开发/生产隔离能力
- 安全配置注入能力

---

## 20. 部署拓扑与低配机策略

### 20.1 标准 Docker 拓扑

`docker-compose.yml` 定义的服务包括：

- `postgres`
- `redis`
- `ollama`
- `backend`
- `admin`
- `gateway`

### 20.2 每个服务的职责

| 服务 | 作用 |
|---|---|
| `postgres` | 业务主数据库，使用 `pgvector/pgvector:pg16` |
| `redis` | 缓存、会话、实时链基础 |
| `ollama` | 本地 AI 服务接入点 |
| `backend` | Drogon 后端主服务 |
| `admin` | Vue 管理端 |
| `gateway` | Nginx 网关统一入口 |

### 20.3 `server-lite` 的意义

`docker-compose.server-lite.yml` 不是重复文件，而是低配环境策略文件。它主要体现为：

- PostgreSQL 更保守的内存参数
- Redis 内存上限与淘汰策略
- 后端更少线程、更小连接池
- 更短 AI 超时和更激进熔断参数
- 默认关闭重型后台扫描任务
- 默认关闭 QUIC

这意味着项目考虑了从“理想开发机”到“2C2G 小服务器”的真实落地差异。

### 20.4 网关职责

`deploy/nginx/gateway.conf` 负责：

- `/api/` 转发后端 API
- `/ws/` 转发 WebSocket
- `/chat/` 转发聊天实时链
- `/admin/` 转发管理端
- `/assets/` 转发管理端静态资源
- `/healthz` 提供健康检查

### 20.5 常用启动与运行方式

推荐启动方式：

```bash
./scripts/docker-up.sh server-lite
cd frontend
flutter pub get
flutter run
```

单模块启动：

后端：

```bash
cd backend
cmake -S . -B build-2c2g -DCMAKE_BUILD_TYPE=Release
cmake --build build-2c2g -j2
```

管理端：

```bash
cd admin
npm install
npm run dev
```

移动端：

```bash
cd frontend
flutter pub get
flutter run
```

健康检查：

```bash
curl -fsS http://127.0.0.1:3000/api/health
curl -fsS http://127.0.0.1:3000/healthz
curl -I -fsS http://127.0.0.1:3000/admin/
```

服务器环境生成：

```bash
./scripts/generate-server-env.sh --host 121.41.195.165 --admin-password 'YourStrongPass'
```

---

## 21. 脚本体系说明工程可交付性

当前 `scripts/` 目录下主要脚本如下：

- `create_full_access_account.sh`
- `deploy-server-lite-local.sh`
- `docker-down.sh`
- `docker-test.sh`
- `docker-up.sh`
- `full_api_smoke.py`
- `generate-server-env.sh`
- `reset_all_data_and_seed.sh`
- `restore-onnx-model.sh`
- `seed_core_content_only.sql`
- `seed_demo_showcase_account.sql`
- `smoke-client-connect.mjs`
- `start-local-cloud-dev.sh`
- `start-services.sh`
- `verify-2c2g.sh`
- `verify-onnx-smoke.sh`

### 21.1 重点脚本及作用

| 脚本 | 作用 |
|---|---|
| `docker-up.sh` | 启动全量、精简或 `server-lite` 环境 |
| `docker-down.sh` | 停止容器，可选 `--purge` 清卷 |
| `start-services.sh` | 对 `docker-up.sh` 的简化包装 |
| `docker-test.sh` | 做网关、后台、匿名登录、隐私、投石、涟漪、情绪、推荐与客户端联调 smoke |
| `verify-2c2g.sh` | Release 构建后端、构建管理端、执行 Flutter 分析 |
| `verify-onnx-smoke.sh` | 开启 ONNX 路径并运行 Edge AI 基准测试 |
| `generate-server-env.sh` | 生成 `server-lite` 环境变量文件 |
| `deploy-server-lite-local.sh` | 本地构建镜像并增量部署到远端主机 |
| `start-local-cloud-dev.sh` | 本地前端直连云端 API/WS 进行联调 |
| `reset_all_data_and_seed.sh` | 重建数据库、执行迁移、注入种子内容、创建演示账号 |
| `create_full_access_account.sh` | 创建具备高权限与演示属性的全功能账号 |
| `restore-onnx-model.sh` | 下载并恢复 ONNX 模型与词表 |
| `full_api_smoke.py` | 更完整的 API 级冒烟验证工具 |
| `smoke-client-connect.mjs` | 匿名登录后验证客户端 HTTP + WebSocket 链路 |

### 21.2 这部分在汇报里有什么用

很多项目只展示“代码有多复杂”，但脚本体系其实能证明另外三件事：

- 这个项目能被快速启动
- 这个项目能被验证
- 这个项目能被部署与交接

---

## 22. 当前测试与验证体系

### 22.1 官方文档里明确的验证命令

```bash
./scripts/verify-2c2g.sh
./scripts/docker-test.sh
VERIFY_ONNX_SMOKE=1 ./scripts/verify-onnx-smoke.sh
```

### 22.2 `docker-test.sh` 覆盖的场景

从脚本本身看，它至少验证：

- 容器状态
- 后端健康检查
- 网关健康检查
- 管理后台首页可访问
- 客户端 HTTP + WebSocket 联调
- 匿名登录
- 账号隐私读取与更新
- 湖面石头列表
- 投石主流程
- 涟漪互动
- 我的涟漪列表
- 情绪日历与热力图
- 好友列表
- 推荐接口

### 22.3 `verify-2c2g.sh` 覆盖的场景

- 后端 `Release` 构建
- 管理端生产构建
- Flutter 静态分析

### 22.4 ONNX smoke

`verify-onnx-smoke.sh` 会：

- 配置开启 ONNX Runtime
- 构建 `benchmark_edge_ai`
- 运行 `SentimentAccuracyBenchmark`

### 22.5 当前测试资产的真实情况

当前仓库里：

- `backend/tests/` 下没有可见测试文件
- `frontend/test/` 下没有可见测试文件
- `tests/` 目录下也没有可见测试文件

这意味着项目当前的主验证方式更偏：

- 构建验证
- 冒烟验证
- 端到端链路验证
- 压测与运行观测

这不是坏事，但在汇报里应当诚实表达：

> 当前项目在“系统级验证”和“部署级验证”上相对完整，但在显式单元测试/集成测试文件沉淀方面仍有继续完善空间。

---

## 23. 当前性能数据可以直接作为汇报素材

最近一次文档中记录的压测结果显示：

- 5 分钟混合 HTTP soak
- 累计请求：`406572`
- 整体吞吐：`1330.91 rps`

关键接口结果：

| 路径 | 平均 | p95 |
|---|---:|---:|
| `/api/lake/stones` | `8.52ms` | `16.61ms` |
| `/api/account/info` | `8.27ms` | `16.32ms` |
| `/api/recommendations/trending` | `7.81ms` | `15.95ms` |
| `/api/stones/{id}/resonance` | `11.33ms` | `19.45ms` |
| `/api/stones/{id}/ripples` | `9.77ms` | `17.77ms` |
| `/api/health` | `7.14ms` | `15.27ms` |

WebSocket 压测结果：

- 并发连接：`120`
- 成功连接：`120/120`
- 握手平均：`42.67ms`
- 握手 p95：`51.62ms`

资源占用抽样：

- backend：`313MiB` 到 `337.2MiB`
- postgres：`68.67MiB` 到 `86.88MiB`
- redis：约 `13.63MiB`
- gateway：约 `4MiB` 到 `6MiB`

这组数据在汇报中的用途很明确：

- 证明项目不是概念原型
- 证明它在低配环境下具备上线级性能
- 证明 AI 与推荐链经过真实压测，而不是空壳接口

---

## 24. API 全景图

下面这部分适合在“系统能力非常完整”的场合展示，也适合答辩时被问到“具体做了哪些接口”时直接参考。

### 24.1 健康检查

- `GET /api/health`
- `GET /api/health/detailed`

### 24.2 认证与用户

- `POST /api/auth/anonymous`
- `POST /api/auth/recover`
- `POST /api/auth/refresh`
- `GET /api/users/{id}`
- `GET /api/users/{id}/stats`
- `GET /api/users/search`
- `GET /api/users/my/boats`
- `PUT /api/users/my/profile`
- `GET /api/users/my/emotion-calendar`
- `GET /api/users/my/emotion-heatmap`

### 24.3 账号与隐私

- `GET /api/account/info`
- `POST /api/account/avatar`
- `PUT /api/account/profile`
- `GET /api/account/stats`
- `GET /api/account/devices`
- `DELETE /api/account/devices/{sessionId}`
- `GET /api/account/login-logs`
- `GET /api/account/security-events`
- `GET /api/account/privacy`
- `PUT /api/account/privacy`
- `GET /api/account/blocked-users`
- `POST /api/account/block/{targetUserId}`
- `DELETE /api/account/unblock/{targetUserId}`
- `POST /api/account/export`
- `GET /api/account/export/{taskId}`
- `POST /api/account/deactivate`
- `POST /api/account/delete-permanent`
- `GET /api/lake/privacy-stats`
- `GET /api/lake/privacy-report`

### 24.4 媒体

- `POST /api/media/upload`
- `GET /api/media/{category}/{filename}`

### 24.5 石头与湖面

- `POST /api/stones`
- `GET /api/lake/stones`
- `GET /api/stones/my`
- `GET /api/stones/{stoneId}`
- `DELETE /api/stones/{stoneId}`
- `GET /api/lake/weather`
- `GET /api/stones/{stoneId}/resonance`

### 24.6 互动

- `POST /api/stones/{stoneId}/ripples`
- `POST /api/stones/{stoneId}/boats`
- `GET /api/stones/{stoneId}/boats`
- `GET /api/interactions/my/ripples`
- `GET /api/interactions/my/boats`
- `DELETE /api/ripples/{rippleId}`
- `DELETE /api/boats/{boatId}`
- `GET /api/boats/{boatId}`
- `GET /api/boats/sent`
- `GET /api/boats/received`
- `POST /api/boats/reply`
- `GET /api/notifications`
- `POST /api/notifications/{notificationId}/read`
- `POST /api/notifications/read-all`
- `GET /api/notifications/unread-count`
- `POST /api/stones/{stoneId}/connections`
- `POST /api/connections`
- `GET /api/connections/{connectionId}/messages`
- `POST /api/connections/{connectionId}/messages`

### 24.7 好友与临时好友

- `POST /api/friends/request`
- `DELETE /api/friends/{friendId}`
- `GET /api/friends`
- `POST /api/friends/{friendId}/messages`
- `GET /api/friends/{friendId}/messages`
- `GET /api/temp-friends`
- `GET /api/temp-friends/{tempFriendId}`
- `DELETE /api/temp-friends/{tempFriendId}`
- `GET /api/temp-friends/check/{targetUserId}`

### 24.8 关怀与增值

- `GET /api/vip/status`
- `GET /api/vip/privileges`
- `GET /api/vip/counseling/check`
- `POST /api/vip/counseling/book`
- `GET /api/vip/ai-comment-frequency`
- `GET /api/safe-harbor/hotlines`
- `GET /api/safe-harbor/tools`
- `GET /api/safe-harbor/prompt`
- `GET /api/safe-harbor/resources`
- `GET /api/safe-harbor/recommend`
- `POST /api/safe-harbor/resources`
- `PUT /api/safe-harbor/resources/{id}`
- `DELETE /api/safe-harbor/resources/{id}`
- `POST /api/safe-harbor/access`
- `GET /api/safe-harbor/access/history`
- `POST /api/consultation/session`
- `POST /api/consultation/key-exchange`
- `POST /api/consultation/message`
- `GET /api/consultation/messages/{sessionId}`
- `GET /api/consultation/sessions`
- `POST /api/reports`
- `GET /api/reports/my`

### 24.9 推荐与 AI

- `GET /api/recommendations/stones`
- `GET /api/recommendations/discover/{mood}`
- `POST /api/recommendations/track`
- `POST /api/recommendations/track-batch`
- `GET /api/recommendations/emotion-trends`
- `GET /api/recommendations/trending`
- `POST /api/recommendations/search`
- `GET /api/recommendations/advanced`
- `GET /api/admin/recommendations/advanced`
- `GET /api/recommendations/similar-stones/{id}`
- `GET /api/recommendations/personalized`
- `POST /api/admin/stones/{id}/embedding`
- `GET /api/edge-ai/status`
- `GET /api/edge-ai/metrics`
- `POST /api/edge-ai/analyze`
- `POST /api/edge-ai/moderate`
- `GET /api/edge-ai/emotion-pulse`
- `POST /api/edge-ai/federated/aggregate`
- `GET /api/edge-ai/privacy-budget`
- `POST /api/edge-ai/vector-search`
- `POST /api/edge-ai/vector-insert`
- `POST /api/edge-ai/emotion-sample`
- `GET /api/edge-ai/pulse-history`
- `POST /api/edge-ai/federated/submit`
- `POST /api/edge-ai/privacy/reset`
- `POST /api/edge-ai/nodes/register`
- `PUT /api/edge-ai/nodes/status`
- `GET /api/edge-ai/nodes/best`
- `POST /api/edge-ai/quantized-forward`
- `POST /api/edge-ai/privacy/add-noise`
- `GET /api/edge-ai/config`
- `PUT /api/edge-ai/config`
- `POST /api/edge-ai/summary`
- `POST /api/lake-god/chat`
- `GET /api/lake-god/history`

### 24.10 管理后台接口

认证与基础信息：

- `POST /api/admin/login`
- `POST /api/admin/logout`
- `GET /api/admin/info`

统计与风控：

- `GET /api/admin/stats/realtime`
- `GET /api/admin/stats/dashboard`
- `GET /api/admin/stats/trending-topics`
- `GET /api/admin/stats/user-growth`
- `GET /api/admin/stats/mood-distribution`
- `GET /api/admin/stats/mood-trend`
- `GET /api/admin/stats/active-time`
- `GET /api/admin/risk/high-risk-users`
- `GET /api/admin/risk/events`
- `GET /api/admin/risk/user/{user_id}/history`
- `POST /api/admin/risk/event/{event_id}/handle`
- `GET /api/admin/security/audit`

内容与用户治理：

- `GET /api/admin/users`
- `GET /api/admin/users/{id}`
- `PUT /api/admin/users/{id}/status`
- `POST /api/admin/users/{id}/ban`
- `POST /api/admin/users/{id}/unban`
- `GET /api/admin/content`
- `GET /api/admin/stones`
- `GET /api/admin/stones/{id}`
- `DELETE /api/admin/stones/{id}`
- `GET /api/admin/boats`
- `DELETE /api/admin/boats/{id}`
- `GET /api/admin/moderation/pending`
- `POST /api/admin/moderation/{id}/approve`
- `POST /api/admin/moderation/{id}/reject`
- `GET /api/admin/moderation/history`
- `GET /api/admin/reports`
- `GET /api/admin/reports/{id}`
- `POST /api/admin/reports/{id}/handle`
- `GET /api/admin/sensitive-words`
- `POST /api/admin/sensitive-words`
- `PUT /api/admin/sensitive-words/{id}`
- `DELETE /api/admin/sensitive-words/{id}`
- `GET /api/admin/config`
- `PUT /api/admin/config`
- `POST /api/admin/broadcast`
- `GET /api/admin/broadcast/history`
- `GET /api/admin/logs`

管理端 EdgeAI 镜像路由：

- `GET /api/admin/edge-ai/status`
- `GET /api/admin/edge-ai/metrics`
- `POST /api/admin/edge-ai/analyze`
- `POST /api/admin/edge-ai/moderate`
- `GET /api/admin/edge-ai/emotion-pulse`
- `POST /api/admin/edge-ai/federated/aggregate`
- `GET /api/admin/edge-ai/privacy-budget`
- `POST /api/admin/edge-ai/vector-search`
- `POST /api/admin/edge-ai/vector-insert`
- `POST /api/admin/edge-ai/emotion-sample`
- `GET /api/admin/edge-ai/pulse-history`
- `POST /api/admin/edge-ai/federated/submit`
- `POST /api/admin/edge-ai/privacy/reset`
- `POST /api/admin/edge-ai/nodes/register`
- `PUT /api/admin/edge-ai/nodes/status`
- `GET /api/admin/edge-ai/nodes/best`
- `POST /api/admin/edge-ai/quantized-forward`
- `POST /api/admin/edge-ai/privacy/add-noise`
- `GET /api/admin/edge-ai/config`
- `PUT /api/admin/edge-ai/config`

### 24.11 WebSocket

- `GET /ws/broadcast`

客户端消息类型：

- `join`
- `leave`
- `room_message`
- `ping`

服务端消息类型：

- `auth_success`
- `pong`
- `error`
- 各类业务广播事件

---

## 25. 典型业务闭环怎么讲最容易让人理解

### 25.1 表达闭环

1. 用户匿名登录进入首页。
2. 用户在投石页发布内容。
3. 系统触发情绪分析与必要审核。
4. 石头入库并广播到湖面。
5. 其他用户看到内容并产生互动。

### 25.2 共鸣闭环

1. 用户浏览发现页或个性化页。
2. 推荐引擎、共鸣引擎与向量检索共同召回候选内容。
3. 系统展示“为什么推荐”“为什么共鸣”。
4. 用户进入石头详情继续互动。

### 25.3 情绪认知闭环

1. 用户持续产生内容与互动。
2. 系统记录情绪轨迹与快照。
3. 用户可在日历、热力图、趋势和脉搏页面查看变化。
4. 平台或湖神可据此提供洞察和建议。

### 25.4 关怀闭环

1. 用户遇到更高情绪压力场景。
2. 系统可提供安全港资源、热线、工具或咨询入口。
3. 咨询链用 E2E 加密保障敏感消息。
4. VIP 能力和守护能力形成更深层支持。

### 25.5 治理闭环

1. 内容、举报、风险事件进入管理后台。
2. 运营与审核人员查看 Dashboard 和队列。
3. 平台完成审核、处置、广播和日志留痕。
4. Edge AI 看板帮助持续观察智能系统运行状态。

---

## 26. 这个项目最适合向外展示的技术亮点

### 26.1 亮点一：匿名但可恢复

很多匿名系统的用户一旦换设备就彻底丢号。HeartLake 用恢复密钥与 refresh session 解决了“匿名与可持续身份”之间的矛盾。

### 26.2 亮点二：情绪不是 UI 装饰，而是主业务数据

情绪数据贯穿：

- 发帖前分析
- 趋势可视化
- 脉搏检测
- 推荐与共鸣
- 湖神洞察
- 后台观察

### 26.3 亮点三：AI 是分层设计，不是单点接大模型

它同时有：

- 规则与词典
- ONNX 本地推理
- 向量检索
- 推荐引擎
- 共鸣引擎
- RAG
- 联邦学习
- 差分隐私

### 26.4 亮点四：心理支持场景下的安全意识较强

关键体现在：

- E2E 咨询
- PASETO
- RBAC
- 证书 pinning
- 差分隐私预算
- 审计与安全日志

### 26.5 亮点五：项目工程化比较完整

体现在：

- 有三端代码
- 有迁移
- 有模型
- 有脚本
- 有部署文件
- 有压测结果
- 有完整文档体系

---

## 27. 当前边界与风险，也要诚实讲

如果是在答辩或正式汇报，建议不要只讲优点，下面这些“已知边界”反而会提升可信度。

### 27.1 当前自动化测试资产更多偏系统级，而不是单测级

仓库里当前没有显式的丰富单元测试文件沉淀，主要依赖：

- 构建验证
- smoke
- docker 联调
- 压测

### 27.2 低配机模式下，部分后台扫描任务默认关闭

在 `server-lite` 下：

- `ENABLE_LAKE_GOD_GUARDIAN=false`
- `ENABLE_EMOTION_TRACKING=false`
- `ENABLE_USER_FOLLOWUP=false`

这意味着低资源部署策略是“先保主链路稳定，再逐步打开重任务”。

### 27.3 数据库仍有持续优化空间

文档记录中已提到：

- PostgreSQL 存在 `collation version mismatch`
- 某些表仍需持续盯顺序扫描与索引表现

### 27.4 QUIC 仍属于预备能力，不是当前主交付链

项目有 QUIC 子项目，但默认主交付链仍是：

- Nginx
- HTTP API
- WebSocket

### 27.5 AI 相关功能存在显式降级语义

这点不是缺点，反而是一种成熟表现。文档明确强调：

- AI 失败不能伪装成成功
- 推荐降级要显式标注
- 页面失败不能静默变空白或默认值

---

## 28. 如果要做现场演示，推荐这样走

### 28.1 用户侧演示路径

1. 打开 App。
2. 介绍匿名登录与恢复密钥。
3. 进入湖面查看内容流。
4. 发布一条石头。
5. 展示发帖前情绪分析或 AI 预览。
6. 在石头详情中演示涟漪/纸船。
7. 打开发现页或个性化页，展示推荐与共鸣。
8. 打开情绪日历或趋势页，展示情绪数据沉淀。
9. 展示湖神聊天或守护洞察。
10. 展示安全港或咨询页，强调 E2E 加密。

### 28.2 后台演示路径

1. 打开管理后台登录页。
2. 进入 Dashboard 看平台总览。
3. 展示 Users 页面与用户详情。
4. 展示 Reports / Moderation 的治理闭环。
5. 展示 SensitiveWords 与 Logs。
6. 打开 EdgeAI 页面，展示情绪脉搏、隐私预算、节点状态。

### 28.3 技术演示路径

1. 展示仓库三端结构。
2. 展示 Docker Compose 与 `server-lite` 配置。
3. 展示脚本体系。
4. 展示压测结果。
5. 展示迁移与模型目录。

---

## 29. 答辩时常见问题与推荐回答

### 29.1 “为什么要匿名？”

因为情绪表达往往是高敏感行为，匿名能降低表达门槛；但单纯匿名又会导致身份无法延续，所以系统加入恢复密钥和 refresh session 解决“匿名可持续性”问题。

### 29.2 “和普通内容社区的区别是什么？”

普通社区以内容分发为中心，HeartLake 以情绪表达、共鸣互动和支持性链路为中心，强调的是表达后被理解、被回应、被接住，而不是单纯流量消费。

### 29.3 “为什么后端用 C++？”

这个项目明显强调性能、实时链、低资源部署和较重的工程控制。Drogon + C++20 在这些场景下具备较好的性能与可控性，也能体现底层工程实现能力。

### 29.4 “AI 具体做了什么？”

不是只做聊天。AI 覆盖：

- 情绪分析
- 内容审核
- 情绪脉搏
- 推荐
- 共鸣匹配
- 向量搜索
- 湖神双记忆对话
- 隐私预算
- 联邦学习

### 29.5 “有没有考虑隐私和安全？”

有，而且是系统级考虑，包括：

- PASETO
- 管理端 RBAC
- 证书 pinning
- 咨询 E2E 加密
- 差分隐私预算
- 安全事件与登录日志

### 29.6 “有没有真实跑起来？”

有。仓库中记录了线上入口、APK、Docker 部署、健康检查、压测结果和验证脚本，说明项目不是纸面方案。

### 29.7 “当前最大的不足是什么？”

当前不足主要是：

- 显式单元测试资产还可以继续补
- 低配模式下部分后台任务默认关闭
- 数据库和 AI 路径仍有继续调优空间

---

## 30. 仓库内容盘点

### 30.1 顶层结构

```text
.
|-- frontend/
|-- admin/
|-- backend/
|-- docs/
|-- doc/
|-- deploy/
|-- datasets/
|-- scripts/
|-- docker-compose.yml
|-- docker-compose.server-lite.yml
|-- README.md
`-- .env.example
```

### 30.2 `frontend/`

移动端主工程，负责最终用户体验。

### 30.3 `admin/`

管理后台主工程，负责运营与治理。

### 30.4 `backend/`

后端主工程，负责 API、管理、实时与 AI。

### 30.5 `docs/`

现有正式文档目录，适合开发与运维人员阅读。

### 30.6 `doc/`

本文件所在目录，适合沉淀更偏汇报、讲解、答辩和交接导向的综合说明。

### 30.7 `deploy/`

部署资源，例如 Nginx 网关配置。

### 30.8 `datasets/`

离线实验与研究数据集目录，不参与线上运行。

### 30.9 `scripts/`

启动、验证、部署、种子数据、模型恢复与 smoke 脚本目录。

---

## 31. 现有文档体系也值得在汇报中提一下

当前 `docs/` 中已有文档包括：

- `00_新手入门指南.md`
- `01_本地启动与运行手册.md`
- `02_API与实时链路手册.md`
- `03_端到端测试与故障排查手册.md`
- `04_技术实现全景手册.md`
- `05_API接口全量清单.md`
- `06_测试验证与压测手册.md`
- `07_编码与注释规范.md`
- `08_参考文献.md`
- `09_架构修补与性能治理方案.md`
- `10_上线检查清单.md`
- `deploy-ubuntu-2c2g.md`

这说明项目不仅实现了代码，还做了比较系统的文档建设。

---

## 32. 最后给汇报者的建议

如果你的听众偏产品或管理，请重点讲：

- 为什么需要匿名情绪社区
- 用户闭环
- 关怀闭环
- 治理闭环
- 已经可运行、可部署、可验证

如果你的听众偏技术，请重点讲：

- 三端架构
- C++ 后端分层
- WebSocket 实时链
- Edge AI 八大子系统
- 推荐/共鸣/RAG
- E2E 咨询加密
- 低配部署与压测结果

如果你的听众既关注业务又关注工程，最佳讲法是：

1. 先用“匿名情绪表达与支持平台”定义场景。
2. 再用“表达、互动、关系、关怀、治理”讲产品闭环。
3. 再用“三端一体 + AI + 安全 + 压测”证明项目深度。

---

## 33. 一段可以直接照着念的总结

HeartLake 不是单纯的社交 App，也不是单纯的 AI Demo，而是一套围绕匿名情绪表达、情绪共鸣、心理支持和平台治理构建的完整系统。它在产品层面完成了从表达、互动到关怀的闭环，在技术层面完成了从移动端、管理端到 C++ 后端、数据库、缓存、实时链、AI 引擎、推荐引擎和隐私安全链的整合，在工程层面具备部署、脚本、迁移、压测和文档体系。因此，这个项目最有价值的地方不只是“功能多”，而是它已经形成了一个相对完整、可运行、可治理、可扩展的情绪社区平台雏形。
