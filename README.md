<h1 align="center">HeartLake</h1>

<p align="center">
  匿名情感社交平台 -- 边缘AI引擎 / 端到端加密 / DDD架构 / 高性能C++20
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?style=flat-square&logo=cplusplus" alt="C++20"/>
  <img src="https://img.shields.io/badge/Drogon-1.9+-green?style=flat-square" alt="Drogon"/>
  <img src="https://img.shields.io/badge/Vue-3.4+-brightgreen?style=flat-square&logo=vuedotjs" alt="Vue3"/>
  <img src="https://img.shields.io/badge/Flutter-3.19+-02569B?style=flat-square&logo=flutter" alt="Flutter"/>
  <img src="https://img.shields.io/badge/PostgreSQL-16-336791?style=flat-square&logo=postgresql" alt="PostgreSQL"/>
  <img src="https://img.shields.io/badge/Redis-7-DC382D?style=flat-square&logo=redis" alt="Redis"/>
  <img src="https://img.shields.io/badge/Docker-Compose-2496ED?style=flat-square&logo=docker" alt="Docker"/>
  <img src="https://img.shields.io/badge/License-AGPL--3.0-orange?style=flat-square" alt="License"/>
  <img src="https://img.shields.io/badge/Edge_AI-8_Subsystems-ff69b4?style=flat-square" alt="Edge AI"/>
  <img src="https://img.shields.io/badge/Architecture-DDD-purple?style=flat-square" alt="DDD"/>
  <img src="https://img.shields.io/badge/Security-PASETO_v4_%7C_E2E_%7C_DP-critical?style=flat-square" alt="Security"/>
</p>

---

## 简介

HeartLake 是一个匿名情感社交平台。用户将心事写成「心石」投入湖中，系统通过边缘AI引擎进行情感分析与同频共鸣匹配，在端到端加密和身份影子映射的保护下实现完全匿名的情感连接。

平台以「关怀」为核心理念 -- VIP 不是付费商品，而是系统检测到用户情绪低落时自动赠送的温暖。心理风险评估模块实时监测用户状态，在检测到高危信号时触发守护者干预机制。

技术上采用 C++20 + Drogon 异步后端、Flutter 跨平台移动端、Vue 3 管理后台的三端架构，内置 8 个边缘AI子系统，通过 DDD + 事件驱动 + ServiceLocator 模式组织代码，全链路 PASETO v4 认证 + X25519/AES-256-GCM 端到端加密。

---

## 架构总览

```
                           +------------------+
                           |   Nginx Reverse  |
                           |      Proxy       |
                           +--------+---------+
                                    |
                    +---------------+---------------+
                    |                               |
           +--------+--------+            +---------+---------+
           |  Flutter Mobile |            |  Vue 3 Admin      |
           |  (Dart/Flutter) |            |  (Vite+ElementPlus)|
           +--------+--------+            +---------+---------+
                    |                               |
                    +---------------+---------------+
                                    |
                           +--------+---------+
                           |  Drogon Backend  |
                           |  (C++20 Async)   |
                           +--------+---------+
                                    |
              +----------+----------+----------+----------+
              |          |          |          |          |
        +-----+--+ +----+---+ +---+----+ +---+----+ +---+------+
        |Interfaces| |Applica-| |Domain  | |Infra-  | |EdgeAI   |
        |  /api/   | |  tion  | |Entities| |structure| |Engine   |
        |20 Ctrl   | |Services| |  DDD   | |Services | |8 Subsys |
        +----------+ +--------+ +--------+ +--------+ +----------+
              |          |          |          |          |
              +----------+----------+----------+----------+
                                    |
                         +----------+----------+
                         |                     |
                   +-----+------+        +-----+------+
                   | PostgreSQL |        |   Redis    |
                   |    16      |        |     7      |
                   +------------+        +------------+
```

分层架构遵循 DDD 原则，自上而下分为四层：

| 层级 | 目录 | 职责 |
|------|------|------|
| Interfaces | `interfaces/api/` | 20个HTTP/WebSocket控制器，请求路由与参数校验 |
| Application | `application/` | 应用服务编排，FriendApplicationService / InteractionApplicationService / StoneApplicationService / UserApplicationService |
| Domain | `domain/` | 领域实体与业务规则，entities / friend / stone / user 子域 |
| Infrastructure | `infrastructure/` | 技术实现，ai / cache / di / events / filters / messaging / privacy / realtime / services / vector |

