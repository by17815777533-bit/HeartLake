# HeartLake — 心湖 · 把心事投进湖里

> 基于 C++20 高性能后端与端侧 AI 隐私计算的匿名情绪表达与心理关怀平台

## 项目概述

HeartLake 是一个面向青年群体的匿名情绪社区平台。用户以匿名身份发布情绪内容（"石头"），其他用户通过轻量互动（"涟漪"）表达共鸣，或通过匿名消息（"纸船"）建立情感连接。系统内置端侧 AI 引擎，实时进行情感分析与心理风险评估，当检测到高风险状态时自动触发安全港（SafeHarbor）危机干预机制。

全系统在匿名与隐私保护的约束下运行，采用差分隐私、联邦学习、端到端加密等技术保障用户数据安全。

## 核心功能

| 功能模块 | 说明 |
|----------|------|
| 投石（Stone） | 匿名发布情绪内容，支持情绪标签、情绪日历、热力图统计 |
| 涟漪（Ripple） | 轻量级互动机制，幂等操作，触发亲密度计算 |
| 纸船（PaperBoat） | 匿名消息传递，建立一对一情感连接 |
| 好友系统 | 基于亲密度门槛的好友关系，支持临时连接与可恢复删除 |
| 情绪共鸣推荐 | 四维评分算法（语义相似度 + 情绪轨迹 DTW + 时间衰减 + 多样性），五策略融合推荐引擎 |
| 心理风险评估 | 五因子模型（自伤意图 / 绝望感 / 时间紧迫性 / 语言风险 / 社交孤立），五级风险等级 |
| SafeHarbor 安全港 | CRITICAL 级别自动触发，提供 24h 心理热线、自助工具、个性化干预资源 |
| Guardian 守望者 | 守望激励、灯火转赠、情绪洞察与湖神陪伴对话 |
| 灯火计划 | 以 `/api/vip/*` 提供状态、权益、咨询额度与 AI 频率配置 |
| 管理后台 | 运营看板、用户管理、内容审核、敏感词管理、系统配置，共 12 个功能模块 |

## 技术栈

| 层级 | 技术选型 | 说明 |
|------|----------|------|
| 后端服务 | C++20 + Drogon | 异步 HTTP/WebSocket 服务，DDD 四层架构（Interfaces / Application / Domain / Infrastructure） |
| 移动客户端 | Flutter 3.x（Dart） | 跨平台移动应用，30+ 页面，Provider 状态管理，GoRouter 路由 |
| 管理后台 | Vue 3 + Element Plus + ECharts | SPA 管理面板，Pinia 状态管理，TypeScript |
| 数据库 | PostgreSQL 16 | 主数据存储，18 个版本化迁移脚本，含回滚支持 |
| 缓存 | Redis 7 | 进程内 LRU（L1）+ Redis（L2）两级缓存架构 |
| AI 推理 | Ollama + EdgeAI 引擎 | 本地 LLM 对话 + 8 子系统端侧 AI（情感分析、内容审核、HNSW 向量检索、差分隐私、联邦学习等） |
| 反向代理 | Nginx 1.27 | 统一网关，路由分发，WebSocket 代理 |
| 容器编排 | Docker Compose（5-6 容器） | 支持全量、本地精简和低配服务器精简部署 |
| CI/CD | GitHub Actions | 自动构建、安全扫描 |

## 快速启动

### 前置条件

- Docker Engine 20.10+
- Docker Compose v2+
- `all` 模式建议至少 4GB 可用内存（含 Ollama）
- `lite` / `server-lite` 模式 2GB 也可运行，但不包含 Ollama

### 启动步骤

