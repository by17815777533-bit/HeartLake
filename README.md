# HeartLake — 匿名情绪社交平台

HeartLake 是一个面向青年群体的匿名情绪表达与心理关怀平台。用户以"石头"为载体倾诉心声，通过"涟漪"产生共鸣，借"纸船"传递温暖。系统在保护隐私的前提下，融合端侧 AI 情感分析、心理风险评估和守护者机制，构建了一个安全、温暖的情绪互助社区。

## 技术栈

| 层级 | 技术选型 | 说明 |
|------|----------|------|
| 后端 | C++20 + Drogon | 高性能异步 HTTP/WebSocket 框架，DDD 分层架构 |
| 前端 | Flutter 3.x (Dart) | 跨端移动应用，30+ 页面 |
| 管理后台 | Vue 3 + Element Plus + ECharts | 运营治理看板，12 个功能模块 |
| 数据库 | PostgreSQL 16 | 主数据存储 |
| 缓存 | Redis 7 | 分布式缓存 + 会话管理 |
| AI 推理 | Ollama + 自研 EdgeAI 引擎 | 本地情感分析 + 远程大模型 |
| 部署 | Docker Compose + Nginx 网关 | 一键启动全部服务 |

## 快速启动

```bash
# 克隆仓库
git clone <repo-url> && cd heartlake

# 一键启动（Docker Compose 编排 6 个服务）
./scripts/docker-up.sh

# 验证服务状态
./scripts/docker-test.sh

# 关闭服务
./scripts/docker-down.sh
```

启动后的访问地址：
- 后端 API: `http://localhost:8080`
- 管理后台: `http://localhost:5173`
- 统一网关: `http://localhost:3000`

## 项目结构

```
heartlake/
├── backend/                 # C++20 后端（Drogon 框架）
│   ├── include/
│   │   ├── domain/          # 领域层：Stone / User / Friend 聚合根
│   │   ├── application/     # 应用层：服务工厂 + 事件处理器
│   │   ├── infrastructure/  # 基础设施层
│   │   │   ├── ai/          # EdgeAI 引擎（8 个子系统）
│   │   │   ├── cache/       # 两级缓存（进程内 LRU + Redis）
│   │   │   ├── events/      # 领域事件总线
│   │   │   ├── filters/     # 认证 / 限流 / 审计过滤器
│   │   │   ├── services/    # 基础设施服务（SafeHarbor / Guardian / VIP 等）
│   │   │   └── di/          # 依赖注入容器
│   │   ├── interfaces/api/  # 20 个 HTTP Controller
│   │   └── utils/           # 工具类（加密 / 过滤 / 熔断器等）
│   ├── src/                 # 实现文件
│   └── tests/               # GTest 单元测试 + 压力测试
├── frontend/                # Flutter 跨端应用
│   ├── lib/
│   │   ├── data/datasources/  # 21 个 API 服务客户端
│   │   ├── domain/entities/   # 领域实体
│   │   ├── edge_ai/           # 端侧 AI（情感分类 + 本地差分隐私）
│   │   └── presentation/      # 30 个页面 + Provider 状态管理
│   └── test/                  # Dart 单元测试
├── admin/                   # Vue 3 管理后台
│   ├── src/views/           # 12 个管理页面
│   ├── src/composables/     # 可组合函数
│   └── src/__tests__/       # Vitest 单元测试 + 压力测试
├── deploy/                  # Nginx 网关配置
├── docs/                    # 项目文档
├── scripts/                 # 运维脚本
└── docker-compose.yml       # 服务编排（6 个容器）
```

## 架构设计

### DDD 分层架构

后端严格遵循领域驱动设计的分层约束，通过 `ArchitectureBootstrap` 按 Infrastructure → Domain → Application → EventHandler 的顺序完成依赖注入：

```
┌─────────────────────────────────────────────┐
│  Interfaces 层 — 20 个 HttpController       │
│  路由注册 + 请求校验 + 响应序列化            │
├─────────────────────────────────────────────┤
│  Application 层 — 应用服务 + 事件处理器      │
│  编排领域逻辑，发布/订阅领域事件             │
├─────────────────────────────────────────────┤
│  Domain 层 — Stone / User / Friend 聚合根    │
│  仓储接口(I*Repository) + 领域服务           │
├─────────────────────────────────────────────┤
│  Infrastructure 层                           │
│  AI引擎 / 缓存 / 事件总线 / 数据库仓储实现   │
└─────────────────────────────────────────────┘
```