事件驱动通过 `infrastructure/events/` 实现领域事件的发布-订阅解耦。依赖注入通过 `infrastructure/di/` 的 ServiceLocator 模式管理服务生命周期。启动引导由 `ArchitectureBootstrap.h` 统一编排。

---

## 核心业务系统

### 心石系统 (Stone)

心石是平台的核心内容载体。用户将心事写成石头投入心湖，系统对内容进行情感分析、内容审核、向量化后存入湖中。

- 石头规格：`small` / `medium` / `large` 三种类型
- 投石频率限制：普通用户每日 3 颗，VIP 用户每日 10 颗
- 内容审核：两级流水线（AC自动机关键词过滤 + AI语义审核）
- 情感向量化：TF-IDF 嵌入后存入 HNSW 向量索引，用于同频共鸣匹配

### 涟漪系统 (Ripple)

涟漪是心石的公开互动机制。用户对心石发送涟漪（类似评论），涟漪通过 WebSocket 实时广播给心石所有者和其他涟漪参与者。

### 纸船系统 (Boat)

纸船是一对一的匿名私信通道。用户可以向心石作者发送纸船，建立匿名对话。纸船消息通过 E2E 加密传输，服务端无法解密消息内容。

### 石友关系 (Friend TTL)

石友是平台的社交关系模型，分为临时石友和永久石友：

- 临时石友：默认 72 小时 TTL，通过 Redis 管理过期
- 永久石友：双方确认后升级，无过期限制
- 过期预警：通过 `TEMP_FRIEND_EXPIRING` 通知类型提前推送

### VIP 关怀系统

VIP 不通过付费获取，而是系统检测到用户情绪持续低落时自动赠送：

- 触发条件：连续多日情绪评分低于阈值
- 赠送内容：额外投石配额、专属涟漪样式、优先匹配权重
- 设计理念：用技术手段实现人文关怀

### 守护者系统

当心理风险评估模块检测到用户处于 HIGH 或 CRITICAL 风险等级时，触发守护者干预：

- 自动推送心理援助资源
- 通知管理后台人工介入
- 通过 `NotificationPushService` 的 `SYSTEM_NOTICE` 类型下发

---

## EdgeAI 引擎

EdgeAI 引擎是平台的核心智能层，包含 8 个子系统，全部定义在 `backend/include/infrastructure/ai/EdgeAIEngine.h`（634行）。

### 1. 情感分析 (SentimentAnalyzer)

基于 TF-IDF 向量化 + 余弦相似度的文本情感分析。

- 算法：将输入文本转换为 TF-IDF 向量，与预定义情感模板计算余弦相似度
- 输出：情感标签（positive / negative / neutral）+ 置信度分数
- 代码位置：`EdgeAIEngine.h` SentimentAnalyzer 内部类

### 2. 文本审核 (TextModerator)

基于 AC 自动机（Aho-Corasick）的多模式字符串匹配，实现高效敏感词过滤。

- 算法：构建敏感词 Trie 树 + failure 链接，单次扫描完成所有模式匹配
- 时间复杂度：O(n + m + z)，n=文本长度，m=模式总长度，z=匹配数
- 审核流水线：第一级 AC 自动机快速过滤 -> 第二级 AI 语义审核（仅对可疑内容）
- 代码位置：`EdgeAIEngine.h` TextModerator 内部类

### 3. 情绪脉搏 (EmotionPulse)

基于滑动窗口的用户情绪趋势追踪。

- 算法：维护固定大小的情绪评分滑动窗口，计算移动平均值和趋势斜率
- 用途：检测情绪持续低落（触发 VIP 关怀）、情绪突变（触发风险评估）
- 代码位置：`EdgeAIEngine.h` EmotionPulse 内部类

### 4. 联邦学习 (FederatedLearning)

基于 FedAvg 算法的联邦学习框架，在不收集用户原始数据的前提下训练全局模型。

- 算法：FedAvg -- 各客户端本地训练后上传梯度，服务端加权平均聚合
- 聚合公式：`w_global = SUM(n_k / n * w_k)`，n_k 为客户端 k 的样本数，n 为总样本数
- 隐私保障：原始数据不离开设备，仅传输模型参数更新
- 代码位置：`EdgeAIEngine.h` FederatedLearning 内部类

