# API 接口全量清单

本文只列当前主要业务会使用的有效入口，不列内部别名和不可用路由。

当前基地址：

- 用户 API：`http://121.41.195.165/api`
- 管理 API：`http://121.41.195.165/api/admin`
- WebSocket：`ws://121.41.195.165/ws/broadcast`

## 1. 通用约定

- 用户接口默认使用 Bearer PASETO。
- 管理接口默认使用管理员 Bearer PASETO。
- 集合接口统一返回 `items / total / page / page_size`。
- 失败接口统一返回明确 `code / message`。

## 2. 健康检查

- `GET /api/health`
- `GET /api/health/detailed`

## 3. 认证与用户

- `POST /api/auth/anonymous`
- `POST /api/auth/recover`
- `POST /api/auth/refresh`
- `GET /api/users/{id}`
- `GET /api/users/{id}/stats`
- `GET /api/users/search`
- `GET /api/users/my/boats`
- `PUT /api/users/my/profile`
- `GET /api/users/my/emotion-calendar`
- `GET /api/users/my/emotion-heatmap`

## 4. 账号与隐私

- `GET /api/account/info`
- `POST /api/account/avatar`
- `PUT /api/account/profile`
- `GET /api/account/stats`
- `GET /api/account/devices`
- `DELETE /api/account/devices/{sessionId}`
- `GET /api/account/login-logs`
- `GET /api/account/security-events`
- `GET /api/account/privacy`
- `PUT /api/account/privacy`
- `GET /api/account/blocked-users`
- `POST /api/account/block/{targetUserId}`
- `DELETE /api/account/unblock/{targetUserId}`
- `POST /api/account/export`
- `GET /api/account/export/{taskId}`
- `POST /api/account/deactivate`
- `POST /api/account/delete-permanent`
- `GET /api/lake/privacy-stats`
- `GET /api/lake/privacy-report`

## 5. 媒体

- `POST /api/media/upload`
- `GET /api/media/{category}/{filename}`

## 6. 石头与湖面

- `POST /api/stones`
- `GET /api/lake/stones`
- `GET /api/stones/my`
- `GET /api/stones/{stoneId}`
- `DELETE /api/stones/{stoneId}`
- `GET /api/lake/weather`
- `GET /api/stones/{stoneId}/resonance`

## 7. 互动

- `POST /api/stones/{stoneId}/ripples`
- `POST /api/stones/{stoneId}/boats`
- `GET /api/stones/{stoneId}/boats`
- `GET /api/interactions/my/ripples`
- `GET /api/interactions/my/boats`
- `DELETE /api/ripples/{rippleId}`
- `DELETE /api/boats/{boatId}`
- `GET /api/boats/{boatId}`
- `GET /api/boats/sent`
- `GET /api/boats/received`
- `POST /api/boats/reply`
- `GET /api/notifications`
- `POST /api/notifications/{notificationId}/read`
- `POST /api/notifications/read-all`
- `GET /api/notifications/unread-count`
- `POST /api/stones/{stoneId}/connections`
- `POST /api/connections`
- `GET /api/connections/{connectionId}/messages`
- `POST /api/connections/{connectionId}/messages`

## 8. 好友与临时好友

- `POST /api/friends/request`
- `DELETE /api/friends/{friendId}`
- `GET /api/friends`
- `POST /api/friends/{friendId}/messages`
- `GET /api/friends/{friendId}/messages`
- `GET /api/temp-friends`
- `GET /api/temp-friends/{tempFriendId}`
- `DELETE /api/temp-friends/{tempFriendId}`
- `GET /api/temp-friends/check/{targetUserId}`

## 9. 关怀与增值

- `GET /api/vip/status`
- `GET /api/vip/privileges`
- `GET /api/vip/counseling/check`
- `POST /api/vip/counseling/book`
- `GET /api/vip/ai-comment-frequency`
- `GET /api/safe-harbor/hotlines`
- `GET /api/safe-harbor/tools`
- `GET /api/safe-harbor/prompt`
- `GET /api/safe-harbor/resources`
- `GET /api/safe-harbor/recommend`
- `POST /api/safe-harbor/resources`
- `PUT /api/safe-harbor/resources/{id}`
- `DELETE /api/safe-harbor/resources/{id}`
- `POST /api/safe-harbor/access`
- `GET /api/safe-harbor/access/history`
- `POST /api/consultation/session`
- `POST /api/consultation/key-exchange`
- `POST /api/consultation/message`
- `GET /api/consultation/messages/{sessionId}`
- `GET /api/consultation/sessions`
- `POST /api/reports`
- `GET /api/reports/my`

