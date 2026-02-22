# HeartLake API 文档

## 概述

HeartLake 后端 API 基于 Drogon 框架构建，提供 RESTful 风格的接口。所有接口统一使用 JSON 格式进行数据交互。

**基础 URL**: `http://localhost:8080`

**认证方式**: PASETO v4 Token（部分接口需要）

**响应格式**:
```json
{
  "code": 200,
  "message": "ok",
  "data": { ... },
  "timestamp": "2026-02-21T12:00:00Z"
}
```

---

## 1. 健康检查模块

### 1.1 基础健康检查
- **Method**: `GET`
- **Path**: `/api/health`
- **描述**: 获取服务基础健康状态
- **认证**: 无需认证
- **响应**: 服务状态、版本号、时间戳

### 1.2 详细健康检查
- **Method**: `GET`
- **Path**: `/api/health/detailed`
- **描述**: 获取详细健康信息（数据库、Redis、内存、运行时间）
- **认证**: 无需认证
- **响应**: 数据库连接状态、Redis状态、内存使用、服务运行时长

---

## 2. 账号管理模块

### 2.1 个人信息管理

#### 获取账号信息
- **Method**: `GET`
- **Path**: `/api/account/info`
- **描述**: 获取完整账号信息
- **认证**: 需要 PASETO Token

#### 更新头像
- **Method**: `POST`
- **Path**: `/api/account/avatar`
- **描述**: 上传并更新用户头像
- **认证**: 需要 PASETO Token

#### 更新个人资料
- **Method**: `PUT`
- **Path**: `/api/account/profile`
- **描述**: 更新昵称、简介等个人信息
- **认证**: 需要 PASETO Token

#### 获取账号统计
- **Method**: `GET`
- **Path**: `/api/account/stats`
- **描述**: 获取账号统计数据（活跃度、内容数量）
- **认证**: 需要 PASETO Token

### 2.2 账号安全

#### 获取登录设备列表
- **Method**: `GET`
- **Path**: `/api/account/devices`
- **描述**: 查看所有登录设备
- **认证**: 需要 PASETO Token

#### 移除登录设备
- **Method**: `DELETE`
- **Path**: `/api/account/devices/{sessionId}`
- **描述**: 强制登出指定设备
- **认证**: 需要 PASETO Token

#### 获取登录日志
- **Method**: `GET`
- **Path**: `/api/account/login-logs`
- **描述**: 查看登录历史记录
- **认证**: 需要 PASETO Token

#### 获取安全事件
- **Method**: `GET`
- **Path**: `/api/account/security-events`
- **描述**: 查看安全相关事件
- **认证**: 需要 PASETO Token

#### 修改密码
- **Method**: `POST`
- **Path**: `/api/account/change-password`
- **描述**: 修改账号密码（需要旧密码验证）
- **认证**: 需要 PASETO Token

#### 绑定邮箱
- **Method**: `POST`
- **Path**: `/api/account/bind-email`
- **描述**: 绑定邮箱地址
- **认证**: 需要 PASETO Token

#### 解绑邮箱
- **Method**: `POST`
- **Path**: `/api/account/unbind-email`
- **描述**: 解除邮箱绑定
- **认证**: 需要 PASETO Token

### 2.3 隐私设置

#### 获取隐私设置
- **Method**: `GET`
- **Path**: `/api/account/privacy`
- **描述**: 获取当前隐私设置
- **认证**: 需要 PASETO Token

#### 更新隐私设置
- **Method**: `PUT`
- **Path**: `/api/account/privacy`
- **描述**: 更新隐私设置（可见性等）
- **认证**: 需要 PASETO Token

#### 获取黑名单
- **Method**: `GET`
- **Path**: `/api/account/blocked-users`
- **描述**: 查看已拉黑的用户列表
- **认证**: 需要 PASETO Token

#### 拉黑用户
- **Method**: `POST`
- **Path**: `/api/account/block/{targetUserId}`
- **描述**: 将指定用户加入黑名单
- **认证**: 需要 PASETO Token

#### 取消拉黑
- **Method**: `DELETE`
- **Path**: `/api/account/unblock/{targetUserId}`
- **描述**: 将用户从黑名单移除
- **认证**: 需要 PASETO Token

### 2.4 数据管理

#### 导出个人数据
- **Method**: `POST`
- **Path**: `/api/account/export`
- **描述**: 创建数据导出任务
- **认证**: 需要 PASETO Token