### 5. 差分隐私 (DifferentialPrivacy)

基于 Laplace 机制的差分隐私保护，为查询结果添加校准噪声。

- 算法：Laplace 机制，噪声分布 Lap(sensitivity / epsilon)
- 参数：epsilon（隐私预算）由调用方指定，sensitivity 根据查询类型确定
- 用途：保护聚合统计数据（如情绪分布、活跃度统计）不泄露个体信息
- 代码位置：`EdgeAIEngine.h` DifferentialPrivacy 内部类

### 6. HNSW 向量搜索 (HNSWIndex)

Hierarchical Navigable Small World 图索引，用于高维向量的近似最近邻搜索。

- 核心参数：
  - `M = 16`：每个节点的最大连接数
  - `Mmax0 = 32`：第 0 层的最大连接数
  - `efConstruction = 200`：构建时的搜索宽度
  - `efSearch = 50`：查询时的搜索宽度
- 算法：多层跳表结构，上层稀疏图快速定位区域，下层密集图精确搜索
- 用途：心石情感向量的同频共鸣匹配，O(log n) 查询复杂度
- 代码位置：`EdgeAIEngine.h` HNSWIndex 内部类

### 7. INT8 量化 (ModelQuantizer)

将浮点模型权重量化为 INT8 整数，降低推理延迟和内存占用。

- 算法：对称量化，scale = max(abs(weights)) / 127
- 压缩比：FP32 -> INT8，理论 4x 内存节省
- 用途：边缘设备上的模型部署优化
- 代码位置：`EdgeAIEngine.h` ModelQuantizer 内部类

### 8. 节点健康监控 (NodeHealthMonitor)

基于断路器模式（Circuit Breaker）的服务健康监控。

- 状态机：`CLOSED` -> `OPEN` -> `HALF_OPEN` -> `CLOSED`
- 关键参数：
  - `failureRateThreshold = 0.5`：失败率阈值，超过则断路器打开
  - `cooldownPeriodSec = 30`：OPEN 状态冷却时间（秒）
  - `halfOpenSuccessThreshold = 3`：HALF_OPEN 状态下连续成功次数阈值，达到则恢复 CLOSED
- 代码位置：`EdgeAIEngine.h` NodeHealthMonitor 内部类

---

## 核心算法

### 情感共鸣公式 (EmotionResonanceEngine)

定义在 `backend/include/infrastructure/ai/EmotionResonanceEngine.h`（104行）。

四维共鸣评分公式：

```
ResonanceScore = alpha * SemanticSimilarity
               + beta  * EmotionTrajectorySimilarity
               + gamma * TemporalDecay
               + delta * DiversityBonus
```

权重参数：

| 参数 | 值 | 含义 |
|------|----|------|
| alpha | 0.30 | 语义相似度权重 |
| beta | 0.35 | 情感轨迹相似度权重（最高权重） |
| gamma | 0.20 | 时间衰减权重 |
| delta | 0.15 | 多样性奖励权重 |

- 情感轨迹相似度：使用 DTW（Dynamic Time Warping）算法计算两个用户的情绪时间序列相似度
- 时间衰减：指数衰减函数 `exp(-lambda * delta_t)`，lambda = 0.1，delta_t 为时间差（小时）
- 多样性奖励：鼓励用户接触不同情感类型的内容，避免信息茧房

### 心理风险评估 (PsychologicalRiskAssessment)

定义在 `backend/include/utils/PsychologicalRiskAssessment.h`（213行）。

五因子加权风险评估模型：

| 因子 | 权重 | 说明 |
|------|------|------|
| selfHarmIntent | 0.9 | 自伤意图检测（最高权重） |
| hopelessness | 0.6 | 绝望感评估 |
| temporalUrgency | 0.5 | 时间紧迫性 |
| linguisticMarkers | 0.3 | 语言学标记（特定词汇模式） |
| socialIsolation | 0.1 | 社交孤立程度 |

风险等级划分：

| 等级 | 分数区间 | 响应 |
|------|----------|------|
| NONE | 0 | 无操作 |
| LOW | (0, 0.3] | 记录日志 |
| MEDIUM | (0.3, 0.6] | 推送关怀资源 |
| HIGH | (0.6, 0.8] | 触发守护者干预 |
| CRITICAL | (0.8, 1.0] | 紧急干预 + 管理后台告警 |