```bash
# 1. 克隆仓库
git clone https://github.com/by17815777533-bit/HeartLake.git
cd HeartLake

# 2. 配置环境变量
cp .env.example .env
# 必填：数据库密码、Redis 密码、PASETO 密钥、管理员密码哈希

# 3. 启动服务
# 4GB+ 内存：./scripts/docker-up.sh all
# 2GB/低配机器：./scripts/docker-up.sh lite
./scripts/docker-up.sh lite

# 4. 验证服务状态
./scripts/docker-test.sh

# 5. 停止服务
./scripts/docker-down.sh
```

`docker-up.sh` 支持多种启动模式：

| 模式 | 命令 | 启动的服务 |
|------|------|-----------|
| 全量 | `./scripts/docker-up.sh all` | 全部 6 个容器 |
| 精简 | `./scripts/docker-up.sh lite` | 除 Ollama 外的全部服务（无 LLM 对话能力，EdgeAI 本地分析正常） |
| 低配服务器 | `./scripts/docker-up.sh server-lite` | 适配 2C2G Ubuntu，内部端口默认仅绑定本机 |
| 低配增量 admin | `./scripts/docker-up.sh server-lite-admin` | 仅重建管理后台，避免顺带重建 backend |
| 低配增量 backend | `./scripts/docker-up.sh server-lite-backend` | 仅重建后端，避免顺带重建 admin |
| 低配增量 gateway | `./scripts/docker-up.sh server-lite-gateway` | 仅重启网关 |
| 仅数据库 | `./scripts/docker-up.sh db` | PostgreSQL + Redis |

### 服务访问地址

| 服务 | 地址 | 说明 |
|------|------|------|
| 统一网关 | `http://localhost:3000` | Nginx 反向代理入口 |
| 后端 API | `http://localhost:8080` | Drogon REST API + WebSocket |
| 管理后台 | `http://localhost:5173` | Vue 3 管理面板 |
| PostgreSQL | `localhost:5432` | 数据库（账号密码见 `.env`） |
| Redis | `localhost:6379` | 缓存（认证密码见 `.env`） |
| Ollama | `localhost:11434` | LLM 推理服务 |

### 数据初始化

首次启动后，后端会自动执行数据库迁移。如需导入演示数据：

```bash
# 重置数据库并导入种子数据（会清空现有数据）
./scripts/reset_all_data_and_seed.sh

# 仅创建表结构，不导入数据
./scripts/reset_all_data_and_seed.sh --schema-only

# 创建具有完整权限的演示账号（VIP + Guardian）
./scripts/create_full_access_account.sh
```

## 项目结构

```
heartlake/
├── backend/                        # C++20 后端服务
│   ├── include/                    # 头文件
│   │   ├── domain/                 # 领域层：Stone / User / Friend 聚合根，仓储接口
│   │   ├── application/            # 应用层：ApplicationServiceFactory，EventHandlers
│   │   ├── infrastructure/         # 基础设施层
│   │   │   ├── ai/                 # EdgeAI 引擎（8 个子系统）
│   │   │   ├── cache/              # 两级缓存（LRU + Redis）
│   │   │   ├── events/             # EventBus 领域事件总线
│   │   │   ├── filters/            # PASETO 认证 / 令牌桶限流 / 安全审计
│   │   │   ├── services/           # VIP、SafeHarbor、Guardian 等基础设施服务
│   │   │   ├── privacy/            # 差分隐私引擎
│   │   │   ├── realtime/           # WebSocket 实时通信
│   │   │   └── di/                 # ServiceLocator 依赖注入
│   │   ├── interfaces/api/         # 21 个接口控制器（20 个 HTTP + 1 个 WebSocket）
│   │   └── utils/                  # 工具类（加密、校验、ID 生成等）
│   ├── src/                        # 源文件（与 include/ 目录结构对应）
│   ├── quic_gateway/               # Rust QUIC 网关（Quinn + Tokio + ChaCha20Poly1305）
│   ├── migrations/                 # 数据库迁移脚本（18 个 up + 18 个 rollback）
│   ├── Dockerfile                  # 多阶段构建（Ubuntu 24.04 → 最小运行时镜像）
│   └── CMakeLists.txt              # CMake 构建配置
├── frontend/                       # Flutter 移动客户端
│   ├── lib/
│   │   ├── data/datasources/       # API 客户端（Dio）、WebSocket、交互服务
│   │   ├── domain/                 # 领域实体 + 端侧 AI（EmotionClassifier、LocalDP）
│   │   ├── presentation/
│   │   │   ├── screens/            # 30+ 页面
│   │   │   ├── providers/          # Provider 状态管理
│   │   │   └── widgets/            # 可复用组件
│   │   ├── router/                 # GoRouter 路由配置
│   │   └── utils/                  # 工具类
├── admin/                          # Vue 3 管理后台
│   ├── src/
│   │   ├── views/                  # 管理页面 + Dashboard
│   │   ├── composables/            # 组合函数（useChartOptions、useDashboardData 等）
│   │   ├── stores/                 # Pinia 状态管理
│   │   └── services/               # WebSocket 服务
│   └── Dockerfile                  # 多阶段构建（Node 20 → Nginx 1.27）
├── deploy/nginx/                   # Nginx 网关配置（gateway.conf）
├── papers/                         # 参考论文（差分隐私、联邦学习、向量检索等）
├── docs/                           # 技术文档（9 份）
├── scripts/                        # 运维脚本（启动、停止、校验、数据初始化）
├── docker-compose.yml              # 6 容器服务编排
├── .env.example                    # 环境变量模板
├── CONTRIBUTING.md                 # 贡献指南
└── README.md                       # 本文件
```

