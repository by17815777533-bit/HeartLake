# HeartLake API 参考文档

> 版本：v1.0 | 最后更新：2025-06-15

## 概述

HeartLake 后端服务基于 RESTful 风格设计，所有接口统一返回 JSON 格式。

**Base URL:** `https://api.heartlake.app/api`

### 统一响应格式

```json
{
  "code": 200,
  "message": "ok",
  "data": { ... },
  "timestamp": 1708387200
}
```

### 通用错误码

| 错误码 | 含义 |
|--------|------|
| 200 | 成功 |
| 400 | 请求参数错误 |
| 401 | 未认证 / Token 过期 |
| 403 | 权限不足 |
| 404 | 资源不存在 |
| 409 | 资源冲突（如重复注册） |
| 422 | 请求体校验失败 |
| 429 | 请求频率超限 |
| 500 | 服务器内部错误 |

### 认证方式

除注册和登录外，所有接口均需在请求头中携带 Bearer Token：

```
Authorization: Bearer <access_token>
```

---

## 1. 认证 API

### 1.1 用户注册

- **方法:** `POST`
- **路径:** `/api/auth/register`
- **描述:** 注册新用户账号，成功后返回用户信息及访问令牌

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |

**请求体:**

```json
{
  "username": "lakewalker",
  "email": "walker@heartlake.app",
  "password": "SecureP@ss123"
}
```

**响应:**

```json
{
  "code": 200,
  "message": "注册成功",
  "data": {
    "user": {
      "id": "u_abc123",
      "username": "lakewalker",
      "email": "walker@heartlake.app",
      "avatar": null,
      "createdAt": "2025-06-15T10:00:00Z"
    },
    "accessToken": "eyJhbGciOiJIUzI1NiIs...",
    "refreshToken": "eyJhbGciOiJIUzI1NiIs..."
  },
  "timestamp": 1708387200
}
```

---

### 1.2 用户登录

- **方法:** `POST`
- **路径:** `/api/auth/login`
- **描述:** 使用邮箱和密码登录，返回访问令牌和刷新令牌

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |

**请求体:**

```json
{
  "email": "walker@heartlake.app",
  "password": "SecureP@ss123"
}
```

**响应:**

```json
{
  "code": 200,
  "message": "登录成功",
  "data": {
    "user": {
      "id": "u_abc123",
      "username": "lakewalker",
      "email": "walker@heartlake.app",
      "avatar": "https://cdn.heartlake.app/avatars/u_abc123.jpg",
      "createdAt": "2025-06-15T10:00:00Z"
    },
    "accessToken": "eyJhbGciOiJIUzI1NiIs...",
    "refreshToken": "eyJhbGciOiJIUzI1NiIs..."
  },
  "timestamp": 1708387200
}
```

---

### 1.3 验证码校验

- **方法:** `POST`
- **路径:** `/api/auth/verify`
- **描述:** 校验用户提交的短信/邮箱验证码，用于注册确认或密码重置

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |

**请求体:**

```json
{
  "email": "walker@heartlake.app",
  "code": "836921",
  "type": "register"
}
```

**响应:**

```json
{
  "code": 200,
  "message": "验证成功",
  "data": {
    "verified": true
  },
  "timestamp": 1708387200
}
```

---

### 1.4 刷新 Token

- **方法:** `POST`
- **路径:** `/api/auth/refresh`
- **描述:** 使用 refreshToken 获取新的 accessToken，实现无感续期

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |

**请求体:**

```json
{
  "refreshToken": "eyJhbGciOiJIUzI1NiIs..."
}
```

**响应:**

```json
{
  "code": 200,
  "message": "刷新成功",
  "data": {
    "accessToken": "eyJhbGciOiJIUzI1NiIs...",
    "refreshToken": "eyJhbGciOiJIUzI1NiIs..."
  },
  "timestamp": 1708387200
}
```

---

## 2. 石头（帖子）API

### 2.1 投放石头

- **方法:** `POST`
- **路径:** `/api/stones`
- **描述:** 创建一块新石头（发布帖子），支持文字、图片、标签

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |
| Authorization | Bearer <access_token> |

**请求体:**

```json
{
  "content": "今天在湖边看到了一只白鹭，心情很好",
  "images": [
    "https://cdn.heartlake.app/uploads/img_001.jpg"
  ],
  "tags": ["心情", "自然"],
  "visibility": "public",
  "location": {
    "latitude": 30.2741,
    "longitude": 120.1551,
    "name": "西湖"
  }
}
```