#### 获取导出任务状态
- **Method**: `GET`
- **Path**: `/api/account/export/{taskId}`
- **描述**: 查询数据导出任务进度
- **认证**: 需要 PASETO Token

#### 注销账号
- **Method**: `POST`
- **Path**: `/api/account/deactivate`
- **描述**: 软删除账号（可恢复）
- **认证**: 需要 PASETO Token

#### 永久删除账号
- **Method**: `POST`
- **Path**: `/api/account/delete-permanent`
- **描述**: 永久删除账号及所有数据
- **认证**: 需要 PASETO Token

---

## 3. 石头模块

### 3.1 创建石头
- **Method**: `POST`
- **Path**: `/api/stones`
- **描述**: 创建新的情绪石头
- **认证**: 需要 PASETO Token + SecurityAuditFilter
- **请求体**: `{ "content": "...", "mood": "...", "tags": [...] }`

### 3.2 获取湖面石头列表
- **Method**: `GET`
- **Path**: `/api/lake/stones`
- **描述**: 获取公开的石头列表（分页）
- **认证**: 无需认证
- **查询参数**: `page`, `page_size`

### 3.3 获取我的石头
- **Method**: `GET`
- **Path**: `/api/stones/my`
- **描述**: 获取当前用户创建的所有石头
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 3.4 获取石头详情
- **Method**: `GET`
- **Path**: `/api/stones/{stoneId}`
- **描述**: 获取指定石头的详细信息
- **认证**: 无需认证

### 3.5 删除石头
- **Method**: `DELETE`
- **Path**: `/api/stones/{stoneId}`
- **描述**: 删除指定石头
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 3.6 获取湖面天气
- **Method**: `GET`
- **Path**: `/api/lake/weather`
- **描述**: 获取湖面整体情绪氛围
- **认证**: 无需认证
- **响应**: 情绪分布、活跃度、天气状态

### 3.7 搜索共鸣石头
- **Method**: `GET`
- **Path**: `/api/stones/{stoneId}/resonance`
- **描述**: 基于向量相似度搜索情感共鸣的石头
- **认证**: 需要 PASETO Token + SecurityAuditFilter

---

## 4. 纸船模块

### 4.1 放纸船（漂流）
- **Method**: `POST`
- **Path**: `/api/boats/drift`
- **描述**: 创建漂流纸船
- **认证**: 需要 PASETO Token
- **请求体**:
```json
{
  "content": "想对世界说的话...",
  "mood": "hopeful",
  "drift_mode": "random",  // random | directed | wish
  "receiver_id": null,
  "boat_style": "paper"
}
```

### 4.2 回复石头
- **Method**: `POST`
- **Path**: `/api/boats/reply`
- **描述**: 创建关联到特定石头的纸船
- **认证**: 需要 PASETO Token
- **请求体**: `{ "stone_id": "...", "content": "...", "mood": "..." }`

### 4.3 捞纸船
- **Method**: `POST`
- **Path**: `/api/boats/catch`
- **描述**: 随机捞取一个漂流中的纸船
- **认证**: 需要 PASETO Token
- **响应**: 纸船内容、是否为AI自动回复

### 4.4 回应纸船
- **Method**: `POST`
- **Path**: `/api/boats/{boatId}/respond`
- **描述**: 回复收到的纸船
- **认证**: 需要 PASETO Token
- **请求体**: `{ "content": "回复内容..." }`

### 4.5 扔回水中
- **Method**: `POST`
- **Path**: `/api/boats/{boatId}/release`
- **描述**: 将纸船重新放回漂流池
- **认证**: 需要 PASETO Token

### 4.6 获取纸船详情
- **Method**: `GET`
- **Path**: `/api/boats/{boatId}`
- **描述**: 查看纸船详细信息
- **认证**: 需要 PASETO Token

### 4.7 获取我发送的纸船
- **Method**: `GET`
- **Path**: `/api/boats/sent`
- **描述**: 查看我发送的所有纸船
- **认证**: 需要 PASETO Token
- **查询参数**: `page`, `page_size`, `status`

### 4.8 获取我收到的纸船
- **Method**: `GET`
- **Path**: `/api/boats/received`
- **描述**: 查看我收到的所有纸船
- **认证**: 需要 PASETO Token
- **查询参数**: `page`, `page_size`

### 4.9 获取漂流中纸船数量
- **Method**: `GET`
- **Path**: `/api/boats/drifting/count`
- **描述**: 获取当前漂流池中的纸船总数
- **认证**: 无需认证

