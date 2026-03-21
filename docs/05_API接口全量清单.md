# API 接口全量清单

> 本文档列出 HeartLake 后端全部 HTTP 接口，按 Controller 分组。所有接口基地址为 `http://localhost:8080`。

## 统一响应格式

```json
{
  "code": 0,
  "message": "success",
  "data": { ... }
}
```

错误码定义：

| 错误码 | 含义 |
|--------|------|
| `0` | 成功 |
| `400` | 参数错误 |
| `401` | 未认证（令牌缺失、过期或无效） |
| `403` | 无权限 |
| `404` | 资源不存在 |
| `429` | 请求限流 |
| `500` | 服务端错误 |

认证方式：`Authorization: Bearer <paseto_token>`

---

## HealthController — 健康检查

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| GET | `/api/health` | 无 | 基础健康检查 |
| GET | `/api/health/detailed` | 无 | 详细健康检查（含数据库 / Redis / AI 状态） |

---

## UserController — 用户认证

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/auth/anonymous` | 无 | 匿名登录 |
| POST | `/api/auth/refresh` | Bearer | 刷新令牌 |
| POST | `/api/auth/recover` | 无 | 关键词恢复账号 |
| GET | `/api/users/my/emotion-calendar` | Bearer | 情绪日历 |
| GET | `/api/users/my/emotion-heatmap` | Bearer | 情绪热力图 |
| GET | `/api/users/my/boats` | Bearer | 我发出的纸船 |
| GET | `/api/users/my/received-boats` | Bearer | 收到的纸船 |

POST `/api/auth/anonymous`

请求体：

```json
{"device_id": "设备唯一标识"}
```

响应：

```json
{
  "token": "v4.local.xxx",
  "user_id": "anonymous_xxx",
  "is_new_user": true,
  "keyword": "星辰-湖畔-微风"
}
```

| 字段 | 说明 |
|------|------|
| `token` | PASETO v4 令牌，后续请求的认证凭据 |
| `user_id` | 系统分配的匿名用户标识 |
| `is_new_user` | 是否为新创建的用户 |
| `keyword` | 账号恢复关键词，用于跨设备找回账号 |

---

## AccountController — 账号管理

所有端点需要 Bearer 认证。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/account/info` | 获取个人资料 |
| POST | `/api/account/avatar` | 更新头像 |
| PUT | `/api/account/profile` | 更新个人资料 |
| GET | `/api/account/stats` | 账号统计（石头数 / 好友数 / 涟漪数） |
| GET | `/api/account/devices` | 登录设备列表 |
| DELETE | `/api/account/devices/{sessionId}` | 移除登录设备 |
| GET | `/api/account/login-logs` | 最近 50 条登录日志 |
| GET | `/api/account/security-events` | 最近 50 条安全事件 |
| GET | `/api/account/privacy` | 隐私设置 |
| PUT | `/api/account/privacy` | 更新隐私设置 |
| GET | `/api/account/blocked-users` | 拉黑用户列表 |
| POST | `/api/account/block/{targetUserId}` | 拉黑用户（幂等） |
| DELETE | `/api/account/unblock/{targetUserId}` | 取消拉黑 |
| POST | `/api/account/export` | 创建数据导出任务（GDPR） |
| GET | `/api/account/export/{taskId}` | 查询导出任务状态 |
| POST | `/api/account/deactivate` | 注销账号（30 天内可恢复） |
| POST | `/api/account/delete-permanent` | 永久删除账号及全部数据 |

PUT `/api/account/profile` 请求体（所有字段可选）：

```json
{
  "nickname": "新昵称",
  "bio": "个性签名",
  "gender": "male",
  "birthday": "2000-01-01"
}
```

POST `/api/account/delete-permanent` 请求体：

```json
{"confirmation": "DELETE"}
```

---