**响应:**

```json
{
  "code": 200,
  "message": "投放成功",
  "data": {
    "id": "s_xyz789",
    "authorId": "u_abc123",
    "content": "今天在湖边看到了一只白鹭，心情很好",
    "images": [
      "https://cdn.heartlake.app/uploads/img_001.jpg"
    ],
    "tags": ["心情", "自然"],
    "visibility": "public",
    "location": {
      "latitude": 30.2741,
      "longitude": 120.1551,
      "name": "西湖"
    },
    "rippleCount": 0,
    "createdAt": "2025-06-15T10:30:00Z"
  },
  "timestamp": 1708387200
}
```

---

### 2.2 获取石头列表

- **方法:** `GET`
- **路径:** `/api/stones`
- **描述:** 分页获取石头列表，支持按标签、时间、热度筛选和排序

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**查询参数:**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| page | number | 否 | 页码，默认 1 |
| limit | number | 否 | 每页数量，默认 20，最大 50 |
| tag | string | 否 | 按标签筛选 |
| sort | string | 否 | 排序方式：`latest`（默认）/ `hot` |
| visibility | string | 否 | 可见性筛选：`public` / `friends` |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "items": [
      {
        "id": "s_xyz789",
        "authorId": "u_abc123",
        "authorName": "lakewalker",
        "authorAvatar": "https://cdn.heartlake.app/avatars/u_abc123.jpg",
        "content": "今天在湖边看到了一只白鹭，心情很好",
        "images": [
          "https://cdn.heartlake.app/uploads/img_001.jpg"
        ],
        "tags": ["心情", "自然"],
        "visibility": "public",
        "rippleCount": 5,
        "createdAt": "2025-06-15T10:30:00Z"
      }
    ],
    "pagination": {
      "page": 1,
      "limit": 20,
      "total": 128,
      "totalPages": 7
    }
  },
  "timestamp": 1708387200
}
```

---

### 2.3 获取石头详情

- **方法:** `GET`
- **路径:** `/api/stones/:id`
- **描述:** 根据石头 ID 获取完整详情，包含作者信息和涟漪统计

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**路径参数:**

| 参数 | 类型 | 说明 |
|------|------|------|
| id | string | 石头 ID |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "id": "s_xyz789",
    "authorId": "u_abc123",
    "authorName": "lakewalker",
    "authorAvatar": "https://cdn.heartlake.app/avatars/u_abc123.jpg",
    "content": "今天在湖边看到了一只白鹭，心情很好",
    "images": [
      "https://cdn.heartlake.app/uploads/img_001.jpg"
    ],
    "tags": ["心情", "自然"],
    "visibility": "public",
    "location": {
      "latitude": 30.2741,
      "longitude": 120.1551,
      "name": "西湖"
    },
    "rippleCount": 5,
    "createdAt": "2025-06-15T10:30:00Z",
    "updatedAt": "2025-06-15T11:00:00Z"
  },
  "timestamp": 1708387200
}
```

---

### 2.4 删除石头

- **方法:** `DELETE`
- **路径:** `/api/stones/:id`
- **描述:** 删除指定石头，仅作者本人或管理员可操作

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**路径参数:**

| 参数 | 类型 | 说明 |
|------|------|------|
| id | string | 石头 ID |

**响应:**

```json
{
  "code": 200,
  "message": "删除成功",
  "data": {
    "id": "s_xyz789",
    "deleted": true
  },
  "timestamp": 1708387200
}
```

---

## 3. 涟漪（互动）API

### 3.1 创建涟漪

- **方法:** `POST`
- **路径:** `/api/stones/:id/ripples`
- **描述:** 对指定石头创建涟漪（评论/回复），支持文字和表情

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |
| Authorization | Bearer <access_token> |

**路径参数:**

| 参数 | 类型 | 说明 |
|------|------|------|
| id | string | 石头 ID |

**请求体:**

```json
{
  "content": "好美的白鹭！我也想去看看",
  "parentRippleId": null,
  "emoji": "heart"
}
```

**响应:**

