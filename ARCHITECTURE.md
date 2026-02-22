# HeartLake 架构文档

## 1. 系统架构概览

HeartLake 是一个匿名心理社交平台，采用三层架构：

| 层级 | 技术栈 | 说明 |
|------|--------|------|
| 移动端 | Flutter (Dart) | 用户端 App |
| 管理后台 | Vue 3 + Vite + Element Plus | 运营管理 |
| 后端服务 | C++20 + Drogon | 高性能异步 HTTP/WebSocket 服务 |
| 基础设施 | PostgreSQL 16 + Redis 7 + Ollama + Nginx | 数据存储、缓存、AI推理、反向代理 |

核心设计原则：
- **DDD (Domain-Driven Design)**：领域驱动，业务逻辑与基础设施解耦
- **事件驱动**：通过 EventBus 实现领域事件的发布-订阅
- **ServiceLocator**：类型擦除的依赖注入容器
- **边缘AI**：8大子系统全部本地运行，P99 < 50ms

## 2. 架构图

```
                          ┌─────────────┐
                          │   Flutter    │
                          │  移动端 App   │
                          └──────┬───────┘
                                 │
                          ┌──────▼───────┐
                          │    Nginx     │
                          │  反向代理     │
                          │  限流/安全头   │
                          └──┬───────┬───┘
                             │       │
                    ┌────────▼─┐   ┌─▼────────┐
                    │ Backend  │   │  Admin    │
                    │ Drogon   │   │  Vue 3    │
                    │ :8080    │   │  :80      │
                    └────┬─────┘   └───────────┘
                         │
          ┌──────────────┼──────────────┐
          │              │              │
   ┌──────▼──────┐ ┌────▼─────┐ ┌──────▼──────┐
   │ PostgreSQL  │ │  Redis   │ │   Ollama    │
   │    :5432    │ │  :6379   │ │   :11434    │
   │  主数据存储  │ │  缓存/队列 │ │  本地AI推理  │
   └─────────────┘ └──────────┘ └─────────────┘

后端 DDD 分层：

  ┌─────────────────────────────────────────────┐
  │  Interfaces (API层) - 21个 Controller        │
  ├─────────────────────────────────────────────┤
  │  Application (应用层) - 4个 Service           │
  │  StoneService / FriendService / AIService   │
  │  SummaryService                             │
  ├─────────────────────────────────────────────┤
  │  Domain (领域层) - 实体/值对象/仓储接口        │
  │  Stone(心石) / User(用户) / Friend(石友)      │
  ├─────────────────────────────────────────────┤
  │  Infrastructure (基础设施层)                  │
  │  ai / cache / di / events / filters /       │
  │  messaging / privacy / realtime / services  │
  │  / vector                                   │
  └─────────────────────────────────────────────┘
```

## 3. 后端架构

### 3.1 DDD 分层

| 层 | 目录 | 职责 | 示例 |
|----|------|------|------|
| Interfaces | `interfaces/api/` | HTTP 控制器，请求/响应转换 | StoneController, UserController (共21个) |
| Application | `application/` | 业务编排，跨领域协调 | StoneService, FriendService, AIService, SummaryService |
| Domain | `domain/` | 核心业务逻辑，实体，仓储接口 | Stone, User, Friend 子域 |
| Infrastructure | `infrastructure/` | 技术实现，外部依赖适配 | Redis, PostgreSQL, AI引擎, WebSocket |

### 3.2 ServiceLocator (依赖注入)

位于 `infrastructure/di/ServiceLocator.h`，单例模式的类型擦除 DI 容器：

```cpp
// 注册实例
ServiceLocator::instance().registerInstance<EventBus>(eventBus);

// 注册接口→实现的单例映射
ServiceLocator::instance().registerSingleton<IStoneRepository, StoneRepository>();

// 解析依赖
auto repo = ServiceLocator::instance().resolveRequired<IStoneRepository>();
```

内部使用 `unordered_map<type_index, shared_ptr<void>>` 存储，`recursive_mutex` 保证线程安全。

### 3.3 EventBus (事件驱动)

位于 `infrastructure/events/EventBus.h`，类型擦除的发布-订阅系统：

| 事件 | 触发时机 | 典型处理 |
|------|---------|---------|
| `StonePublishedEvent` | 心石发布 | 情感分析、向量索引 |
| `EmotionAnalyzedEvent` | 情感分析完成 | 缓存更新、推荐刷新 |
| `RippleCreatedEvent` | 涟漪创建 | 通知、缓存失效 |
| `BoatSentEvent` | 纸船发送 | 消息推送 |

