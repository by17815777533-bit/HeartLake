# HeartLake 管理后台

HeartLake 社交应用的管理后台，基于 Vue 3 构建，提供用户管理、内容审核、数据统计、边缘 AI 监控等功能。通过 RESTful API 与 C++ Drogon 后端通信，采用 PASETO v4 认证机制。

## 技术栈

| 类别 | 技术 | 版本 |
|------|------|------|
| 框架 | Vue | ^3.4 |
| 构建工具 | Vite | ^5.0 |
| UI 组件库 | Element Plus | ^2.5 |
| 状态管理 | Pinia | ^2.1 |
| 路由 | Vue Router | ^4.2 |
| HTTP 客户端 | Axios | ^1.6 |
| 图表 | ECharts + vue-echarts | ^5.5 / ^6.6 |
| 样式 | SCSS (sass ^1.70) | - |
| 日期处理 | Day.js | ^1.11 |
| 图标 | @element-plus/icons-vue | ^2.3 |
| 测试 | Vitest + @vue/test-utils | ^1.6 / ^2.4 |

## 目录结构

```
src/
├── api/            # API 请求模块，统一的 Axios 实例与接口定义
├── assets/         # 静态资源（logo.svg 等）
├── layouts/        # 页面布局组件（MainLayout.vue 侧边栏+顶栏布局）
├── router/         # Vue Router 路由配置与导航守卫
├── services/       # 业务服务（WebSocket 实时通信）
├── stores/         # Pinia 状态管理
├── styles/         # 全局样式（Material Design 3 主题 SCSS）
├── utils/          # 工具函数（错误处理 helper）
├── views/          # 页面视图组件（10 个管理页面）
├── App.vue         # 根组件
└── main.js         # 应用入口
```

## 环境要求

- Node.js >= 18
- npm >= 9（或 pnpm / yarn）

## 开发与构建

```bash
# 安装依赖
npm install

# 启动开发服务器（端口 5173，自动代理 /api 到后端 8080）
npm run dev

# 生产构建
npm run build

# 预览构建产物
npm run preview

# 运行测试
npm run test
```

开发服务器代理配置（`vite.config.js`）：

- `/api` -> `http://127.0.0.1:8080`（HTTP API）
- `/ws` -> `ws://127.0.0.1:8080`（WebSocket）

## 页面功能

| 路由 | 页面 | 功能说明 |
|------|------|----------|
| `/login` | Login | 管理员登录，PASETO v4 token 认证 |
| `/dashboard` | Dashboard | 数据大屏，用户增长、心情分布、活跃时段、热门话题等 ECharts 图表，实时统计轮询 |
| `/users` | Users | 用户管理，用户列表查询、封禁/解封操作 |
| `/content` | Content | 内容管理，心石（stones）的查看与删除 |
| `/moderation` | Moderation | 内容审核，待审内容列表、审核通过/拒绝、审核历史记录 |
| `/reports` | Reports | 举报处理，查看用户举报并进行处理 |
| `/sensitive-words` | SensitiveWords | 敏感词管理，敏感词的增删改查 |
| `/logs` | Logs | 操作日志，管理员操作记录查询 |
| `/settings` | Settings | 系统设置，系统配置修改、AI 连接测试、全站广播消息 |
| `/edge-ai` | EdgeAI | 边缘 AI 监控，AI 引擎状态、性能指标、情感脉搏、联邦学习聚合、隐私预算、向量搜索、文本分析与审核测试 |

## API 集成

`src/api/index.js` 基于 Axios 封装了统一的 HTTP 客户端：

**Axios 实例配置：**
- `baseURL: '/api'` — 所有请求自动加 `/api` 前缀，由 Vite 代理转发到后端
- `timeout: 15000` — 请求超时 15 秒
- 内置 CSRF 防护（`csrf_token` cookie + `X-CSRF-Token` header）

**请求拦截器：**
- 自动注入 `Authorization: Bearer <token>` 认证头
- 触发全局 loading 状态（可通过 `{ skipLoading: true }` 跳过）

**响应拦截器：**
- `401` — 清除 token，跳转登录页（防重复跳转）
- `403` — 提示无权限
- `429` — 提示频率限制，显示 `Retry-After` 秒数
- `5xx` — 提示服务器错误
- 网络超时/断连 — 对应错误提示

**API 分组：**

| 模块 | 接口数 | 说明 |
|------|--------|------|
| Auth | 1 | 管理员登录 |
| Dashboard | 8 | 统计数据（用户增长、心情分布、热门话题、活跃时段、隐私预算、情感共鸣等） |
| Users | 3 | 用户列表、封禁、解封 |
| Content | 4 | 石头/漂流瓶的查询与删除 |
| Reports | 2 | 举报列表与处理 |
| Moderation | 4 | 待审列表、审核历史、通过、拒绝 |
| Sensitive Words | 4 | 敏感词 CRUD |
| Settings | 4 | 系统配置读写、AI 连接测试、广播消息 |
| Logs | 1 | 操作日志查询 |
| Edge AI | 8 | AI 状态/指标/情感脉搏/联邦聚合/隐私预算/向量搜索/配置管理 |
| AI Analysis | 2 | 文本情感分析、文本内容审核 |
| Recommendations | 3 | 热门内容、情感趋势、推荐统计 |

## 状态管理

使用 Pinia Composition API 风格，单一 store（`useAppStore`）管理全局状态：

| 状态 | 说明 |
|------|------|
| `token` | PASETO v4 认证 token，持久化到 `localStorage` |
| `userInfo` | 管理员用户信息（从登录响应获取，PASETO token 不可客户端解码） |
| `isDark` | 暗色模式开关，持久化到 `localStorage`，通过 CSS class `dark` 切换 |
| `isGlobalLoading` | 全局加载状态，基于引用计数支持并发请求 |

提供 `setToken`、`clearToken`、`checkTokenValid`、`toggleDark`、`startLoading`/`stopLoading` 等方法。

## 样式

- 全局主题文件：`src/styles/m3-theme.scss`，基于 Material Design 3 设计语言
- 使用 SCSS 预处理器（`sass ^1.70`）
- Element Plus 组件库样式 + 自定义主题覆盖
- 支持暗色模式切换（通过 `document.documentElement.classList.toggle('dark')` 实现）

## 路由与权限

- 使用 `createWebHistory` 模式（需 nginx 配置 `try_files` 支持）
- 全局前置守卫：未登录用户访问非 `/login` 页面自动跳转登录
- 已登录用户访问 `/login` 自动跳转 `/dashboard`
- 404 路由统一重定向到 `/dashboard`
- 路由懒加载：所有页面组件使用 `() => import()` 动态导入

## 部署

1. 执行 `npm run build`，产物输出到 `dist/` 目录
2. 通过 nginx 提供静态文件服务，参考配置：

```nginx
server {
    listen 80;
    server_name admin.heartlake.com;
    root /path/to/admin/dist;
    index index.html;

    # SPA 路由支持
    location / {
        try_files $uri $uri/ /index.html;
    }

    # API 反向代理
    location /api/ {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }

    # WebSocket 代理
    location /ws {
        proxy_pass http://127.0.0.1:8080;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

项目已集成 Docker 容器化部署，详见根目录 `docker-compose.yml`。
