# API 与实时链路手册

> 本文档详述 HeartLake 后端的 REST API 接口分组、WebSocket 实时通信协议、认证机制、请求过滤链及缓存策略。

## 1. 接入地址

| 链路 | 地址 | 说明 |
|------|------|------|
| REST API | `http://121.41.195.165/api` | 当前云端 HTTP 接口 |
| WebSocket | `ws://121.41.195.165/ws/broadcast` | 当前云端实时事件推送 |
| 管理后台 API | `http://121.41.195.165/api/admin` | 当前云端管理端接口（独立认证） |

## 2. 认证机制

### 2.1 PASETO v4 令牌

系统采用 PASETO v4 替代 JWT 作为无状态令牌方案，从根本上杜绝 JWT 的算法混淆攻击（`alg: none`）。

所有需要认证的请求须携带以下请求头：

```
Authorization: Bearer <paseto_token>
```

`SecurityAuditFilter` 在请求到达 Controller 之前统一校验令牌有效性，并将解析出的 `user_id` 注入请求属性。

### 2.2 匿名登录

系统采用匿名登录机制，用户无需注册即可使用：

```bash
curl -X POST http://121.41.195.165/api/auth/anonymous \
  -H 'Content-Type: application/json' \
  -d '{"device_id":"my_device_001"}'
```

响应示例：

```json
{
  "code": 0,
  "data": {
    "token": "v4.local.xxx...",
    "refresh_token": "rt_xxx...",
    "user_id": "anonymous_8a92d411ffb0",
    "is_new_user": true,
    "recovery_key": "星辰-湖畔-微风",
    "session_id": "3d25b8f1-5cb2-4c56-8b2f-9f09cbfb1f6d"
  }
}
```

响应字段说明：

| 字段 | 说明 |
|------|------|
| `token` | PASETO v4 令牌，后续请求的认证凭据 |
| `refresh_token` | 长期会话续期凭据，客户端需安全持久化 |
| `user_id` | 系统分配的匿名用户标识 |
| `is_new_user` | 是否为新创建的用户 |
| `recovery_key` | 账号恢复关键词，用于跨设备找回账号 |
| `session_id` | 当前登录设备对应的服务端会话 ID |

### 2.3 令牌刷新

```bash
curl -X POST http://121.41.195.165/api/auth/refresh \
  -H 'Content-Type: application/json' \
  -d '{"refresh_token":"rt_xxx..."}'
```

该接口可直接用请求体里的 `refresh_token` 调用，也接受 `Authorization: Bearer <current_token>` 头；访问令牌仍有效时，服务端会同步回补 `refresh_token` 和 `user_sessions` 记录。

### 2.4 管理后台认证

管理后台使用独立的认证体系（`AdminAuthFilter` + 独立签名密钥），与用户端完全隔离：

```bash
curl -X POST http://121.41.195.165/api/admin/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"admin","password":"<ADMIN_PASSWORD>"}'
```

## 3. 统一响应格式

所有接口遵循统一的响应结构：

```json
{
  "code": 0,
  "message": "success",
  "data": { ... }
}
```

错误码定义：

| 错误码 | 含义 | 典型场景 |
|--------|------|----------|
| `0` | 成功 | — |
| `400` | 参数错误 | 请求体缺少必填字段、字段类型不匹配 |
| `401` | 未认证 | 令牌缺失、令牌过期、令牌无效 |
| `403` | 无权限 | 亲密度不足、用户被拉黑、普通用户访问管理接口 |
| `404` | 资源不存在 | 石头 / 用户 / 好友关系不存在 |
| `429` | 请求限流 | 触发令牌桶限流策略 |
| `500` | 服务端错误 | 内部异常 |

### 3.1 集合响应契约

分页或列表接口在 `data` 节点下统一遵循集合响应契约。后端通过 `ResponseUtil::buildCollectionPayload()` 输出：

- 语义主键，如 `boats`、`notifications`、`topics`、`users`
- 通用别名 `items` 与 `list`
- 分页字段 `total/page/page_size/has_more`
- 聚合分页对象 `pagination`

管理端与移动端统一按“`data` → 语义键 / `items` / `list`”的优先级解析，文档基线只认这套集合契约。

## 4. API 分组