```json
{
  "code": 200,
  "message": "涟漪已泛起",
  "data": {
    "id": "r_rip001",
    "stoneId": "s_xyz789",
    "authorId": "u_def456",
    "authorName": "stargazer",
    "authorAvatar": "https://cdn.heartlake.app/avatars/u_def456.jpg",
    "content": "好美的白鹭！我也想去看看",
    "parentRippleId": null,
    "emoji": "heart",
    "createdAt": "2025-06-15T11:15:00Z"
  },
  "timestamp": 1708387200
}
```

---

### 3.2 获取涟漪列表

- **方法:** `GET`
- **路径:** `/api/stones/:id/ripples`
- **描述:** 获取指定石头下的涟漪列表，支持分页

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**路径参数:**

| 参数 | 类型 | 说明 |
|------|------|------|
| id | string | 石头 ID |

**查询参数:**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| page | number | 否 | 页码，默认 1 |
| limit | number | 否 | 每页数量，默认 20 |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "items": [
      {
        "id": "r_rip001",
        "stoneId": "s_xyz789",
        "authorId": "u_def456",
        "authorName": "stargazer",
        "authorAvatar": "https://cdn.heartlake.app/avatars/u_def456.jpg",
        "content": "好美的白鹭！我也想去看看",
        "parentRippleId": null,
        "emoji": "heart",
        "createdAt": "2025-06-15T11:15:00Z"
      }
    ],
    "pagination": {
      "page": 1,
      "limit": 20,
      "total": 5,
      "totalPages": 1
    }
  },
  "timestamp": 1708387200
}
```

---

## 4. 纸船（私信）API

### 4.1 发送纸船

- **方法:** `POST`
- **路径:** `/api/boats`
- **描述:** 向指定用户发送一只纸船（私信），支持文字和图片

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |
| Authorization | Bearer <access_token> |

**请求体:**

```json
{
  "receiverId": "u_def456",
  "content": "你好，想和你聊聊关于白鹭的事",
  "image": null
}
```

**响应:**

```json
{
  "code": 200,
  "message": "纸船已送出",
  "data": {
    "id": "b_boat001",
    "senderId": "u_abc123",
    "receiverId": "u_def456",
    "content": "你好，想和你聊聊关于白鹭的事",
    "image": null,
    "read": false,
    "createdAt": "2025-06-15T12:00:00Z"
  },
  "timestamp": 1708387200
}
```

---

### 4.2 获取纸船列表

- **方法:** `GET`
- **路径:** `/api/boats`
- **描述:** 获取当前用户的纸船收发列表，支持分页和方向筛选

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**查询参数:**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| page | number | 否 | 页码，默认 1 |
| limit | number | 否 | 每页数量，默认 20 |
| direction | string | 否 | `received`（收到）/ `sent`（发出）/ `all`（默认） |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "items": [
      {
        "id": "b_boat001",
        "senderId": "u_abc123",
        "senderName": "lakewalker",
        "senderAvatar": "https://cdn.heartlake.app/avatars/u_abc123.jpg",
        "receiverId": "u_def456",
        "receiverName": "stargazer",
        "content": "你好，想和你聊聊关于白鹭的事",
        "image": null,
        "read": false,
        "createdAt": "2025-06-15T12:00:00Z"
      }
    ],
    "pagination": {
      "page": 1,
      "limit": 20,
      "total": 3,
      "totalPages": 1
    }
  },
  "timestamp": 1708387200
}
```

---

### 4.3 获取纸船详情

- **方法:** `GET`
- **路径:** `/api/boats/:id`
- **描述:** 获取指定纸船的完整内容，接收方查看后自动标记为已读

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**路径参数:**