## 系统架构

### DDD 四层架构

后端严格遵循领域驱动设计分层，`ArchitectureBootstrap` 按依赖顺序初始化：

```
┌─────────────────────────────────────────────────────────────┐
│  Interfaces 层 — 21 个接口控制器（20 个 HTTP + 1 个 WebSocket） │
│  路由注册 · 请求校验 · PASETO v4 认证 · 响应序列化            │
├─────────────────────────────────────────────────────────────┤
│  Application 层 — ApplicationServiceFactory + EventHandlers │
│  编排跨聚合业务流程，发布/订阅领域事件                         │
├─────────────────────────────────────────────────────────────┤
│  Domain 层 — Stone · User · Friend 聚合根                    │
│  仓储接口（IStoneRepository / IUserRepository / IFriend...） │
├─────────────────────────────────────────────────────────────┤
│  Infrastructure 层                                          │
│  EdgeAI 引擎 · 两级缓存 · EventBus · 数据库仓储实现           │
└─────────────────────────────────────────────────────────────┘
```

### 事件驱动流程

| 领域事件 | 触发时机 | 处理器 | 后续动作 |
|----------|----------|--------|----------|
| `StonePublishedEvent` | 石头发布 | `StonePublishedHandler` | AI 情感分析 → 情绪追踪 → 风险评估 |
| `EmotionAnalyzedEvent` | 情感分析完成 | `EmotionAnalyzedHandler` | 缓存失效 → 推荐引擎刷新 |
| `RippleCreatedEvent` | 涟漪互动 | `RippleCreatedHandler` | 通知推送 → 亲密度计算 |
| `BoatSentEvent` | 纸船发送 | `BoatSentHandler` | 匿名消息投递 |

### 部署拓扑

```
                    ┌──────────────┐
                    │  Nginx 网关   │ :3000
                    └──────┬───────┘
                           │
              ┌────────────┼────────────┐
              │            │            │
     ┌────────▼───┐  ┌─────▼─────┐  ┌──▼──────────┐
     │  Admin 前端 │  │  Backend  │  │  Flutter App │
     │  (Vue 3)   │  │  (Drogon) │  │  (移动端)    │
     │  :5173     │  │  :8080    │  │              │
     └────────────┘  └─────┬─────┘  └──────────────┘
                           │
              ┌────────────┼────────────┐
              │            │            │
     ┌────────▼───┐  ┌─────▼─────┐  ┌──▼──────────┐
     │ PostgreSQL  │  │   Redis   │  │   Ollama    │
     │  :5432     │  │  :6379    │  │  :11434     │
     └────────────┘  └───────────┘  └─────────────┘
```

