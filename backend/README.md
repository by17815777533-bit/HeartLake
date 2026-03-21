# HeartLake Backend

HeartLake 匿名情感社交平台的后端服务，基于 C++20 + Drogon 异步框架构建，采用 DDD 分层架构 + 事件驱动 + ServiceLocator 模式，内置 8 个边缘 AI 子系统。

---

## 技术栈

| 组件 | 版本 / 说明 |
|------|------------|
| C++ | C++20 (GCC 12+ / Clang 15+) |
| Drogon | 1.9.11+ (异步 HTTP/WebSocket 框架) |
| PostgreSQL | 16 |
| Redis | 7 |
| ONNX Runtime | 1.22.0（支持 CPU/GPU；2c2g 默认优先低资源 CPU 配置） |
| 认证 | PASETO v4 |
| 加密 | X25519 / AES-256-GCM 端到端加密 |
| 构建 | CMake 3.16+ |
| 容器 | Docker（Ubuntu 24.04 多阶段镜像） |

---

## 目录结构

```
backend/
├── include/                    # 头文件（接口定义）
│   ├── interfaces/api/         # 20 个 HTTP/WebSocket 控制器
│   ├── application/            # 应用服务 + 事件处理器
│   │   └── handlers/
│   ├── domain/                 # 领域层
│   │   ├── entities/           # 领域实体
│   │   ├── friend/             # 好友子域 (repositories, services)
│   │   ├── stone/              # 心石子域 (repositories, services)
│   │   └── user/               # 用户子域 (repositories)
│   ├── infrastructure/         # 基础设施层
│   │   ├── ai/                 # AI 服务 (EdgeAIEngine, ONNX)
│   │   ├── cache/              # Redis 缓存
│   │   ├── di/                 # 依赖注入 (ServiceLocator)
│   │   ├── events/             # 领域事件 (发布-订阅)
│   │   ├── filters/            # Drogon 过滤器 (认证/限流)
│   │   ├── messaging/          # 消息通信
│   │   ├── privacy/            # 差分隐私
│   │   ├── realtime/           # WebSocket 实时通信
│   │   ├── services/           # 基础设施服务实现
│   │   └── vector/             # HNSW 向量索引
│   ├── config/                 # 配置管理
│   ├── middleware/              # 中间件
│   └── utils/                  # 工具类
├── src/                        # 源文件（与 include/ 镜像对应）
├── migrations/                 # PostgreSQL 数据库迁移脚本
├── cmake/                      # CMake 模块 (FindOnnxRuntime 等)
├── CMakeLists.txt
├── Dockerfile
├── start-docker.sh             # Docker 启动脚本 (配置生成 + 迁移 + 启动)
└── .env.example                # 环境变量模板
```

---

## 环境要求

- GCC 12+ 或 Clang 15+（需支持 C++20）
- CMake 3.16+
- 系统依赖：OpenSSL, libcurl, jsoncpp, c-ares, hiredis, postgresql-libs
- 可选：ONNX Runtime 1.22.0（启用本地情感分析模型）

### Arch Linux

```bash
pacman -S base-devel cmake git openssl curl jsoncpp postgresql-libs hiredis c-ares
```

### Ubuntu / Debian

```bash
apt install build-essential cmake git libssl-dev libcurl4-openssl-dev \
    libjsoncpp-dev libc-ares-dev libhiredis-dev libpq-dev
```

---

## 构建与运行

### 本地构建