| 参数 | 类型 | 说明 |
|------|------|------|
| id | string | 纸船 ID |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "id": "b_boat001",
    "senderId": "u_abc123",
    "senderName": "lakewalker",
    "senderAvatar": "https://cdn.heartlake.app/avatars/u_abc123.jpg",
    "receiverId": "u_def456",
    "receiverName": "stargazer",
    "receiverAvatar": "https://cdn.heartlake.app/avatars/u_def456.jpg",
    "content": "你好，想和你聊聊关于白鹭的事",
    "image": null,
    "read": true,
    "createdAt": "2025-06-15T12:00:00Z",
    "readAt": "2025-06-15T12:05:00Z"
  },
  "timestamp": 1708387200
}
```

---

## 5. 好友 API

### 5.1 发送好友请求

- **方法:** `POST`
- **路径:** `/api/friends/request`
- **描述:** 向指定用户发送好友请求，可附带验证消息

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |
| Authorization | Bearer <access_token> |

**请求体:**

```json
{
  "targetUserId": "u_def456",
  "message": "我们在湖边聊过，加个好友吧"
}
```

**响应:**

```json
{
  "code": 200,
  "message": "好友请求已发送",
  "data": {
    "id": "f_req001",
    "fromUserId": "u_abc123",
    "toUserId": "u_def456",
    "message": "我们在湖边聊过，加个好友吧",
    "status": "pending",
    "createdAt": "2025-06-15T13:00:00Z"
  },
  "timestamp": 1708387200
}
```

---

### 5.2 获取好友列表

- **方法:** `GET`
- **路径:** `/api/friends`
- **描述:** 获取当前用户的好友列表，支持分页和在线状态筛选

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**查询参数:**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| page | number | 否 | 页码，默认 1 |
| limit | number | 否 | 每页数量，默认 50 |
| status | string | 否 | `accepted`（已通过）/ `pending`（待处理） |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "items": [
      {
        "id": "f_req001",
        "userId": "u_def456",
        "username": "stargazer",
        "avatar": "https://cdn.heartlake.app/avatars/u_def456.jpg",
        "status": "accepted",
        "online": true,
        "friendSince": "2025-06-15T14:00:00Z"
      }
    ],
    "pagination": {
      "page": 1,
      "limit": 50,
      "total": 12,
      "totalPages": 1
    }
  },
  "timestamp": 1708387200
}
```

---

### 5.3 接受好友请求

- **方法:** `PUT`
- **路径:** `/api/friends/:id/accept`
- **描述:** 接受指定的好友请求，双方成为好友

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**路径参数:**

| 参数 | 类型 | 说明 |
|------|------|------|
| id | string | 好友请求 ID |

**响应:**

```json
{
  "code": 200,
  "message": "已成为好友",
  "data": {
    "id": "f_req001",
    "fromUserId": "u_abc123",
    "toUserId": "u_def456",
    "status": "accepted",
    "acceptedAt": "2025-06-15T14:00:00Z"
  },
  "timestamp": 1708387200
}
```

---

### 5.4 删除好友

- **方法:** `DELETE`
- **路径:** `/api/friends/:id`
- **描述:** 删除指定好友关系，双方均会解除好友状态

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**路径参数:**

| 参数 | 类型 | 说明 |
|------|------|------|
| id | string | 好友关系 ID |

**响应:**

```json
{
  "code": 200,
  "message": "好友已删除",
  "data": {
    "id": "f_req001",
    "deleted": true
  },
  "timestamp": 1708387200
}
```

---

## 6. 边缘 AI API

### 6.1 引擎状态

- **方法:** `GET`
- **路径:** `/api/edge-ai/status`
- **描述:** 获取边缘 AI 引擎的运行状态，包括模型加载情况、可用能力等

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "engine": "heartlake-edge-ai",
    "version": "1.2.0",
    "status": "running",
    "models": {
      "sentiment": { "loaded": true, "version": "v3.1" },
      "moderation": { "loaded": true, "version": "v2.4" },
      "vectorizer": { "loaded": true, "version": "v1.8" }
    },
    "capabilities": ["sentiment_analysis", "content_moderation", "vector_search", "federated_learning"],
    "uptime": 86400,
    "lastHealthCheck": "2025-06-15T09:55:00Z"
  },
  "timestamp": 1708387200
}
```

---

### 6.2 性能指标

- **方法:** `GET`
- **路径:** `/api/edge-ai/metrics`
- **描述:** 获取边缘 AI 引擎的性能指标，包括推理延迟、吞吐量、资源占用等

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "inference": {
      "totalRequests": 52480,
      "avgLatencyMs": 12.5,
      "p95LatencyMs": 28.3,
      "p99LatencyMs": 45.1,
      "throughputRps": 156.8
    },
    "resources": {
      "cpuUsagePercent": 34.2,
      "memoryUsageMb": 512,
      "memoryLimitMb": 2048,
      "gpuUsagePercent": 67.8
    },
    "models": {
      "sentiment": { "requestCount": 21000, "avgLatencyMs": 8.2 },
      "moderation": { "requestCount": 18500, "avgLatencyMs": 11.4 },
      "vectorizer": { "requestCount": 12980, "avgLatencyMs": 18.7 }
    },
    "period": "last_24h"
  },
  "timestamp": 1708387200
}
```

---

### 6.3 本地情感分析

