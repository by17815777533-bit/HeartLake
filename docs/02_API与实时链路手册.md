# API 与实时链路手册

## 接入地址

| 链路 | 地址 | 说明 |
|------|------|------|
| REST API | `http://localhost:8080/api` | 全部 HTTP 接口 |
| WebSocket | `ws://localhost:8080/ws/broadcast` | 实时消息推送 |
| 管理后台 API | `http://localhost:8080/api/admin` | 管理端数据接口 |

## 认证机制

### PASETO v4 令牌

HeartLake 使用 PASETO v4（Platform-Agnostic Security Tokens）替代 JWT，避免 JWT 的算法混淆攻击风险。令牌通过 `SecurityAuditFilter` 在请求到达 Controller 之前统一校验。

所有需要认证的请求在 Header 中携带：
```
Authorization: Bearer <paseto_token>
```

### 匿名登录

产品定位为低门槛匿名社区，用户无需注册即可使用：

```bash
curl -X POST http://localhost:8080/api/auth/anonymous \
  -H 'Content-Type: application/json' \
  -d '{"device_id":"my_device_001"}'
```

响应：
```json
{
  "code": 0,
  "data": {
    "token": "v4.local.xxx...",
    "user_id": "anonymous_8a92d411ffb0",
    "is_new_user": true,
    "keyword": "星辰-湖畔-微风"
  }
}
```

- `token`: PASETO v4 令牌，后续所有请求的认证凭据
- `user_id`: 系统分配的匿名用户 ID
- `keyword`: 恢复关键词，用于在其他设备找回账号

### 令牌刷新

```bash
curl -X POST http://localhost:8080/api/auth/refresh \
  -H 'Authorization: Bearer <old_token>'
```

### 管理后台认证

管理后台使用独立的认证体系（`AdminAuthFilter`），与用户端隔离：

```bash
curl -X POST http://localhost:8080/api/admin/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"admin","password":"HeartLake"}'
```

## API 分组概览

### 认证与账号

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/auth/anonymous` | 匿名登录 |
| POST | `/api/auth/refresh` | 刷新令牌 |
| POST | `/api/auth/recover` | 关键词恢复账号 |
| GET | `/api/account/info` | 获取个人资料 |
| PUT | `/api/account/profile` | 更新个人资料 |
| POST | `/api/account/avatar` | 更新头像 |
| GET | `/api/account/stats` | 账号统计 |
| GET | `/api/account/devices` | 登录设备列表 |
| DELETE | `/api/account/devices/{id}` | 移除设备 |
| GET | `/api/account/privacy` | 隐私设置 |
| PUT | `/api/account/privacy` | 更新隐私设置 |
| POST | `/api/account/block/{id}` | 拉黑用户 |
| DELETE | `/api/account/unblock/{id}` | 取消拉黑 |
| POST | `/api/account/export` | 数据导出（GDPR） |
| POST | `/api/account/deactivate` | 注销账号（30天可恢复） |
| POST | `/api/account/delete-permanent` | 永久删除 |

### 石头（核心内容）

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/stones` | 发布石头 |
| GET | `/api/stones/{id}` | 石头详情 |
| DELETE | `/api/stones/{id}` | 删除石头 |
| GET | `/api/lake/stones` | 心湖瀑布流（分页） |
| GET | `/api/users/my/stones` | 我的石头 |
| GET | `/api/users/my/emotion-calendar` | 情绪日历 |
| GET | `/api/users/my/emotion-heatmap` | 情绪热力图 |

发布石头时，后端会触发事件链：
1. `StonePublishedEvent` → AI 情感分析（`SentimentAnalyzer`）
2. `EmotionAnalyzedEvent` → 缓存更新 + 情绪追踪（`EmotionTrackingService`）
3. 心理风险评估（`PsychologicalRiskAssessment`）→ 必要时触发 SafeHarbor

### 互动

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/stones/{id}/ripples` | 点涟漪（幂等） |
| GET | `/api/interactions/my/ripples` | 我收到的涟漪 |
| POST | `/api/stones/{id}/collect` | 收藏石头 |
| POST | `/api/reports` | 举报内容 |

涟漪接口设计为幂等操作——重复点击返回 `already_rippled: true` 而非报错，适配前端高频点击场景。

### 纸船（匿名消息）

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/boats` | 发送纸船 |
| GET | `/api/users/my/boats` | 我发出的纸船 |
| GET | `/api/users/my/received-boats` | 收到的纸船 |

### 好友与社交

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/friends/request` | 发送好友请求 |
| GET | `/api/friends` | 好友列表 |
| DELETE | `/api/friends/{id}` | 删除好友（可恢复隐藏） |
| GET | `/api/friends/{id}/messages` | 聊天记录 |
| POST | `/api/friends/{id}/messages` | 发送消息（亲密度 >= 12） |
| POST | `/api/temp-friends/connect` | 创建临时连接 |
| GET | `/api/temp-friends` | 临时好友列表 |

好友删除采用"可恢复隐藏"语义（`mode: intimacy_auto_hidden`），而非硬删除。重新发送好友请求可恢复关系。私聊需要亲密度达到 12 分，已移除的好友返回 403。

### 推荐与搜索

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/recommendations/trending` | 热门推荐 |
| GET | `/api/recommendations/similar-stones/{id}` | 相似石头 |
| GET | `/api/recommendations/emotion-trends` | 情绪趋势 |
| GET | `/api/recommendations/personalized` | 个性化推荐 |
| POST | `/api/vector/search` | 语义向量搜索 |