领域事件驱动的业务流转：
- `StonePublishedEvent` → AI 情感分析 → 情绪追踪 → 心理风险评估
- `EmotionAnalyzedEvent` → 缓存更新 → 推荐引擎刷新
- `RippleCreatedEvent` → 通知推送 → 亲密度计算
- `BoatSentEvent` → 匿名消息投递

### EdgeAI 引擎

`EdgeAIEngine` 采用门面模式统一管理 8 个子系统，单例 + `std::once_flag` 保证线程安全的一次性初始化：

| 子系统 | 类名 | 核心能力 |
|--------|------|----------|
| 情感分析 | `SentimentAnalyzer` | 三层融合（规则 + 词典 + 统计），LRU 缓存 + 飞行中请求去重，可选 ONNX 推理 |
| 内容审核 | `ContentModerator` | AC 自动机 O(n) 多模式匹配 + 五因子心理风险评估（自伤意图/绝望感/孤立感/紧迫性/语言标记） |
| 情绪脉搏 | `EmotionPulseDetector` | 滑动窗口统计，时间衰减加权 + EWMA 趋势 + MAD 离群点检测 |
| 联邦学习 | `FederatedLearner` | FedAvg 加权聚合，支持 L2 梯度裁剪 + DP 噪声注入 + FedProx 近端正则化 |
| 差分隐私 | `EdgeDifferentialPrivacy` | Laplace + Gaussian 双机制，zCDP 框架预算追踪（ε/δ/ρ 三计数器） |
| 向量检索 | `HNSWIndex` | 多层图 + Ada-EF 自适应搜索宽度 + Matryoshka 重排序 + RND 多样性邻居选择 |
| 模型量化 | `ModelQuantizer` | float32→INT8 对称量化，4 路展开矩阵乘法，量化前向推理 |
| 节点监控 | `EdgeNodeMonitor` | 边缘节点注册/心跳/负载均衡选择 |

### 情绪共鸣推荐

`EmotionResonanceEngine` 实现了四维共鸣评分算法：

```
ResonanceScore = 0.30·SemanticSim + 0.35·TrajectoryDTW + 0.20·TemporalDecay + 0.15·DiversityBonus
```

- 语义相似度：基于 `AdvancedEmbeddingEngine` 的 128 维文本向量（TF-IDF + 情感 + 统计 + N-gram 四层特征）
- 轨迹匹配：DTW 动态时间规整 + LB_Improved 双向包络剪枝 + Early Abandoning 提前终止
- 时间衰减：指数衰减 `exp(-λ·Δt)`
- 多样性奖励：避免回音室效应
- 权重通过 EMA 在线学习自适应调整，`atomic<shared_ptr>` 无锁一致读取

`RecommendationEngine` 融合五种推荐策略（User-CF / Item-CF / 内容推荐 / UCB 探索 / 图传播），通过 MMR 多样性重排序输出最终结果。

### 语义缓存

`SemanticCache` 实现两级缓存架构：
- L1 精确匹配：SHA256 哈希，O(1) 查找
- L2 语义匹配：复用 `HNSWIndex` 做 ANN 检索，O(log n)，相似度阈值 0.92
- LFU 淘汰 + TTL 过期清理

### 双记忆 RAG

`DualMemoryRAG` 基于 SoulSpeak 论文的双记忆架构：
- 短期记忆：最近 5 次交互上下文，基于相关性淘汰
- 长期记忆：用户情绪画像（均值/波动度/趋势/连续负面天数），Ebbinghaus 指数衰减加权
- 隐私保护：所有记忆使用 shadow_id，不关联真实身份

## 安全体系

### 认证与加密

- PASETO v4 令牌认证（替代 JWT，防止算法混淆攻击）
- E2E 端到端加密：X25519 ECDH 密钥交换 + HKDF-SHA256 派生 + AES-256-GCM 认证加密
- 前向保密：临时密钥对，每次会话独立

