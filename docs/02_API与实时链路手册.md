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

## 9. 关键接口契约（含状态码语义）

### 9.1 涟漪接口（幂等行为）
- `POST /stones/{stone_id}/ripples`
- 状态码：
  - `200`：首次点涟漪成功，或重复点击返回幂等成功。
  - `400`：石头不存在等参数/业务错误。
- 关键字段：
  - `ripple_id`
  - `ripple_count`
  - `already_rippled`（重复点击时为 `true`）

设计原因：
- 前端高频点击场景下，幂等成功比“重复操作报错”更符合交互预期，也能减少误判为权限问题。

### 9.2 好友消息接口
- `GET /friends/{friend_id}/messages`
- `POST /friends/{friend_id}/messages`
- 状态码：
  - `200`：可聊天，读取或发送成功。
  - `403`：亲密分不足或该关系已被用户主动移除。

设计原因：
- 维持“非无门槛”社交治理，同时让用户可主动控制联系人可见性。

### 9.3 情绪日历接口
- `GET /users/my/emotion-calendar?year=YYYY&month=MM`
- 返回：
  - `days` 对象（键可能为日期或日序，前端会做兼容归一化）
  - 每天包含 `score`，可选 `mood`

设计原因：
- 服务端与前端历史版本存在 key 形式差异，客户端兼容可以避免评审现场因为字段格式小差异而功能失效。

## 10. 实时事件字段规范（前后端一致性）

### 10.1 `new_stone`
```json
{
  "type": "new_stone",
  "stone": { "...": "..." },
  "triggered_by": "anonymous_xxx",
  "timestamp": 1700000000
}
```

### 10.2 `ripple_update`
```json
{
  "type": "ripple_update",
  "stone_id": "stone_xxx",
  "ripple_count": 12,
  "triggered_by": "anonymous_xxx",
  "timestamp": 1700000000
}
```

### 10.3 `new_friend_message`
```json
{
  "type": "new_friend_message",
  "sender_id": "anonymous_xxx",
  "receiver_id": "anonymous_yyy",
  "content": "消息内容",
  "timestamp": 1700000000
}
```

字段约束原因：
- `triggered_by/sender_id` 是前端去重与“是否本机触发”判断的核心。
- 时间戳统一为秒级整数，减少多端解析歧义。

## 11. 缓存与实时一致性设计说明
- GET 缓存：用于高频列表与统计接口提速。
- 变更失效：当 POST/PUT/DELETE 成功后，按用户前缀与业务前缀清理缓存。
- 实时推送：事件抵达后按业务实体刷新本地状态（如石头计数、消息列表）。

为什么这样做：
- 单靠短 TTL 会出现“明明提交成功但 UI 仍旧旧值”的窗口期。
- 单靠实时事件会在断链时漏更新；缓存 + 事件组合能提升收敛稳定性。

## 12. 近期实测样例（可直接放评审材料）

### 12.1 重复涟漪幂等返回（已验证）
```json
{
  "code": 0,
  "data": {
    "already_rippled": true,
    "ripple_count": 3,
    "ripple_id": "showcase_ripple_67b908c035bbaac696bd2962",
    "stone_id": "showcase_stone_0071_01",
    "user_id": "anonymous_8a92d411ffb0"
  },
  "message": "已经点过涟漪了"
}
```

### 12.2 删除好友后隐藏（已验证）
```json
{
  "code": 0,
  "data": {
    "friend_id": "anonymous_a9b683ffa7e0",
    "mode": "intimacy_auto_hidden",
    "success": true
  },
  "message": "已从好友列表移除"
}
```

### 12.3 被移除后发送私聊（已验证）
```json
{
  "code": 403,
  "data": {},
  "message": "你已移除此好友，暂不可私聊"
}
```

## 13. 情绪模块同步策略（趋势/热力图/日历）

### 13.1 为什么三者都要做“缓存旁路 + 实时触发”
- 共性问题：三者都展示“用户最近状态”，如果沿用普通短 TTL 缓存，会出现“发布后页面不变”的主观故障。
- 统一策略：
  - 请求侧：相关接口默认 `useCache=false`。
  - 事件侧：监听 `new_stone` 且按 `triggered_by` 过滤为当前用户后再刷新。
  - 房间侧：显式 `joinRoom('lake')`，避免只注册监听但收不到房间消息。

### 13.2 适用接口
- `GET /users/my/emotion-calendar`
- `GET /users/my/emotion-heatmap`
- `GET /recommendations/emotion-trends`
- `GET /edge-ai/emotion-pulse`

### 13.3 触发流程（发布石头后）
1. 客户端调用 `POST /stones`。  
2. 服务端投递 `new_stone` 到 `lake` 房间。  
3. 情绪页监听器收到事件并校验 `triggered_by == currentUserId`。  
4. 页面主动调用对应查询接口刷新视图。  

收益：
- 解决“必须手动刷新才能看到更新”的体验问题。

## 14. 好友域契约细节（删除/恢复/消息）

### 14.1 删除好友语义
- 接口：`DELETE /friends/{friend_id}`
- 语义：在亲密分自动模式下为“当前用户隐藏该好友”，而非全局硬删除。
- 返回关键字段：
  - `mode=intimacy_auto_hidden`

设计原因：
- 保留亲密分自动关系模型，不引入复杂的人工状态机。

### 14.2 恢复好友显示
- 接口：`POST /friends/request`
- 自动模式下会尝试清理当前用户对该好友的 `removed` 标记。
- 返回扩展字段：
  - `restored_hidden_friend`：是否执行了恢复动作。

设计原因：
- 防止“删了就再也回不来”，同时保持非无门槛聊天策略。

### 14.3 消息能力前置条件
- `score >= 12` 才可私聊。
- 若当前用户已将对方标记为 `removed`，消息读写统一返回 `403`。

## 15. 接口演进与兼容策略
- 原则 1：路径不变，字段增量扩展。
- 原则 2：前端对关键字段做兼容兜底（如 `friend_id/user_id`）。
- 原则 3：服务端错误语义明确化，避免前端只能提示“网络异常”。

## 16. 调试建议（接口级）
- 优先看响应体 `code/message`，再看 HTTP 状态码。
- 对“看起来成功但界面没刷新”的问题，先检查：
  - 请求是否 `useCache=false`
  - 是否已加入对应 WS 房间
  - 是否按 `triggered_by` 做了过滤

## 17. 评审答辩可用的技术说明句
- “我们把情绪三件套统一成了事件驱动刷新，不依赖短 TTL 兜底。”  
- “好友删除不是硬删除，而是可恢复隐藏，既保留治理又避免关系不可逆。”  
- “消息权限由亲密分和用户显式隐藏共同约束，不是简单放开也不是纯封锁。”  