- **方法:** `POST`
- **路径:** `/api/edge-ai/analyze`
- **描述:** 对输入文本进行本地情感分析，返回情感标签和置信度，数据不离开本地

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |
| Authorization | Bearer <access_token> |

**请求体:**

```json
{
  "text": "今天在湖边看到了一只白鹭，心情很好",
  "language": "zh"
}
```

**响应:**

```json
{
  "code": 200,
  "message": "分析完成",
  "data": {
    "sentiment": "positive",
    "confidence": 0.934,
    "emotions": [
      { "label": "joy", "score": 0.82 },
      { "label": "serenity", "score": 0.71 },
      { "label": "surprise", "score": 0.15 }
    ],
    "processedLocally": true,
    "latencyMs": 8
  },
  "timestamp": 1708387200
}
```

---

### 6.4 本地内容审核

- **方法:** `POST`
- **路径:** `/api/edge-ai/moderate`
- **描述:** 对内容进行本地审核，检测违规信息（暴力、色情、垃圾广告等），数据不离开本地

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |
| Authorization | Bearer <access_token> |

**请求体:**

```json
{
  "text": "这是一段需要审核的内容",
  "images": [
    "https://cdn.heartlake.app/uploads/img_002.jpg"
  ]
}
```

**响应:**

```json
{
  "code": 200,
  "message": "审核完成",
  "data": {
    "approved": true,
    "categories": {
      "violence": { "detected": false, "score": 0.01 },
      "pornography": { "detected": false, "score": 0.00 },
      "spam": { "detected": false, "score": 0.03 },
      "harassment": { "detected": false, "score": 0.02 }
    },
    "overallRisk": "low",
    "processedLocally": true,
    "latencyMs": 15
  },
  "timestamp": 1708387200
}
```

---

### 6.5 社区情绪脉搏

- **方法:** `GET`
- **路径:** `/api/edge-ai/emotion-pulse`
- **描述:** 获取社区整体情绪脉搏数据，基于近期石头内容的聚合情感分析

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**查询参数:**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| period | string | 否 | 时间范围：`1h` / `24h`（默认）/ `7d` |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "period": "24h",
    "totalAnalyzed": 1280,
    "dominantEmotion": "joy",
    "emotions": {
      "joy": 0.38,
      "serenity": 0.22,
      "sadness": 0.12,
      "anger": 0.05,
      "surprise": 0.10,
      "fear": 0.03,
      "neutral": 0.10
    },
    "trend": "improving",
    "trendDelta": 0.05,
    "hotTags": ["春天", "旅行", "美食"],
    "updatedAt": "2025-06-15T10:00:00Z"
  },
  "timestamp": 1708387200
}
```

---

### 6.6 联邦学习聚合

- **方法:** `POST`
- **路径:** `/api/edge-ai/federated/aggregate`
- **描述:** 提交本地模型梯度参与联邦学习聚合，仅上传加密梯度，原始数据不离开本地

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |
| Authorization | Bearer <access_token> |

**请求体:**

```json
{
  "modelId": "sentiment_v3",
  "roundId": 42,
  "gradients": {
    "encrypted": true,
    "format": "base64",
    "data": "SGVhcnRMYWtlIEZlZGVyYXRlZC..."
  },
  "sampleCount": 256,
  "localMetrics": {
    "loss": 0.234,
    "accuracy": 0.912
  }
}
```

**响应:**

```json
{
  "code": 200,
  "message": "聚合已接收",
  "data": {
    "roundId": 42,
    "accepted": true,
    "participantCount": 18,
    "globalMetrics": {
      "loss": 0.198,
      "accuracy": 0.928
    },
    "nextRoundAt": "2025-06-15T12:00:00Z"
  },
  "timestamp": 1708387200
}
```

---

### 6.7 隐私预算

- **方法:** `GET`
- **路径:** `/api/edge-ai/privacy-budget`
- **描述:** 查询当前用户的差分隐私预算使用情况（epsilon 消耗）

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <access_token> |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "userId": "u_abc123",
    "totalBudget": 10.0,
    "usedBudget": 3.42,
    "remainingBudget": 6.58,
    "queries": [
      {
        "queryId": "q_001",
        "type": "sentiment_analysis",
        "epsilonCost": 0.12,
        "timestamp": "2025-06-15T08:00:00Z"
      },
      {
        "queryId": "q_002",
        "type": "federated_aggregate",
        "epsilonCost": 1.50,
        "timestamp": "2025-06-15T09:30:00Z"
      }
    ],
    "resetAt": "2025-03-01T00:00:00Z"
  },
  "timestamp": 1708387200
}
```