### 内容安全

`ContentFilter` 三级过滤架构：
1. `CountingBloomFilter` 预检测 — 快速排除安全文本
2. `ACAutomaton` 精确匹配 — O(n) Aho-Corasick 多模式匹配
3. `ShardedLRUCache` 结果缓存 — 热点文本直接命中

支持运行时热更新敏感词库，词条按类别（self_harm / violence / sexual / profanity）和级别分级，白名单机制减少误报。

### 隐私保护

- 差分隐私：`DifferentialPrivacyEngine` 提供 Laplace / Gaussian 双机制噪声注入
  - Laplace：标量和低维向量，组合定理均分 ε
  - Gaussian：高维场景 σ = Δ₂·√(2·ln(1.25/δ))/ε，噪声尺度 O(√d)
  - zCDP 框架追踪：ρ = Δ²/(2σ²) 线性可加，同时维护 ε/δ/ρ 三个原子计数器
- 联邦学习：本地训练 + 安全聚合，梯度裁剪防止模型反演

### 基础设施安全

- `CircuitBreaker` 熔断器：CLOSED → OPEN → HALF_OPEN 三态状态机，两阶段锁策略
- `SecurityAuditFilter`：全链路安全审计日志
- `AdminAuthFilter`：管理后台独立认证
- `RateLimiter`：请求速率限制

## 心理关怀体系

HeartLake 的核心差异化在于将心理关怀深度融入产品机制：

### SafeHarbor 安全港

当系统检测到用户可能处于心理危机状态时，自动触发安全港机制：
- 提供 24 小时心理援助热线
- 推送自助工具和温暖提示
- 根据情绪类型个性化推荐资源

### 心理风险评估

`PsychologicalRiskAssessment` 融合两个维度：
- 语言学分析：自伤意图（权重 0.9）、绝望感（0.6）、时间紧迫性（0.5）、语言标记（0.3）、社交孤立（0.1）
- 行为模式分析：发帖频率、参与度变化、活跃时段、连续负面天数
- 五级风险等级：NONE / LOW / MEDIUM / HIGH / CRITICAL

### Guardian 守护者

守护者系统允许信任的人关注用户情绪状态，在异常时收到告警通知。配合 `GuardianIncentiveService` 激励机制，鼓励社区互助。

## API 概览

后端提供 20 个 Controller，覆盖完整业务功能：

| Controller | 路由前缀 | 功能 |
|------------|----------|------|
| `HealthController` | `/api/health` | 健康检查 |
| `UserController` | `/api/users` | 注册 / 登录 / 令牌刷新 |
| `AccountController` | `/api/account` | 个人资料 / 设备管理 / 隐私设置 / 数据导出 / 账号注销 |
| `StoneController` | `/api/stones` | 石头 CRUD / 情感分析 / 情绪日历 |
| `InteractionController` | `/api/interactions` | 涟漪 / 收藏 / 举报 |
| `FriendController` | `/api/friends` | 好友请求 / 列表 / 删除 |
| `TempFriendController` | `/api/temp-friends` | 临时连接（限时好友） |
| `PaperBoatController` | `/api/boats` | 纸船匿名消息 |
| `RecommendationController` | `/api/recommendations` | 个性化推荐 |
| `VectorSearchController` | `/api/vector` | 语义搜索 |
| `SafeHarborController` | `/api/safe-harbor` | 安全港资源 / 热线 / 自助工具 |
| `GuardianController` | `/api/guardian` | 守护者关系管理 |
| `ConsultationController` | `/api/consultation` | 心理咨询 |
| `EdgeAIController` | `/api/edge-ai` | AI 引擎状态 / 情绪脉搏 / 联邦学习 |
| `ReportController` | `/api/reports` | 举报管理 |
| `PrivacyController` | `/api/privacy` | 隐私设置 |
| `VIPController` | `/api/vip` | 会员服务 |
| `AdminController` | `/api/admin` | 管理后台数据接口 |
| `AdminManagementController` | `/api/admin/manage` | 用户管理 / 内容审核 / 系统配置 |
| `BroadcastWebSocketController` | `/ws` | WebSocket 实时通信 |