## EdgeAI 引擎

`EdgeAIEngine` 采用门面模式统一管理 8 个子系统，单例初始化（`std::once_flag`）保证线程安全：

| 子系统 | 功能 | 核心技术 |
|--------|------|----------|
| SentimentAnalyzer | 情感分析 | 规则 + 词典 + 统计三层融合，LRU 缓存 + 飞行请求去重，可选 ONNX Runtime |
| ContentModerator | 内容审核 + 心理风险评估 | Aho-Corasick O(n) 多模式匹配 + 五因子风险模型 |
| HNSWIndex | 近似最近邻向量检索 | Ada-EF 自适应搜索宽度 + Matryoshka 重排 + RND 多样性剪枝 + ADSampling 距离截断 |
| EmotionPulseDetector | 实时情绪统计 | 滑动窗口 + 时间衰减 + EWMA 平滑 + MAD 离群检测 |
| FederatedLearner | 联邦学习 | FedAvg 加权聚合 + L2 梯度裁剪 + DP 噪声 + FedProx 近端正则 |
| EdgeDifferentialPrivacy | 差分隐私 | Laplace + Gaussian 双机制，zCDP 隐私预算追踪 |
| ModelQuantizer | 模型量化推理 | float32 → INT8 对称量化，4 路展开矩阵乘法 |
| EdgeNodeMonitor | 边缘节点监控 | 节点注册 / 心跳检测 / 负载均衡 |

### 情绪共鸣推荐算法

四维评分公式：

```
ResonanceScore = 0.30·SemanticSim + 0.35·TrajectoryDTW + 0.20·TemporalDecay + 0.15·DiversityBonus
```

- 语义相似度（0.30）：128 维文本向量（TF-IDF + 情感特征 + 统计特征 + N-gram），余弦相似度
- 情绪轨迹匹配（0.35）：DTW 动态时间规整 + LB_Improved 双向包络剪枝 + Early Abandoning 提前终止
- 时间衰减（0.20）：`exp(-λ·Δt)` 指数衰减，抑制过旧内容
- 多样性奖励（0.15）：防止回音室效应
- 权重通过 EMA 在线学习自适应调整，`atomic<shared_ptr>` 无锁读取

五策略推荐引擎：User-CF / Item-CF / 内容匹配 / UCB 探索 / 图传播，MMR 多样性重排序。

## 安全体系

### 认证与加密

- PASETO v4 令牌认证（替代 JWT，杜绝算法混淆攻击），管理后台使用独立签名密钥
- E2E 端到端加密：X25519 密钥交换 + HKDF-SHA256 密钥派生 + AES-256-GCM 认证加密
- 前向保密：临时密钥对，每次会话独立

### 内容安全

三级过滤架构：CountingBloomFilter 预检 → ACAutomaton O(n) 精确匹配 → ShardedLRUCache 结果缓存。支持运行时热更新敏感词库，白名单机制降低误报率。

### 隐私保护

- 差分隐私：Laplace 机制（低维标量）+ Gaussian 机制（高维向量，O(√d) 噪声），zCDP 预算追踪
- 联邦学习：本地训练 + 安全聚合，L2 梯度裁剪防止模型反演攻击
- 身份隔离：`IdentityShadowMap` 将物理用户 ID 映射为匿名 shadow_id，AI 子系统仅接触匿名标识

### 基础设施防护

- 熔断器：CLOSED → OPEN → HALF_OPEN 三态状态机，两阶段锁策略避免长时间持锁
- 令牌桶限流：`RateLimitFilter` 按 IP / 用户维度限流
- 全链路安全审计日志：`SecurityAuditFilter` 记录所有认证请求

