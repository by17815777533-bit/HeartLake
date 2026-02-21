# HeartLake API Reference

> 本文档覆盖 HeartLake 后端全部 21 个 Controller、179+ 个 API 端点。
> 基于 `backend/include/interfaces/api/*.h` 源码自动整理，最后更新：2026-02-21。

## 目录

1. [认证与用户](#1-认证与用户)
2. [内容管理](#2-内容管理)
3. [社交互动](#3-社交互动)
4. [守望者与咨询](#4-守望者与咨询)
5. [推荐与搜索](#5-推荐与搜索)
6. [边缘AI](#6-边缘ai)
7. [安全与隐私](#7-安全与隐私)
8. [VIP](#8-vip)
9. [系统](#9-系统)
10. [管理后台](#10-管理后台)

---

## 通用约定

### 基础URL

```
https://api.heartlake.app
```

### 统一响应格式

```json
{
  "code": 200,
  "message": "ok",
  "data": { ... },
  "timestamp": "2025-06-15T12:00:00Z"
}
```

### 认证方式

| 类型 | Header | 说明 |
|------|--------|------|
| 用户认证 | `Authorization: Bearer <token>` | SecurityAuditFilter，大部分用户接口 |
| 管理员认证 | `Authorization: Bearer <PASETO-token>` | AdminAuthFilter，管理后台接口 |
| 无需认证 | - | 健康检查、公开浏览等 |

### 错误码

| code | 说明 |
|------|------|
| 200 | 成功 |
| 400 | 请求参数错误 |
| 401 | 未认证 |
| 403 | 无权限 |
| 404 | 资源不存在 |
| 409 | 冲突（如重复注册） |
| 429 | 请求过于频繁 |
| 500 | 服务器内部错误 |

---

## 1. 认证与用户

### 1.1 UserController（19 个端点）

#### 匿名登录

```
POST /api/auth/anonymous
```

无需认证。创建匿名用户并返回 token。

**响应示例：**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "token": "v4.public.xxx",
    "userId": "uuid-xxx",
    "isAnonymous": true
  }
}
```

#### 用户注册

```
POST /api/auth/register
POST /api/v1/auth/register
```

无需认证。

**请求体：**

```json
{
  "username": "heartlaker",
  "password": "SecurePass123!",
  "nickname": "湖心人"
}
```

#### 邮箱注册

```
POST /api/auth/register/email
```

无需认证。

**请求体：**

```json
{
  "email": "user@example.com",
  "password": "SecurePass123!",
  "verificationCode": "123456",
  "nickname": "湖心人"
}
```

#### 用户登录

```
POST /api/auth/login
POST /api/v1/auth/login
```

无需认证。

**请求体：**

```json
{
  "username": "heartlaker",
  "password": "SecurePass123!"
}
```

#### 邮箱登录

```
POST /api/auth/login/email
```

无需认证。

**请求体：**

```json
{
  "email": "user@example.com",
  "password": "SecurePass123!"
}
```

#### 刷新令牌

```
POST /api/auth/refresh
```

无需认证。使用 refresh token 获取新的 access token。

#### 发送验证码（短信）

```
POST /api/auth/verification-code
```

无需认证。

#### 发送验证码（邮箱）

```
POST /api/auth/email/verification-code
```

无需认证。

#### 修改密码

```
POST /api/auth/change-password
```

需要认证。

**请求体：**

```json
{
  "oldPassword": "OldPass123!",
  "newPassword": "NewPass456!"
}
```

#### 发送重置密码验证码

```
POST /api/auth/reset-password/code
```

无需认证。

#### 重置密码

```
POST /api/auth/reset-password
```

无需认证。

**请求体：**

```json
{
  "email": "user@example.com",
  "verificationCode": "123456",
  "newPassword": "NewPass456!"
}
```

#### 注销账号

```
POST /api/auth/delete-account
```

需要认证。

#### 搜索用户

```
GET /api/users/search?keyword=xxx&page=1&pageSize=20
```

需要认证。

#### 获取用户信息

```
GET /api/users/{userId}
```

需要认证。

#### 获取用户统计

```
GET /api/users/{userId}/stats
```

需要认证。

#### 获取我的漂流瓶

```
GET /api/users/my/boats
```

需要认证。

#### 获取我的情绪日历

```
GET /api/users/my/emotion-calendar
```

需要认证。

#### 更新昵称

```
PUT /api/users/my/nickname
```

需要认证。

**请求体：**

```json
{
  "nickname": "新昵称"
}
```

#### 更新个人资料

```
PUT /api/users/my/profile
```

需要认证。

---

### 1.2 AccountController（20 个端点）

#### 个人信息管理

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/account/info` | 获取完整账号信息 |
| POST | `/api/account/avatar` | 更新头像 |
| PUT | `/api/account/profile` | 更新个人资料 |
| GET | `/api/account/stats` | 获取账号统计数据 |

#### 账号安全

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/account/devices` | 获取登录设备列表 |
| DELETE | `/api/account/devices/{sessionId}` | 移除登录设备 |
| GET | `/api/account/login-logs` | 获取登录日志 |
| GET | `/api/account/security-events` | 获取安全事件 |
| POST | `/api/account/change-password` | 修改密码（需旧密码） |
| POST | `/api/account/bind-email` | 绑定邮箱 |
| POST | `/api/account/unbind-email` | 解绑邮箱 |

#### 隐私设置

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/account/privacy` | 获取隐私设置 |
| PUT | `/api/account/privacy` | 更新隐私设置 |
| GET | `/api/account/blocked-users` | 获取黑名单 |
| POST | `/api/account/block/{targetUserId}` | 拉黑用户 |
| DELETE | `/api/account/unblock/{targetUserId}` | 取消拉黑 |

#### 数据管理

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/account/export` | 导出个人数据 |
| GET | `/api/account/export/{taskId}` | 获取导出任务状态 |
| POST | `/api/account/deactivate` | 注销账号（软删除） |
| POST | `/api/account/delete-permanent` | 永久删除账号 |


---

## 2. 内容管理

### 2.1 StoneController（7 个端点）

心石（Stone）是 HeartLake 的核心内容载体，用户将情感写入心石投入湖中。

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/stones` | SecurityAuditFilter | 投放心石 |
| GET | `/api/lake/stones` | 无 | 浏览湖中心石（公开） |
| GET | `/api/stones/my` | SecurityAuditFilter | 获取我的心石 |
| GET | `/api/stones/{stoneId}` | 无 | 获取心石详情 |
| DELETE | `/api/stones/{stoneId}` | SecurityAuditFilter | 删除心石 |
| GET | `/api/lake/weather` | 无 | 获取湖面天气（情绪氛围） |
| GET | `/api/stones/{stoneId}/resonance` | SecurityAuditFilter | 获取心石共鸣数据 |

**投放心石请求示例：**

```json
{
  "content": "今天的夕阳很美",
  "mood": "calm",
  "tags": ["日落", "感悟"],
  "isAnonymous": true
}
```

### 2.2 MediaController（4 个端点）

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/media/upload` | 上传单个媒体文件 |
| POST | `/api/media/upload/multiple` | 批量上传媒体文件 |
| GET | `/api/media/{mediaId}` | 获取媒体文件 |
| DELETE | `/api/media/{mediaId}` | 删除媒体文件 |

---

## 3. 社交互动

### 3.1 InteractionController（15 个端点）

涟漪（Ripple）和漂流瓶（Boat）是心石的互动方式，连接（Connection）是用户间的临时关系。

#### 涟漪与漂流瓶

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/stones/{stoneId}/ripples` | 对心石发送涟漪（评论） |
| POST | `/api/stones/{stoneId}/boats` | 对心石放漂流瓶 |
| GET | `/api/stones/{stoneId}/boats` | 获取心石的漂流瓶列表 |
| GET | `/api/interactions/my/ripples` | 获取我发出的涟漪 |
| GET | `/api/interactions/my/boats` | 获取我发出的漂流瓶 |
| DELETE | `/api/ripples/{rippleId}` | 删除涟漪 |
| DELETE | `/api/boats/{boatId}` | 删除漂流瓶 |

#### 通知

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/notifications` | 获取通知列表 |
| POST | `/api/notifications/{notificationId}/read` | 标记通知已读 |
| POST | `/api/notifications/read-all` | 标记全部已读 |

#### 连接

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/stones/{stoneId}/connections` | 通过心石建立连接 |
| POST | `/api/connections` | 创建连接 |
| POST | `/api/connections/{connectionId}/friend` | 将连接升级为好友 |
| GET | `/api/connections/{connectionId}/messages` | 获取连接消息 |
| POST | `/api/connections/{connectionId}/messages` | 发送连接消息 |

### 3.2 PaperBoatController（10 个端点）

漂流瓶系统，支持随机漂流、定向投递、许愿模式。

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/boats/drift` | 放出漂流瓶 |
| POST | `/api/boats/reply` | 回复漂流瓶 |
| POST | `/api/boats/catch` | 捞起漂流瓶 |
| POST | `/api/boats/{boatId}/respond` | 回应漂流瓶 |
| POST | `/api/boats/{boatId}/release` | 放回漂流瓶 |
| GET | `/api/boats/{boatId}` | 获取漂流瓶详情 |
| GET | `/api/boats/sent` | 获取已发送的漂流瓶 |
| GET | `/api/boats/received` | 获取已接收的漂流瓶 |
| GET | `/api/boats/drifting/count` | 获取漂流中的瓶子数量 |
| GET | `/api/boats/{boatId}/status` | 获取漂流瓶状态 |

**放出漂流瓶请求示例：**

```json
{
  "content": "希望捡到这个瓶子的人今天开心",
  "drift_mode": "random",
  "boat_style": "origami",
  "mood": "hopeful"
}
```

`drift_mode` 可选值：`random`（随机漂流）、`directed`（定向投递）、`wish`（许愿模式）

`boat_style` 可选值：`paper`（纸船）、`origami`（折纸船）、`lotus`（莲花船）

### 3.3 FriendController（8 个端点）

所有端点均需 SecurityAuditFilter 认证。

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/friends/request` | 发送好友请求 |
| POST | `/api/friends/accept/{requestId}` | 接受好友请求 |
| POST | `/api/friends/reject/{requestId}` | 拒绝好友请求 |
| DELETE | `/api/friends/{friendId}` | 删除好友 |
| GET | `/api/friends` | 获取好友列表 |
| GET | `/api/friends/requests/pending` | 获取待处理的好友请求 |
| POST | `/api/friends/{friendId}/messages` | 发送好友消息 |
| GET | `/api/friends/{friendId}/messages` | 获取好友消息记录 |

### 3.4 TempFriendController（6 个端点）

临时好友系统，所有端点均需 SecurityAuditFilter 认证。部分路径有别名。

| 方法 | 路径 | 别名 | 说明 |
|------|------|------|------|
| POST | `/api/temp-friends` | `/api/friends/temp` | 创建临时好友关系 |
| GET | `/api/temp-friends` | `/api/friends/temp` | 获取临时好友列表 |
| GET | `/api/temp-friends/{tempFriendId}` | - | 获取临时好友详情 |
| POST | `/api/temp-friends/{tempFriendId}/upgrade` | - | 升级为正式好友 |
| DELETE | `/api/temp-friends/{tempFriendId}` | - | 删除临时好友 |
| GET | `/api/temp-friends/check/{userId}` | - | 检查是否为临时好友 |


---

## 4. 守望者与咨询

### 4.1 GuardianController（3 个端点）

守望者系统提供灯火转赠与情绪洞察功能。所有端点均需 SecurityAuditFilter 认证。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/guardian/stats` | 获取守望者统计数据 |
| GET | `/api/guardian` | 获取守望者统计数据（别名） |
| POST | `/api/guardian/transfer-lamp` | 灯火转赠 |
| GET | `/api/guardian/insights` | 获取情绪洞察 |

**灯火转赠请求示例：**

```json
{
  "targetUserId": "uuid-xxx",
  "lampCount": 1,
  "message": "送你一盏灯火，愿你温暖"
}
```

### 4.2 ConsultationController（4 个端点）

端到端加密（E2EE）咨询会话系统。所有端点均需 SecurityAuditFilter 认证。

> 命名空间：`heartlake::api`

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/consultation/session` | 创建咨询会话 |
| POST | `/api/consultation/key-exchange` | 密钥交换（E2EE） |
| POST | `/api/consultation/message` | 发送加密消息 |
| GET | `/api/consultation/messages/{sessionId}` | 获取会话消息列表 |

**创建咨询会话请求示例：**

```json
{
  "counselorId": "uuid-xxx",
  "topic": "情绪管理",
  "publicKey": "base64-encoded-public-key"
}
```

**密钥交换请求示例：**

```json
{
  "sessionId": "uuid-xxx",
  "publicKey": "base64-encoded-public-key"
}
```

---

## 5. 推荐与搜索

### 5.1 RecommendationController（8 个端点）

混合推荐算法：协同过滤 40% + 内容推荐 40% + 随机探索 20%。所有端点均需 SecurityAuditFilter 认证。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/recommendations` | 获取个性化推荐 |
| GET | `/api/recommendations/explore` | 探索推荐（随机性更高） |
| GET | `/api/recommendations/mood` | 按情绪推荐 |
| GET | `/api/discover/mood` | 按情绪发现（别名） |
| GET | `/api/recommendations/similar/{stoneId}` | 获取相似心石 |
| GET | `/api/recommendations/trending` | 获取热门心石 |
| POST | `/api/recommendations/feedback` | 提交推荐反馈 |
| GET | `/api/recommendations/history` | 获取推荐历史 |

**查询参数（通用）：**

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| page | int | 1 | 页码 |
| pageSize | int | 20 | 每页数量 |
| mood | string | - | 情绪过滤 |

**推荐反馈请求示例：**

```json
{
  "stoneId": "uuid-xxx",
  "action": "like",
  "source": "recommendation"
}
```

### 5.2 VectorSearchController（3 个端点）

基于 HNSW 的向量相似度搜索。

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/search/similar` | SecurityAuditFilter | 搜索相似心石 |
| GET | `/api/search/personalized` | SecurityAuditFilter | 个性化搜索 |
| POST | `/api/admin/search/embedding` | AdminAuthFilter | 管理员：手动生成嵌入向量 |

**相似搜索请求示例：**

```json
{
  "query": "温暖的故事",
  "topK": 10,
  "threshold": 0.6
}
```

---

## 6. 边缘AI

### 6.1 EdgeAIController（10 个端点）

整合本地AI推理、联邦学习、差分隐私与向量检索。

#### 公开接口（SecurityAuditFilter）

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/edge-ai/status` | 获取边缘AI引擎运行状态 |
| GET | `/api/edge-ai/metrics` | 获取详细性能指标 |
| POST | `/api/edge-ai/analyze` | 本地情感分析 |
| POST | `/api/edge-ai/moderate` | 本地内容审核 |
| GET | `/api/edge-ai/emotion-pulse` | 社区实时情绪脉搏 |
| POST | `/api/edge-ai/federated/aggregate` | 触发联邦学习聚合 |
| GET | `/api/edge-ai/privacy-budget` | 查询差分隐私预算状态 |
| POST | `/api/edge-ai/vector-search` | 本地向量相似度搜索 |

#### 管理接口（AdminAuthFilter + PASETO）

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/edge-ai/config` | 获取边缘AI配置 |
| PUT | `/api/admin/edge-ai/config` | 更新边缘AI配置 |

**本地情感分析请求示例：**

```json
{
  "text": "今天心情很好",
  "language": "zh"
}
```

**本地内容审核请求示例：**

```json
{
  "text": "待审核文本",
  "strictMode": false
}
```

**情绪脉搏查询参数：**

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| windowHours | int | 1 | 时间窗口（小时） |
| epsilon | double | 2.0 | 差分隐私预算 |

**联邦学习聚合请求示例：**

```json
{
  "round": 1,
  "minParticipants": 5,
  "epsilon": 1.0,
  "clippingBound": 1.0
}
```

**向量搜索请求示例：**

```json
{
  "query": "寻找温暖的故事",
  "topK": 10,
  "threshold": 0.6,
  "mood": "calm"
}
```

**管理员配置更新请求示例：**

```json
{
  "localModel": {
    "embeddingDim": 384,
    "maxBatchSize": 32,
    "timeoutMs": 5000
  },
  "privacy": {
    "maxEpsilon": 10.0,
    "defaultEpsilon": 1.0,
    "budgetResetIntervalHours": 24
  },
  "federated": {
    "minParticipants": 3,
    "aggregationStrategy": "fedavg",
    "clippingBound": 1.0
  },
  "cache": {
    "maxSize": 10000,
    "similarityThreshold": 0.85,
    "ttlSeconds": 3600
  }
}
```

## 7. 安全与隐私

### 7.1 SafeHarborController（10 个端点）

安全港系统提供危机干预资源、心理工具和安全推荐。

#### 公开接口（SecurityAuditFilter）

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/safe-harbor/hotlines` | 获取危机热线列表 |
| GET | `/api/safe-harbor/tools` | 获取心理自助工具列表 |
| GET | `/api/safe-harbor/prompt` | 获取安全提示语 |
| GET | `/api/safe-harbor/access-history` | 获取用户访问安全港历史 |
| POST | `/api/safe-harbor/access` | 记录安全港访问 |
| GET | `/api/safe-harbor/recommend` | 获取个性化安全资源推荐 |

#### 管理接口（AdminAuthFilter）

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/admin/safe-harbor/resources` | 创建安全资源 |
| GET | `/api/admin/safe-harbor/resources` | 获取安全资源列表 |
| PUT | `/api/admin/safe-harbor/resources/{resourceId}` | 更新安全资源 |
| DELETE | `/api/admin/safe-harbor/resources/{resourceId}` | 删除安全资源 |

**创建安全资源请求示例：**

```json
{
  "type": "hotline",
  "name": "全国心理援助热线",
  "contact": "400-161-9995",
  "description": "24小时免费心理危机干预热线",
  "priority": 1,
  "isActive": true
}
```

### 7.2 PrivacyController（2 个端点）

差分隐私统计与报告。所有端点均需 SecurityAuditFilter 认证。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/privacy/stats` | 获取隐私保护统计数据 |
| GET | `/api/privacy/report` | 获取个人隐私报告 |

### 7.3 ReportController（2 个端点）

内容举报系统。所有端点均需 SecurityAuditFilter 认证。

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/reports` | 提交举报 |
| GET | `/api/reports/my` | 获取我的举报记录 |

**提交举报请求示例：**

```json
{
  "targetType": "stone",
  "targetId": "uuid-xxx",
  "reason": "inappropriate_content",
  "description": "包含不当内容"
}
```

---

## 8. VIP

### 8.1 VIPController（5 个端点）

VIP 会员系统，提供会员状态查询、特权列表、专属咨询与AI评论频率。所有端点均需 SecurityAuditFilter 认证。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/vip/status` | 获取VIP会员状态 |
| GET | `/api/vip/privileges` | 获取VIP特权列表 |
| GET | `/api/vip/counseling/check` | 检查VIP咨询资格 |
| POST | `/api/vip/counseling/book` | 预约VIP专属咨询 |
| GET | `/api/vip/ai-comment/frequency` | 获取AI评论频率限制 |

**预约VIP咨询请求示例：**

```json
{
  "counselorId": "uuid-xxx",
  "scheduledTime": "2025-06-20T14:00:00Z",
  "topic": "情绪管理"
}
```

---

## 9. 系统

### 9.1 HealthController（2 个端点）

服务健康检查，无需认证。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/health` | 基础健康检查（状态、版本、时间戳） |
| GET | `/api/health/detailed` | 详细健康检查（数据库、Redis、运行时间、内存） |

**基础健康检查响应示例：**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "status": "healthy",
    "version": "1.0.0",
    "timestamp": "2025-06-15T12:00:00Z"
  }
}
```

**详细健康检查响应示例：**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "status": "healthy",
    "version": "1.0.0",
    "uptime": "72h30m",
    "database": {
      "status": "connected",
      "latencyMs": 2
    },
    "redis": {
      "status": "connected",
      "latencyMs": 1
    },
    "memoryUsageMB": 128.5
  }
}
```

### 9.2 BroadcastWebSocketController（1 个 WebSocket 端点）

实时广播 WebSocket 服务，支持全局广播、定向推送和房间消息。

```
WS /ws/broadcast
```

**静态方法：**

| 方法 | 说明 |
|------|------|
| `broadcast(const std::string &message)` | 向所有连接的客户端广播消息 |
| `sendToUser(const std::string &userId, const std::string &message)` | 向指定用户推送消息 |
| `sendToRoom(const std::string &room, const std::string &message)` | 向指定房间推送消息 |
| `startHeartbeatTimer()` | 启动心跳定时器 |

**WebSocket 消息格式：**

```json
{
  "type": "notification",
  "data": {
    "event": "new_ripple",
    "stoneId": "uuid-xxx",
    "message": "有人对你的心石发送了涟漪"
  }
}
```

---

## 10. 管理后台

### 10.1 AdminController（14 个端点）

管理员核心功能：登录、仪表盘、风险监控、安全审计。

#### 管理员认证

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/admin/login` | 无 | 管理员登录（返回 PASETO 令牌） |
| POST | `/api/admin/logout` | AdminAuthFilter | 管理员登出 |

**管理员登录请求示例：**

```json
{
  "username": "admin",
  "password": "AdminPass123!"
}
```

**管理员登录响应示例：**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "token": "v4.public.xxx",
    "adminId": "uuid-xxx",
    "role": "super_admin",
    "expiresAt": "2025-06-16T12:00:00Z"
  }
}
```

#### 仪表盘与统计

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/dashboard` | 获取仪表盘概览数据 |
| GET | `/api/admin/stats/overview` | 获取系统统计概览 |
| GET | `/api/admin/stats/users` | 获取用户统计数据 |
| GET | `/api/admin/stats/content` | 获取内容统计数据 |
| GET | `/api/admin/stats/trends` | 获取趋势数据 |

#### 风险监控

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/risk/high-risk-users` | 获取高风险用户列表 |
| GET | `/api/admin/risk/alerts` | 获取风险告警列表 |
| POST | `/api/admin/risk/alerts/{alertId}/handle` | 处理风险告警 |

#### 安全审计

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/security/audit-logs` | 获取安全审计日志 |
| GET | `/api/admin/security/login-anomalies` | 获取登录异常记录 |
| POST | `/api/admin/security/ip-blacklist` | 添加IP黑名单 |

以上所有管理接口（除登录外）均需 AdminAuthFilter 认证。

### 10.2 AdminManagementController（26 个端点）

管理后台的内容管理、用户管理、审核与系统配置。所有端点均需 AdminAuthFilter 认证。

#### 用户管理

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/users` | 获取用户列表（分页） |
| GET | `/api/admin/users/{userId}` | 获取用户详情 |
| PUT | `/api/admin/users/{userId}/status` | 更新用户状态（启用/禁用） |
| POST | `/api/admin/users/{userId}/warn` | 向用户发送警告 |
| POST | `/api/admin/users/{userId}/ban` | 封禁用户 |
| POST | `/api/admin/users/{userId}/unban` | 解封用户 |

#### 心石管理

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/stones` | 获取心石列表（分页） |
| GET | `/api/admin/stones/{stoneId}` | 获取心石详情 |
| DELETE | `/api/admin/stones/{stoneId}` | 删除心石 |
| PUT | `/api/admin/stones/{stoneId}/visibility` | 更新心石可见性 |

#### 漂流瓶管理

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/boats` | 获取漂流瓶列表（分页） |
| DELETE | `/api/admin/boats/{boatId}` | 删除漂流瓶 |

#### 内容审核

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/moderation/pending` | 获取待审核内容列表 |
| POST | `/api/admin/moderation/{itemId}/approve` | 审核通过 |
| POST | `/api/admin/moderation/{itemId}/reject` | 审核拒绝 |
| POST | `/api/admin/moderation/batch` | 批量审核 |

#### 举报处理

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/reports` | 获取举报列表 |
| PUT | `/api/admin/reports/{reportId}` | 处理举报 |

#### 敏感词管理

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/sensitive-words` | 获取敏感词列表 |
| POST | `/api/admin/sensitive-words` | 添加敏感词 |
| DELETE | `/api/admin/sensitive-words/{wordId}` | 删除敏感词 |

#### 系统配置

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/admin/config` | 获取系统配置 |
| PUT | `/api/admin/config` | 更新系统配置 |

#### 广播与日志

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/admin/broadcast` | 发送系统广播消息 |
| GET | `/api/admin/logs` | 获取系统操作日志 |

---

## 附录：端点统计

| 模块 | Controller | 端点数 |
|------|-----------|--------|
| 认证与用户 | UserController | 19 |
| 认证与用户 | AccountController | 20 |
| 内容管理 | StoneController | 7 |
| 内容管理 | MediaController | 4 |
| 社交互动 | InteractionController | 15 |
| 社交互动 | PaperBoatController | 10 |
| 社交互动 | FriendController | 8 |
| 社交互动 | TempFriendController | 6 |
| 守望者与咨询 | GuardianController | 3 |
| 守望者与咨询 | ConsultationController | 4 |
| 推荐与搜索 | RecommendationController | 8 |
| 推荐与搜索 | VectorSearchController | 3 |
| 边缘AI | EdgeAIController | 10 |
| 安全与隐私 | SafeHarborController | 10 |
| 安全与隐私 | PrivacyController | 2 |
| 安全与隐私 | ReportController | 2 |
| VIP | VIPController | 5 |
| 系统 | HealthController | 2 |
| 系统 | BroadcastWebSocketController | 1 (WebSocket) |
| 管理后台 | AdminController | 14 |
| 管理后台 | AdminManagementController | 26 |
| **合计** | **21 个 Controller** | **179+ 端点** |