---

### 6.8 向量搜索

- **方法:** `POST`
- **路径:** `/api/edge-ai/vector-search`
- **描述:** 基于语义向量进行相似内容搜索，支持文本和图片输入

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |
| Authorization | Bearer <access_token> |

**请求体:**

```json
{
  "query": "湖边散步的宁静时光",
  "type": "text",
  "topK": 10,
  "threshold": 0.75,
  "filters": {
    "tags": ["心情"],
    "dateRange": {
      "from": "2025-02-01T00:00:00Z",
      "to": "2025-06-15T23:59:59Z"
    }
  }
}
```

**响应:**

```json
{
  "code": 200,
  "message": "搜索完成",
  "data": {
    "results": [
      {
        "stoneId": "s_xyz789",
        "content": "今天在湖边看到了一只白鹭，心情很好",
        "similarity": 0.92,
        "authorName": "lakewalker",
        "createdAt": "2025-06-15T10:30:00Z"
      },
      {
        "stoneId": "s_abc456",
        "content": "傍晚沿着湖边慢跑，夕阳真美",
        "similarity": 0.87,
        "authorName": "sunrunner",
        "createdAt": "2025-02-19T17:45:00Z"
      }
    ],
    "totalFound": 2,
    "processedLocally": true,
    "latencyMs": 22
  },
  "timestamp": 1708387200
}
```

---

## 7. 管理后台 API

### 7.1 管理员登录

- **方法:** `POST`
- **路径:** `/api/admin/login`
- **描述:** 管理员使用独立账号登录后台，返回管理员令牌

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |

**请求体:**

```json
{
  "username": "admin",
  "password": "Admin@HeartLake2025"
}
```

**响应:**

```json
{
  "code": 200,
  "message": "登录成功",
  "data": {
    "admin": {
      "id": "a_root01",
      "username": "admin",
      "role": "super_admin",
      "lastLoginAt": "2025-02-19T22:00:00Z"
    },
    "accessToken": "eyJhbGciOiJIUzI1NiIs..."
  },
  "timestamp": 1708387200
}
```

---

### 7.2 仪表盘数据

- **方法:** `GET`
- **路径:** `/api/admin/stats/dashboard`
- **描述:** 获取管理后台仪表盘概览数据，包括用户数、内容数、活跃度等

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <admin_access_token> |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "users": {
      "total": 15820,
      "newToday": 128,
      "activeToday": 3456,
      "bannedTotal": 23
    },
    "stones": {
      "total": 89450,
      "newToday": 1024,
      "reportedPending": 12
    },
    "ripples": {
      "total": 234500,
      "newToday": 5600
    },
    "boats": {
      "total": 45200,
      "newToday": 890
    },
    "edgeAi": {
      "status": "running",
      "inferenceToday": 52480,
      "avgLatencyMs": 12.5
    },
    "period": "today"
  },
  "timestamp": 1708387200
}
```

---

### 7.3 用户列表

- **方法:** `GET`
- **路径:** `/api/admin/users`
- **描述:** 分页获取平台用户列表，支持按状态、注册时间筛选和搜索

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <admin_access_token> |

**查询参数:**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| page | number | 否 | 页码，默认 1 |
| limit | number | 否 | 每页数量，默认 20 |
| status | string | 否 | 用户状态：`active` / `banned` / `all`（默认） |
| search | string | 否 | 按用户名或邮箱搜索 |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "items": [
      {
        "id": "u_abc123",
        "username": "lakewalker",
        "email": "walker@heartlake.app",
        "avatar": "https://cdn.heartlake.app/avatars/u_abc123.jpg",
        "status": "active",
        "stoneCount": 42,
        "createdAt": "2025-01-15T08:00:00Z",
        "lastActiveAt": "2025-06-15T09:30:00Z"
      }
    ],
    "pagination": {
      "page": 1,
      "limit": 20,
      "total": 15820,
      "totalPages": 791
    }
  },
  "timestamp": 1708387200
}
```

---

### 7.4 封禁用户

- **方法:** `POST`
- **路径:** `/api/admin/users/:id/ban`
- **描述:** 封禁指定用户，封禁后该用户无法登录和使用任何功能

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |
| Authorization | Bearer <admin_access_token> |

