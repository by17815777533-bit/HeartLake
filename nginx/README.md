# HeartLake Nginx 反向代理

HeartLake 平台的 Nginx 反向代理层，负责请求路由、速率限制、安全头注入、WebSocket 代理和静态资源缓存。

## 架构

```
客户端 → Nginx (:80/:443)
              ├── /api/*     → backend:8080  (C++ Drogon API)
              ├── /ws/*      → backend:8080  (WebSocket)
              └── /*         → admin:80      (Vue 3 管理后台)
```

## 配置文件

| 文件 | 说明 |
|------|------|
| `nginx.conf` | 主配置：worker 进程、安全头、gzip、速率限制 zone 定义 |
| `conf.d/default.conf` | 虚拟主机：upstream 定义、location 路由规则 |

## 安全策略

### HTTP 安全头

| Header | 值 | 作用 |
|--------|-----|------|
| `X-Frame-Options` | `DENY` | 防止点击劫持 |
| `X-Content-Type-Options` | `nosniff` | 防止 MIME 类型嗅探 |
| `X-XSS-Protection` | `1; mode=block` | XSS 过滤 |
| `Referrer-Policy` | `strict-origin-when-cross-origin` | 控制 Referer 泄露 |
| `Permissions-Policy` | 禁用 camera/microphone/geolocation | 限制浏览器 API |
| `Content-Security-Policy` | `default-src 'self'; ...` | 内容安全策略 |
| `Strict-Transport-Security` | `max-age=31536000; includeSubDomains` | 强制 HTTPS |

### 速率限制

| Zone | 速率 | 适用路径 | Burst |
|------|------|----------|-------|
| `api` | 30 req/s | `/api/*` | 50 |
| `login` | 5 req/min | `/api/auth/login`, `/api/admin/login` | 3 |
| `ws` | 10 req/s | `/ws/*` | — |
| `ai_api` | 5 req/s | `/api/ai/*` | 10 |

WebSocket 连接数限制：每 IP 最多 10 个并发连接（`ws_conn` zone）。

## 路由规则

| Location | Upstream | 说明 |
|----------|----------|------|
| `/nginx-health` | — | 健康检查端点，返回 200 |
| `/api/ai/` | backend | 边缘 AI 接口，严格速率限制 |
| `/api/auth/login` | backend | 登录接口，防暴力破解 |
| `/api/admin/login` | backend | 管理员登录，防暴力破解 |
| `/api/` | backend | 通用 API，请求体限制 10MB |
| `/ws/` | backend | WebSocket 代理，超时 3600s |
| `/` | admin | 管理后台 SPA，`try_files` 支持 |
| 静态资源 | — | `.js/.css/.png/.jpg/.svg/.ico/.woff2`，缓存 30 天 |

## 性能优化

- **Gzip 压缩**：level 6，覆盖 text/html、CSS、JS、JSON、XML、SVG
- **静态资源缓存**：30 天过期 + `Cache-Control: public`
- **Worker 进程**：`auto`（自动匹配 CPU 核心数）
- **连接数**：每 worker 1024 连接

## Docker 部署

在 `docker-compose.yml` 中作为 `nginx` 服务运行：

```yaml
nginx:
  image: nginx:alpine
  ports:
    - "80:80"
    - "443:443"
  volumes:
    - ./nginx/nginx.conf:/etc/nginx/nginx.conf:ro
    - ./nginx/conf.d:/etc/nginx/conf.d:ro
  depends_on:
    backend:
      condition: service_healthy
    admin:
      condition: service_started
```

资源限制：128MB 内存 / 0.5 CPU。

## 本地开发

本地开发时无需启动 Nginx，Vite 开发服务器已配置代理：

- `/api` → `http://127.0.0.1:8080`
- `/ws` → `ws://127.0.0.1:8080`

Nginx 仅在 Docker 生产部署时使用。