## 心理关怀机制

### 风险评估模型

`PsychologicalRiskAssessment` 融合语言学分析与行为模式分析两个维度：

| 评估因子 | 权重 | 检测内容 |
|----------|------|----------|
| 自伤意图 | 0.9 | 自伤相关关键词与表达模式 |
| 绝望感 | 0.6 | 绝望、无助类语言表达 |
| 时间紧迫性 | 0.5 | "最后一次"等紧迫性表达 |
| 语言风险标记 | 0.3 | 高风险语言模式匹配 |
| 社交孤立 | 0.1 | 孤立感相关表达 |

五级风险等级：NONE → LOW → MEDIUM → HIGH → CRITICAL

CRITICAL 级别自动触发 SafeHarbor 安全港机制，提供 24 小时心理援助热线、自助工具和个性化干预资源。

### Guardian 守望者系统

当前守望者系统不再走“绑定守护人”模式，而是围绕社区互助做三件事：统计共鸣贡献、在达标后开放灯火转赠、为用户提供情绪洞察和湖神陪伴对话。核心能力由 `GuardianIncentiveService` 和 `DualMemoryRAG` 驱动。

## API 概览

后端提供 21 个接口控制器（20 个 HTTP + 1 个 WebSocket），完整接口文档参见 [API 接口全量清单](docs/05_API接口全量清单.md)。

| Controller | 路由前缀 | 功能 |
|------------|----------|------|
| HealthController | `/api/health` | 服务健康检查 |
| UserController | `/api/auth` | 匿名登录、令牌刷新、账号恢复 |
| AccountController | `/api/account` | 用户资料、设备管理、隐私设置、GDPR 数据导出、账号注销 |
| MediaController | `/api/media` | 头像 / 图片 / 音视频上传与公开访问 |
| StoneController | `/api/stones` | 石头 CRUD、情绪日历、情绪热力图 |
| InteractionController | `/api/interactions` | 涟漪（幂等）、收藏 |
| PaperBoatController | `/api/boats` | 纸船匿名消息 |
| FriendController | `/api/friends` | 好友管理、消息（亲密度门槛）、可恢复删除 |
| TempFriendController | `/api/temp-friends` | 临时连接 |
| RecommendationController | `/api/recommendations` | 个性化推荐、热门内容、趋势 |
| VectorSearchController | `/api/vector` | 语义向量搜索 |
| EdgeAIController | `/api/edge-ai` | 情感分析、情绪脉搏、隐私预算查询、联邦学习状态 |
| SafeHarborController | `/api/safe-harbor` | 安全港资源、心理热线 |
| GuardianController | `/api/guardian` | 守望统计、灯火转赠、情绪洞察、AI 陪伴对话 |
| ConsultationController | `/api/consultation` | 心理咨询 |
| ReportController | `/api/reports` | 内容举报 |
| PrivacyController | `/api/privacy` | 隐私设置管理 |
| VIPController | `/api/vip` | 灯火计划权益、咨询额度与 AI 频率配置 |
| AdminController | `/api/admin` | 管理后台数据接口 |
| AdminManagementController | `/api/admin/manage` | 用户管理、内容审核、敏感词管理、系统配置 |
| BroadcastWebSocketController | `/ws/broadcast` | WebSocket 实时通信 |

## 校验

当前仓库不再保留三端单测目录，验收以构建和联调冒烟为主。

| 模块 | 校验方式 | 运行命令 |
|------|----------|----------|
| 后端 + 管理端 + 移动端 | 2C2G 资源标准构建校验 | `./scripts/verify-2c2g.sh` |
| Docker 联调冒烟 | 网关 + 后端 + 管理端联调 | `./scripts/docker-test.sh` |

## 环境变量

