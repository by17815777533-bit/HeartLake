<p align="center">
  <img src="docs/assets/heartlake-logo.png" alt="HeartLake Logo" width="200"/>
</p>

<h1 align="center">HeartLake 心湖</h1>

<p align="center">
  <strong>匿名情感社交平台 · 边缘AI驱动 · 安全架构</strong>
</p>

<p align="center">
  <em>Edge AI · DDD架构 · 差分隐私 · 联邦学习 · E2E加密 · PASETO认证 · 高性能C++20</em>
</p>

<p align="center">
  <a href="#快速开始">快速开始</a> ·
  <a href="#技术亮点">技术亮点</a> ·
  <a href="#架构总览">架构总览</a> ·
  <a href="#边缘ai引擎详解">边缘AI引擎</a> ·
  <a href="#api概览">API文档</a> ·
  <a href="#安全架构">安全架构</a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?style=flat-square&logo=cplusplus" alt="C++20"/>
  <img src="https://img.shields.io/badge/Drogon-1.9+-green?style=flat-square" alt="Drogon"/>
  <img src="https://img.shields.io/badge/Vue-3.4+-brightgreen?style=flat-square&logo=vuedotjs" alt="Vue3"/>
  <img src="https://img.shields.io/badge/Flutter-3.19+-02569B?style=flat-square&logo=flutter" alt="Flutter"/>
  <img src="https://img.shields.io/badge/PostgreSQL-16+-336791?style=flat-square&logo=postgresql" alt="PostgreSQL"/>
  <img src="https://img.shields.io/badge/Redis-7.2+-DC382D?style=flat-square&logo=redis" alt="Redis"/>
  <img src="https://img.shields.io/badge/Docker-Compose-2496ED?style=flat-square&logo=docker" alt="Docker"/>
  <img src="https://img.shields.io/badge/License-AGPL--3.0-orange?style=flat-square" alt="License"/>
  <img src="https://img.shields.io/badge/Edge_AI-8_Engines-ff69b4?style=flat-square" alt="Edge AI"/>
  <img src="https://img.shields.io/badge/Architecture-DDD-purple?style=flat-square" alt="DDD"/>
  <img src="https://img.shields.io/badge/Security-PASETO%20%7C%20E2E%20%7C%20DP-critical?style=flat-square" alt="Security"/>
  <img src="https://img.shields.io/badge/Realtime-WebSocket-blueviolet?style=flat-square" alt="WebSocket"/>
</p>

---

## 目录