### 4.1 认证与账号

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/auth/anonymous` | 无 | 匿名登录 |
| POST | `/api/auth/refresh` | 无（也接受 Bearer） | 刷新令牌 / 回补会话 |
| POST | `/api/auth/recover` | 无 | 关键词恢复账号 |
| POST | `/api/auth/delete-account` | Bearer | 账号停用别名路由（30 天内可恢复） |
| GET | `/api/account/info` | Bearer | 获取个人资料 |
| PUT | `/api/account/profile` | Bearer | 更新个人资料 |
| POST | `/api/account/avatar` | Bearer | 写入头像 URL（通常配合媒体上传使用） |
| POST | `/api/media/upload` | Bearer | 上传头像 / 图片 / 音视频文件 |
| GET | `/api/media/{category}/{filename}` | 无 | 访问已上传媒体文件 |
| GET | `/api/account/stats` | Bearer | 账号统计（石头数 / 好友数 / 涟漪数） |
| GET | `/api/account/devices` | Bearer | 登录设备列表 |
| DELETE | `/api/account/devices/{id}` | Bearer | 移除登录设备 |
| GET | `/api/account/privacy` | Bearer | 隐私设置 |
| PUT | `/api/account/privacy` | Bearer | 更新隐私设置 |
| POST | `/api/account/block/{targetUserId}` | Bearer | 拉黑用户（幂等） |
| DELETE | `/api/account/unblock/{targetUserId}` | Bearer | 取消拉黑 |
| POST | `/api/account/export` | Bearer | 创建数据导出任务（GDPR） |
| GET | `/api/account/export/{taskId}` | Bearer | 查询导出任务状态 |
| POST | `/api/account/deactivate` | Bearer | 停用账号（30 天内可恢复） |
| POST | `/api/account/delete-permanent` | Bearer | 永久删除账号及全部数据 |

说明：

- `POST /api/account/deactivate` 与 `POST /api/auth/delete-account` 必须显式提供 `confirmation`，接受 `DEACTIVATE` 和 `DELETE`
- `POST /api/account/delete-permanent` 需要请求体携带 `{"confirmation":"DELETE"}`；服务端不再接受空确认
- 二进制头像上传统一走 `POST /api/media/upload`，返回 URL 后再调用 `POST /api/account/avatar` 或 `PUT /api/users/my/profile`
- 移动端登录恢复链现在只接受完整持久化会话；客户端若只剩 `token`、缺失 `user_id`，或 `POST /api/auth/refresh` 成功包缺少 `token/user_id`，都会按失败处理，不再继续放行首页

### 4.2 石头（核心内容）

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/stones` | Bearer | 发布石头 |
| GET | `/api/stones/{id}` | 无 | 石头详情 |
| DELETE | `/api/stones/{id}` | Bearer | 删除自己的石头 |
| GET | `/api/lake/stones` | 无 | 湖面石头流（分页，支持 `page` / `page_size` / `mood` / `sort` 参数） |
| GET | `/api/stones/my` | Bearer | 我的石头列表 |
| GET | `/api/lake/weather` | 无 | 湖面天气（全站情绪氛围统计） |
| GET | `/api/stones/{id}/resonance` | Bearer | 情绪共鸣搜索 |
| GET | `/api/users/my/emotion-calendar` | Bearer | 情绪日历 |
| GET | `/api/users/my/emotion-heatmap` | Bearer | 情绪热力图 |

石头发布后触发的领域事件链：

```
StonePublishedEvent → AI 情感分析 → 情绪追踪 → 心理风险评估 → [CRITICAL] SafeHarbor 触发
```

查询参数说明：
- `sort` 接受 `latest` / `hot`，也接受 `created_at` / `ripple_count` / `boat_count` / `view_count`
- `hot` 当前映射到 `ripple_count`，`latest` 映射到 `created_at`
- `emotion-calendar` / `emotion-heatmap` 的 `days` 必须是对象映射；客户端不再把坏载荷、缺失分数或缺失情绪类型的数据伪装成空日历、空热力图或默认中性记录

`POST /api/stones` 推荐请求体：

```json
{
  "content": "今天想把心事写下来",
  "stone_type": "medium",
  "stone_color": "#7A92A3",
  "mood_type": "calm",
  "tags": ["记录", "夜晚"],
  "is_anonymous": true
}
```

- `stone_type / stone_color / mood_type` 未显式传入时，服务端默认回落到 `medium / #7A92A3 / calm`