```bash
# 1. 安装 Drogon（如未安装）
git clone --depth 1 --branch v1.9.11 https://github.com/drogonframework/drogon.git
cd drogon && git submodule update --init
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_CTL=OFF -DBUILD_ORM=OFF
make -j$(nproc) && sudo make install

# 2. 构建 HeartLake
cd backend
mkdir build && cd build

# 不启用 ONNX
cmake .. -DCMAKE_BUILD_TYPE=Release
# 或启用 ONNX
cmake .. -DCMAKE_BUILD_TYPE=Release -DHEARTLAKE_USE_ONNX=ON

make -j$(nproc)

# 3. 配置环境变量
cp ../.env.example ../.env
# 编辑 .env 填入实际配置

# ONNX 小模型（可选）
# - EDGE_AI_ENABLED 控制 EdgeAI 总开关；若后台管理端已保存 ai 配置，启动时会优先读取持久化值
# - EDGE_AI_ONNX_ENABLED=true/false 时为显式开关
# - 不设置 EDGE_AI_ONNX_ENABLED 时，服务会自动探测模型与词表是否存在，存在即启用
# - EDGE_AI_MODEL_PATH 可写目录（例如 ./models）或模型文件路径
# - 可选：EDGE_AI_SENTIMENT_LEXICON_PATH 指向「短语\t权重」领域词典（未设置则不加载）
EDGE_AI_MODEL_PATH=./models
EDGE_AI_ONNX_THREADS=1
EDGE_AI_ONNX_SESSION_POOL=4
EDGE_AI_SENTIMENT_LEXICON_PATH=./models/sentiment_domain_lexicon.tsv
# 双路策略已内置固定路由与背压，不再暴露路由/过载阈值开关
# Ollama 自动拉起（默认关闭；仅 AI_PROVIDER=ollama 且设为 true 时触发）
AI_OLLAMA_AUTOSTART=false
# GPU 相关参数按需开启；代码默认已改成 CPU 优先（未显式配置时走 2 线程 / 1536 ctx）
# 低配 CPU 机器建议保持关闭，只有明确有 GPU 时再打开
AI_OLLAMA_FORCE_GPU=false
AI_OLLAMA_NUM_GPU=0
AI_OLLAMA_MAIN_GPU=0
# 情绪分析服务化（缓存 + 并发合并 + 自适应调度）
AI_SENTIMENT_LOCAL_CONF_THRESHOLD=0.72
AI_SENTIMENT_CACHE_TTL_SEC=7200
AI_SENTIMENT_CACHE_MAX_SIZE=30000
AI_SENTIMENT_ADAPTIVE_INFLIGHT_THRESHOLD=8
AI_SENTIMENT_ADAPTIVE_LOCAL_CONF_DELTA=0.20
# Edge 本地情绪分析高并发缓存（/api/edge-ai/analyze 热点路径）
EDGE_AI_SENTIMENT_CACHE_TTL_SEC=180
EDGE_AI_SENTIMENT_CACHE_MAX_SIZE=8192
EDGE_AI_ONNX_FORCE_GPU=true
EDGE_AI_ONNX_GPU_DEVICE=0
EDGE_AI_ONNX_GPU_HARD_FAIL=false

# 4. 运行（推荐统一入口）
# - 自动加载 .env
# - 自动生成 config.runtime.json
# - 本地 PostgreSQL/Redis 不可用时自动拉起（数据持久化到项目根目录 .runtime）
# - 自动执行 migrations
# - 仅当 AI_PROVIDER=ollama 且 AI_OLLAMA_AUTOSTART=true 时自动检查并拉取 AI_MODEL（默认 heartlake-qwen）
cd ..
./backend/start.sh

# 停止本地运行栈
./backend/stop.sh

# 也可直接运行二进制（需自行保障依赖服务可用）
cd backend/build
./HeartLake
```

`start.sh` 运行时说明（与当前代码一致）：
- 启动期会先读取 `backend/data/admin-config.json` 中持久化的 `ai` 配置；`EDGE_AI_*` 环境变量作为默认值与兜底值参与合成最终启动配置。
- 自动优先使用 `backend/third_party/onnxruntime-linux-x64-gpu-1.22.0`。
- 若 GPU 包不存在，自动回退到 `onnxruntime-linux-x64-1.22.0`（CPU）。
- 自动拼接 `LD_LIBRARY_PATH`（ONNX + Ollama CUDA 库路径）。
- 若机器缺少 `libcurand.so.10` / `libcufft.so.11` / `libnvrtc.so.12`，ONNX 会记录日志并回退 CPU，不影响服务可用性。
- 当 `edge_ai_enabled=false` 时，EdgeAI 会保持禁用态启动，并延迟初始化子系统；适合 2c2g 机器按需开启本地 AI 能力链。

### Docker 构建

```bash
cd backend
docker build --build-arg BUILD_JOBS=1 -t heartlake-backend .
docker run -p 8080:8080 --env-file .env heartlake-backend
```

推荐使用项目根目录的 Compose 启动完整环境；低配服务器优先使用 `server-lite`：

```bash
cd /path/to/heartlake
./scripts/start-services.sh lite
```

---

## API 控制器列表

共 20 个控制器，覆盖平台全部业务：

