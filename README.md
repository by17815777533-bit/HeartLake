<p align="center">
  <img src="docs/assets/heartlake-logo.png" alt="HeartLake Logo" width="200"/>
</p>

<h1 align="center">HeartLake 心湖</h1>

<p align="center">
  <strong>匿名情感社交平台 · 边缘AI驱动 · 安全架构</strong>
</p>

<p align="center">
  <em>Edge AI · DDD架构 · E2E加密 · PASETO认证 · 高性能C++20</em>
</p>

<p align="center">
  <a href="#快速开始">快速开始</a> ·
  <a href="#核心业务系统">核心业务</a> ·
  <a href="#技术架构">技术架构</a> ·
  <a href="#安全体系">安全体系</a> ·
  <a href="#技术栈">技术栈</a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?style=flat-square&logo=cplusplus" alt="C++20"/>
  <img src="https://img.shields.io/badge/Drogon-1.9+-green?style=flat-square" alt="Drogon"/>
  <img src="https://img.shields.io/badge/Vue-3.4+-brightgreen?style=flat-square&logo=vuedotjs" alt="Vue3"/>
  <img src="https://img.shields.io/badge/Flutter-3.19+-02569B?style=flat-square&logo=flutter" alt="Flutter"/>
  <img src="https://img.shields.io/badge/PostgreSQL-16+-336791?style=flat-square&logo=postgresql" alt="PostgreSQL"/>
  <img src="https://img.shields.io/badge/Redis-7.2+-DC382D?style=flat-square&logo=redis" alt="Redis"/>
  <img src="https://img.shields.io/badge/Docker-Compose-2496ED?style=flat-square&logo=docker" alt="Docker"/>
  <img src="https://img.shields.io/badge/License-AGPL--3.0-orange?style=flat-square" alt="License"/>
  <img src="https://img.shields.io/badge/Edge_AI-8_Engines-ff69b4?style=flat-square" alt="Edge AI"/>
  <img src="https://img.shields.io/badge/Architecture-DDD-purple?style=flat-square" alt="DDD"/>
  <img src="https://img.shields.io/badge/Security-PASETO%20%7C%20E2E%20%7C%20DP-critical?style=flat-square" alt="Security"/>
  <img src="https://img.shields.io/badge/Realtime-WebSocket-blueviolet?style=flat-square" alt="WebSocket"/>
</p>

---

## 目录