### 4.3 互动

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/stones/{id}/ripples` | Bearer | 点涟漪（幂等，重复操作返回 `already_rippled: true`） |
| GET | `/api/interactions/my/ripples` | Bearer | 我发出的涟漪列表 |
| POST | `/api/stones/{id}/collect` | Bearer | 收藏石头 |
| DELETE | `/api/stones/{id}/collect` | Bearer | 取消收藏 |
| GET | `/api/interactions/my/collections` | Bearer | 我的收藏列表 |

### 4.4 纸船

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/boats/reply` | Bearer | 语义化纸船回复入口 |
| POST | `/api/stones/{id}/boats` | Bearer | 互动主链纸船回复入口 |
| GET | `/api/boats/sent` | Bearer | 我发出的纸船 |
| GET | `/api/boats/received` | Bearer | 收到的纸船 |
| GET | `/api/boats/{boatId}` | Bearer | 纸船详情 |

说明：

- `/api/boats/reply` 与 `/api/stones/{id}/boats` 现在共享同一条应用服务链，纸船计数、通知入库、临时好友续期、风险评估和缓存失效保持一致
- 实时广播仍由 Controller 负责，业务落库和副作用编排统一由 `InteractionApplicationService` 处理

### 4.5 好友

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/friends/request` | Bearer | 恢复隐藏关系 / 返回亲密分状态 |
| POST | `/api/friends/accept/{id}` | Bearer | 不可用，固定返回 400 |
| POST | `/api/friends/reject/{id}` | Bearer | 不可用，固定返回 400 |
| GET | `/api/friends` | Bearer | 自动关系好友列表 |
| GET | `/api/friends/requests/pending` | Bearer | 不可用，固定返回 400 |
| DELETE | `/api/friends/{id}` | Bearer | 删除好友（软删除，返回 `mode: intimacy_auto_hidden`） |
| GET | `/api/friends/{id}/messages` | Bearer | 聊天记录 |
| POST | `/api/friends/{id}/messages` | Bearer | 发送消息（要求亲密度 >= 12） |
| GET | `/api/temp-friends` | Bearer | 临时好友列表 |
| GET | `/api/temp-friends/{id}` | Bearer | 临时好友详情 |
| DELETE | `/api/temp-friends/{id}` | Bearer | 断开临时连接 |
| GET | `/api/temp-friends/check/{userId}` | Bearer | 检查临时好友状态 |

好友删除机制说明：
- 删除操作为软删除，返回 `mode: intimacy_auto_hidden`
- 重新发送好友请求可恢复关系
- `POST /api/friends/accept/{id}`、`POST /api/friends/reject/{id}` 与 `GET /api/friends/requests/pending` 当前不可用，固定返回 400
- 已删除好友间发送消息返回 403
- 临时连接一旦过期，对应的 connection 历史读取与发消息入口都会拒绝访问
- 任一方建立拉黑关系后，石头派生 connection、纸船派生 connection 的后续建连、历史读取、发消息都会统一返回 403
- `POST /api/temp-friends` 与 `POST /api/temp-friends/{id}/upgrade` 当前不可用，固定返回 400
- 石头回复链和纸船链派生的临时连接会保留业务来源（如 `stone` / `boat`）

### 4.6 推荐与搜索

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| GET | `/api/recommendations/trending` | Bearer | 热门推荐 |
| GET | `/api/recommendations/similar-stones/{id}` | Bearer | 相似石头推荐 |
| GET | `/api/recommendations/emotion-trends` | Bearer | 情绪趋势 |
| GET | `/api/recommendations/stones` | Bearer | 个性化推荐 |
| GET | `/api/recommendations/advanced` | Bearer | 高级共鸣推荐，返回 `algorithm / reference_stone_id / reference_source` 等字段 |
| GET | `/api/admin/recommendations/advanced` | Admin Bearer | 管理端查看指定 `user_id` 的高级推荐结果 |
| POST | `/api/vector/search` | Bearer | 语义向量搜索（返回相似度，并尽量回填 stone 内容预览） |
| GET | `/api/vector/stats` | Bearer | 向量索引统计 |

`/api/recommendations/stones` 是个性化推荐链，融合 User-CF / Item-CF / 内容匹配 / UCB 探索 / 图传播五种策略，并通过 MMR 重排序保证结果多样性。
`/api/recommendations/advanced` 与 `/api/admin/recommendations/advanced` 是同口径的高级推荐链：有参考石头时先返回 `emotion_temporal_resonance` 结果，剩余名额再由 `multi_armed_bandit_mmr` 高级融合引擎补齐；显式传入的 `stone_id` 必须是有效已发布石头，否则返回 `404`。管理端用户详情固定使用 `/api/admin/recommendations/advanced?user_id=...` 核对 `algorithm / reference_stone_id / reference_source`，不再把普通推荐结果当成高级链路展示。

### 4.7 EdgeAI

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/edge-ai/analyze` | Bearer | 文本情感分析 |
| GET | `/api/edge-ai/emotion-pulse` | Bearer | 实时情绪脉搏 |
| GET | `/api/edge-ai/privacy-budget` | Bearer | 差分隐私预算查询 |
| GET | `/api/edge-ai/stats` | Bearer | 引擎统计信息 |
| GET | `/api/edge-ai/federated/status` | Bearer | 联邦学习状态 |