- [简介](#简介)
- [技术亮点](#技术亮点)
- [架构总览](#架构总览)
- [边缘AI引擎详解](#边缘ai引擎详解)
- [安全架构](#安全架构)
- [核心功能](#核心功能)
- [快速开始](#快速开始)
- [API概览](#api概览)
- [项目结构](#项目结构)
- [技术栈](#技术栈)
- [许可证](#许可证)

---

## 简介

HeartLake 心湖是一个面向现代情感社交场景的全栈匿名平台，旨在为用户提供一个安全、温暖、智能的情感表达空间。用户可以将心事写成「心石」投入湖中，通过边缘AI引擎实现情绪识别、同频共鸣匹配和智能推荐，在完全匿名的环境下找到情感共鸣。

项目采用 C++20 + Drogon 高性能后端、Flutter 跨平台移动端、Vue3 管理后台的三端架构，以 DDD（领域驱动设计）为核心架构范式，配合 PostgreSQL 16、Redis 7、Docker Compose 实现生产级部署。

核心差异化在于自研的边缘AI引擎（8大子系统），将 AI 推理下沉到端侧，结合差分隐私和联邦学习，在保护用户隐私的前提下提供智能化的情感社交体验。

### 为什么选择 HeartLake？

| 维度 | HeartLake 方案 | 传统社交平台 |
|------|---------------|-------------|
| 隐私保护 | 差分隐私 + E2E加密 + PASETO | 明文存储 + Cookie Session |
| AI推理 | 端侧Edge AI，数据不出设备 | 云端集中式推理 |
| 匿名机制 | 零知识匿名 + 石友TTL自动过期 | 实名制 + 永久关系链 |
| 内容安全 | AC自动机 + 图片审核 + 湖神守护 | 人工审核为主 |
| 性能 | C++20原生性能，亚毫秒级响应 | Java/Go/Node.js |
| 架构 | DDD四层 + 事件驱动 + CQRS | 传统MVC三层 |

---

## 技术亮点

### 1. 高性能 C++20 后端

- 基于 Drogon 异步框架，单机支撑万级并发连接
- C++20 Concepts / Coroutines / Ranges 全面应用
- CMake 构建系统，支持 LTO（Link-Time Optimization）优化
- GTest 单元测试框架，CI/CD 友好

### 2. 严格的 DDD 四层架构

```
Interfaces（接口层）  →  21个 Controller，RESTful + WebSocket
Application（应用层） →  4大 Application Service，编排领域逻辑
Domain（领域层）      →  实体 / 仓储接口 / 领域服务，纯业务逻辑
Infrastructure（基础设施层） →  AI / Cache / Privacy / Vector / Realtime
```

### 3. 边缘AI引擎（8大子系统）

在端侧完成情绪分类、文本嵌入、隐私保护等核心AI任务，最大限度保护用户隐私：

| 子系统 | 技术 | 职责 |
|--------|------|------|
| AC自动机 | Aho-Corasick | 高速敏感词过滤，O(n)复杂度 |
| HNSW向量索引 | Hierarchical NSW | 同频共鸣语义搜索 |
| 差分隐私引擎 | Laplace/Gaussian机制 | 统计数据脱敏 |
| 联邦学习 | FedAvg | 模型训练不出设备 |
| 情绪脉搏 | Emotion Classifier | 实时情绪识别与追踪 |
| INT8量化 | Post-Training Quantization | 模型体积压缩4x，推理加速 |
| 节点监控 | Health Monitor | AI引擎健康度实时监控 |
| 熔断降级 | Circuit Breaker | 故障自动隔离与优雅降级 |

### 4. 安全架构

- PASETO v4 Token 认证（替代 JWT，无算法混淆攻击）
- E2E 端到端加密（咨询会话全程加密）
- 差分隐私（Differential Privacy）保护统计查询
- 多层过滤器链：Auth → RateLimit → SecurityAudit → Trace
- 安全港（Safe Harbor）机制，危机干预自动触发

### 5. 实时通信与智能推荐

- WebSocket Hub 实时消息推送
- Dual-Memory RAG（双记忆检索增强生成）
- 语义缓存（Semantic Cache）减少重复推理
- Qdrant / Milvus 双向量数据库支持
- 情感共鸣引擎（Emotion Resonance Engine）

### 6. 全平台覆盖

- Flutter 移动端：26+ 页面，情绪日历、纸船漂流、湖神对话等
- Vue3 管理后台：用户管理、内容审核、数据看板
- Nginx 反向代理 + SSL 终结
---

## 核心功能

### 心石系统（Stone）

心石是 HeartLake 的核心内容载体，用户将心事写成心石投入湖中。

- 匿名发布：无需暴露身份，自由表达情感
- 情绪标注：Edge AI 自动识别情绪类型并标注
- 向量嵌入：每颗心石生成语义向量，支持同频搜索
- 涟漪互动：其他用户可以对心石产生「涟漪」（共鸣反馈）
- TTL机制：心石可设置生命周期，到期自动沉入湖底

### 纸船漂流（Paper Boat）

随机匿名通信机制，将消息装入纸船随机漂向另一位用户。

- 随机投递：系统随机匹配接收者，创造意外的情感连接
- 匿名回复：接收者可匿名回复，开启一段未知的对话
- 漂流轨迹：可查看自己发出和收到的纸船历史

### 石友系统（Temp Friend）

基于心石共鸣建立的临时好友关系，自带 TTL 过期机制。

- 共鸣触发：对同一心石产生共鸣的用户可申请成为石友
- TTL过期：石友关系默认有时间限制，到期自动解除
- 升级机制：双方可选择延长或升级为长期好友
- FriendshipTTLEngine：后台引擎自动管理关系生命周期

### 同频共鸣搜索（Resonance Search）

基于语义向量的情感共鸣匹配，找到与你「同频」的人和内容。

- 语义搜索：不是关键词匹配，而是理解情感语义的深层搜索
- HNSW加速：亚毫秒级向量近邻搜索
- 多维度匹配：情绪类型 + 语义相似度 + 时间衰减综合排序
- ResonanceSearchService：封装完整的共鸣搜索业务逻辑

### 湖神守护（Lake God Guardian）

AI驱动的社区守护系统，保障社区安全与用户心理健康。

- 内容巡检：AC自动机 + 图片审核双引擎实时过滤
- 心理预警：情绪分析检测到极端情绪自动触发干预
- 安全港机制：SafeHarborService 提供危机干预通道
- 守护者激励：GuardianIncentiveService 激励社区志愿守护者
- 暖心语录：WarmQuoteService 在低落时刻推送温暖内容

### 情绪追踪（Emotion Tracking）

长期情绪数据追踪与可视化，帮助用户了解自己的情绪变化。

- 情绪日历：按日/周/月展示情绪变化趋势
- 情绪脉搏：实时情绪状态可视化
- 大气背景：Flutter端根据情绪动态切换沉浸式背景效果
- 深潜层效果：情绪低谷时的特殊视觉体验

### VIP会员体系

- 专属功能解锁：高级情绪分析、无限纸船、扩展石友数量
- A/B测试：ABTestService 支持功能灰度发布
- 会员管理：完整的VIP生命周期管理

### 管理后台（Admin）

Vue3 构建的全功能管理后台。

- 用户管理：查看/封禁/解封用户
- 内容审核：心石/评论/纸船内容审核队列
- 数据看板：实时统计数据可视化
- 系统配置：敏感词库管理、参数配置
- 安全审计：操作日志、安全事件追踪

---

## 快速开始

### 环境要求

| 依赖 | 最低版本 | 说明 |
|------|---------|------|
| Docker | 24.0+ | 容器运行时 |
| Docker Compose | 2.20+ | 多容器编排 |
| Git | 2.40+ | 代码版本管理 |

### 一键部署（推荐）

```bash
# 1. 克隆仓库
git clone https://github.com/your-org/heartlake.git
cd heartlake

# 2. 配置环境变量
cp backend/.env.example backend/.env
# 编辑 backend/.env，设置必要的密钥和数据库密码

# 3. 启动全栈服务
docker compose up -d

# 4. 等待健康检查通过
docker compose ps

# 5. 访问服务
# API 后端:    http://localhost:8080
# 管理后台:    http://localhost:80/admin
# API 健康检查: http://localhost:8080/api/health
```

### 本地开发（后端）

```bash
# 安装依赖
sudo apt install cmake g++ libssl-dev libcurl4-openssl-dev

# 安装 Drogon 框架
git clone https://github.com/drogonframework/drogon
cd drogon && git submodule update --init
mkdir build && cd build
cmake .. && make -j$(nproc) && sudo make install

# 构建 HeartLake 后端
cd heartlake/backend
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 启动数据库和缓存
docker compose up -d postgres redis

# 运行数据库迁移
for f in migrations/*.sql; do
  psql -h localhost -U postgres -d heartlake -f "$f"
done

# 启动后端服务
./HeartLake
```

### 本地开发（Flutter 移动端）

```bash
cd heartlake/frontend

# 安装依赖
flutter pub get

# 运行测试
flutter test

# 启动开发模式（请在终端手动执行）
# flutter run
```

### 本地开发（Vue3 管理后台）

```bash
cd heartlake/admin

# 安装依赖
npm install

# 启动开发模式（请在终端手动执行）
# npm run dev
```

### 运行测试

```bash
# 后端单元测试
cd backend/build
ctest --output-on-failure

# Flutter 测试
cd frontend
flutter test

# 全栈集成测试
docker compose up -d
curl -f http://localhost:8080/api/health
```

---

## API概览

HeartLake 后端提供 RESTful API + WebSocket 实时通信，共 21 个 Controller。

### 认证相关

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/auth/register` | 匿名注册 | 无 |
| POST | `/api/auth/login` | 登录获取 PASETO Token | 无 |
| POST | `/api/auth/refresh` | Token 续期 | 需要 |

### 心石（Stone）

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/stones` | 投放心石 | 需要 |
| GET | `/api/stones` | 获取心石列表（湖面） | 需要 |
| GET | `/api/stones/:id` | 获取心石详情 | 需要 |
| DELETE | `/api/stones/:id` | 删除自己的心石 | 需要 |

### 互动（Interaction）

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/stones/:id/ripple` | 对心石产生涟漪 | 需要 |
| POST | `/api/stones/:id/resonate` | 同频共鸣 | 需要 |

### 纸船（Paper Boat）

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/paper-boats` | 放出纸船 | 需要 |
| GET | `/api/paper-boats/received` | 收到的纸船 | 需要 |
| POST | `/api/paper-boats/:id/reply` | 回复纸船 | 需要 |

### 好友（Friend）

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/friends/request` | 发送石友申请 | 需要 |
| GET | `/api/friends` | 好友列表 | 需要 |
| POST | `/api/friends/:id/chat` | 发送消息 | 需要 |

### 临时好友（Temp Friend）

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/temp-friends` | 临时好友列表 | 需要 |
| POST | `/api/temp-friends/:id/extend` | 延长石友关系 | 需要 |

### 湖神与守护

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/lake-god/chat` | 与湖神对话 | 需要 |
| GET | `/api/guardian/status` | 守护者状态 | 需要 |
| POST | `/api/safe-harbor/report` | 安全港求助 | 需要 |

### 向量搜索与推荐

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/vector/search` | 语义向量搜索 | 需要 |
| GET | `/api/recommendations` | 个性化推荐 | 需要 |

### Edge AI

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/edge-ai/status` | AI引擎状态 | 管理员 |
| POST | `/api/edge-ai/analyze` | 内容AI分析 | 需要 |

### 管理后台

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/admin/users` | 用户管理 | 管理员 |
| GET | `/api/admin/stones` | 内容审核 | 管理员 |
| GET | `/api/admin/stats` | 数据统计 | 管理员 |

### 其他

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/health` | 健康检查 | 无 |
| GET | `/api/account/profile` | 个人资料 | 需要 |
| POST | `/api/media/upload` | 媒体上传 | 需要 |
| GET | `/api/privacy/export` | 数据导出（GDPR） | 需要 |
| POST | `/api/report` | 举报内容 | 需要 |
| GET | `/api/vip/status` | VIP状态 | 需要 |
| WS | `/ws/broadcast` | 实时广播频道 | 需要 |
| POST | `/api/consultation` | 心理咨询（E2E加密） | 需要 |

---

## 项目结构

```
heartlake/
├── backend/                          # C++20 Drogon 后端
│   ├── CMakeLists.txt                # CMake 构建配置
│   ├── include/                      # 头文件（按DDD分层）
│   │   ├── application/              # 应用层
│   │   │   ├── handlers/
│   │   │   │   └── EventHandlers.h           # 事件处理器
│   │   │   ├── ApplicationServiceFactory.h   # 服务工厂
│   │   │   ├── FriendApplicationService.h    # 好友应用服务
│   │   │   ├── InteractionApplicationService.h # 互动应用服务
│   │   │   ├── StoneApplicationService.h     # 心石应用服务
│   │   │   └── UserApplicationService.h      # 用户应用服务
│   │   ├── config/
│   │   │   └── ConfigManager.h               # 配置管理
│   │   ├── domain/                   # 领域层
│   │   │   ├── entities/
│   │   │   │   ├── Stone.h                   # 心石实体
│   │   │   │   └── User.h                    # 用户实体
│   │   │   ├── friend/
│   │   │   │   ├── repositories/
│   │   │   │   │   ├── FriendRepository.h
│   │   │   │   │   └── IFriendRepository.h   # 好友仓储接口
│   │   │   │   └── services/
│   │   │   │       └── FriendService.h       # 好友领域服务
│   │   │   ├── stone/
│   │   │   │   ├── repositories/
│   │   │   │   │   ├── IStoneRepository.h    # 心石仓储接口
│   │   │   │   │   └── StoneRepository.h
│   │   │   │   └── services/
│   │   │   │       └── StoneService.h        # 心石领域服务
│   │   │   └── user/
│   │   │       └── repositories/
│   │   │           ├── IUserRepository.h     # 用户仓储接口
│   │   │           └── UserRepository.h
│   │   ├── infrastructure/           # 基础设施层
│   │   │   ├── ai/                   # AI引擎
│   │   │   │   ├── AdvancedEmbeddingEngine.h # 高级嵌入引擎
│   │   │   │   ├── AIService.h               # AI服务门面
│   │   │   │   ├── DualMemoryRAG.h           # 双记忆RAG
│   │   │   │   ├── EdgeAIEngine.h            # 边缘AI引擎
│   │   │   │   ├── EmotionResonanceEngine.h  # 情感共鸣引擎
│   │   │   │   ├── ImageModerationEngine.h   # 图片审核引擎
│   │   │   │   ├── LocalEmbeddingService.h   # 本地嵌入服务
│   │   │   │   ├── RecommendationEngine.h    # 推荐引擎
│   │   │   │   ├── SemanticCache.h           # 语义缓存
│   │   │   │   └── SummaryService.h          # 摘要服务
│   │   │   ├── cache/                # 缓存
│   │   │   │   ├── CacheManager.h
│   │   │   │   └── RedisCache.h
│   │   │   ├── di/
│   │   │   │   └── ServiceLocator.h          # 依赖注入
│   │   │   ├── events/
│   │   │   │   └── EventBus.h                # 事件总线
│   │   │   ├── filters/              # 过滤器链
│   │   │   │   ├── AdminAuthFilter.h
│   │   │   │   ├── AuthFilter.h
│   │   │   │   ├── RateLimitFilter.h
│   │   │   │   ├── SecurityAuditFilter.h
│   │   │   │   └── TraceFilter.h
│   │   │   ├── messaging/
│   │   │   │   └── NatsService.h             # NATS消息队列
│   │   │   ├── privacy/
│   │   │   │   └── DifferentialPrivacyEngine.h # 差分隐私
│   │   │   ├── realtime/
│   │   │   │   └── WebSocketHub.h            # WebSocket中心
│   │   │   ├── services/             # 业务基础服务
│   │   │   │   ├── DataExportService.h       # 数据导出(GDPR)
│   │   │   │   ├── EmailService.h
│   │   │   │   ├── EmotionTrackingService.h  # 情绪追踪
│   │   │   │   ├── FriendshipTTLEngine.h     # 石友TTL引擎
│   │   │   │   ├── GuardianIncentiveService.h # 守护者激励
│   │   │   │   ├── LakeGodGuardianService.h  # 湖神守护
│   │   │   │   ├── MediaService.h
│   │   │   │   ├── NotificationPushService.h # 推送通知
│   │   │   │   ├── PsychologicalRiskIntegration.h # 心理风险
│   │   │   │   ├── ResonanceSearchService.h  # 共鸣搜索
│   │   │   │   ├── SafeHarborService.h       # 安全港
│   │   │   │   ├── UserFollowUpService.h
│   │   │   │   ├── VerificationService.h
│   │   │   │   ├── VIPService.h
│   │   │   │   └── WarmQuoteService.h        # 暖心语录
│   │   │   ├── vector/               # 向量数据库
│   │   │   │   ├── MilvusClient.h
│   │   │   │   └── QdrantClient.h
│   │   │   └── ArchitectureBootstrap.h       # 架构引导
│   │   ├── interfaces/               # 接口层
│   │   │   └── api/                  # 21个Controller
│   │   │       ├── AccountController.h
│   │   │       ├── AdminController.h
│   │   │       ├── AdminManagementController.h
│   │   │       ├── BroadcastWebSocketController.h
│   │   │       ├── ConsultationController.h
│   │   │       ├── EdgeAIController.h
│   │   │       ├── FriendController.h
│   │   │       ├── GuardianController.h
│   │   │       ├── HealthController.h
│   │   │       ├── InteractionController.h
│   │   │       ├── MediaController.h
│   │   │       ├── PaperBoatController.h
│   │   │       ├── PrivacyController.h
│   │   │       ├── RecommendationController.h
│   │   │       ├── ReportController.h
│   │   │       ├── SafeHarborController.h
│   │   │       ├── StoneController.h
│   │   │       ├── TempFriendController.h
│   │   │       ├── UserController.h
│   │   │       ├── VectorSearchController.h
│   │   │       └── VIPController.h
│   │   ├── middleware/
│   │   │   └── RateLimiter.h
│   │   └── utils/
│   ├── src/                          # 源文件（与include对应）
│   ├── tests/                        # GTest 单元测试
│   ├── migrations/                   # 数据库迁移（21个SQL）
│   │   ├── 001_users.sql
│   │   ├── 002_stones.sql
│   │   ├── 003_stone_embeddings.sql
│   │   ├── 004_guardian_system.sql
│   │   ├── ...
│   │   └── 021_edge_ai_system.sql
│   ├── config.json                   # 本地开发配置
│   ├── config.docker.json            # Docker环境配置
│   ├── Dockerfile
│   └── start.sh
│
├── frontend/                         # Flutter 移动端
│   ├── lib/
│   │   ├── data/                     # 数据层
│   │   │   ├── datasources/          # 19个API服务
│   │   │   │   ├── api_client.dart           # HTTP客户端
│   │   │   │   ├── auth_service.dart         # 认证服务
│   │   │   │   ├── stone_service.dart        # 心石服务
│   │   │   │   ├── friend_service.dart       # 好友服务
│   │   │   │   ├── paper_boat_service.dart   # 纸船服务
│   │   │   │   ├── lake_god_service.dart     # 湖神服务
│   │   │   │   ├── guardian_service.dart      # 守护者服务
│   │   │   │   ├── websocket_manager.dart    # WebSocket管理
│   │   │   │   └── ...
│   │   │   └── repositories/         # 仓储实现
│   │   ├── domain/                   # 领域层
│   │   │   └── entities/             # 实体模型
│   │   │       ├── stone.dart
│   │   │       ├── user.dart
│   │   │       ├── paper_boat.dart
│   │   │       ├── mood.dart
│   │   │       └── emotion_type.dart
│   │   ├── edge_ai/                  # 端侧AI引擎
│   │   │   ├── edge_ai.dart                  # AI引擎入口
│   │   │   ├── emotion_classifier.dart       # 情绪分类器
│   │   │   └── local_dp.dart                 # 本地差分隐私
│   │   ├── emotion_effects/          # 情绪视觉效果
│   │   │   ├── atmospheric_background.dart   # 大气背景
│   │   │   ├── deep_dive_layer.dart          # 深潜层
│   │   │   ├── mood_background.dart          # 情绪背景
│   │   │   ├── mood_colors.dart              # 情绪色彩
│   │   │   └── water_background.dart         # 水面背景
│   │   ├── presentation/             # 表现层
│   │   │   ├── providers/            # 状态管理
│   │   │   ├── screens/              # 26个页面
│   │   │   │   ├── home_screen.dart
│   │   │   │   ├── lake_screen.dart          # 湖面主页
│   │   │   │   ├── publish_screen.dart       # 投石页
│   │   │   │   ├── stone_detail_screen.dart  # 心石详情
│   │   │   │   ├── paper_boat_screen.dart    # 纸船漂流
│   │   │   │   ├── emotion_calendar_screen.dart # 情绪日历
│   │   │   │   ├── lake_god_chat_screen.dart # 湖神对话
│   │   │   │   ├── guardian_screen.dart      # 守护者
│   │   │   │   ├── vip_screen.dart           # VIP
│   │   │   │   └── ...
│   │   │   └── widgets/              # 可复用组件
│   │   ├── utils/                    # 工具类
│   │   └── main.dart                 # 应用入口
│   ├── test/                         # 测试
│   ├── pubspec.yaml                  # Flutter依赖
│   └── analysis_options.yaml         # Lint配置
│
├── admin/                            # Vue3 管理后台
│   ├── src/
│   │   ├── api/                      # API接口
│   │   ├── views/                    # 页面视图
│   │   ├── stores/                   # Pinia状态管理
│   │   ├── router/                   # 路由配置
│   │   ├── services/                 # 业务服务
│   │   ├── layouts/                  # 布局组件
│   │   ├── utils/                    # 工具函数
│   │   ├── App.vue                   # 根组件
│   │   └── main.js                   # 入口文件
│   ├── package.json
│   ├── vite.config.js                # Vite构建配置
│   └── Dockerfile
│
├── nginx/                            # Nginx配置
│   ├── nginx.conf                    # 主配置
│   ├── conf.d/                       # 站点配置
│   └── ssl/                          # SSL证书
│
├── docker-compose.yml                # 全栈编排
└── README.md                         # 本文件
```

---

## 技术栈

### 后端

| 技术 | 版本 | 用途 |
|------|------|------|
| C++ | 20 | 主开发语言，Concepts/Coroutines/Ranges |
| Drogon | 1.9+ | 高性能异步Web框架 |
| PostgreSQL | 16+ | 主数据库，支持JSONB/向量扩展 |
| Redis | 7.2+ | 缓存/会话/限流/发布订阅 |
| CMake | 3.16+ | 构建系统 |
| GTest | latest | 单元测试框架 |
| OpenSSL | 3.0+ | 加密/TLS |
| libcurl | latest | HTTP客户端 |
| NATS | latest | 消息队列 |
| Qdrant | latest | 向量数据库 |
| Milvus | latest | 向量数据库（备选） |

### 移动端

| 技术 | 版本 | 用途 |
|------|------|------|
| Flutter | 3.19+ | 跨平台UI框架 |
| Dart | 3.3+ | 开发语言 |
| Provider | latest | 状态管理 |
| Dio | latest | HTTP客户端 |

### 管理后台

| 技术 | 版本 | 用途 |
|------|------|------|
| Vue | 3.4+ | 前端框架 |
| Vite | 5.0+ | 构建工具 |
| Pinia | latest | 状态管理 |
| Vue Router | 4.0+ | 路由管理 |

### 基础设施

| 技术 | 版本 | 用途 |
|------|------|------|
| Docker | 24.0+ | 容器化 |
| Docker Compose | 2.20+ | 多容器编排 |
| Nginx | alpine | 反向代理/SSL终结/静态资源 |

### AI与安全

| 技术 | 说明 |
|------|------|
| PASETO v4 | 替代JWT的安全Token方案 |
| XChaCha20-Poly1305 | PASETO底层加密算法 |
| Aho-Corasick | 多模式字符串匹配（敏感词） |
| HNSW | 近似最近邻向量搜索 |
| Differential Privacy | Laplace/Gaussian差分隐私 |
| Federated Learning | FedAvg联邦学习 |
| INT8 Quantization | 模型量化压缩 |
| E2E Encryption | 端到端加密 |

---

## 许可证

本项目基于 [AGPL-3.0](LICENSE) 许可证开源。

```
HeartLake 心湖 - 匿名情感社交平台
Copyright (C) 2024-2026

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
```

---

<p align="center">
  <sub>用技术温暖每一颗孤独的心 · Built with love and C++20</sub>
</p>
## 架构总览

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Client Layer                                 │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐  │
│  │  Flutter Mobile   │  │  Vue3 Admin      │  │  Third-party     │  │
│  │  (iOS / Android)  │  │  (管理后台)       │  │  (API Consumer)  │  │
│  │                   │  │                   │  │                   │  │
│  │  ┌─────────────┐  │  └──────────────────┘  └──────────────────┘  │
│  │  │  Edge AI     │  │                                             │
│  │  │  Engine      │  │         ▼ HTTPS / WSS                       │
│  │  └─────────────┘  │                                              │
│  └──────────────────┘                                               │
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                    ┌──────▼──────┐
                    │    Nginx     │
                    │  (SSL终结 +   │
                    │   反向代理)   │
                    └──────┬──────┘
                           │
┌──────────────────────────▼──────────────────────────────────────────┐
│                   Backend (C++20 / Drogon)                          │
│                                                                     │
│  ┌─── Interfaces ──────────────────────────────────────────────┐   │
│  │  StoneController · UserController · FriendController        │   │
│  │  GuardianController · VIPController · EdgeAIController      │   │
│  │  PaperBoatController · RecommendationController             │   │
│  │  SafeHarborController · BroadcastWebSocketController        │   │
│  │  AdminController · MediaController · PrivacyController ...  │   │
│  └──────────────────────────┬──────────────────────────────────┘   │
│                              │                                      │
│  ┌─── Middleware ───────────┤──────────────────────────────────┐   │
│  │  AuthFilter → RateLimitFilter → SecurityAuditFilter         │   │
│  │  → TraceFilter → AdminAuthFilter → RateLimiter              │   │
│  └──────────────────────────┤──────────────────────────────────┘   │
│                              │                                      │
│  ┌─── Application ──────────▼──────────────────────────────────┐   │
│  │  UserApplicationService    StoneApplicationService           │   │
│  │  FriendApplicationService  InteractionApplicationService     │   │
│  │  EventHandlers (事件处理器)                                   │   │
│  └──────────────────────────┬──────────────────────────────────┘   │
│                              │                                      │
│  ┌─── Domain ───────────────▼──────────────────────────────────┐   │
│  │  Entities: User · Stone                                      │   │
│  │  Services: FriendService · StoneService                      │   │
│  │  Repositories: IUserRepo · IStoneRepo · IFriendRepo          │   │
│  └──────────────────────────┬──────────────────────────────────┘   │
│                              │                                      │
│  ┌─── Infrastructure ───────▼──────────────────────────────────┐   │
│  │                                                              │   │
│  │  AI Layer:                                                   │   │
│  │    EdgeAIEngine · EmotionResonanceEngine · AIService         │   │
│  │    AdvancedEmbeddingEngine · DualMemoryRAG · SemanticCache   │   │
│  │    ImageModerationEngine · RecommendationEngine              │   │
│  │    LocalEmbeddingService · SummaryService                    │   │
│  │                                                              │   │
│  │  Service Layer:                                              │   │
│  │    LakeGodGuardianService · EmotionTrackingService           │   │
│  │    FriendshipTTLEngine · ResonanceSearchService              │   │
│  │    SafeHarborService · GuardianIncentiveService              │   │
│  │    NotificationPushService · VIPService · MediaService       │   │
│  │    WarmQuoteService · PsychologicalRiskIntegration           │   │
│  │    VerificationService · DataExportService · EmailService    │   │
│  │                                                              │   │
│  │  Infra Layer:                                                │   │
│  │    RedisCache · CacheManager · EventBus · WebSocketHub       │   │
│  │    DifferentialPrivacyEngine · ServiceLocator · NatsService  │   │
│  │    QdrantClient · MilvusClient · ABTestService               │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                     │
└──────────────┬──────────────────────────┬──────────────────────────┘
               │                          │
        ┌──────▼──────┐           ┌───────▼───────┐
        │ PostgreSQL   │           │    Redis       │
        │ 16-alpine    │           │  7-alpine      │
        │              │           │                │
        │ · users      │           │ · session      │
        │ · stones     │           │ · rate_limit   │
        │ · embeddings │           │ · cache        │
        │ · friends    │           │ · pub/sub      │
        │ · guardians  │           │ · leaderboard  │
        │ · 20+ tables │           │                │
        └─────────────┘           └────────────────┘
```

### 数据流概览

```
用户投石 → Flutter端侧情绪分类(Edge AI) → API请求 → Nginx → Drogon路由
  → AuthFilter鉴权 → RateLimitFilter限流 → StoneController
  → StoneApplicationService编排 → StoneService领域逻辑
  → StoneRepository持久化 → PostgreSQL
  → EdgeAIEngine嵌入生成 → Qdrant/Milvus向量存储
  → EmotionTrackingService情绪追踪 → WebSocketHub实时推送
  → RecommendationEngine同频推荐 → 返回响应
```

---

## 边缘AI引擎详解

HeartLake 的边缘AI引擎是整个平台的智能核心，由8大子系统协同工作，实现端侧智能与云端协同的混合AI架构。

### 整体架构

```
┌──────────────────────────────────────────────────────────────┐
│                    EdgeAIEngine (调度中心)                     │
│                                                              │
│  ┌────────────┐  ┌────────────┐  ┌────────────────────────┐  │
│  │ AC自动机    │  │ HNSW索引   │  │ DifferentialPrivacy    │  │
│  │ 敏感词过滤  │  │ 向量近邻    │  │ Engine 差分隐私        │  │
│  └────────────┘  └────────────┘  └────────────────────────┘  │
│                                                              │
│  ┌────────────┐  ┌────────────┐  ┌────────────────────────┐  │
│  │ 联邦学习    │  │ 情绪脉搏    │  │ INT8量化引擎           │  │
│  │ FedAvg     │  │ Classifier │  │ Post-Training Quant    │  │
│  └────────────┘  └────────────┘  └────────────────────────┘  │
│                                                              │
│  ┌────────────────────────┐  ┌────────────────────────────┐  │
│  │ 节点监控 HealthMonitor  │  │ 熔断降级 CircuitBreaker    │  │
│  └────────────────────────┘  └────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 子系统 1：AC自动机（Aho-Corasick）

高性能多模式字符串匹配引擎，用于实时敏感词过滤。

- 算法复杂度：O(n + m + z)，n为文本长度，m为模式总长，z为匹配数
- 构建 Trie 树 + Failure Link，一次扫描匹配所有敏感词
- 支持动态词库热更新，无需重启服务
- 应用场景：心石内容审核、聊天消息过滤、用户昵称检测

### 子系统 2：HNSW 向量索引

基于 Hierarchical Navigable Small World 图的近似最近邻搜索，驱动「同频共鸣」核心功能。

- 多层跳表结构，搜索复杂度 O(log n)
- 支持余弦相似度 / 欧氏距离 / 内积三种度量
- 与 Qdrant / Milvus 向量数据库深度集成
- 应用场景：相似心石推荐、情感共鸣匹配、用户画像聚类

### 子系统 3：差分隐私引擎（Differential Privacy）

为统计查询注入可控噪声，防止通过聚合数据反推个体信息。

- 支持 Laplace 机制（数值型查询）和 Gaussian 机制（高维查询）
- 可配置隐私预算 epsilon（ε），平衡隐私保护与数据可用性
- 组合定理自动追踪累计隐私消耗
- 应用场景：情绪统计报表、用户行为分析、热门话题排行

### 子系统 4：联邦学习（Federated Learning）

采用 FedAvg 算法，在不收集原始数据的前提下训练全局模型。

- 端侧本地训练 → 上传梯度 → 服务端聚合 → 下发全局模型
- 支持安全聚合（Secure Aggregation），防止梯度泄露
- 差分隐私梯度裁剪，双重隐私保障
- 应用场景：情绪分类模型优化、推荐模型个性化、内容质量评估

### 子系统 5：情绪脉搏（Emotion Classifier）

端侧实时情绪识别引擎，为每条内容标注情绪标签。

- 支持多维情绪分类：喜悦、悲伤、愤怒、恐惧、惊讶、厌恶、平静
- 轻量级模型，推理延迟 < 10ms
- 情绪追踪时间线，生成用户情绪日历
- 应用场景：心石情绪标注、情绪日历生成、危机预警触发

### 子系统 6：INT8量化引擎

训练后量化（Post-Training Quantization），将 FP32 模型压缩为 INT8。

- 模型体积压缩约 4 倍，内存占用大幅降低
- 推理速度提升 2-3 倍（利用 SIMD 整数指令）
- 精度损失控制在 1% 以内（校准数据集优化）
- 应用场景：端侧嵌入模型、情绪分类模型、文本摘要模型

### 子系统 7：节点监控（Health Monitor）

AI引擎各子系统的健康度实时监控与告警。

- 心跳检测：定期探测各子系统存活状态
- 性能指标：推理延迟 P50/P95/P99、吞吐量、错误率
- 资源监控：CPU / 内存 / GPU 使用率
- 应用场景：运维告警、自动扩缩容决策、SLA保障

### 子系统 8：熔断降级（Circuit Breaker）

故障自动隔离与优雅降级机制，保障系统整体可用性。

- 三态模型：Closed（正常）→ Open（熔断）→ Half-Open（探测恢复）
- 可配置失败率阈值、熔断时长、探测窗口
- 降级策略：返回缓存结果 / 默认值 / 简化模型
- 应用场景：AI服务故障隔离、第三方API降级、数据库过载保护

---

## 安全架构

HeartLake 将安全视为第一优先级，构建了多层纵深防御体系。

### 认证与授权

```
┌─────────────────────────────────────────────┐
│              PASETO v4 Token                 │
│                                             │
│  Header:  v4.local                          │
│  Payload: { uid, role, exp, iat, jti }      │
│  加密:    XChaCha20-Poly1305                │
│  优势:    无算法混淆 · 强制加密 · 无None攻击 │
└─────────────────────────────────────────────┘
```

- PASETO v4 替代 JWT，从协议层消除算法混淆攻击
- 基于角色的访问控制（RBAC）：普通用户 / VIP / 管理员
- AdminAuthFilter 独立管理员认证链路
- Token 自动续期 + Redis 黑名单吊销

### 请求安全链

每个请求经过多层过滤器链，层层防护：

```
Request → AuthFilter（身份认证）
        → RateLimitFilter（速率限制，令牌桶算法）
        → SecurityAuditFilter（安全审计日志）
        → TraceFilter（分布式追踪）
        → Controller（业务处理）
```

### 数据安全

| 层级 | 措施 | 说明 |
|------|------|------|
| 传输层 | TLS 1.3 + Nginx SSL终结 | 全链路HTTPS加密 |
| 会话层 | E2E端到端加密 | 咨询会话内容服务端不可读 |
| 存储层 | PostgreSQL加密 + 字段级加密 | 敏感字段独立加密 |
| 统计层 | 差分隐私 | 聚合查询注入噪声 |
| 应用层 | AC自动机 + 图片审核 | 内容安全实时过滤 |

### 隐私保护特色

- 匿名机制：用户无需实名，零知识身份验证
- 石友TTL：好友关系自动过期，不留永久社交痕迹
- 数据导出：GDPR合规，用户可导出/删除全部个人数据
- 安全港（Safe Harbor）：检测到心理危机自动触发干预流程
- 湖神守护：AI驱动的社区守护者，7x24小时内容安全巡检

---

## 💡 核心功能

### 🪨 石头系统（匿名帖子）

用户将心事写成「心石」投入心湖，石头分为三种重量：

| 重量 | 类型 | 说明 |
|------|------|------|
| Light | 轻石 | 轻松日常、随想碎片，漂浮在湖面，曝光度最高 |
| Medium | 中石 | 情感倾诉、生活感悟，沉入中层，适度曝光 |
| Heavy | 重石 | 深层心事、沉重情感，沉入湖底，仅同频者可见 |

- 石头支持文字 + 图片（图片经 Edge AI 端侧审核）
- 石头生命周期：投入 → 漂浮/沉入 → 被捡起（互动）→ 自然消融（TTL过期）
- 匿名保护：石头与用户身份完全解耦，服务端无法反向追踪

### 🌊 涟漪系统（互动回应）

涟漪是对石头的轻量级互动回应，类似「匿名评论」但更温暖：

- 文字涟漪：简短的文字回应（限280字）
- 情绪涟漪：预设情绪表情回应（共鸣、拥抱、加油、心疼等）
- 涟漪扩散：高质量涟漪会扩散，让更多人看到原石头
- 涟漪计数影响石头的「温度值」，温度越高曝光越广

### 🚢 纸船系统（私信 / 深度回应）

纸船是用户之间的深度私密沟通通道：

- 折纸船：对某颗石头发起深度回应，生成一只纸船漂向石头主人
- E2E加密：纸船内容端到端加密，服务端不可读
- 纸船对话：双方可在纸船内持续对话，形成临时匿名聊天
- 纸船有效期：默认72小时，双方可协商延长
- 纸船转石友：双方互相认可后可升级为「石友」关系

### 🔮 湖神守护

湖神是 HeartLake 的 AI 守护者，7x24小时守护社区：

- 零互动关怀：自动检测长时间零互动的石头，派送暖心纸船
- 内容安全巡检：AC自动机 + Edge AI 双重内容审核
- 社区氛围维护：检测并干预负面情绪蔓延
- 新用户引导：为新用户的第一颗石头送上欢迎纸船
- 节日关怀：特殊节日自动发送温暖祝福

### 📈 情绪追踪

基于 Edge AI 的端侧情绪分析系统：

- 72小时负面情绪监测：连续检测到负面情绪自动触发关怀干预
- 情绪趋势图：用户可查看自己的情绪变化趋势（数据仅存端侧）
- 差分隐私聚合：平台仅获取加噪后的社区情绪大盘数据
- 安全港机制：检测到严重心理危机自动触发专业干预流程

### 🤝 石友关系TTL

HeartLake 独创的「有时效性的友谊」机制：

- 石友关系默认TTL为30天
- 双方持续互动（发纸船、涟漪）可续期
- 超过TTL未互动，关系自动解除，不留社交痕迹
- 设计哲学：珍惜当下的连接，减轻社交负担

### 🔍 同频共鸣搜索

基于向量嵌入的语义相似度匹配系统：

- Edge AI 端侧生成文本向量嵌入
- pgvector 存储匿名化向量（与用户身份解耦）
- 余弦相似度匹配，找到情感共鸣的石头
- 联邦学习优化推荐模型，数据不出设备

### 🛡️ 守护者激励

鼓励用户成为社区守护者的激励体系：

- 守护者等级：见习守护者 → 守护者 → 高级守护者 → 湖之守护
- 守护行为：回应零互动石头、发送暖心涟漪、举报违规内容
- 守护者勋章与专属标识
- 守护者排行榜（匿名展示）

### 🧠 心理风险评估

AI驱动的心理健康风险评估系统：

- 多维度风险评分：情绪强度、负面持续时间、关键词检测、行为模式
- 三级预警：关注（黄色）→ 警告（橙色）→ 危机（红色）
- 危机干预：红色预警自动推送专业心理援助资源
- 隐私优先：风险评估在端侧完成，服务端仅接收脱敏预警信号

### 👑 VIP系统

为深度用户提供增值服务：

- VIP特权：更大石头容量、更长纸船有效期、专属涟漪样式
- VIP等级：银湖 → 金湖 → 星湖
- VIP专属功能：石头置顶、涟漪高亮、纸船加急
- 订阅制付费，支持月付/年付

---

## 🚀 快速开始

### 环境要求

| 依赖 | 最低版本 | 推荐版本 |
|------|---------|---------|
| Docker | 24.0+ | 25.0+ |
| Docker Compose | 2.20+ | 2.24+ |
| Git | 2.40+ | 2.43+ |
| Node.js（前端开发） | 18.0+ | 20.11+ |
| Flutter（移动端开发） | 3.19+ | 3.22+ |
| CMake（后端开发） | 3.22+ | 3.28+ |
| GCC/Clang（后端开发） | GCC 13+ / Clang 17+ | GCC 14 / Clang 18 |

### 一键启动

```bash
# 克隆仓库
git clone https://github.com/heartlake/heartlake.git
cd heartlake

# 复制环境配置
cp backend/.env.example backend/.env

# 编辑 .env 配置数据库、Redis、PASETO密钥等
# vim backend/.env

# 一键启动所有服务
docker compose up -d
```

### 配置说明

`backend/.env` 关键配置项：

```env
# 数据库
DB_HOST=postgres
DB_PORT=5432
DB_NAME=heartlake
DB_USER=heartlake
DB_PASSWORD=<your-secure-password>

# Redis
REDIS_HOST=redis
REDIS_PORT=6379

# PASETO认证
PASETO_SECRET_KEY=<your-paseto-secret-key>
TOKEN_EXPIRE_HOURS=24

# Edge AI
EDGE_AI_MODEL_PATH=/models
EDGE_AI_THREADS=4

# 差分隐私
DP_EPSILON=1.0
DP_DELTA=1e-5

# 内容安全
CONTENT_FILTER_ENABLED=true
IMAGE_AUDIT_ENABLED=true
```

### 服务端口

| 服务 | 端口 | 说明 |
|------|------|------|
| Backend API | 8080 | Drogon C++20 后端 |
| Admin Frontend | 3000 | Vue3 管理后台 |
| PostgreSQL | 5432 | 主数据库 |
| Redis | 6379 | 缓存 & 会话 |
| Nginx | 80/443 | 反向代理 & SSL终结 |

### 开发模式

```bash
# 后端开发（需要本地C++20编译环境）
cd backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
./heartlake_server

# 前端管理后台开发
cd admin-frontend
npm install
npm run dev

# 移动端开发
cd mobile
flutter pub get
flutter run
```

---

## 📡 API概览

### 认证模块

| 方法 | 端点 | 说明 |
|------|------|------|
| POST | `/api/v1/auth/register` | 匿名注册 |
| POST | `/api/v1/auth/login` | 登录获取PASETO Token |
| POST | `/api/v1/auth/refresh` | Token续期 |
| POST | `/api/v1/auth/logout` | 登出（Token加入黑名单） |

### 石头模块

| 方法 | 端点 | 说明 |
|------|------|------|
| POST | `/api/v1/stones` | 投入新石头 |
| GET | `/api/v1/stones` | 获取石头列表（分页） |
| GET | `/api/v1/stones/:id` | 获取石头详情 |
| DELETE | `/api/v1/stones/:id` | 删除自己的石头 |
| GET | `/api/v1/stones/resonance` | 同频共鸣搜索 |
| GET | `/api/v1/stones/trending` | 热门石头（温度排序） |

### 涟漪模块

| 方法 | 端点 | 说明 |
|------|------|------|
| POST | `/api/v1/stones/:id/ripples` | 对石头发送涟漪 |
| GET | `/api/v1/stones/:id/ripples` | 获取石头的涟漪列表 |
| DELETE | `/api/v1/ripples/:id` | 删除自己的涟漪 |

### 纸船模块

| 方法 | 端点 | 说明 |
|------|------|------|
| POST | `/api/v1/stones/:id/boats` | 向石头主人发送纸船 |
| GET | `/api/v1/boats` | 获取我的纸船列表 |
| GET | `/api/v1/boats/:id/messages` | 获取纸船对话消息 |
| POST | `/api/v1/boats/:id/messages` | 在纸船内发送消息 |
| PUT | `/api/v1/boats/:id/extend` | 延长纸船有效期 |

### 好友模块

| 方法 | 端点 | 说明 |
|------|------|------|
| POST | `/api/v1/friends/request` | 发送石友请求 |
| PUT | `/api/v1/friends/:id/accept` | 接受石友请求 |
| GET | `/api/v1/friends` | 获取石友列表 |
| DELETE | `/api/v1/friends/:id` | 解除石友关系 |
| GET | `/api/v1/friends/:id/ttl` | 查看石友关系剩余TTL |

### 边缘AI模块

| 方法 | 端点 | 说明 |
|------|------|------|
| POST | `/api/v1/edge-ai/sentiment` | 情感分析（端侧） |
| POST | `/api/v1/edge-ai/embedding` | 文本向量嵌入（端侧） |
| POST | `/api/v1/edge-ai/risk-assess` | 心理风险评估（端侧） |
| GET | `/api/v1/edge-ai/model/status` | 模型状态查询 |
| POST | `/api/v1/edge-ai/federated/upload` | 联邦学习梯度上传 |

### 管理模块

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/api/v1/admin/dashboard` | 管理仪表盘数据 |
| GET | `/api/v1/admin/users` | 用户管理列表 |
| PUT | `/api/v1/admin/users/:id/ban` | 封禁用户 |
| GET | `/api/v1/admin/reports` | 举报列表 |
| PUT | `/api/v1/admin/reports/:id` | 处理举报 |
| GET | `/api/v1/admin/stats` | 平台统计（差分隐私） |

---

## 📁 项目结构

```
heartlake/
├── backend/                          # C++20 Drogon 后端
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── main.cc                   # 入口
│   │   ├── domain/                   # 领域层（DDD核心）
│   │   │   ├── stone/                # 石头聚合根
│   │   │   ├── ripple/               # 涟漪实体
│   │   │   ├── boat/                 # 纸船聚合根
│   │   │   ├── friend/               # 石友关系
│   │   │   └── user/                 # 用户聚合根
│   │   ├── application/              # 应用层（用例编排）
│   │   │   ├── stone_service.h
│   │   │   ├── ripple_service.h
│   │   │   ├── boat_service.h
│   │   │   └── friend_service.h
│   │   ├── infrastructure/           # 基础设施层
│   │   │   ├── persistence/          # 数据库仓储实现
│   │   │   ├── cache/                # Redis缓存
│   │   │   ├── messaging/            # 事件总线
│   │   │   └── security/             # 安全组件
│   │   ├── interfaces/               # 接口层（Controller）
│   │   │   ├── rest/                 # REST API控制器
│   │   │   ├── websocket/            # WebSocket处理器
│   │   │   └── filters/              # 过滤器链
│   │   └── edge_ai/                  # 边缘AI引擎
│   │       ├── sentiment/            # 情感分析
│   │       ├── embedding/            # 向量嵌入
│   │       ├── risk/                 # 风险评估
│   │       ├── content_filter/       # 内容过滤
│   │       ├── federated/            # 联邦学习
│   │       └── privacy/              # 差分隐私
│   ├── tests/                        # 单元测试 & 集成测试
│   ├── migrations/                   # 数据库迁移
│   └── .env.example                  # 环境变量模板
├── admin-frontend/                   # Vue3 管理后台
│   ├── src/
│   │   ├── views/                    # 页面组件
│   │   ├── components/               # 通用组件
│   │   ├── stores/                   # Pinia状态管理
│   │   ├── api/                      # API请求封装
│   │   └── router/                   # 路由配置
│   └── package.json
├── mobile/                           # Flutter 移动端
│   ├── lib/
│   │   ├── domain/                   # 领域模型
│   │   ├── data/                     # 数据层
│   │   ├── presentation/             # UI层
│   │   ├── edge_ai/                  # 端侧AI推理
│   │   └── core/                     # 核心工具
│   └── pubspec.yaml
├── docker-compose.yml                # Docker编排
├── nginx/                            # Nginx配置
│   └── nginx.conf
├── docs/                             # 项目文档
│   ├── api/                          # API详细文档
│   ├── architecture/                 # 架构设计文档
│   └── assets/                       # 静态资源
└── README.md                         # 本文件
```

---

## 📊 技术栈

### 后端

| 技术 | 版本 | 用途 |
|------|------|------|
| C++ | 20 | 主开发语言 |
| Drogon | 1.9+ | 高性能Web框架 |
| PostgreSQL | 16+ | 主数据库 |
| pgvector | 0.6+ | 向量相似度搜索 |
| Redis | 7.2+ | 缓存 / 会话 / 速率限制 |
| PASETO | v4 | 安全令牌认证 |
| libsodium | 1.0.19+ | 加密库（E2E加密） |
| nlohmann/json | 3.11+ | JSON处理 |
| spdlog | 1.13+ | 高性能日志 |

### 前端（管理后台）

| 技术 | 版本 | 用途 |
|------|------|------|
| Vue.js | 3.4+ | UI框架 |
| TypeScript | 5.3+ | 类型安全 |
| Vite | 5.0+ | 构建工具 |
| Pinia | 2.1+ | 状态管理 |
| Element Plus | 2.5+ | UI组件库 |
| ECharts | 5.5+ | 数据可视化 |

### 移动端

| 技术 | 版本 | 用途 |
|------|------|------|
| Flutter | 3.19+ | 跨平台框架 |
| Dart | 3.3+ | 开发语言 |
| Riverpod | 2.5+ | 状态管理 |
| TensorFlow Lite | 2.15+ | 端侧AI推理 |
| Hive | 2.2+ | 本地存储 |

### 数据库 & 缓存

| 技术 | 版本 | 用途 |
|------|------|------|
| PostgreSQL | 16+ | 关系型主数据库 |
| pgvector | 0.6+ | 向量索引与相似度搜索 |
| Redis | 7.2+ | 缓存 / 消息队列 / 速率限制 |

### 基础设施

| 技术 | 用途 |
|------|------|
| Docker Compose | 容器编排 |
| Nginx | 反向代理 / SSL终结 / 负载均衡 |
| GitHub Actions | CI/CD |
| Prometheus + Grafana | 监控 & 告警 |

### AI & 隐私

| 技术 | 用途 |
|------|------|
| TensorFlow Lite | 端侧模型推理 |
| ONNX Runtime | 模型格式兼容 |
| 差分隐私（DP） | 数据聚合隐私保护 |
| 联邦学习（FL） | 分布式模型训练 |
| AC自动机 | 高性能敏感词过滤 |

---

## 👥 团队

HeartLake 由一群热爱技术、关注心理健康的开发者共同打造。

我们相信技术可以让世界更温暖，每一颗投入心湖的石头都值得被温柔以待。

欢迎加入我们，一起守护心湖。

---

## 📄 许可证

本项目采用 [AGPL-3.0](LICENSE) 许可证开源。

```
Copyright (C) 2024-2026 HeartLake Contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
```

---

<p align="center">
  <sub>用技术守护每一颗心 | Built with ❤️ by HeartLake Team</sub>
</p>