所有需要认证的端点通过 `SecurityAuditFilter` 过滤器验证 PASETO 令牌。

## 管理后台

Vue 3 + Element Plus 构建的运营治理看板，12 个功能模块：

- Dashboard：实时数据卡片 + ECharts 图表（用户增长、情绪分布、内容趋势）
- Users：用户列表 / 搜索 / 详情 / 封禁管理
- Content：内容列表 / 筛选 / 审核操作
- Moderation：待审核队列 / 批量通过/拒绝
- EdgeAI：AI 引擎 8 子系统实时监控面板
- SensitiveWords：敏感词库 CRUD / 导入导出
- Reports：举报处理流程
- Logs：操作日志 / 时间范围筛选
- Settings：系统配置管理
- CareFeedback：关怀反馈统计

WebSocket 实时推送数据更新，composable 函数封装图表配置和分页逻辑。

## Flutter 前端

30 个页面覆盖完整用户旅程：

- 认证流程：闪屏 → 引导页 → 登录/注册
- 核心功能：首页信息流 → 湖面（石头瀑布流）→ 发布石头 → 石头详情 → 涟漪互动
- 情绪追踪：情绪日历 → 热力图 → 趋势分析
- 社交功能：好友列表 → 聊天 → 临时连接 → 纸船收发
- 心理关怀：安全港 → 帮助中心 → 心理咨询
- 个人中心：个人资料 → 隐私设置 → VIP → 我的石头/涟漪/纸船

端侧 AI 能力：
- `EmotionClassifier`：本地情感分类，无需网络请求
- `LocalDP`：本地差分隐私，数据出端前加噪保护

## 测试覆盖

| 模块 | 框架 | 测试文件数 | 覆盖范围 |
|------|------|-----------|----------|
| 后端 | Google Test | 57 | 单元测试 + AI 引擎 benchmark + 并发 torture test |
| 前端 | Dart test | 33 | 模型测试 + EdgeAI 测试 + 服务测试 |
| 管理后台 | Vitest | 25 | 组件测试 + API 测试 + 压力测试 |
| 端到端 | Playwright | - | 全页面全功能浏览器测试 |

```bash
# 后端测试
cd backend/build && cmake .. && make -j$(nproc) && ctest --output-on-failure

# 前端测试
cd frontend && flutter test

# 管理后台测试
cd admin && npx vitest run

# 全部测试
./tests/clients/run_all.sh
```

## 部署架构

```
                    ┌──────────────┐
                    │  Nginx 网关   │ :3000
                    │  (gateway)   │
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

Docker Compose 编排 6 个服务容器，健康检查确保启动顺序：
1. PostgreSQL + Redis 先启动并通过健康检查
2. Backend 等待数据库就绪后启动
3. Admin 和 Gateway 最后启动

## 环境变量

复制 `.env.example` 为 `.env` 并按需修改：

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `DB_NAME` | heartlake | 数据库名 |
| `DB_USER` | postgres | 数据库用户 |
| `DB_PASSWORD` | HeartLake | 数据库密码 |
| `REDIS_PASSWORD` | HeartLake | Redis 密码 |
| `PASETO_KEY` | HeartLake...（32字节） | PASETO 签名密钥 |
| `AI_PROVIDER` | ollama | AI 提供者 |
| `AI_MODEL` | heartlake-qwen | AI 模型名 |

## 文档索引

| 文档 | 说明 |
|------|------|
| [本地启动与运行手册](docs/01_本地启动与运行手册.md) | 环境要求、依赖安装、构建步骤 |
| [API 与实时链路手册](docs/02_API与实时链路手册.md) | API 设计理念、认证机制、WebSocket |
| [端到端测试与故障排查](docs/03_端到端测试与故障排查手册.md) | 测试策略、故障排查流程 |
| [技术实现全景手册](docs/04_技术实现全景手册.md) | 核心算法详解、架构决策 |
| [API 接口全量清单](docs/05_API接口全量清单.md) | 每个端点的请求/响应格式 |
| [测试验证与压测手册](docs/06_测试验证与压测手册.md) | 测试用例、压测方案、性能指标 |
| [贡献指南](CONTRIBUTING.md) | 开发规范、提交流程 |