情感分析请求示例：

```bash
curl -X POST http://121.41.195.165/api/edge-ai/analyze \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"text":"待分析文本"}'
```

响应示例：

```json
{
  "score": 0.65,
  "mood": "happy",
  "confidence": 0.82,
  "method": "lexicon+rule+statistical"
}
```

### 4.8 心理关怀

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| GET | `/api/safe-harbor/hotlines` | Bearer | 心理援助热线 |
| GET | `/api/safe-harbor/self-help` | Bearer | 自助工具 |
| GET | `/api/safe-harbor/resources` | Bearer | 关怀资源列表 |
| POST | `/api/safe-harbor/resources` | Bearer | 添加关怀资源 |
| PUT | `/api/safe-harbor/resources/{id}` | Bearer | 更新关怀资源 |
| DELETE | `/api/safe-harbor/resources/{id}` | Bearer | 删除关怀资源 |
| GET | `/api/safe-harbor/recommend` | Bearer | 按情绪类型推荐资源，请求参数接受 `emotion` 或 `mood` |
| GET | `/api/guardian/stats` | Bearer | 守望统计 |
| GET | `/api/guardian` | Bearer | 守望统计简写 |
| POST | `/api/guardian/transfer-lamp` | Bearer | 转赠灯火 |
| GET | `/api/guardian/insights` | Bearer | 获取情绪洞察 |
| POST | `/api/guardian/chat` | Bearer | 湖神陪伴对话 |
| POST | `/api/consultation/session` | Bearer | 创建心理咨询会话 |
| POST | `/api/consultation/key-exchange` | Bearer | 完成咨询会话密钥交换 |
| POST | `/api/consultation/message` | Bearer | 发送咨询消息 |
| GET | `/api/consultation/messages/{sessionId}` | Bearer | 咨询消息分页列表 |
| GET | `/api/consultation/sessions` | Bearer | 当前用户咨询会话列表 |

咨询链路约定：

- `key-exchange` 发送 `client_public_key`，服务端返回 `server_public_key + salt`
- `message` 发送结构化 AES-GCM envelope：`encrypted.ciphertext / encrypted.iv / encrypted.tag`
- `messages` / `sessions` 都保留标准分页字段，客户端按集合载荷解析
- `sessions` 额外返回 `counselor_name/counselor_avatar_url/last_message/updated_at`，并补齐 `counterpart_id`，便于用户端与咨询师端使用同一套会话列表结构
- `guardian/chat` 与 `/api/lake-god/chat` 在本地降级时会显式返回 `response_source=local_fallback`、`degraded=true` 与 `warning/ai_error`，不再把本地陪伴回复伪装成远端 AI 成功

