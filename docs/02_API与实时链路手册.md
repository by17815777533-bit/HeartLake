# HeartLake API 与实时链路手册

## 1. 设计目标
本手册面向交付评审，说明三件事：
- API 链路如何完整跑通
- 实时链路如何鉴权并保持稳定
- 每个关键技术约束为什么这样设计

## 2. 核心地址
- API 基地址：`http://127.0.0.1:8080/api`
- WebSocket：`ws://127.0.0.1:8080/ws/broadcast`

## 3. 鉴权体系

### 3.1 匿名登录
请求：
```bash
curl -X POST http://127.0.0.1:8080/api/auth/anonymous \
  -H 'Content-Type: application/json' \
  -d '{"device_id":"web_local_001"}'
```

返回关键字段：
- `token`：后续请求鉴权凭据
- `user_id`：当前匿名用户主键
- `is_new_user`：新用户引导判定

为什么是匿名登录：
- 产品定位强调低门槛进入，匿名可降低首登流失。
- 通过 `device_id` + 恢复关键词组合，兼顾“低门槛”和“可找回”。

### 3.2 Token 透传规则
HTTP 请求头：
```text
Authorization: Bearer <token>
```

技术原因：
- 与 WebSocket query 机制统一 token 来源，降低多端适配复杂度。

## 4. 高频接口分组（联调必测）

### 4.1 心湖与推荐
- `GET /lake/stones?page=1&page_size=20&sort=latest`
- `GET /recommendations/trending?limit=20`
- `GET /recommendations/similar-stones/{stone_id}?limit=6`

设计原因：
- 心湖主流量入口必须优先缓存与降级，保证首页可用。

### 4.2 情绪与 AI
- `POST /edge-ai/analyze`
- `GET /edge-ai/privacy-budget`
- `GET /edge-ai/emotion-pulse`
- `GET /recommendations/emotion-trends`

设计原因：
- 把“重计算接口”和“轻查询接口”拆分，便于在高压下单独限流。

### 4.3 社交与个人域
- `GET /friends`
- `GET /vip/status`
- `GET /users/my/boats?page=1&page_size=100`
- `GET /interactions/my/ripples?page=1&page_size=50`

设计原因：
- 个人域读接口天然适合缓存，提高页面二次进入速度。

## 5. WebSocket 鉴权与稳定性策略

### 5.1 Query 鉴权（优先）
```text
ws://127.0.0.1:8080/ws/broadcast?token=<url_encoded_token>
```

原因：
- 在握手阶段即完成鉴权，减少无效连接占用资源。

### 5.2 首包鉴权（兼容旧客户端）
```json
{"type":"auth","token":"<token>"}
```

原因：
- 历史客户端不带 query token 时仍可接入，保证兼容升级。

### 5.3 房间事件
- 加入：`{"type":"join","room":"lake"}`
- 离开：`{"type":"leave","room":"lake"}`
- 心跳：服务端 `ping`，客户端 `pong`

原因：
- 心跳用于快速识别断链，避免“假在线”导致消息不同步。

## 6. 缓存策略与一致性约束
- 高频 GET 使用短 TTL 缓存（如 180s / 300s）。
- 实时事件到达时按前缀主动清理相关缓存（如 `stones_`）。

原因：
- 在稳定性与一致性之间取平衡：页面秒开 + 可控时延同步。

## 7. 错误处理约束
- HTTP `5xx` 必须记录后端日志并快速定位 schema/业务异常。
- 前端遇到可恢复错误不允许直接白屏，必须给出可操作反馈。
- 网络抖动不应触发无限重试风暴。

## 8. 兼容性红线
- 不变更已发布 endpoint path。
- 不变更核心字段名（`token`、`stone_id`、`mood`、`user_id`）。
- 新增能力通过兼容字段/可选字段扩展。

原因：
- 前后端与移动端并行迭代时，接口稳定性优先级高于局部重构收益。