| # | 控制器 | 职责 |
|---|--------|------|
| 1 | `UserController` | 匿名登录、关键词恢复、会话续期、用户信息 |
| 2 | `AccountController` | 账号管理、个人信息、设备管理 |
| 3 | `StoneController` | 心石 CRUD、投石、捞石 |
| 4 | `InteractionController` | 涟漪、点亮等互动操作 |
| 5 | `FriendController` | 好友关系管理 |
| 6 | `TempFriendController` | 临时好友（限时匿名聊天） |
| 7 | `PaperBoatController` | 纸船漂流（匿名信件） |
| 8 | `ConsultationController` | 咨询室（E2EE 端到端加密通信） |
| 9 | `VIPController` | VIP 状态查询与管理 |
| 10 | `RecommendationController` | 内容推荐（多算法融合） |
| 11 | `VectorSearchController` | 向量相似度搜索（HNSW） |
| 12 | `EdgeAIController` | 边缘 AI 推理、联邦学习、差分隐私接口 |
| 13 | `PrivacyController` | 差分隐私统计、隐私预算报告 |
| 14 | `GuardianController` | 守护者系统（灯火转赠与激励） |
| 15 | `SafeHarborController` | 安全港湾（危机干预资源） |
| 16 | `ReportController` | 举报处理 |
| 17 | `AdminController` | 管理员认证与操作 |
| 18 | `AdminManagementController` | 管理员账号管理 |
| 19 | `HealthController` | 健康检查、服务状态监控 |
| 20 | `BroadcastWebSocketController` | WebSocket 实时广播 |

---

## 数据库迁移

共 15 个迁移文件，按序号执行：

| 文件 | 说明 |
|------|------|
| `001_users.sql` | 用户表 |
| `002_stones.sql` | 心石表 |
| `003_social.sql` | 社交关系（好友、临时好友、纸船） |
| `004_interactions.sql` | 互动（涟漪、点亮） |
| `005_ai_system.sql` | AI 系统（情感分析、向量索引） |
| `006_guardian.sql` | 守护者系统 |
| `007_admin.sql` | 管理后台 |
| `008_notifications.sql` | 通知系统 |
| `009_consultation.sql` | 咨询室 |
| `010_data_export.sql` | 数据导出 |
| `011_performance_schema_hardening.sql` | 性能索引与表结构兼容增强 |
| `012_users_username_compat.sql` | users 表兼容列补齐（`username`/`recovery_key_hash`） |
| `013_schema_fixes.sql` | 外键约束与时间字段兼容修复 |
| `014_stones_embedding_vector.sql` | 向量列与索引能力补齐 |
| `015_runtime_schema_hotfix.sql` | 运行时热修复表结构（账户辅助表、恢复关键词长度） |

### 手动执行迁移

```bash
for f in $(ls migrations/*.sql | sort); do
    PGPASSWORD="$DB_PASSWORD" psql -h "$DB_HOST" -p "$DB_PORT" \
        -U "$DB_USER" -d "$DB_NAME" -v ON_ERROR_STOP=1 -f "$f"
done
```

Docker 环境下迁移由 `start-docker.sh` 自动执行。

---

## 配置说明

### 环境变量

复制 `.env.example` 为 `.env` 并填入实际值：

```bash
cp .env.example .env
```

主要配置项：