### 4.9 其他

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/reports` | Bearer | 提交内容举报 |
| GET | `/api/reports/my` | Bearer | 我的举报记录 |
| GET | `/api/privacy/settings` | Bearer | 隐私设置 |
| PUT | `/api/privacy/settings` | Bearer | 更新隐私设置 |
| GET | `/api/vip/status` | Bearer | 灯火状态 |
| GET | `/api/vip/privileges` | Bearer | 灯火权益列表 |
| GET | `/api/vip/counseling/check` | Bearer | 免费咨询额度检查 |
| POST | `/api/vip/counseling/book` | Bearer | 预约心理咨询 |
| GET | `/api/vip/ai-comment-frequency` | Bearer | AI 评论频率配置 |

### 4.10 管理后台

所有管理后台接口需要 Admin Bearer 认证（独立签名密钥）。

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/admin/login` | 管理员登录 |
| POST | `/api/admin/logout` | 管理员登出 |
| GET | `/api/admin/info` | 当前管理员信息 |
| GET | `/api/admin/stats/dashboard` | 仪表盘综合统计 |
| GET | `/api/admin/stats/realtime` | 实时统计 |
| GET | `/api/admin/stats/user-growth` | 用户增长趋势 |
| GET | `/api/admin/stats/mood-distribution` | 情绪分布 |
| GET | `/api/admin/stats/mood-trend` | 情绪趋势 |
| GET | `/api/admin/stats/trending-topics` | 热门话题 |
| GET | `/api/admin/stats/active-time` | 活跃时段统计 |
| GET | `/api/admin/risk/high-risk-users` | 高风险用户列表 |
| GET | `/api/admin/risk/events` | 风险事件列表 |

说明：

- Dashboard 与主布局实时状态在接口失败时保留最近一次成功结果，但必须同时暴露 stale / warning 状态；管理端不再把接口失败静默降级成“看起来正常”
- `stats/dashboard`、`stats/realtime` 与 WebSocket `stats_update` 共同构成后台统计链；客户端若收到未知消息类型、解析失败或心跳超时，会进入显式 `socket_error/socket_status` 事件流，而不是只在控制台打印
| GET | `/api/admin/risk/user/{user_id}/history` | 用户风险历史 |
| POST | `/api/admin/risk/event/{event_id}/handle` | 处置风险事件 |
| GET | `/api/admin/security/audit` | 安全审计日志 |
| GET | `/api/admin/users` | 用户列表（分页 / 搜索） |
| GET | `/api/admin/users/{id}` | 用户详情 |
| PUT | `/api/admin/users/{id}/status` | 更新用户状态 |
| POST | `/api/admin/users/{id}/ban` | 封禁用户 |
| POST | `/api/admin/users/{id}/unban` | 解封用户 |
| GET | `/api/admin/content` | 统一内容列表（石头 + 纸船，数据库统一分页） |
| GET | `/api/admin/stones` | 石头列表 |
| GET | `/api/admin/stones/{id}` | 石头详情 |
| DELETE | `/api/admin/stones/{id}` | 删除石头 |
| GET | `/api/admin/boats` | 纸船列表 |
| DELETE | `/api/admin/boats/{id}` | 删除纸船 |
| GET | `/api/admin/moderation/pending` | 待审核队列 |
| GET | `/api/admin/moderation/history` | 审核历史 |
| POST | `/api/admin/moderation/{id}/approve` | 审核通过 |
| POST | `/api/admin/moderation/{id}/reject` | 审核拒绝 |
| GET | `/api/admin/sensitive-words` | 敏感词列表 |
| POST | `/api/admin/sensitive-words` | 添加敏感词 |
| PUT | `/api/admin/sensitive-words/{id}` | 更新敏感词 |
| DELETE | `/api/admin/sensitive-words/{id}` | 删除敏感词 |
| GET | `/api/admin/reports` | 举报列表 |
| GET | `/api/admin/reports/{id}` | 举报详情 |
| POST | `/api/admin/reports/{id}/handle` | 处理举报 |
| GET | `/api/admin/logs` | 操作日志 |
| GET | `/api/admin/config` | 系统配置 |
| PUT | `/api/admin/config` | 更新系统配置 |
| POST | `/api/admin/broadcast` | 发送全站广播 |
| GET | `/api/admin/broadcast/history` | 广播历史 |

管理端 `stats/*`、`risk/*` 与后台分页接口统一按集合响应契约输出。前端按 `data` 节点中的语义键、`items`、`list` 顺序读取。

## 5. WebSocket 实时通信

### 5.1 连接鉴权

方式一（推荐）：URL 参数鉴权

```
ws://121.41.195.165/ws/broadcast?token=<url_encoded_token>
```

服务端鉴权成功后会主动回：

```json
{"type":"auth_success","user_id":"anonymous_xxx","authenticated":true}
```

### 5.2 房间订阅

客户端通过加入 / 离开房间来订阅特定事件：

```json
{"type": "join", "room": "lake"}
{"type": "leave", "room": "lake"}
```

客户端向房间发消息时也必须先 `join`，服务端会校验私有房间参与者身份，并且只信任客户端提交的业务内容字段；`sender_id`、时间戳和消息类型由服务端统一盖章，避免伪造。