### 4.10 获取纸船状态
- **Method**: `GET`
- **Path**: `/api/boats/{boatId}/status`
- **描述**: 查询纸船当前状态（漂流中/已捞起/已回复）
- **认证**: 需要 PASETO Token

---

## 5. 好友模块

### 5.1 发送好友请求
- **Method**: `POST`
- **Path**: `/api/friends/request`
- **描述**: 向指定用户发送好友申请
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 5.2 接受好友请求
- **Method**: `POST`
- **Path**: `/api/friends/accept/{userId}`
- **描述**: 接受好友申请
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 5.3 拒绝好友请求
- **Method**: `POST`
- **Path**: `/api/friends/reject/{userId}`
- **描述**: 拒绝好友申请
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 5.4 删除好友
- **Method**: `DELETE`
- **Path**: `/api/friends/{friendId}`
- **描述**: 删除好友关系
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 5.5 获取好友列表
- **Method**: `GET`
- **Path**: `/api/friends`
- **描述**: 获取所有好友
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 5.6 获取待处理请求
- **Method**: `GET`
- **Path**: `/api/friends/requests/pending`
- **描述**: 获取待处理的好友申请列表
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 5.7 发送消息
- **Method**: `POST`
- **Path**: `/api/friends/{friendId}/messages`
- **描述**: 向好友发送私信
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 5.8 获取消息记录
- **Method**: `GET`
- **Path**: `/api/friends/{friendId}/messages`
- **描述**: 获取与好友的聊天记录
- **认证**: 需要 PASETO Token + SecurityAuditFilter

---

## 6. 推荐模块

### 6.1 获取推荐石头
- **Method**: `GET`
- **Path**: `/api/recommendations/stones`
- **描述**: 获取个性化推荐的石头（混合推荐算法）
- **认证**: 需要 PASETO Token + SecurityAuditFilter
- **算法**: 协同过滤 40% + 内容过滤 40% + 随机探索 20%

### 6.2 基于情绪发现
- **Method**: `GET`
- **Path**: `/api/recommendations/discover/{mood}`
- **描述**: 发现特定情绪的用户和内容
- **认证**: 需要 PASETO Token + SecurityAuditFilter
- **路径参数**: `mood` - 情绪类型（如 happy, sad, anxious）

### 6.3 基于情绪发现（别名）
- **Method**: `GET`
- **Path**: `/api/discover/{mood}`
- **描述**: 同上（简短路径）
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 6.4 记录用户交互
- **Method**: `POST`
- **Path**: `/api/recommendations/track`
- **描述**: 记录用户交互行为（用于学习偏好）
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 6.5 获取情绪趋势
- **Method**: `GET`
- **Path**: `/api/recommendations/emotion-trends`
- **描述**: 获取个人情绪变化曲线
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 6.6 获取热门内容
- **Method**: `GET`
- **Path**: `/api/recommendations/trending`
- **描述**: 获取当前热门的石头、标签、情绪
- **认证**: 无需认证

### 6.7 搜索推荐内容
- **Method**: `POST`
- **Path**: `/api/recommendations/search`
- **描述**: 全文搜索 + 智能排序
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 6.8 批量追踪交互
- **Method**: `POST`
- **Path**: `/api/recommendations/track-batch`
- **描述**: 批量上传用户交互数据
- **认证**: 需要 PASETO Token + SecurityAuditFilter

### 6.9 高级推荐
- **Method**: `GET`
- **Path**: `/api/recommendations/advanced`
- **描述**: 使用 UCB、Thompson Sampling、MMR 等高级算法
- **认证**: 需要 PASETO Token + SecurityAuditFilter

---

## 7. 向量搜索模块

### 7.1 获取相似石头
- **Method**: `GET`
- **Path**: `/api/recommendations/similar-stones/{stoneId}`
- **描述**: 基于向量相似度查找相似石头
- **认证**: 无需认证

### 7.2 获取个性化推荐
- **Method**: `GET`
- **Path**: `/api/recommendations/personalized`
- **描述**: 基于用户画像的个性化推荐
- **认证**: 无需认证

### 7.3 更新石头向量
- **Method**: `POST`
- **Path**: `/api/admin/stones/{stoneId}/embedding`
- **描述**: 手动更新石头的向量嵌入（管理员）
- **认证**: 需要管理员权限

---

## 8. 边缘AI模块

### 8.1 公开接口

#### 获取AI引擎状态
- **Method**: `GET`
- **Path**: `/api/edge-ai/status`
- **描述**: 获取边缘AI引擎运行状态
- **认证**: SecurityAuditFilter
- **响应**: 各子系统健康状态、版本信息