### 双记忆 RAG (DualMemoryRAG)

定义在 `backend/include/infrastructure/ai/DualMemoryRAG.h`（107行）。基于 SoulSpeak 论文（arXiv, 2024.12）。

双记忆架构：

- 短期记忆：最近 `MAX_SHORT_TERM = 5` 条对话，提供即时上下文
- 长期记忆：用户情感画像（情绪分布、关注话题、表达风格），持久化存储

RAG 流程：查询向量化 -> HNSW 检索相关长期记忆 -> 拼接短期记忆上下文 -> 构建 prompt -> LLM 生成回复

---

## 安全体系

### 端到端加密 (E2EEncryption)

定义在 `backend/include/utils/E2EEncryption.h`（107行）。

提供 IND-CCA2 安全性，通过临时密钥对实现前向保密：

| 组件 | 算法 | 参数 |
|------|------|------|
| 密钥交换 | X25519 ECDH | X25519_KEY_SIZE = 32 bytes |
| 密钥派生 | HKDF-SHA256 | 从共享密钥派生会话密钥 |
| 认证加密 | AES-256-GCM | KEY_SIZE = 32, IV_SIZE = 12, TAG_SIZE = 16 |

加密流程：

1. 双方各生成 X25519 临时密钥对（`generateX25519KeyPair()`）
2. 交换公钥，通过 ECDH 计算共享密钥（`computeSharedSecret()`）
3. HKDF-SHA256 从共享密钥派生会话密钥（`deriveSessionKey()`）
4. AES-256-GCM 认证加密消息（`encrypt()` / `decrypt()`）

输出结构 `EncryptedMessage`：ciphertext + iv + tag，全部 Base64 编码。

### 身份影子映射 (IdentityShadowMap)

定义在 `backend/include/utils/IdentityShadowMap.h`（89行）。

实现物理标识与匿名ID的完全隔离，即使数据库泄露也无法溯源真实身份：

- 影子ID生成：`getOrCreateShadowId()` 为每个用户生成唯一影子ID
- IP匿名化：`anonymizeIp()` 单向哈希，不可逆
- 设备指纹匿名化：`anonymizeFingerprint()` 单向哈希，不可逆
- 影子ID轮换：`rotateShadowId()` 定期轮换，增强隐私
- 数据脱敏：`desensitize()` 支持 email / phone / name 三种类型
- 内部实现：HMAC-SHA256 哈希，`shared_mutex` 线程安全

存储结构 `ShadowIdentity`：shadowId + hashedFingerprint + createdAt + lastRotatedAt。

### 认证

PASETO v4 令牌认证，替代 JWT 避免算法混淆攻击。

### 差分隐私

Laplace 机制保护聚合查询，详见 EdgeAI 引擎第 5 节。

---

## 实时通信

### WebSocketHub

定义在 `backend/include/infrastructure/realtime/WebSocketHub.h`（182行）。

WebSocket 实时通信中心，支持房间管理、心跳检测、消息广播：

| 功能 | 接口 | 说明 |
|------|------|------|
| 连接管理 | `addConnection()` / `removeConnection()` | 用户上线/下线 |
| 房间管理 | `joinRoom()` / `leaveRoom()` | 加入/离开房间（涟漪广播用） |
| 定向发送 | `sendToUser()` | 发送给指定用户的所有连接 |
| 房间广播 | `sendToRoom(room, msg, exclude)` | 广播给房间内所有连接，可排除发送者 |
| 全局广播 | `broadcast()` | 广播给所有在线连接 |
| 心跳检测 | `startHeartbeat(interval=30s, timeout=90s)` | 定时发送 ping，超时强制断开 |
| 在线查询 | `isUserOnline()` / `getConnectionCount()` / `getRoomSize()` | 在线状态查询 |

线程安全：`mutable std::shared_mutex`，写操作 `unique_lock`，读操作 `shared_lock`。

连接信息 `ConnectionInfo`：userId + rooms(unordered_set) + lastPing + supportsCompression。

### NotificationPushService

定义在 `backend/include/infrastructure/services/NotificationPushService.h`（145行）。

基于 WebSocketHub 的通知推送服务，支持 8 种通知类型：

