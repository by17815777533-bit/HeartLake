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

**构建后端**:
```bash
cd backend
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

**运行后端**:
```bash
# 启动 PostgreSQL 和 Redis
sudo systemctl start postgresql redis

# 配置数据库（首次运行）
psql -U postgres -c "CREATE DATABASE heartlake;"

# 运行服务
./heartlake_backend
```

### 管理后台开发环境

**系统要求**: Node.js 20+

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

**系统要求**: Flutter 3.24.0+

**安装 Flutter**:
```bash
# 参考官方文档: https://flutter.dev/docs/get-started/install
```

**安装依赖**:
```bash
cd frontend
flutter pub get
```

**运行应用**:
```bash
flutter run
```

**代码分析**:
```bash
flutter analyze
flutter format .
```

## 代码规范

### C++ 代码规范

- **命名约定**:
  - 类名: `PascalCase` (例: `StoneController`)
  - 函数名: `camelCase` (例: `createStone`)
  - 变量名: `snake_case` (例: `user_id`)
  - 常量: `UPPER_SNAKE_CASE` (例: `MAX_RETRY_COUNT`)

- **文件组织**:
  - 头文件: `include/` 目录
  - 实现文件: `src/` 目录
  - 测试文件: `tests/` 目录

- **注释规范**:
  - 使用 Doxygen 风格注释
  - 每个公开接口必须有文档注释
  - 复杂逻辑需要行内注释说明

- **代码风格**:
  - 缩进: 4 空格
  - 大括号: K&R 风格
  - 每行最大 120 字符

**示例**:
```cpp
/**
 * @brief 创建新的情绪石头
 * @param req HTTP 请求对象
 * @param callback 响应回调函数
 */
void StoneController::createStone(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    // 实现逻辑
}
```

### TypeScript/Vue 代码规范

- **命名约定**:
  - 组件名: `PascalCase` (例: `StoneCard.vue`)
  - 函数名: `camelCase` (例: `fetchStones`)
  - 变量名: `camelCase` (例: `userId`)
  - 常量: `UPPER_SNAKE_CASE` (例: `API_BASE_URL`)

- **Vue 组件结构**:
```vue
<script setup lang="ts">
// 导入
// 类型定义
// 响应式状态
// 计算属性
// 方法
// 生命周期钩子
</script>

<template>
  <!-- 模板 -->
</template>

<style scoped>
/* 样式 */
</style>
```

- **代码风格**:
  - 缩进: 2 空格
  - 使用 ESLint + Prettier
  - 优先使用 Composition API

### Dart/Flutter 代码规范

- **命名约定**:
  - 类名: `PascalCase`
  - 函数/变量: `camelCase`
  - 常量: `lowerCamelCase`
  - 私有成员: `_leadingUnderscore`

- **代码风格**:
  - 缩进: 2 空格
  - 使用 `flutter format` 自动格式化
  - 遵循 [Effective Dart](https://dart.dev/guides/language/effective-dart)

## Git 工作流

### 分支策略

- `main`: 主分支，保持稳定可发布状态
- `feature/*`: 功能分支 (例: `feature/add-emotion-analysis`)
- `bugfix/*`: 修复分支 (例: `bugfix/fix-login-error`)
- `hotfix/*`: 紧急修复分支

### 提交规范

**Commit Message 格式**:
```
<type>(<scope>): <subject>

<body>

<footer>
```

**Type 类型**:
- `feat`: 新功能
- `fix`: 修复 bug
- `docs`: 文档更新
- `style`: 代码格式调整（不影响功能）
- `refactor`: 重构
- `test`: 测试相关
- `chore`: 构建/工具链相关

**示例**:
```
feat(stone): 添加情绪共鸣搜索功能

实现基于向量相似度的石头推荐算法，支持：
- 语义相似度计算
- 情绪兼容性匹配
- 时间衰减因子

Closes #123
```

### Pull Request 流程

1. **Fork 仓库** (外部贡献者) 或 **创建分支** (团队成员)

2. **开发功能**:
```bash
git checkout -b feature/your-feature-name
# 进行开发
git add .
git commit -m "feat: 添加新功能"
```

3. **保持同步**:
```bash
git fetch origin
git rebase origin/main
```

4. **推送分支**:
```bash
git push origin feature/your-feature-name
```

5. **创建 Pull Request**:
   - 标题清晰描述改动
   - 详细说明实现思路
   - 关联相关 Issue
   - 添加截图/演示（如适用）

6. **代码审查**:
   - 至少需要 1 位团队成员审查
   - 解决所有审查意见
   - 确保 CI 通过

7. **合并**:
   - 使用 Squash and Merge（保持主分支历史清晰）
   - 删除已合并的分支

## 测试要求

### 后端测试

- **单元测试**: 使用 Google Test (GTest)
- **测试覆盖率**: 核心业务逻辑 > 80%
- **测试文件位置**: `backend/tests/`

**运行测试**:
```bash
cd backend/build
ctest --output-on-failure
```

**编写测试示例**:
```cpp
TEST(StoneServiceTest, CreateStone) {
    StoneService service;
    auto result = service.createStone("test content", "happy");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.stoneId.empty());
}
```

### 前端测试

- **单元测试**: Vitest
- **组件测试**: Vue Test Utils
- **测试文件**: `*.test.ts` 或 `*.spec.ts`

**运行测试**:
```bash
cd admin
npm run test:unit
```

**编写测试示例**:
```typescript
import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'
import StoneCard from '@/components/StoneCard.vue'

describe('StoneCard', () => {
  it('renders stone content', () => {
    const wrapper = mount(StoneCard, {
      props: { content: 'Test stone', mood: 'happy' }
    })
    expect(wrapper.text()).toContain('Test stone')
  })
})
```

### Flutter 测试

- **单元测试**: `flutter test`
- **Widget 测试**: Flutter Testing Framework
- **测试文件**: `test/` 目录

**运行测试**:
```bash
cd frontend
flutter test
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