### 3.4 启动流程 (ArchitectureBootstrap)

4阶段初始化，严格按依赖顺序：

1. **Infrastructure**：CacheManager → EventBus
2. **Domain**：IStoneRepository → IUserRepository → IFriendRepository
3. **Application**：StoneService → FriendService → AIService → SummaryService → EdgeAIEngine → MilvusClient 等
4. **Event Handlers**：StonePublishedHandler → EmotionAnalyzedHandler → RippleCreatedHandler → BoatSentHandler

## 4. 认证与安全

### 4.1 PASETO v4.local

替代 JWT，使用 ChaCha20-Poly1305 对称加密：

| 参数 | 值 |
|------|-----|
| Header | `v4.local.` |
| 密钥长度 | 32 bytes |
| Nonce | 12 bytes |
| Tag | 16 bytes |
| 默认过期 | 24 小时 |
| 编码 | Base64URL (无填充) |

分为用户 Token (`generateToken`) 和管理员 Token (`generateAdminToken`，含 role 字段)。

### 4.2 E2EE 端到端加密

用于心理咨询会话 (`consultation_sessions`)：

```
密钥交换: X25519 ECDH (32 bytes)
    ↓
密钥派生: HKDF-SHA256 (sharedSecret + salt → sessionKey)
    ↓
消息加密: AES-256-GCM (key=32B, iv=12B, tag=16B)
```

提供 IND-CCA2 安全性，临时密钥实现前向保密。

### 4.3 Nginx 安全层

**限流策略**（4个 zone）：

| Zone | 速率 | 用途 |
|------|------|------|
| `api` | 30 req/s | 通用 API |
| `login` | 5 req/min | 登录接口 |
| `ws` | 10 req/s | WebSocket |
| `ai_api` | 5 req/s | AI 接口 |

**安全头**：X-Frame-Options DENY, X-Content-Type-Options nosniff, CSP, HSTS (1年), Permissions-Policy。

## 5. 边缘AI引擎 (EdgeAIEngine)

8大子系统全部在本地运行，设计目标：模型 < 20MB (INT8)，内存 < 100MB，支持离线。

| # | 子系统 | 算法 | 说明 |
|---|--------|------|------|
| 1 | 情感分析 | TF-IDF + 规则 + 词典 + 统计三层融合 | 中文情感极性判断 |
| 2 | 内容审核 | AC 自动机 | 敏感词高效匹配 |
| 3 | 心理风险评估 | 5因子模型 | 自伤/抑郁/焦虑/孤独/危机 |
| 4 | 向量搜索 | HNSW (128维) | LRU 缓存 10000 条，相似内容检索 |
| 5 | 差分隐私 | Laplace 机制 | 隐私预算追踪，数据脱敏 |
| 6 | 联邦学习 | FedAvg 加权聚合 | 模型更新不暴露原始数据 |
| 7 | 情感共鸣 | DTW + 时间衰减 + 多样性 | 情感脉搏追踪 |
| 8 | 推荐引擎 | CF + 内容 + 探索多算法融合 | 个性化推荐 |

云边协作：边缘处理实时请求 (P99 < 50ms)，Ollama (`heartlake-qwen` 模型) 处理复杂生成任务。

## 6. 数据流

### 6.1 典型请求流程

```
Flutter/Admin
    │
    ▼
Nginx (限流 + 安全头 + SSL)
    │
    ▼ proxy_pass
Drogon Controller (Interfaces 层)
    │
    ├─ SecurityAuditFilter (安全审计)
    ├─ RateLimitFilter (应用层限流)
    ├─ PASETO Token 验证
    │
    ▼
Application Service (业务编排)
    │
    ├─ EdgeAIEngine (情感分析/内容审核/风险评估)
    ├─ EventBus.publish() (领域事件)
    │
    ▼
Domain Repository (仓储接口)
    │
    ├─ CacheManager (内存 LRU, 10000条)
    ├─ RedisCache (分布式缓存)
    │
    ▼
PostgreSQL (持久化)
```

### 6.2 WebSocket 连接

```
Flutter → Nginx (/ws/, 3600s timeout, limit_conn 10)
    → Drogon WebSocket Handler
    → 实时消息推送 (涟漪通知/纸船消息/好友消息)
```

## 7. 容器化部署

6个服务通过 Docker Compose 编排，共享 `heartlake-net` bridge 网络：