## 10. 推荐与 AI

- `GET /api/recommendations/stones`
- `GET /api/recommendations/discover/{mood}`
- `POST /api/recommendations/track`
- `POST /api/recommendations/track-batch`
- `GET /api/recommendations/emotion-trends`
- `GET /api/recommendations/trending`
- `POST /api/recommendations/search`
- `GET /api/recommendations/advanced`
- `GET /api/admin/recommendations/advanced`
- `GET /api/recommendations/similar-stones/{id}`
- `GET /api/recommendations/personalized`
- `POST /api/admin/stones/{id}/embedding`
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
- `GET /api/edge-ai/pulse-history`
- `POST /api/edge-ai/federated/submit`
- `POST /api/edge-ai/privacy/reset`
- `POST /api/edge-ai/nodes/register`
- `PUT /api/edge-ai/nodes/status`
- `GET /api/edge-ai/nodes/best`
- `POST /api/edge-ai/quantized-forward`
- `POST /api/edge-ai/privacy/add-noise`
- `GET /api/edge-ai/config`
- `PUT /api/edge-ai/config`
- `POST /api/edge-ai/summary`
- `POST /api/lake-god/chat`
- `GET /api/lake-god/history`

## 11. 管理后台

### 认证与基础信息

- `POST /api/admin/login`
- `POST /api/admin/logout`
- `GET /api/admin/info`

### 统计与风控

- `GET /api/admin/stats/realtime`
- `GET /api/admin/stats/dashboard`
- `GET /api/admin/stats/trending-topics`
- `GET /api/admin/stats/user-growth`
- `GET /api/admin/stats/mood-distribution`
- `GET /api/admin/stats/mood-trend`
- `GET /api/admin/stats/active-time`
- `GET /api/admin/risk/high-risk-users`
- `GET /api/admin/risk/events`
- `GET /api/admin/risk/user/{user_id}/history`
- `POST /api/admin/risk/event/{event_id}/handle`
- `GET /api/admin/security/audit`

### 内容与用户治理

- `GET /api/admin/users`
- `GET /api/admin/users/{id}`
- `PUT /api/admin/users/{id}/status`
- `POST /api/admin/users/{id}/ban`
- `POST /api/admin/users/{id}/unban`
- `GET /api/admin/content`
- `GET /api/admin/stones`
- `GET /api/admin/stones/{id}`
- `DELETE /api/admin/stones/{id}`
- `GET /api/admin/boats`
- `DELETE /api/admin/boats/{id}`
- `GET /api/admin/moderation/pending`
- `POST /api/admin/moderation/{id}/approve`
- `POST /api/admin/moderation/{id}/reject`
- `GET /api/admin/moderation/history`
- `GET /api/admin/reports`
- `GET /api/admin/reports/{id}`
- `POST /api/admin/reports/{id}/handle`
- `GET /api/admin/sensitive-words`
- `POST /api/admin/sensitive-words`
- `PUT /api/admin/sensitive-words/{id}`
- `DELETE /api/admin/sensitive-words/{id}`
- `GET /api/admin/config`
- `PUT /api/admin/config`
- `POST /api/admin/broadcast`
- `GET /api/admin/broadcast/history`
- `GET /api/admin/logs`

### 管理端 EdgeAI 镜像路由

- `GET /api/admin/edge-ai/status`
- `GET /api/admin/edge-ai/metrics`
- `POST /api/admin/edge-ai/analyze`
- `POST /api/admin/edge-ai/moderate`
- `GET /api/admin/edge-ai/emotion-pulse`
- `POST /api/admin/edge-ai/federated/aggregate`
- `GET /api/admin/edge-ai/privacy-budget`
- `POST /api/admin/edge-ai/vector-search`
- `POST /api/admin/edge-ai/vector-insert`
- `POST /api/admin/edge-ai/emotion-sample`
- `GET /api/admin/edge-ai/pulse-history`
- `POST /api/admin/edge-ai/federated/submit`
- `POST /api/admin/edge-ai/privacy/reset`
- `POST /api/admin/edge-ai/nodes/register`
- `PUT /api/admin/edge-ai/nodes/status`
- `GET /api/admin/edge-ai/nodes/best`
- `POST /api/admin/edge-ai/quantized-forward`
- `POST /api/admin/edge-ai/privacy/add-noise`
- `GET /api/admin/edge-ai/config`
- `PUT /api/admin/edge-ai/config`

## 12. WebSocket

- `GET /ws/broadcast`

当前 WebSocket 接受 `join`、`leave`、`room_message`、`ping`，并返回 `auth_success`、`pong`、`error` 与业务广播事件。