推荐引擎融合 User-CF / Item-CF / 内容推荐 / UCB 探索 / 图传播五种策略，通过 MMR 重排序保证多样性。

### EdgeAI 引擎

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/edge-ai/analyze` | 文本情感分析 |
| GET | `/api/edge-ai/emotion-pulse` | 实时情绪脉搏 |
| GET | `/api/edge-ai/privacy-budget` | 差分隐私预算 |
| GET | `/api/edge-ai/stats` | 引擎统计信息 |
| GET | `/api/edge-ai/federated/status` | 联邦学习状态 |

### 心理关怀

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/safe-harbor/hotlines` | 心理援助热线 |
| GET | `/api/safe-harbor/self-help` | 自助工具 |
| GET | `/api/safe-harbor/resources` | 关怀资源 |
| GET | `/api/guardian/status` | 守护者状态 |
| POST | `/api/guardian/bindGuardian` | 绑定守护者 |
| POST | `/api/consultation/start` | 开始心理咨询 |

### 管理后台

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/admin/login` | 管理员登录 |
| GET | `/api/admin/dashboard` | 仪表盘数据 |
| GET | `/api/admin/manage/users` | 用户管理 |
| GET | `/api/admin/manage/content` | 内容管理 |
| GET | `/api/admin/manage/moderation` | 审核队列 |
| GET | `/api/admin/manage/sensitive-words` | 敏感词管理 |
| GET | `/api/admin/manage/reports` | 举报管理 |
| GET | `/api/admin/manage/logs` | 操作日志 |
| GET | `/api/admin/manage/settings` | 系统配置 |

## WebSocket 实时通信

### 连接与鉴权

两种鉴权方式，优先使用 Query 参数：

方式一（推荐）— 握手阶段鉴权：
```
ws://localhost:8080/ws/broadcast?token=<url_encoded_token>
```

方式二 — 首包鉴权（兼容旧客户端）：
```json
{"type": "auth", "token": "<token>"}
```

### 房间机制

客户端通过加入房间订阅特定类型的实时事件：

```json
{"type": "join", "room": "lake"}
{"type": "leave", "room": "lake"}
```

### 心跳保活

服务端定期发送 `ping`，客户端回复 `pong`。超时未回复视为断连，服务端主动清理连接资源。

### 实时事件类型

新石头发布：
```json
{
  "type": "new_stone",
  "stone": { "id": "...", "content": "...", "mood": "..." },
  "triggered_by": "anonymous_xxx",
  "timestamp": 1700000000
}
```

涟漪更新：
```json
{
  "type": "ripple_update",
  "stone_id": "stone_xxx",
  "ripple_count": 12,
  "triggered_by": "anonymous_xxx",
  "timestamp": 1700000000
}
```

好友消息：
```json
{
  "type": "new_friend_message",
  "sender_id": "anonymous_xxx",
  "receiver_id": "anonymous_yyy",
  "content": "消息内容",
  "timestamp": 1700000000
}
```

前端通过 `triggered_by` / `sender_id` 判断是否为本机触发，避免重复渲染。时间戳统一为秒级整数。

## 缓存策略

两级缓存体系：
- L1 进程内缓存（`CacheManager`）：LRU + TTL，O(1) 读写，默认 300s
- L2 Redis 缓存（`RedisCache`）：分布式共享，适合多实例部署

一致性保障：
- 写操作成功后，按前缀主动失效相关缓存（如 `stones_user_xxx`）
- WebSocket 事件到达后，客户端主动刷新对应数据
- 短 TTL 作为兜底，即使事件丢失也能在窗口期内自动恢复

## 错误响应格式

所有接口统一返回格式：
```json
{
  "code": 0,
  "message": "success",
  "data": { ... }
}
```

错误码语义：
- `0`: 成功
- `400`: 参数错误 / 业务校验失败
- `401`: 未认证（令牌缺失或过期）
- `403`: 无权限（如亲密度不足、已被拉黑）
- `404`: 资源不存在
- `429`: 请求频率超限（RateLimiter 触发）
- `500`: 服务器内部错误

## 安全过滤器链

每个请求经过的过滤器链路：

```
请求 → RateLimiter → SecurityAuditFilter(PASETO校验) → Controller
                                    ↓
                            审计日志记录
```

管理后台请求：
```
请求 → AdminAuthFilter(独立PASETO校验) → AdminController
```

`SecurityAuditFilter` 从令牌中提取 `user_id` 并注入请求属性，Controller 通过 `req->getAttributes()->get<std::string>("user_id")` 获取当前用户身份。