### 5.3 心跳保活

服务端定期发送 `ping`，客户端须回复 `pong`。超时未回复则服务端断开连接并清理资源。
客户端只有在收到 `auth_success` 后才应视为“连接成功”，再回放房间订阅和离线消息。
当前版本只支持握手阶段 URL `token` 鉴权，连接建立后发送 `auth` 首包不会触发认证。

### 5.4 事件类型

新石头发布：

```json
{
  "type": "new_stone",
  "event": "new_stone",
  "stone": {"stone_id": "...", "content": "...", "mood_type": "..."},
  "triggered_by": "anonymous_xxx",
  "timestamp": 1700000000
}
```

涟漪更新：

```json
{
  "type": "ripple_update",
  "event": "ripple_update",
  "stone_id": "stone_xxx",
  "ripple_count": 12,
  "triggered_by": "anonymous_xxx",
  "sender_id": "anonymous_xxx",
  "stone_owner_id": "anonymous_owner",
  "timestamp": 1700000000
}
```

纸船更新：

```json
{
  "type": "boat_update",
  "event": "boat_update",
  "stone_id": "stone_xxx",
  "boat_id": "boat_xxx",
  "boat_count": 3,
  "triggered_by": "anonymous_xxx",
  "sender_id": "anonymous_xxx",
  "stone_owner_id": "anonymous_owner",
  "timestamp": 1700000000
}
```

通知推送：

```json
{
  "type": "new_notification",
  "event": "new_notification",
  "notification_id": "notif_xxx",
  "title": "湖面来信",
  "notification_type": "boat",
  "content": "有人给你的石头回了一封纸船",
  "related_id": "boat_xxx",
  "related_type": "boat",
  "is_read": false,
  "unread_count": 3,
  "timestamp": 1700000000
}
```

说明：

- 广播事件统一为扁平结构，核心字段至少包含 `type / event / timestamp`
- 常见标识会镜像 snake / camel 别名，例如 `stone_id / stoneId`
- `new_notification.type` 固定为事件名，实际通知语义请读取 `notification_type`
- 管理端与移动端应直接消费标准事件，不依赖额外的 `data/payload` 嵌套结构

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

通知：

```json
{
  "type": "notification",
  "...": "..."
}
```

客户端通过 `triggered_by` / `sender_id` 判断操作者，通过 `stone_owner_id` 判断石头归属者，避免全量刷新。时间戳为秒级 Unix 时间戳。

## 6. 请求过滤链

### 6.1 用户端请求

```
请求 → RateLimitFilter（令牌桶限流） → SecurityAuditFilter（PASETO v4 校验 + 审计日志） → Controller
```

`SecurityAuditFilter` 从令牌中提取 `user_id` 并注入请求属性，Controller 通过 `req->getAttributes()->get<std::string>("user_id")` 获取当前用户标识。

### 6.2 管理后台请求

```
请求 → AdminAuthFilter（独立 PASETO v4 校验） → AdminController
```

管理后台使用独立的签名密钥（`ADMIN_PASETO_KEY`），与用户端认证体系完全隔离。

## 7. 缓存策略

### 7.1 两级缓存架构

```
请求 → CacheManager（L1 进程内） → RedisCache（L2 分布式） → PostgreSQL
```

- L1 `CacheManager`：LRU 双向链表 + `unordered_map` 索引，O(1) 读写，`shared_mutex` 读写锁保护，默认容量 10000 条，TTL 300 秒
- L2 `RedisCache`：分布式共享缓存，适用于多实例部署场景

### 7.2 一致性保障

- 写后失效：写操作成功后按前缀主动失效相关缓存（`invalidatePattern`）
- 事件驱动刷新：WebSocket 事件到达后客户端主动刷新数据
- TTL 兜底：短 TTL 确保即使事件丢失，数据也能在窗口期内恢复一致

2026-03 补充的热路径缓存规则：
- 石头列表改成共享基础结果缓存：`stone_list:v2:*` 不再绑定 `currentUserId`，跨用户复用同一页结果
- 用户-石头的互动状态改成独立短 TTL 缓存：`stone:rippled:{userId}:{stoneId}`
- 石头详情、石头列表、用户主页在发布石头、删除石头、创建/删除涟漪、创建/删除纸船、好友关系变化后统一失效，避免三端看到不一致计数