## StoneController — 石头（核心内容）

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/stones` | Bearer | 发布石头 |
| GET | `/api/lake/stones` | 无 | 湖面石头流（分页） |
| GET | `/api/stones/my` | Bearer | 我的石头列表 |
| GET | `/api/stones/{stoneId}` | 无 | 石头详情 |
| DELETE | `/api/stones/{stoneId}` | Bearer | 删除自己的石头 |
| GET | `/api/lake/weather` | 无 | 湖面天气（全站情绪氛围统计） |
| GET | `/api/stones/{stoneId}/resonance` | Bearer | 情绪共鸣搜索 |

POST `/api/stones` 请求体：

```json
{
  "content": "心事内容",
  "mood": "calm|happy|sad|anxious|angry|hopeful",
  "tags": ["标签1", "标签2"],
  "anonymous": true
}
```

GET `/api/lake/stones` 查询参数：`page`、`page_size`、`mood`、`sort`

---

## InteractionController — 互动

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/stones/{stoneId}/ripples` | Bearer | 点涟漪（幂等） |
| GET | `/api/interactions/my/ripples` | Bearer | 我收到的涟漪 |
| POST | `/api/stones/{stoneId}/collect` | Bearer | 收藏石头 |
| DELETE | `/api/stones/{stoneId}/collect` | Bearer | 取消收藏 |
| GET | `/api/interactions/my/collections` | Bearer | 我的收藏 |

涟漪接口为幂等设计：重复操作返回 `already_rippled: true`，不返回错误。

---

## PaperBoatController — 纸船

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/boats` | Bearer | 发送纸船 |
| GET | `/api/boats/my` | Bearer | 我发出的纸船 |
| GET | `/api/boats/received` | Bearer | 收到的纸船 |
| GET | `/api/boats/{boatId}` | Bearer | 纸船详情 |

---

## FriendController — 好友

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/friends/request` | Bearer | 发送好友请求 |
| GET | `/api/friends` | Bearer | 好友列表 |
| DELETE | `/api/friends/{friendId}` | Bearer | 删除好友（软删除，可恢复） |
| GET | `/api/friends/{friendId}/messages` | Bearer | 聊天记录 |
| POST | `/api/friends/{friendId}/messages` | Bearer | 发送消息（要求亲密度 >= 12） |

删除好友返回 `mode: intimacy_auto_hidden`，重新发送好友请求可恢复关系。已删除好友间发送消息返回 403。

---

## TempFriendController — 临时连接

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/temp-friends/connect` | Bearer | 创建临时连接 |
| GET | `/api/temp-friends` | Bearer | 临时好友列表 |
| DELETE | `/api/temp-friends/{id}` | Bearer | 断开临时连接 |

---

## RecommendationController — 推荐

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| GET | `/api/recommendations/trending` | Bearer | 热门推荐 |
| GET | `/api/recommendations/similar-stones/{stoneId}` | Bearer | 相似石头推荐 |
| GET | `/api/recommendations/emotion-trends` | Bearer | 情绪趋势 |
| GET | `/api/recommendations/personalized` | Bearer | 个性化推荐 |

---

## VectorSearchController — 语义搜索

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/vector/search` | Bearer | 语义向量搜索 |
| GET | `/api/vector/stats` | Bearer | 向量索引统计 |

---