复制 `.env.example` 为 `.env` 后按需修改：

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `DB_NAME` | `heartlake` | PostgreSQL 数据库名 |
| `DB_USER` | `postgres` | PostgreSQL 用户名 |
| `DB_PASSWORD` | 必填 | PostgreSQL 密码 |
| `REDIS_PASSWORD` | 必填 | Redis 认证密码 |
| `PASETO_KEY` | 必填 | PASETO v4 用户端签名密钥（至少 32 字节） |
| `ADMIN_PASETO_KEY` | 必填 | PASETO v4 管理端签名密钥（至少 32 字节） |
| `ADMIN_PASSWORD_HASH` | 必填 | 管理员密码 PBKDF2 哈希（`salt:hash`） |
| `AI_PROVIDER` | `ollama` | AI 推理提供者 |
| `AI_MODEL` | `heartlake-qwen` | Ollama 模型名称 |
| `AI_TIMEOUT` | `30` | AI 推理超时时间（秒） |
| `GATEWAY_PORT` | `3000` | Nginx 网关端口 |
| `BACKEND_PORT` | `8080` | 后端服务端口 |
| `ADMIN_PORT` | `5173` | 管理后台端口 |
| `POSTGRES_PORT` | `5432` | PostgreSQL 端口 |
| `REDIS_PORT` | `6379` | Redis 端口 |
| `OLLAMA_PORT` | `11434` | Ollama 端口 |

低配 Ubuntu 服务器请优先参考 [docs/deploy-ubuntu-2c2g.md](./docs/deploy-ubuntu-2c2g.md)。

## 理论基础

### 隐私计算

- 联邦学习端侧情感建模：FedMultiEmo、FED-PsyAU、FedProx 近端正则
- 最优高斯差分隐私：高维 O(√d) 噪声 + zCDP 预算追踪

### 向量检索

- HNSW 增强：Ada-EF 自适应搜索宽度、Hub Highway 长尾路由优化、Vamana 多样性剪枝
- ADSampling 距离截断：部分和超过阈值时提前终止
- DTW 早停：Early Abandoning + LB_Improved 双向包络下界
- 特征哈希降维：签名哈希 + IDF 增量学习

### Edge AI

- INT8 对称量化：float32 → INT8 推理加速
- 双记忆 RAG：短期上下文 + Ebbinghaus 衰减长期画像
- 语义缓存：SHA256 精确匹配 + HNSW 语义匹配（相似度阈值 0.92）

> 完整参考文献列表见 [参考文献](docs/08_参考文献.md)

## 文档索引

| 文档 | 内容 |
|------|------|
| [新手入门指南](docs/00_新手入门指南.md) | 从零开始：环境准备、启动服务、验证运行、常见问题 |
| [本地启动与运行手册](docs/01_本地启动与运行手册.md) | 环境要求、Docker / 本地启动、环境变量配置 |
| [API 与实时链路手册](docs/02_API与实时链路手册.md) | 认证机制、API 分组、WebSocket 协议、缓存策略 |
| [端到端测试与故障排查](docs/03_端到端测试与故障排查手册.md) | 联调冒烟入口、关键链路验收、故障排查流程 |
| [技术实现全景手册](docs/04_技术实现全景手册.md) | 架构设计、算法细节、工程决策 |
| [API 接口全量清单](docs/05_API接口全量清单.md) | 全部端点的请求 / 响应规格 |
| [校验与压测手册](docs/06_测试验证与压测手册.md) | 2C2G 构建校验、Docker 冒烟、ONNX 烟雾验证 |
| [编码与注释规范](docs/07_编码与注释规范.md) | Doxygen 注释标准、各端编码规范 |
| [参考文献](docs/08_参考文献.md) | 论文与技术参考文献列表 |
| [贡献指南](CONTRIBUTING.md) | 开发环境搭建、Git 工作流、代码规范 |

## 许可证与联系

- GitHub: https://github.com/by17815777533-bit/HeartLake
- 维护者: 白洋 (jokerbai)
- 邮箱: by17815777533@gmail.com