| 枚举值 | 含义 |
|--------|------|
| RIPPLE_RECEIVED | 收到涟漪 |
| BOAT_RECEIVED | 收到纸船 |
| FRIEND_REQUEST | 好友请求 |
| FRIEND_ACCEPTED | 好友接受 |
| TEMP_FRIEND_EXPIRING | 临时好友即将过期 |
| NEW_MESSAGE | 新消息 |
| SYSTEM_NOTICE | 系统通知 |
| AI_REPLY | AI回复 |

---

## 管理后台

Vue 3 + Vite + Element Plus 构建的管理后台，位于 `admin/` 目录，包含 10 个管理页面：

| 页面 | 功能 |
|------|------|
| Dashboard | 数据概览、实时统计 |
| 用户管理 | 用户列表、状态管理、VIP管理 |
| 内容审核 | 心石审核、涟漪审核、举报处理 |
| 心石管理 | 心石列表、内容查看、状态变更 |
| 涟漪管理 | 涟漪列表、关联心石查看 |
| 纸船管理 | 纸船对话管理（仅元数据，无法查看加密内容） |
| 通知管理 | 系统通知发送、通知历史 |
| 风险监控 | 心理风险告警、守护者干预记录 |
| 系统配置 | 参数配置、功能开关 |
| 数据统计 | 用户增长、活跃度、情绪分布图表 |

---

## 技术栈

| 层级 | 技术选型 | 版本/规格 | 说明 |
|------|---------|-----------|------|
| 后端框架 | C++20 + Drogon | C++20 / Drogon 1.9+ | 高性能异步 HTTP/WebSocket 框架 |
| 管理后台 | Vue 3 + Vite + Element Plus | Vue 3.4+ / Vite 5 | 现代化 SPA 管理界面 |
| 移动端 | Flutter | 3.19+ | 跨平台移动应用（iOS/Android） |
| 主数据库 | PostgreSQL | 16-alpine | 关系型数据存储 |
| 缓存 | Redis | 7-alpine | 缓存、TTL管理、实时数据 |
| AI服务 | DeepSeek API + 本地 TF-IDF + EdgeAI | -- | 情感分析、内容审核、推荐 |
| 容器化 | Docker Compose | -- | 5 服务编排，一键部署 |
| 认证 | PASETO | v4 | 安全令牌，避免 JWT 算法混淆 |
| 加密 | X25519 + HKDF-SHA256 + AES-256-GCM | -- | E2E 端到端加密，IND-CCA2 安全 |
| 实时通信 | WebSocket | -- | 涟漪广播、通知推送、心跳检测 |
| 向量索引 | HNSW | M=16, ef=200/50 | 近似最近邻搜索 |
| 隐私保护 | 差分隐私 + 联邦学习 + 身份影子映射 | -- | 多层隐私防护 |
| 测试 | GTest + Vitest | -- | 后端单元测试 + 前端单元测试 |

---

## 快速开始

### 环境要求

- Docker 20.10+
- Docker Compose v2+

### 部署

```bash
# 克隆仓库
git clone https://github.com/by17815777533-bit/HeartLake.git
cd HeartLake

# 配置环境变量
cp .env.example .env
# 编辑 .env 填写数据库密码、Redis密码、DeepSeek API Key 等

# 一键启动
docker-compose up -d
```

### Docker Compose 服务

| 服务 | 镜像 | 内存限制 | 端口 |
|------|------|----------|------|
| postgres | postgres:16-alpine | 512M | 5432 |
| redis | redis:7-alpine | 256M | 6379 |
| backend | 自构建 (C++20/Drogon) | 1G | 8080 |
| admin | 自构建 (Vue3/Nginx) | 128M | 3000 |
| nginx | nginx:alpine | 128M | 80/443 |

启动后访问：

- 移动端 API：`http://localhost:80/api/`
- 管理后台：`http://localhost:80/admin/`

---

## 项目统计

| 指标 | 数值 |
|------|------|
| C++ 代码行数（后端） | 39,400 行 |
| Vue/JS/SCSS 代码行数（管理后台） | 7,808 行 |
| Dart 代码行数（移动端） | 26,950 行 |
| 后端头文件数 | 92 个 |
| API 控制器数 | 20 个 |
| 管理后台页面数 | 10 个 |
| EdgeAI 子系统数 | 8 个 |
| 通知类型数 | 8 种 |

---

## 许可证

本项目采用 [AGPL-3.0](LICENSE) 许可证开源。

```
Copyright (C) 2024-2026 HeartLake Contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
```