**路径参数:**

| 参数 | 类型 | 说明 |
|------|------|------|
| id | string | 用户 ID |

**请求体:**

```json
{
  "reason": "多次发布违规内容",
  "duration": "permanent"
}
```

**响应:**

```json
{
  "code": 200,
  "message": "用户已封禁",
  "data": {
    "userId": "u_bad456",
    "username": "spammer01",
    "status": "banned",
    "reason": "多次发布违规内容",
    "duration": "permanent",
    "bannedAt": "2025-06-15T11:00:00Z",
    "bannedBy": "a_root01"
  },
  "timestamp": 1708387200
}
```

---

### 7.5 内容列表

- **方法:** `GET`
- **路径:** `/api/admin/stones`
- **描述:** 管理后台获取内容列表，支持按审核状态筛选，用于内容管理和审核

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <admin_access_token> |

**查询参数:**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| page | number | 否 | 页码，默认 1 |
| limit | number | 否 | 每页数量，默认 20 |
| status | string | 否 | 审核状态：`pending` / `approved` / `rejected` / `all`（默认） |
| reported | boolean | 否 | 是否仅显示被举报内容 |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "items": [
      {
        "id": "s_xyz789",
        "authorId": "u_abc123",
        "authorName": "lakewalker",
        "content": "今天在湖边看到了一只白鹭，心情很好",
        "images": [
          "https://cdn.heartlake.app/uploads/img_001.jpg"
        ],
        "tags": ["心情", "自然"],
        "status": "approved",
        "reportCount": 0,
        "aiModeration": {
          "approved": true,
          "overallRisk": "low"
        },
        "createdAt": "2025-06-15T10:30:00Z"
      }
    ],
    "pagination": {
      "page": 1,
      "limit": 20,
      "total": 89450,
      "totalPages": 4473
    }
  },
  "timestamp": 1708387200
}
```

---

### 7.6 获取边缘 AI 配置

- **方法:** `GET`
- **路径:** `/api/admin/edge-ai/config`
- **描述:** 获取边缘 AI 引擎的当前配置参数

**请求头:**

| 字段 | 值 |
|------|----|
| Authorization | Bearer <admin_access_token> |

**响应:**

```json
{
  "code": 200,
  "message": "ok",
  "data": {
    "sentimentAnalysis": {
      "enabled": true,
      "model": "sentiment_v3",
      "threshold": 0.7,
      "languages": ["zh", "en"]
    },
    "contentModeration": {
      "enabled": true,
      "model": "moderation_v2",
      "autoReject": true,
      "autoRejectThreshold": 0.95
    },
    "vectorSearch": {
      "enabled": true,
      "model": "vectorizer_v1",
      "dimensions": 768,
      "indexType": "HNSW"
    },
    "federatedLearning": {
      "enabled": true,
      "minParticipants": 10,
      "roundIntervalHours": 6,
      "privacyBudgetPerUser": 10.0
    },
    "resourceLimits": {
      "maxCpuPercent": 80,
      "maxMemoryMb": 2048,
      "maxGpuPercent": 90
    }
  },
  "timestamp": 1708387200
}
```

---

### 7.7 更新边缘 AI 配置

- **方法:** `PUT`
- **路径:** `/api/admin/edge-ai/config`
- **描述:** 更新边缘 AI 引擎的配置参数，修改后实时生效

**请求头:**

| 字段 | 值 |
|------|----|
| Content-Type | application/json |
| Authorization | Bearer <admin_access_token> |

**请求体:**

```json
{
  "sentimentAnalysis": {
    "enabled": true,
    "threshold": 0.8
  },
  "contentModeration": {
    "autoRejectThreshold": 0.90
  },
  "federatedLearning": {
    "minParticipants": 15,
    "roundIntervalHours": 4
  },
  "resourceLimits": {
    "maxMemoryMb": 4096
  }
}
```

**响应:**

```json
{
  "code": 200,
  "message": "配置已更新",
  "data": {
    "updated": true,
    "changedFields": [
      "sentimentAnalysis.threshold",
      "contentModeration.autoRejectThreshold",
      "federatedLearning.minParticipants",
      "federatedLearning.roundIntervalHours",
      "resourceLimits.maxMemoryMb"
    ],
    "appliedAt": "2025-06-15T11:30:00Z"
  },
  "timestamp": 1708387200
}
```
