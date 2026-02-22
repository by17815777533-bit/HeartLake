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
| ONNX Runtime | 1.17.0 (可选，中文情感分析模型推理) |
| 认证 | PASETO v4 |
| 加密 | X25519 / AES-256-GCM 端到端加密 |
| 构建 | CMake 3.16+ |
| 容器 | Docker (Arch Linux base) |

---

## 目录结构

```
backend/
├── include/                    # 头文件（接口定义）
│   ├── interfaces/api/         # 21 个 HTTP/WebSocket 控制器
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
- 可选：ONNX Runtime 1.17.0（启用本地情感分析模型）

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

# 4. 运行
./HeartLake
```

### Docker 构建

```bash
cd backend
docker build -t heartlake-backend .
docker run -p 8080:8080 --env-file .env heartlake-backend
```

推荐使用项目根目录的 `docker-compose.yml` 一键启动完整环境（含 PostgreSQL、Redis、Nginx）：

```bash
cd /path/to/heartlake
docker compose up -d
```

---

## API 控制器列表

共 21 个控制器，覆盖平台全部业务：

| # | 控制器 | 职责 |
|---|--------|------|
| 1 | `UserController` | 匿名登录、关键词恢复、用户信息 |
| 2 | `AccountController` | 账号管理、个人信息、设备管理 |
| 3 | `StoneController` | 心石 CRUD、投石、捞石 |
| 4 | `InteractionController` | 涟漪、点亮等互动操作 |
| 5 | `FriendController` | 好友关系管理 |
| 6 | `TempFriendController` | 临时好友（限时匿名聊天） |
| 7 | `PaperBoatController` | 纸船漂流（匿名信件） |
| 8 | `ConsultationController` | 咨询室（E2EE 端到端加密通信） |
| 9 | `MediaController` | 媒体文件上传与管理 |
| 10 | `VIPController` | VIP 状态查询与管理 |
| 11 | `RecommendationController` | 内容推荐（多算法融合） |
| 12 | `VectorSearchController` | 向量相似度搜索（HNSW） |
| 13 | `EdgeAIController` | 边缘 AI 推理、联邦学习、差分隐私接口 |
| 14 | `PrivacyController` | 差分隐私统计、隐私预算报告 |
| 15 | `GuardianController` | 守望者系统（灯火转赠与激励） |
| 16 | `SafeHarborController` | 安全港湾（危机干预资源） |
| 17 | `ReportController` | 举报处理 |
| 18 | `AdminController` | 管理员认证与操作 |
| 19 | `AdminManagementController` | 管理员账号管理 |
| 20 | `HealthController` | 健康检查、服务状态监控 |
| 21 | `BroadcastWebSocketController` | WebSocket 实时广播 |

---

## 数据库迁移

共 10 个迁移文件，按序号执行：

| 文件 | 说明 |
|------|------|
| `001_users.sql` | 用户表 |
| `002_stones.sql` | 心石表 |
| `003_social.sql` | 社交关系（好友、临时好友、纸船） |
| `004_interactions.sql` | 互动（涟漪、点亮） |
| `005_ai_system.sql` | AI 系统（情感分析、向量索引） |
| `006_guardian.sql` | 守望者系统 |
| `007_admin.sql` | 管理后台 |
| `008_notifications.sql` | 通知系统 |
| `009_consultation.sql` | 咨询室 |
| `010_data_export.sql` | 数据导出 |

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
| 数据库 | `DB_HOST` / `DB_PORT` / `DB_NAME` | PostgreSQL 连接信息 |
| 数据库 | `DB_USER` / `DB_PASSWORD` | PostgreSQL 认证 |
| 数据库 | `DB_POOL_SIZE` | 连接池大小，默认 `20` |
| Redis | `REDIS_HOST` / `REDIS_PORT` | Redis 连接信息 |
| Redis | `REDIS_POOL_SIZE` | 连接池大小，默认 `30` |
| 安全 | `PASETO_KEY` | 用户 PASETO v4 签名密钥（生产环境必须更换） |
| 安全 | `ADMIN_PASETO_KEY` | 管理员 PASETO v4 签名密钥 |
| 管理 | `ADMIN_USERNAME` / `ADMIN_PASSWORD` | 管理员初始账号 |
| AI | `AI_PROVIDER` | AI 服务商 (`deepseek` 等) |
| AI | `AI_API_KEY` / `AI_BASE_URL` / `AI_MODEL` | AI API 配置 |
| 限流 | `RATE_AI_PER_HOUR` | AI 接口每小时调用上限，默认 `30` |
| CORS | `CORS_ALLOWED_ORIGIN` | 允许的跨域来源 |

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

- **Interfaces**：21 个控制器负责 HTTP/WebSocket 请求路由、参数校验、响应序列化
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