| 分类 | 变量 | 说明 |
|------|------|------|
| 服务 | `SERVER_PORT` | 监听端口，默认 `8080` |
| 服务 | `SERVER_THREADS` | 工作线程数，默认 `4` |
| 性能 | `EMBEDDING_WARMUP_ON_BOOT` | 启动时是否预热Embedding训练，默认 `false`（建议低配机保持关闭） |
| 性能 | `ENABLE_LAKE_GOD_GUARDIAN` / `LAKE_GOD_STARTUP_DELAY_SEC` / `LAKE_GOD_SCAN_BATCH_SIZE` | 湖神服务开关 + 冷启动延迟 + 每轮批量 |
| 性能 | `ENABLE_EMOTION_TRACKING` / `EMOTION_TRACKING_STARTUP_DELAY_SEC` | 情绪追踪开关 + 冷启动延迟 |
| 性能 | `ENABLE_USER_FOLLOWUP` / `USER_FOLLOWUP_STARTUP_DELAY_SEC` | 回访服务开关 + 冷启动延迟 |
| 性能 | `ENABLE_WS_HEARTBEAT` | WebSocket心跳开关，默认 `true` |
| 数据库 | `DB_HOST` / `DB_PORT` / `DB_NAME` | PostgreSQL 连接信息 |
| 数据库 | `DB_USER` / `DB_PASSWORD` | PostgreSQL 认证 |
| 数据库 | `DB_POOL_SIZE` | 连接池大小，默认 `20` |
| Redis | `REDIS_HOST` / `REDIS_PORT` | Redis 连接信息 |
| Redis | `REDIS_POOL_SIZE` / `REDIS_MAX_POOL_SIZE` | 初始/最大连接池大小，默认 `12/24` |
| 安全 | `PASETO_KEY` | 用户 PASETO v4 签名密钥（生产环境必须更换） |
| 安全 | `ADMIN_PASETO_KEY` | 管理员 PASETO v4 签名密钥 |
| 管理 | `ADMIN_USERNAME` / `ADMIN_PASSWORD_HASH` | 管理员登录配置（推荐使用哈希，不再依赖明文默认密码） |
| AI | `AI_PROVIDER` | AI 服务商 (`deepseek` 等) |
| AI | `AI_API_KEY` / `AI_BASE_URL` / `AI_MODEL` | AI API 配置 |
| 限流 | `RATE_AI_PER_HOUR` | AI 接口每小时调用上限，默认 `30` |
| CORS | `CORS_ALLOWED_ORIGIN` | 允许的跨域来源 |

运行时补充说明：
- 后端二进制启动时会自动尝试加载 `.env`（查找顺序：`.env` → `../.env` → `./backend/.env`，或 `HEARTLAKE_ENV_PATH` 指定路径）。
- 若未显式配置 AI 变量，默认走本地优先：`AI_PROVIDER=ollama`、`AI_BASE_URL=http://127.0.0.1:11434`、`AI_MODEL=heartlake-qwen`。

### 配置文件

Docker 环境下 `start-docker.sh` 会根据环境变量自动生成 `config.json`（Drogon 配置），包含数据库连接池和 Redis 连接池配置。本地开发可手动创建或设置 `HEARTLAKE_CONFIG_PATH` 指向配置文件。

---

## 测试

项目使用 Google Test (GTest) 框架。CMakeLists.txt 中通过 `HeartLake_lib` 静态库支持测试链接。

```bash
cd build

# 构建并运行测试（如有 test target）
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
ctest --output-on-failure
```

---

## 架构说明

### DDD 分层架构

```
Interfaces (API 层)
    ↓ 调用
Application (应用服务层)
    ↓ 编排
Domain (领域层)
    ↓ 依赖倒置
Infrastructure (基础设施层)
```

- **Interfaces**：20 个控制器负责 HTTP/WebSocket 请求路由、参数校验、响应序列化
- **Application**：应用服务编排业务流程，包含 `FriendApplicationService`、`InteractionApplicationService`、`StoneApplicationService`、`UserApplicationService` 及事件处理器
- **Domain**：纯业务逻辑，划分为 `user`、`stone`、`friend` 三个子域，每个子域包含 entities、repositories（接口）、services
- **Infrastructure**：技术实现层，提供 AI、缓存、事件总线、过滤器、隐私、实时通信、向量索引等基础能力

### 事件驱动

通过 `infrastructure/events/` 实现领域事件的发布-订阅机制，解耦业务模块间的依赖。例如投石事件触发情感分析、内容审核、向量化等异步处理。

### ServiceLocator

`infrastructure/di/` 提供 ServiceLocator 模式的依赖注入容器，管理所有服务的生命周期。启动时由 `ArchitectureBootstrap` 统一注册和初始化。

### EdgeAI Engine 8 大子系统

| # | 子系统 | 说明 |
|---|--------|------|
| 1 | 本地情感分析 | 三层融合：规则 + 词典 + 统计（可选 ONNX 模型） |
| 2 | 内容审核 | AC 自动机关键词过滤 + 心理风险评估 |
| 3 | HNSW 向量索引 | 128 维向量，LRU 缓存 10000 条 |
| 4 | 联邦学习 | FedAvg 加权聚合 |
| 5 | 差分隐私 | Laplace 机制，隐私预算追踪 |
| 6 | 情感共鸣引擎 | DTW + 时间衰减 + 多样性 |
| 7 | 双记忆 RAG | 短期记忆 (5 条) + 长期记忆 (30 天) |
| 8 | 推荐引擎 | 多算法融合 (协同过滤 + 内容 + 探索) |