## EdgeAIController — AI 引擎

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/edge-ai/analyze` | Bearer | 文本情感分析 |
| GET | `/api/edge-ai/emotion-pulse` | Bearer | 实时情绪脉搏 |
| GET | `/api/edge-ai/privacy-budget` | Bearer | 差分隐私预算查询 |
| GET | `/api/edge-ai/stats` | Bearer | 引擎统计信息 |
| GET | `/api/edge-ai/federated/status` | Bearer | 联邦学习状态 |

POST `/api/edge-ai/analyze` 请求体：

```json
{"text": "待分析文本"}
```

响应：

```json
{
  "score": 0.65,
  "mood": "happy",
  "confidence": 0.82,
  "method": "lexicon+rule+statistical"
}
```

| 字段 | 说明 |
|------|------|
| `score` | 情感分数，范围 [-1.0, 1.0] |
| `mood` | 情绪类型（happy / sad / angry / anxious / neutral 等） |
| `confidence` | 置信度，范围 [0.0, 1.0] |
| `method` | 使用的分析方法 |

---

## SafeHarborController — 安全港

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| GET | `/api/safe-harbor/hotlines` | Bearer | 心理援助热线 |
| GET | `/api/safe-harbor/self-help` | Bearer | 自助工具 |
| GET | `/api/safe-harbor/resources` | Bearer | 关怀资源列表 |
| POST | `/api/safe-harbor/resources` | Bearer | 添加关怀资源 |
| PUT | `/api/safe-harbor/resources/{id}` | Bearer | 更新关怀资源 |
| DELETE | `/api/safe-harbor/resources/{id}` | Bearer | 删除关怀资源 |
| GET | `/api/safe-harbor/recommend` | Bearer | 按情绪类型推荐资源 |

---

## GuardianController — 守护者

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| GET | `/api/guardian/status` | Bearer | 守护者状态 |
| POST | `/api/guardian/bindGuardian` | Bearer | 绑定守护者 |
| DELETE | `/api/guardian/unbind` | Bearer | 解除守护关系 |
| GET | `/api/guardian/alerts` | Bearer | 守护告警列表 |

---

## ConsultationController — 心理咨询

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/consultation/start` | Bearer | 开始咨询会话 |
| POST | `/api/consultation/message` | Bearer | 发送咨询消息 |
| GET | `/api/consultation/history` | Bearer | 咨询历史记录 |

---

## ReportController — 举报

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/reports` | Bearer | 提交举报 |
| GET | `/api/reports/my` | Bearer | 我的举报记录 |

---

## PrivacyController — 隐私设置

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| GET | `/api/privacy/settings` | Bearer | 获取隐私设置 |
| PUT | `/api/privacy/settings` | Bearer | 更新隐私设置 |

---

## VIPController — 会员

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| GET | `/api/vip/status` | Bearer | VIP 状态查询 |
| POST | `/api/vip/activate` | Bearer | 激活 VIP |
| GET | `/api/vip/benefits` | Bearer | VIP 权益列表 |

---

## BroadcastWebSocketController — WebSocket 实时通信

| 路径 | 说明 |
|------|------|
| `ws://localhost:8080/ws/broadcast` | 实时消息推送 |

鉴权方式：

- URL 参数：`?token=<url_encoded_token>`（推荐）
- 首包鉴权：`{"type":"auth","token":"<token>"}`

事件类型：`new_stone` / `ripple_update` / `new_friend_message` / `notification`

---

## 管理后台接口

### AdminController — 管理数据

所有端点需要 Admin Bearer 认证（独立签名密钥）。

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
| GET | `/api/admin/risk/user/{user_id}/history` | 用户风险历史 |
| POST | `/api/admin/risk/event/{event_id}/handle` | 处置风险事件 |
| GET | `/api/admin/security/audit` | 安全审计日志 |

### AdminManagementController — 管理操作

分页型后台接口当前统一向集合响应契约收敛：推荐从 `data` 节点中的语义键、`items`、`list` 读取列表，并使用 `pagination` 或顶层分页字段读取页码信息。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/users` | 用户列表（分页 / 搜索） |
| GET | `/api/admin/users/{id}` | 用户详情 |
| PUT | `/api/admin/users/{id}/status` | 更新用户状态 |
| POST | `/api/admin/users/{id}/ban` | 封禁用户 |
| POST | `/api/admin/users/{id}/unban` | 解封用户 |
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
| GET | `/api/admin/config` | 系统配置 |
| PUT | `/api/admin/config` | 更新系统配置 |
| POST | `/api/admin/broadcast` | 发送全站广播 |
| GET | `/api/admin/broadcast/history` | 广播历史 |
| GET | `/api/admin/logs` | 操作日志 |