#### 获取AI性能指标
- **Method**: `GET`
- **Path**: `/api/edge-ai/metrics`
- **描述**: 获取详细性能指标
- **认证**: SecurityAuditFilter
- **响应**: 缓存命中率、推理延迟、隐私预算消耗

#### 本地情感分析
- **Method**: `POST`
- **Path**: `/api/edge-ai/analyze`
- **描述**: 本地情感分析（不调用云端API）
- **认证**: SecurityAuditFilter
- **请求体**: `{ "text": "今天心情很好" }`

#### 本地内容审核
- **Method**: `POST`
- **Path**: `/api/edge-ai/moderate`
- **描述**: 本地内容安全审核
- **认证**: SecurityAuditFilter
- **请求体**: `{ "text": "...", "check_types": ["toxicity", "violence"] }`

#### 语义相似度计算
- **Method**: `POST`
- **Path**: `/api/edge-ai/similarity`
- **描述**: 计算两段文本的语义相似度
- **认证**: SecurityAuditFilter
- **请求体**: `{ "text1": "...", "text2": "..." }`

#### 向量检索
- **Method**: `POST`
- **Path**: `/api/edge-ai/vector-search`
- **描述**: 基于向量的语义搜索
- **认证**: SecurityAuditFilter
- **请求体**: `{ "query": "...", "top_k": 10 }`

#### 差分隐私查询
- **Method**: `POST`
- **Path**: `/api/edge-ai/dp-query`
- **描述**: 差分隐私保护的统计查询
- **认证**: SecurityAuditFilter
- **请求体**: `{ "query_type": "count", "epsilon": 1.0 }`

#### 获取隐私预算
- **Method**: `GET`
- **Path**: `/api/edge-ai/privacy-budget`
- **描述**: 查询当前隐私预算余额
- **认证**: SecurityAuditFilter

### 8.2 管理员接口

#### 联邦学习聚合
- **Method**: `POST`
- **Path**: `/api/edge-ai/admin/federated-aggregate`
- **描述**: 执行联邦学习模型聚合
- **认证**: AdminAuthFilter
- **请求体**: `{ "round": 1, "gradients": [...], "epsilon": 1.0 }`

#### 更新AI配置
- **Method**: `POST`
- **Path**: `/api/edge-ai/admin/config`
- **描述**: 更新边缘AI引擎配置
- **认证**: AdminAuthFilter
- **请求体**: 配置JSON对象

---

## 9. 隐私统计模块

### 9.1 差分隐私统计
- **Method**: `GET`
- **Path**: `/api/lake/privacy-stats`
- **描述**: 获取差分隐私保护的湖面情绪统计
- **认证**: 无需认证
- **响应**: 添加噪声后的情绪分布

### 9.2 隐私预算报告
- **Method**: `GET`
- **Path**: `/api/lake/privacy-report`
- **描述**: 获取隐私预算消耗报告
- **认证**: 无需认证
- **响应**: 预算使用情况、剩余预算

---

## 10. 守望者模块

### 10.1 获取守望者统计
- **Method**: `GET`
- **Path**: `/api/guardian/stats`
- **描述**: 获取守望者系统统计数据
- **认证**: SecurityAuditFilter

### 10.2 获取守望者统计（别名）
- **Method**: `GET`
- **Path**: `/api/guardian`
- **描述**: 同上（简短路径）
- **认证**: SecurityAuditFilter

### 10.3 转赠灯火
- **Method**: `POST`
- **Path**: `/api/guardian/transfer-lamp`
- **描述**: 向其他用户转赠灯火（激励机制）
- **认证**: SecurityAuditFilter
- **请求体**: `{ "to_user_id": "...", "amount": 1 }`

### 10.4 获取情绪洞察
- **Method**: `GET`
- **Path**: `/api/guardian/insights`
- **描述**: 获取个人情绪洞察报告
- **认证**: SecurityAuditFilter

---

## 11. 媒体模块

### 11.1 上传媒体文件
- **Method**: `POST`
- **Path**: `/api/media/upload`
- **描述**: 上传单个媒体文件（图片、音频、视频）
- **认证**: 需要 PASETO Token
- **请求**: multipart/form-data

### 11.2 批量上传媒体
- **Method**: `POST`
- **Path**: `/api/media/upload/multiple`
- **描述**: 批量上传多个媒体文件
- **认证**: 需要 PASETO Token
- **请求**: multipart/form-data