| 服务 | 镜像 | 资源限制 | 健康检查 |
|------|------|---------|---------|
| `postgres` | postgres:16-alpine | 512M / 1 CPU | `pg_isready` |
| `redis` | redis:7-alpine | 256M / 0.5 CPU | `redis-cli ping` |
| `ollama` | ollama/ollama:latest | 4G / 2 CPU | HTTP /api/tags |
| `backend` | 自构建 | 1.5G / 2 CPU | HTTP /api/health |
| `admin` | 自构建 | 128M / 0.5 CPU | curl localhost |
| `nginx` | nginx:alpine | 128M / 0.5 CPU | curl localhost |

**启动依赖链**：
```
postgres + redis + ollama → backend → admin + nginx
```

**数据卷** (6个)：pgdata, redisdata, backend-uploads, backend-logs, backend-models, ollama-models

## 8. 数据库设计

PostgreSQL 16，共 10 个迁移文件，25+ 张表。核心表关系：

```
users (核心)
  ├── stones (心石, 1:N)
  │     ├── stone_embeddings (向量, 1:1)
  │     ├── ripples (涟漪/评论, 1:N)
  │     └── paper_boats (纸船, 1:N)
  ├── friends / temp_friends (石友, M:N)
  │     └── friend_messages (好友消息)
  ├── connections (匹配连接, M:N)
  │     └── connection_messages (连接消息)
  ├── user_similarity (相似度, M:N)
  ├── notifications (通知, 1:N)
  ├── consultation_sessions (咨询, 1:N)
  │     └── consultation_messages (E2EE加密消息)
  ├── data_export_tasks (数据导出, 1:N)
  └── user_emotion_history (情感历史, 1:N)

独立表:
  ├── admin_users (管理员)
  ├── sensitive_words (敏感词库)
  ├── operation_logs (操作日志)
  ├── lake_god_messages (湖神消息)
  ├── federated_models (联邦学习模型)
  ├── edge_nodes (边缘节点)
  ├── vector_index_metadata (向量索引元数据)
  ├── emotion_tracking (情感追踪)
  ├── resonance_points (共鸣点)
  └── intervention_log (干预日志)
```

关键设计：
- 所有用户关联表通过 `ON DELETE CASCADE` 级联删除
- 咨询消息使用 `ciphertext/iv/tag` 三字段存储 E2EE 密文
- `user_emotion_profile` 为视图，聚合情感历史数据
- 广泛使用复合索引优化查询 (如 `idx_notifications_user` 覆盖 user_id + is_read + created_at)

## 9. 缓存策略

### 9.1 两级缓存架构

| 层级 | 实现 | 容量 | TTL | 用途 |
|------|------|------|-----|------|
| L1 内存 | CacheManager (LRU) | 10000 条 | 默认 300s | 热点数据，零网络开销 |
| L2 分布式 | Redis 7 | 256MB (LRU淘汰) | 按场景配置 | 跨实例共享，持久化 (AOF) |

### 9.2 Redis 连接池

| 参数 | 值 |
|------|-----|
| 初始连接数 | 30 |
| 最大连接数 | 100 |
| 空闲超时 | 30s |
| 连接超时 | 5s |
| 自动扩缩 | 开启 |

### 9.3 专用缓存

- **AIResponseCache**：缓存 AI 回复 (`cacheReply/getReply`) 和内容审核结果 (`cacheModeration/getModeration`)，避免重复推理
- **CacheManager**：支持 pattern 批量失效 (`invalidatePattern`)，用于领域事件触发的缓存刷新
- 支持 JSON 序列化/反序列化 (`setJson/getJson`)

## 10. 扩展性考虑

### 架构扩展点

| 扩展方向 | 机制 | 说明 |
|---------|------|------|
| 新领域事件 | EventBus subscribe | 继承 `DomainEvent`，注册 Handler 即可 |
| 新服务注入 | ServiceLocator register | 定义接口 + 实现，Bootstrap 中注册 |
| 新 AI 子系统 | EdgeAIEngine 扩展 | 遵循现有子系统接口模式 |
| 新 API 端点 | Drogon Controller | 添加 Controller，自动路由注册 |
| 新数据表 | SQL Migration | 按编号递增添加迁移文件 (当前到 010) |
| 新缓存场景 | RedisCache / CacheManager | 异步接口，支持 JSON 和批量操作 |

### 水平扩展路径

- **后端**：Drogon 本身支持多线程异步，单实例可承载高并发；多实例部署需引入 Redis 做 Session/WebSocket 状态共享
- **数据库**：读写分离 (PostgreSQL Streaming Replication)，分表分库 (按 user_id hash)
- **AI推理**：Ollama 可独立扩容，EdgeAIEngine 无状态可随后端水平扩展
- **缓存**：Redis Cluster 模式，CacheManager 作为 L1 天然随实例扩展