- [简介](#简介)
- [核心业务系统](#核心业务系统)
  - [心石系统](#心石系统)
  - [涟漪系统](#涟漪系统)
  - [纸船系统](#纸船系统)
  - [VIP关怀系统](#vip关怀系统)
  - [湖神守护](#湖神守护)
  - [情绪追踪与负重者监测](#情绪追踪与负重者监测)
  - [石友关系TTL](#石友关系ttl)
  - [同频共鸣搜索](#同频共鸣搜索)
  - [守护者系统](#守护者系统)
- [AI能力](#ai能力)
  - [双记忆RAG湖神AI](#双记忆rag湖神ai)
  - [心理风险评估](#心理风险评估)
  - [内容审核两级流水线](#内容审核两级流水线)
  - [推荐系统](#推荐系统)
- [安全体系](#安全体系)
- [技术架构](#技术架构)
- [快速开始](#快速开始)
- [技术栈](#技术栈)
- [许可证](#许可证)

---

## 简介

HeartLake 心湖是一个匿名情感社交平台。用户将心事写成「心石」投入湖中，通过 AI 情感分析实现同频共鸣匹配，在完全匿名的环境下找到情感共鸣。平台以「关怀」而非「商业化」为核心理念——VIP 不是付费购买的商品，而是系统检测到用户情绪低落时自动赠送的温暖。

项目采用 C++20 + Drogon 高性能后端、Flutter 跨平台移动端、Vue 3 管理后台的三端架构，配合 PostgreSQL 16、Redis 7、Docker Compose 实现生产级部署。

---

## 核心业务系统

### 心石系统

心石是平台的核心内容载体，用户将心事写成石头投入心湖。

- **石头规格**：分为 `small`（轻）、`medium`（中）、`large`（重）三种类型
- **石头颜色**：`#RRGGBB` 格式，默认 `#7A92A3`
- **情绪标签**：支持 10 种情绪——`calm`、`happy`、`sad`、`angry`、`anxious`、`hopeful`、`lonely`、`grateful`、`confused`、`peaceful`
- **内容限制**：最多 2000 字，支持自定义标签
- **异步处理流水线**：石头发布后自动触发以下异步任务：
  1. AI 情感分析
  2. 向量嵌入生成（用于同频共鸣搜索）
  3. 心理风险评估
  4. 通知推送
- **安全拦截**：高危内容直接拦截，不予发布，同时向用户展示心理健康提示与求助资源

### 涟漪系统

涟漪是对心石的共鸣表达，类似「点赞」但更具情感温度。

- 唯一约束 `(stone_id, user_id)` 防止重复涟漪
- 原子递增 `ripple_count`，保证并发安全
- WebSocket 实时广播涟漪事件，投石者即时感知共鸣

### 纸船系统

纸船是用户之间传递温暖的匿名信使。

- **漂流模式**：
  - `random`（随机漂流）——投入湖中随机送达
  - `directed`（定向传递）——指定送给某颗心石的主人
  - `wish`（许愿漂流）——带着心愿漂流
- **纸船样式**：`paper`（纸船）、`origami`（折纸）、`lotus`（莲花）
- **情绪色彩**：`hopeful`、`sad`、`calm`、`anxious`、`happy`、`lonely`、`grateful`
- **交互操作**：支持捞纸船、回应纸船、放回湖中

### VIP关怀系统

> VIP 是平台对情绪低落用户的关怀赠送，不是付费购买的商品。

VIP 通过以下三条路径自动赠送：

| 路径 | 触发条件 | 赠送内容 | 备注 |
|------|---------|---------|------|
| 路径1：情绪关怀 (`auto_emotion`) | 用户情绪评分低于全网 P20 百分位 | VIP Level 1，30 天 | 自动触发 |
| 路径2：极端负重灯 (`extreme_burden_lamp`) | 72小时内持续发布负面石头（>= `MIN_POST_COUNT` 条，均分 <= `EXTREME_BURDEN_THRESHOLD`） | VIP 7 天 | 72小时冷却期，防重复赠送 |
| 路径3：守护者传灯 | 守护者消耗共鸣积分为他人传灯 | VIP 7 天 | 守护者主动触发 |

**VIP 特权**：
- 免费心理咨询预约
- AI 回复频率提升：VIP 用户 30 分钟/次，普通用户 2 小时/次

**过期保护**：若用户有进行中的免费咨询预约，VIP 状态不会过期，直到咨询完成。

### 湖神守护

湖神是心湖的 AI 守护者，专门关照那些无人回应的心石。

- 后台线程定时扫描零互动石头（`ripple_count = 0 AND boat_count = 0`，超过配置阈值小时数）
- AI 生成暖心回复，以 `lake_god`（湖神）身份发送纸船给投石者
- 防重复机制：先插入 `pending` 状态占位记录，避免并发重复发送

### 情绪追踪与负重者监测

后台线程定时扫描，识别持续情绪低落的用户并主动干预。

- **触发条件**：72 小时内发布负面石头数 >= 配置阈值，且情绪均分 <= 极端负重阈值
- **干预措施**：
  1. 自动赠送 VIP（路径2）
  2. 发送暖心语录
  3. 推送系统通知
- **去重保护**：72 小时内不会对同一用户重复干预
- **数据清理**：自动清理 30 天前的追踪数据

### 石友关系TTL

心湖中的友谊默认是临时的，缘起缘灭皆有时。

- 石友关系默认 24 小时有效期
- Redis 管理 TTL 倒计时
- 过期前 1 小时发送提醒通知
- 双方可选择升级为永久好友
- 过期后发送「缘尽」通知，关系自动解除

### 同频共鸣搜索

帮助用户找到与自己情感频率相近的心石，核心匹配算法采用四维评分：

| 维度 | 说明 |
|------|------|
| 语义相似度 | 基于向量嵌入的文本语义匹配 |
| 情绪轨迹 DTW | 动态时间规整算法比较情绪变化轨迹 |
| 时间衰减 | 越新的石头权重越高 |
| 多样性奖励 | 避免结果过于同质化 |

- **互补情绪加分**：`sad ↔ hopeful`、`anxious ↔ calm`、`angry ↔ grateful`
- **过滤规则**：排除自己的石头，仅搜索 30 天内已发布状态的石头

### 守护者系统

守护者是心湖社区中积极传递温暖的用户。

- 用户通过互动积累共鸣积分
- 积分达到阈值后自动晋升为守护者
- 守护者可消耗共鸣积分为他人「传灯」——即赠送 VIP 7 天
- 传灯是守护者对陌生人的善意，体现社区互助精神

---

## AI能力

### 双记忆RAG湖神AI

湖神 AI 采用双记忆架构，兼顾对话连贯性与长期情感理解。

- **短期记忆**：滑动窗口保留最近 N 条交互，维持对话上下文
- **长期记忆**：30 天情绪画像，包含：
  - 总帖数、情绪均分、波动度
  - 主导情绪、情绪趋势
  - 连续负面天数
- **角色设定**：心湖湖神——温暖、包容、不说教
- **兜底文案**：当 AI 无法生成合适回复时，使用兜底文案：「我感受到了你此刻的心情，无论如何，你并不孤单。」

### 心理风险评估

多因子加权评估模型，实时识别心理危机信号。

| 因子 | 权重 | 说明 |
|------|------|------|
| 自伤意图 | 0.9 | 最高权重，直接关联安全 |
| 绝望感 | — | 对未来的消极预期 |
| 孤立感 | — | 社交隔离程度 |
| 紧迫性 | — | 时间紧迫程度 |
| 语言标记 | 0.3 | 特定词汇和表达模式 |

**风险等级**：`NONE` → `LOW` → `MEDIUM` → `HIGH` → `CRITICAL`

- `HIGH` / `CRITICAL` 级别：内容直接拦截，向用户展示心理健康提示和专业求助资源

### 内容审核两级流水线

两级审核确保内容安全与用户体验的平衡。

1. **第一级：本地 AC 自动机敏感词检测**——高性能本地过滤，毫秒级响应
2. **第二级：DeepSeek API 深度审核**——AI 语义理解，识别隐晦违规内容
3. **容错策略**：AI 审核服务不可用时默认放行，但标记为 `pending_review` 待人工复核

### 推荐系统

多算法融合推荐，平衡精准推荐与内容探索。

- **MF**（矩阵分解）——协同过滤基础
- **UCB**（Upper Confidence Bound）——探索-利用平衡
- **Thompson Sampling**——贝叶斯探索策略
- **MMR**（Maximal Marginal Relevance）——结果多样性保障

---

## 安全体系

### E2E 端到端加密

- **密钥交换**：X25519 椭圆曲线 Diffie-Hellman
- **对称加密**：AES-256-GCM
- **前向安全**：每次会话独立密钥，历史消息不可追溯解密

### 认证体系

- PASETO v4 令牌认证，替代传统 JWT
- 无状态、防篡改、抗重放

### 匿名保护

- 石友关系 TTL 自动过期，不留永久社交痕迹
- 零知识匿名投石

---

## 技术架构

```
┌─────────────────────────────────────────────────────────┐
│                    客户端层                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │
│  │ Flutter App  │  │  Vue 3 Admin │  │  WebSocket 客户端│  │
│  │  (移动端)    │  │  (管理后台)   │  │  (实时通信)      │  │
│  └──────┬──────┘  └──────┬──────┘  └────────┬────────┘  │
└─────────┼───────────────┼──────────────────┼────────────┘
          │               │                  │
          ▼               ▼                  ▼
┌─────────────────────────────────────────────────────────┐
│                  API 网关层 (Drogon)                      │
│  ┌──────────┐ ┌──────────┐ ┌───────────┐ ┌───────────┐  │
│  │PASETO认证 │ │ 限流器   │ │ CORS处理  │ │ 路由分发   │  │
│  └──────────┘ └──────────┘ └───────────┘ └───────────┘  │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                    业务服务层                             │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌──────────────────┐  │
│  │心石服务 │ │纸船服务 │ │涟漪服务 │ │  VIP关怀服务     │  │
│  └────────┘ └────────┘ └────────┘ └──────────────────┘  │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌──────────────────┐  │
│  │石友服务 │ │守护者   │ │推荐引擎 │ │  同频共鸣搜索    │  │
│  └────────┘ └────────┘ └────────┘ └──────────────────┘  │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                     AI 能力层                            │
│  ┌──────────┐ ┌──────────┐ ┌───────────┐ ┌───────────┐  │
│  │情感分析   │ │风险评估   │ │双记忆RAG  │ │内容审核    │  │
│  │          │ │          │ │(湖神AI)   │ │(两级流水线)│  │
│  └──────────┘ └──────────┘ └───────────┘ └───────────┘  │
│  ┌──────────┐ ┌──────────┐ ┌───────────┐               │
│  │向量嵌入   │ │推荐算法   │ │情绪追踪    │               │
│  │(TF-IDF)  │ │(MF+UCB)  │ │(负重监测)  │               │
│  └──────────┘ └──────────┘ └───────────┘               │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                    后台守护层                             │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────┐  │
│  │ 湖神守护线程  │ │ 负重者监测    │ │ 石友TTL管理      │  │
│  │(零互动扫描)   │ │ (情绪扫描)   │ │ (过期/提醒)      │  │
│  └──────────────┘ └──────────────┘ └──────────────────┘  │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                    数据存储层                             │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │ PostgreSQL 16 │  │   Redis 7    │  │  向量存储      │  │
│  │ (持久化存储)   │  │ (缓存/TTL)   │  │ (嵌入索引)    │  │
│  └──────────────┘  └──────────────┘  └───────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 环境要求

- Docker & Docker Compose
- C++20 编译器（GCC 12+ / Clang 15+）
- CMake 3.20+
- Node.js 18+（管理后台）
- Flutter 3.19+（移动端）

### 启动服务

```bash
# 克隆项目
git clone https://github.com/heartlake/heartlake.git
cd heartlake

# 使用 Docker Compose 启动全部服务
docker compose up -d

# 查看服务状态
docker compose ps
```

### 本地开发

```bash
# 后端编译
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# 管理后台
cd admin && npm install && npm run dev

# 移动端
cd mobile && flutter pub get && flutter run
```

---

## 技术栈

| 层级 | 技术选型 | 说明 |
|------|---------|------|
| 后端 | C++20 + Drogon | 高性能异步 HTTP/WebSocket 框架 |
| 管理后台 | Vue 3 + Vite 5 + Element Plus | 现代化管理界面 |
| 移动端 | Flutter 3.19+ | 跨平台移动应用 |
| 数据库 | PostgreSQL 16 | 主数据存储 |
| 缓存 | Redis 7 | 缓存、TTL管理、实时数据 |
| AI | DeepSeek API + 本地 TF-IDF 嵌入 + 边缘AI引擎 | 情感分析、内容审核、推荐 |
| 容器化 | Docker Compose | 一键部署 |
| 认证 | PASETO v4 | 安全令牌认证 |
| 加密 | X25519 + AES-256-GCM | E2E 端到端加密 |
| 实时通信 | WebSocket | 涟漪广播、通知推送 |

---

## 许可证

本项目采用 [AGPL-3.0](LICENSE) 许可证开源。

```
Copyright (C) 2024-2025 HeartLake Contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
```

---

<p align="center">
  <sub>用技术守护每一颗心 | Built with care by HeartLake Team</sub>
</p>