### 11.3 获取媒体信息
- **Method**: `GET`
- **Path**: `/api/media/{mediaId}`
- **描述**: 获取媒体文件元信息
- **认证**: 无需认证

### 11.4 删除媒体
- **Method**: `DELETE`
- **Path**: `/api/media/{mediaId}`
- **描述**: 删除指定媒体文件
- **认证**: 需要 PASETO Token

---

## 12. 管理员模块

### 12.1 认证管理

#### 管理员登录
- **Method**: `POST`
- **Path**: `/api/admin/login`
- **描述**: 管理员登录
- **认证**: 无需认证
- **请求体**: `{ "username": "...", "password": "..." }`

#### 管理员登出
- **Method**: `POST`
- **Path**: `/api/admin/logout`
- **描述**: 管理员登出
- **认证**: AdminAuthFilter

#### 获取管理员信息
- **Method**: `GET`
- **Path**: `/api/admin/info`
- **描述**: 获取当前管理员信息
- **认证**: AdminAuthFilter

### 12.2 统计数据

#### 获取热门话题
- **Method**: `GET`
- **Path**: `/api/admin/stats/trending-topics`
- **描述**: 获取热门话题统计
- **认证**: AdminAuthFilter

#### 获取实时统计
- **Method**: `GET`
- **Path**: `/api/admin/stats/realtime`
- **描述**: 获取实时在线用户、活跃度等数据
- **认证**: AdminAuthFilter

#### 获取仪表盘统计
- **Method**: `GET`
- **Path**: `/api/admin/stats/dashboard`
- **描述**: 获取管理后台仪表盘综合统计
- **认证**: AdminAuthFilter

#### 获取用户增长统计
- **Method**: `GET`
- **Path**: `/api/admin/stats/user-growth`
- **描述**: 获取用户增长趋势数据
- **认证**: AdminAuthFilter

#### 获取情绪分布
- **Method**: `GET`
- **Path**: `/api/admin/stats/mood-distribution`
- **描述**: 获取全局情绪分布统计
- **认证**: AdminAuthFilter

#### 获取活跃时段统计
- **Method**: `GET`
- **Path**: `/api/admin/stats/active-time`
- **描述**: 获取用户活跃时段分布
- **认证**: AdminAuthFilter

### 12.3 风险管理

#### 获取高风险用户
- **Method**: `GET`
- **Path**: `/api/admin/risk/high-risk-users`
- **描述**: 获取高风险用户列表
- **认证**: AdminAuthFilter

#### 获取风险事件
- **Method**: `GET`
- **Path**: `/api/admin/risk/events`
- **描述**: 获取风险事件列表
- **认证**: AdminAuthFilter

#### 获取用户风险历史
- **Method**: `GET`
- **Path**: `/api/admin/risk/user/{user_id}/history`
- **描述**: 获取指定用户的风险历史记录
- **认证**: AdminAuthFilter

#### 处理风险事件
- **Method**: `POST`
- **Path**: `/api/admin/risk/event/{event_id}/handle`
- **描述**: 标记风险事件为已处理
- **认证**: AdminAuthFilter

### 12.4 安全审计

#### 获取安全审计日志
- **Method**: `GET`
- **Path**: `/api/admin/security/audit`
- **描述**: 获取安全审计日志
- **认证**: AdminAuthFilter
- **查询参数**: `page`, `page_size`, `start_date`, `end_date`

---

## 错误码说明

| 错误码 | 说明 |
|--------|------|
| 200 | 成功 |
| 400 | 请求参数错误 |
| 401 | 未认证 |
| 403 | 权限不足 |
| 404 | 资源不存在 |
| 409 | 资源冲突 |
| 429 | 请求过于频繁 |
| 500 | 服务器内部错误 |

---

## 认证说明

### PASETO Token 使用

1. 登录后获取 Token
2. 在请求头中携带: `Authorization: Bearer <token>`
3. Token 有效期: 7天
4. Token 刷新: 需重新登录

### SecurityAuditFilter

所有标记 `SecurityAuditFilter` 的接口会自动记录审计日志，包括：
- 请求时间
- 用户ID
- IP地址
- 操作类型
- 请求参数

### AdminAuthFilter

管理员接口需要额外的管理员权限验证，使用独立的 PASETO Token。

---

## 速率限制

- 普通用户: 100 请求/分钟
- 认证用户: 300 请求/分钟
- 管理员: 1000 请求/分钟

---

## 版本信息

- API 版本: v1.0
- 文档更新日期: 2026-02-21
- 后端框架: Drogon 1.9.x
- C++ 标准: C++20
