# 贡献指南

感谢你对 HeartLake 项目的关注！本文档将帮助你快速上手开发。

## 开发环境搭建

### 后端开发环境

**系统要求**: Linux (推荐 Arch Linux / Ubuntu 22.04+)

**依赖安装** (Arch Linux):
```bash
sudo pacman -S base-devel cmake git gcc clang \
  postgresql-libs redis hiredis jsoncpp yaml-cpp \
  brotli c-ares zlib openssl curl
```

**依赖安装** (Ubuntu):
```bash
sudo apt update
sudo apt install build-essential cmake git g++ \
  libpq-dev redis-server libhiredis-dev libjsoncpp-dev \
  libyaml-cpp-dev libbrotli-dev libc-ares-dev zlib1g-dev \
  libssl-dev libcurl4-openssl-dev
```

**安装 Drogon 框架**:
```bash
git clone https://github.com/drogonframework/drogon.git
cd drogon
git submodule update --init
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

**安装 ONNX Runtime**（可选，启用本地情感分析模型）:
```bash
# 下载 ONNX Runtime 1.22.0
wget https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-x64-1.22.0.tgz
tar xzf onnxruntime-linux-x64-1.22.0.tgz
sudo cp -r onnxruntime-linux-x64-1.22.0/include/* /usr/local/include/
sudo cp -r onnxruntime-linux-x64-1.22.0/lib/* /usr/local/lib/
sudo ldconfig
```

**构建后端**:
```bash
cd backend
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

**运行后端**（推荐使用统一启动脚本）:
```bash
cd backend

# 方式一：统一启动脚本（推荐）
# 自动生成运行时配置、启动 PostgreSQL/Redis、执行迁移、检查 Ollama 模型
./start.sh

# 方式二：手动启动
# 1. 确保 PostgreSQL 和 Redis 已运行
# 2. 复制并编辑环境变量
cp .env.example .env
# 编辑 .env 填入数据库连接信息等

# 3. 运行服务（自动加载 .env）
./build/HeartLake
```

**停止后端**:
```bash
cd backend
./stop.sh
```

### 管理后台开发环境

**系统要求**: Node.js >= 18

**安装依赖**:
```bash
cd admin
npm install
```

**开发模式**:
```bash
npm run dev
```

**构建生产版本**:
```bash
npm run build
```

**运行测试**:
```bash
npm run test:unit
```

### 移动端开发环境

**系统要求**: Flutter 3.19+ / Dart 3.3+

**安装依赖**:
```bash
cd frontend
flutter pub get
```

**开发运行**:
```bash
flutter run
```

**构建发布版**:
```bash
flutter build apk --release    # Android
flutter build ios --release    # iOS
```

### Docker 一键部署

```bash
# 复制环境变量模板
cp .env.example .env
# 编辑 .env 填入必要配置

# 启动所有服务
docker compose up -d

# 查看服务状态
docker compose ps

# 查看日志
docker compose logs -f backend
```

## Git 工作流

### 分支策略

- `main` — 主分支，保持可部署状态
- `feature/*` — 功能分支
- `fix/*` — 修复分支
- `refactor/*` — 重构分支

### 提交规范

使用中文 commit message，格式：

```
<type>(<scope>): <description>

[可选正文]
```

**type 类型**:
- `feat` — 新功能
- `fix` — 修复
- `refactor` — 重构
- `perf` — 性能优化
- `docs` — 文档
- `chore` — 杂项
- `style` — 样式
- `test` — 测试

**示例**:
```
feat(EdgeAI): 集成 ONNX Runtime 本地情感分析
fix(好友): 修复聊天消息退出即消失问题
perf(WebSocket): 优化离线队列刷新性能 O(n²)→O(n)
docs(README): 更新项目统计数据
```

### PR 流程

1. 从 `main` 创建功能分支
2. 开发完成后提交 PR
3. 通过 CI 检查（构建 + 测试）
4. 代码审查通过后合并

## 代码规范

### 后端 (C++20)

- 遵循 DDD 四层架构：Interfaces → Application → Domain → Infrastructure
- 使用智能指针管理内存，禁止裸 `new/delete`
- 并发场景使用 `std::shared_mutex` / `std::mutex`
- 异常处理使用 `try-catch`，不允许未捕获异常
- 头文件与实现分离（`.h` 在 `include/`，`.cpp` 在 `src/`）
- 命名规范：类名 `PascalCase`，方法名 `camelCase`，成员变量 `camelCase_` 后缀下划线

### 管理后台 (Vue 3)

- 使用 `<script setup>` 组合式 API
- 事件处理函数命名 `handleXxx`，数据获取函数命名 `fetchXxx`
- 统一使用 `errorHelper.js` 处理错误
- 危险操作（删除、封禁）必须有二次确认弹窗

### 移动端 (Flutter)

- 遵循 DDD 分层：data/datasources → domain/entities → presentation
- 使用 Provider 进行状态管理
- 网络请求通过 Service 层封装，Screen 不直接调用 `ApiClient`
- 使用 `ServiceResponse<T>` 统一封装 API 响应
- 命名规范：文件名 `snake_case`，类名 `PascalCase`

## 项目结构

```
heartlake/
├── backend/          # C++20 + Drogon 后端（39,400 行）
├── frontend/         # Flutter 移动端（26,950 行）
├── admin/            # Vue 3 管理后台（7,808 行）
├── nginx/            # Nginx 反向代理配置
├── docker-compose.yml
├── .env.example      # 环境变量模板
├── README.md
├── CONTRIBUTING.md   # 本文件
├── CHANGELOG.md
└── docs/             # 项目文档
```

## 代码审查清单

审查者应检查以下内容：

- [ ] 代码符合项目规范
- [ ] 功能实现正确且完整
- [ ] 有适当的错误处理
- [ ] 有必要的测试覆盖
- [ ] 没有安全漏洞（SQL 注入、XSS 等）
- [ ] 性能考虑合理
- [ ] 文档/注释清晰
- [ ] 没有引入不必要的依赖
- [ ] CI 检查全部通过

## 问题反馈

- **Bug 报告**: 使用 GitHub Issues，包含复现步骤、环境信息、错误日志
- **功能建议**: 详细描述使用场景和预期效果
- **安全问题**: 请私下联系维护者，不要公开披露

## 许可证

贡献代码即表示你同意将代码以项目相同的许可证发布。

## 联系方式

- **GitHub**: https://github.com/by17815777533-bit/HeartLake
- **维护者**: 白洋 (jokerbai)
- **邮箱**: by17815777533@gmail.com

---

再次感谢你的贡献！让我们一起打造更好的 HeartLake。
