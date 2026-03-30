# API 与实时链路手册

系统只有一套统一入口：

- API：`http://121.41.195.165/api`
- 管理 API：`http://121.41.195.165/api/admin`
- WebSocket：`ws://121.41.195.165/ws/broadcast`

## 1. 链路总览

```text
+----------------------+      +----------------------+      +----------------------+
| Flutter / Admin      |----->| Gateway              |----->| Drogon API / WS      |
| HTTP / WebSocket     |      | /api /ws             |      | Controllers          |
+----------------------+      +----------------------+      +----------+-----------+
                                                                        |
                                         +------------------------------+------------------------------+
                                         |                                                             |
                                         v                                                             v
                               +----------------------+                                     +----------------------+
                               | PostgreSQL / Redis   |                                     | WebSocketHub         |
                               | 查询 / 写入 / 缓存   |                                     | buildRealtimeEvent() |
                               +----------------------+                                     +----------+-----------+
                                                                                                      |
                                                                                                      v
                                                                                           +----------------------+
                                                                                           | Flutter / Admin      |
                                                                                           | 实时事件广播         |
                                                                                           +----------------------+
```

## 2. 认证

- 用户端：Bearer PASETO
- 管理端：管理员专用 Bearer PASETO
- WebSocket：连接时通过 query `token` 鉴权

## 3. HTTP 响应约定

### 业务壳

- `code`
- `message`
- `data`

成功时 `code = 0`。失败返回明确错误码和错误信息。

### 集合壳

- `items`
- `total`
- `page`
- `page_size`

页面和管理端都直接消费这套集合壳，不自行推导总数。

### 错误语义

- 契约错误：直接报错
- 权限错误：直接返回鉴权失败
- AI / 推荐降级：显式标注 `degraded` 或来源字段

## 4. WebSocket 约定

### 握手成功消息

- `type = auth_success`
- `user_id`
- `authenticated`
- `timestamp`

### 常用消息类型

- 客户端发：`join`、`leave`、`room_message`、`ping`
- 服务端发：`auth_success`、`pong`、`error`、业务广播事件

### 业务事件

- `new_stone`
- `stone_deleted`
- `ripple_update`
- `ripple_deleted`
- `boat_update`
- `boat_deleted`
- `new_notification`
- `new_friend_message`
- `friend_removed`
- `temp_friend_expired`
- `new_report`
- `new_moderation`
- `broadcast`
- `stats_update`

### 统一事件字段

- `type`
- `event`
- `timestamp`

常用 ID 字段会同时带 snake_case 和 camelCase。

## 5. 业务主链

- 内容链：投石、湖面流、石头详情、删除、共鸣搜索
- 互动链：涟漪、纸船、通知、连接消息
- 关系链：好友、临时好友、守护
- 情绪链：情绪日历、热力图、情绪趋势、脉搏
- AI 链：推荐、搜索、EdgeAI、湖神、安全港、咨询、VIP
- 管理链：Dashboard、审核、举报、配置、日志、广播

## 6. AI 接口分组

### Edge AI 公开接口

- `GET /api/edge-ai/status`
- `GET /api/edge-ai/metrics`
- `POST /api/edge-ai/analyze`
- `POST /api/edge-ai/moderate`
- `GET /api/edge-ai/emotion-pulse`
- `POST /api/edge-ai/federated/aggregate`
- `GET /api/edge-ai/privacy-budget`
- `POST /api/edge-ai/vector-search`
- `POST /api/edge-ai/vector-insert`
- `POST /api/edge-ai/emotion-sample`

### 推荐与共鸣接口

- `GET /api/recommendations/trending`
- `GET /api/recommendations/discover/{mood}`
- `GET /api/recommendations/advanced`
- `GET /api/recommendations/personalized`
- `GET /api/stones/{id}/resonance`

### 湖神与洞察接口

- `GET /api/guardian/insights`
- `POST /api/guardian/chat`

## 7. 八大 Edge AI 子系统对应到接口

1. `SentimentAnalyzer`
   暴露在 `POST /api/edge-ai/analyze`
2. `ContentModerator`
   暴露在 `POST /api/edge-ai/moderate`
3. `EmotionPulseDetector`
   暴露在 `GET /api/edge-ai/emotion-pulse` 与 `POST /api/edge-ai/emotion-sample`
4. `FederatedLearner`
   暴露在 `POST /api/edge-ai/federated/aggregate`
5. `EdgeDifferentialPrivacy`
   暴露在 `GET /api/edge-ai/privacy-budget`
6. `HNSWIndex`
   暴露在 `POST /api/edge-ai/vector-search` 与 `POST /api/edge-ai/vector-insert`
7. `ModelQuantizer`
   通过本地推理链间接服务 `analyze` 等接口
8. `EdgeNodeMonitor`
   通过 `status`、`metrics` 和管理端 `EdgeAI` 页面暴露

## 8. 相关手册

- 详细接口： [05_API接口全量清单.md](05_API接口全量清单.md)
- 架构总览： [04_技术实现全景手册.md](04_技术实现全景手册.md)
- 压测结果： [06_测试验证与压测手册.md](06_测试验证与压测手册.md)
