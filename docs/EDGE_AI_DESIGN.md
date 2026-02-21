# 边缘AI引擎技术设计文档

## 目录

1. [概述](#1-概述)
2. [系统架构](#2-系统架构)
3. [三层情感融合分析](#3-三层情感融合分析)
4. [AC自动机内容审核](#4-ac自动机内容审核)
5. [五因子心理风险评估](#5-五因子心理风险评估)
6. [HNSW向量搜索](#6-hnsw向量搜索)
7. [差分隐私Laplace机制](#7-差分隐私laplace机制)
8. [联邦学习FedAvg](#8-联邦学习fedavg)
9. [情绪脉搏追踪](#9-情绪脉搏追踪)
10. [INT8量化推理](#10-int8量化推理)
11. [熔断器状态机](#11-熔断器状态机)
12. [情绪共鸣引擎](#12-情绪共鸣引擎)
13. [节点监控](#13-节点监控)
14. [云端协同](#14-云端协同)
15. [未来演进](#15-未来演进)

---

## 1. 概述

### 1.1 为什么需要边缘AI

传统云端AI架构面临三大核心挑战：

- **延迟敏感**：情感陪伴场景要求响应延迟 < 100ms，云端往返延迟通常 200-500ms
- **隐私合规**：用户情感数据属于高度敏感信息，GDPR/个人信息保护法要求数据最小化传输
- **带宽成本**：持续上传音频/视频流到云端，带宽成本随用户规模线性增长

边缘AI引擎将推理能力下沉到设备端，实现：

| 指标         | 云端方案   | 边缘方案     |
|-------------|-----------|-------------|
| 推理延迟     | 200-500ms | 10-50ms     |
| 数据离开设备  | 全量上传   | 仅聚合梯度   |
| 离线可用     | 不可用     | 完全可用     |
| 单用户成本   | 高        | 边际成本趋零  |

### 1.2 设计目标

- 端侧推理延迟 P99 < 50ms
- 模型体积 < 20MB（INT8量化后）
- 内存占用 < 100MB
- 支持完全离线运行
- 隐私数据零泄露

### 1.3 八大子系统总览

| 子系统 | 核心算法 | 时间复杂度 | 代码位置 |
|-------|---------|-----------|---------|
| 三层情感融合 | 规则+词典+统计集成 | O(n) | EdgeAIEngine.cpp:395-590 |
| AC自动机审核 | Aho-Corasick多模式匹配 | O(n+m+z) | EdgeAIEngine.cpp:596-789 |
| 五因子风险评估 | 加权多因子评分 | O(n*k) | PsychologicalRiskAssessment.cpp |
| HNSW向量搜索 | 分层可导航小世界图 | O(log n) | EdgeAIEngine.cpp:100-393 |
| 差分隐私 | Laplace机制+原子预算 | O(1) | EdgeAIEngine.cpp:1057-1099 |
| 联邦学习 | FedAvg加权聚合 | O(K*d) | EdgeAIEngine.cpp:955-1019 |
| 情绪脉搏 | 滑动窗口+线性回归 | O(w) | EdgeAIEngine.cpp:824-941 |
| INT8量化 | 对称量化+INT32累加 | O(n) | EdgeAIEngine.cpp:1100+ |

---

## 2. 系统架构

### 2.1 整体架构图

```
+-------------------------------------------------------------------+
|                        云端协同层 (Cloud)                           |
|  +----------+  +--------------+  +-----------+  +-----------+     |
|  | 模型仓库  |  | 联邦聚合服务  |  | 监控大盘   |  | A/B实验    |     |
|  +-----+----+  +------+-------+  +-----+-----+  +-----+-----+     |
+--------|--------------|--------------|--------------|--------------+
         |   加密梯度    |    健康心跳   |   实验配置    |
  -------+----------- --+--- ----------+--- ----------+--------
         |              |              |              |
+--------|--------------|--------------|--------------|--------------+
|        v              v              v              v              |
|  +-------------------------------------------------------------+ |
|  |                    边缘网关 (Edge Gateway)                     | |
|  |  +---------+  +----------+  +---------+  +----------+       | |
|  |  | 熔断器   |  | 限流器    |  | 路由器   |  | TLS终结   |       | |
|  |  +---------+  +----------+  +---------+  +----------+       | |
|  +-----------------------------+-------------------------------+ |
|                                |                                  |
|  +-----------------------------v-------------------------------+ |
|  |                  AI推理引擎 (Inference Engine)                 | |
|  |                                                             | |
|  |  +------------+  +-------------+  +------------+            | |
|  |  | 三层情感分析 |  | AC自动机审核 |  | 五因子风险  |            | |
|  |  +------------+  +-------------+  +------------+            | |
|  |                                                             | |
|  |  +------------+  +-------------+  +------------+            | |
|  |  | HNSW索引   |  | INT8量化推理 |  | 情绪共鸣    |            | |
|  |  | 向量搜索   |  | 模型执行    |  | 引擎       |            | |
|  |  +------------+  +-------------+  +------------+            | |
|  +-----------------------------+-------------------------------+ |
|                                |                                  |
|  +-----------------------------v-------------------------------+ |
|  |                  隐私保护层 (Privacy Layer)                    | |
|  |  +------------+  +-------------+  +------------+            | |
|  |  | 差分隐私    |  | 联邦学习     |  | 安全聚合   |            | |
|  |  | Laplace    |  | FedAvg      |  | SecAgg    |            | |
|  |  +------------+  +-------------+  +------------+            | |
|  +-----------------------------+-------------------------------+ |
|                                |                                  |
|  +-----------------------------v-------------------------------+ |
|  |                  运维监控层 (Ops Layer)                        | |
|  |  +------------+  +-------------+  +------------+            | |
|  |  | 情绪脉搏    |  | 节点监控     |  | 云端同步   |            | |
|  |  | 追踪       |  | 健康检查    |  | 离线缓存   |            | |
|  |  +------------+  +-------------+  +------------+            | |
|  +-------------------------------------------------------------+ |
+-------------------------------------------------------------------+
```

### 2.2 数据流

```
用户输入 --> 三层情感分析 --> AC自动机审核 --> 五因子风险评估
                |                                    |
                v                                    v
          情绪脉搏追踪                          风险等级判定
                |                                    |
                v                                    v
          HNSW向量搜索 <-- INT8量化推理      情绪共鸣引擎
                |                                    |
                v                                    v
          推荐结果 <----- 熔断器保护 -----> 共鸣推荐结果
                |
                v
          差分隐私加噪 --> 联邦学习上传 --> 云端聚合
```

---

## 3. 三层情感融合分析

三层情感融合分析是HeartLake情感理解的核心，采用规则层、词典层、统计层三路并行分析，最终加权融合得到综合情感分数。

**代码位置**: `backend/src/infrastructure/ai/EdgeAIEngine.cpp:395-590`

### 3.1 架构概览

```
输入文本
  |
  +---> [规则层] 表情符号/标点/大写检测 ---> 权重 0.25
  |
  +---> [词典层] 情感词典匹配+否定/强化 ---> 权重 0.50
  |
  +---> [统计层] 5维特征统计分类       ---> 权重 0.25
  |
  v
加权融合: finalScore = 0.25*rule + 0.50*lexicon + 0.25*statistical
  |
  v
scoresToMood() 映射为情绪类别
```

### 3.2 规则层 (Rule-based Sentiment)

基于手工规则检测文本中的情感信号：

- **颜文字/Emoji检测**: 识别 `:)`, `:(`, `XD`, `T_T` 等模式，直接映射为正/负情感分数
- **标点模式**: 多个感叹号 `!!!` 增强情感强度，多个问号 `???` 表示困惑/质疑
- **全大写检测**: 全大写文本情感强度乘以 1.5 倍（表示强烈情绪）

**算法伪代码**:
```
function ruleSentiment(text):
    score = 0.0
    // 颜文字检测
    for pattern in POSITIVE_KAOMOJI:
        if text contains pattern: score += 0.3
    for pattern in NEGATIVE_KAOMOJI:
        if text contains pattern: score -= 0.3
    // 标点模式
    exclamation_count = count('!' in text)
    if exclamation_count >= 3: score *= 1.2
    // 全大写放大
    if text is ALL_CAPS: score *= 1.5
    return clamp(score, -1.0, 1.0)
```

**复杂度**: O(n)，n为文本长度

### 3.3 词典层 (Lexicon-based Sentiment)

基于情感词典进行精细化情感分析，支持否定词和强化词处理：

- **情感词典匹配**: 正面词(+score)和负面词(-score)
- **否定词处理**: 检测到否定词时，后续情感词分数乘以 -0.75（非简单取反，保留部分原始语义）
- **强化词处理**: "非常"、"极其"等强化词放大后续情感词的分数
- **归一化**: `score = totalScore / sqrt(matchedCount)`，使用平方根归一化避免长文本偏差

**算法伪代码**:
```
function lexiconSentiment(text):
    tokens = tokenize(text)
    totalScore = 0.0
    matchedCount = 0
    negation = false
    for token in tokens:
        if token in NEGATION_WORDS:
            negation = true
            continue
        if token in INTENSIFIERS:
            intensify = INTENSIFIERS[token]
            continue
        if token in SENTIMENT_LEXICON:
            wordScore = SENTIMENT_LEXICON[token] * intensify
            if negation:
                wordScore *= -0.75
                negation = false
            totalScore += wordScore
            matchedCount++
            intensify = 1.0
    if matchedCount == 0: return 0.0
    return totalScore / sqrt(matchedCount)
```

**复杂度**: O(n)，n为token数量

### 3.4 统计层 (Statistical Sentiment)

基于5维文本统计特征进行分类：

| 特征 | 说明 | 计算方式 |
|-----|------|---------|
| TTR | 词汇丰富度 | unique_words / total_words |
| 平均词长 | 用词复杂度 | total_chars / total_words |
| 文本长度 | 表达充分度 | min(char_count / 500.0, 1.0) |
| 正负比 | 情感倾向 | positive_count / (negative_count + 1) |
| 重复字符 | 情绪强度 | repeated_char_sequences / total_chars |

**算法伪代码**:
```
function statisticalSentiment(text):
    features = [TTR, avgWordLen, textLen, posNegRatio, repeatChars]
    // 简单线性分类器
    weights = trained_weights  // 预训练权重
    score = dot(features, weights) + bias
    return tanh(score)  // 映射到[-1, 1]
```

**复杂度**: O(n)，n为文本长度

### 3.5 融合与情绪映射

三层分数加权融合后，通过阈值映射为情绪类别：

```
function scoresToMood(score):
    if score > 0.6:  return "joy"
    if score > 0.2:  return "surprise"
    if score > -0.2: return "neutral"
    if score > -0.5: return "sadness"
    if score > -0.7: return "fear"
    return "anger"
```

**关键参数**:

| 参数 | 值 | 说明 |
|-----|---|------|
| wRule | 0.25 | 规则层权重 |
| wLexicon | 0.50 | 词典层权重（主导） |
| wStatistical | 0.25 | 统计层权重 |
| 否定系数 | -0.75 | 否定词对情感分数的翻转系数 |
| 大写放大 | 1.5x | 全大写文本的情感强度放大倍数 |

---

## 4. AC自动机内容审核

基于Aho-Corasick多模式匹配算法实现高效内容审核，支持中英文敏感词同时检测。

**代码位置**: `backend/src/infrastructure/ai/EdgeAIEngine.cpp:596-789`

### 4.1 算法原理

AC自动机在Trie树基础上增加失败指针（failure link），实现一次扫描同时匹配所有模式串，避免逐词匹配的O(n*k)复杂度。

### 4.2 四类敏感词分类

| 类别ID | 类别名称 | 说明 | 风险权重 |
|-------|---------|------|---------|
| 1 | self_harm | 自伤/自杀相关 | 最高，触发1.2x风险乘数 |
| 2 | violence | 暴力相关 | 高 |
| 3 | sexual | 色情相关 | 高 |
| 0 | general_profanity | 一般脏话 | 中 |

### 4.3 构建过程

```
function buildModerationAC():
    trie = new AhoCorasick()
    // 插入四类中英文敏感词
    for category in [self_harm, violence, sexual, profanity]:
        for word in category.chinese_patterns:
            trie.insert(word, category.id)
        for word in category.english_patterns:
            trie.insert(word, category.id)
    trie.buildFailureLinks()  // BFS构建失败指针
    return trie
```

### 4.4 语义风险分析

匹配到敏感词后，进行语义级别的风险评估：

```
function semanticRiskAnalysis(text, matches):
    riskScore = 0.0
    for match in matches:
        baseRisk = CATEGORY_RISK[match.category]
        // 自伤类别额外加权
        if match.category == SELF_HARM:
            baseRisk *= 1.2
        // 上下文分析：否定语境降低风险
        if hasNegationContext(text, match.position):
            baseRisk *= 0.5
        riskScore = max(riskScore, baseRisk)
    return riskScore
```

### 4.5 风险等级判定

| 等级 | 分数范围 | 处理策略 |
|-----|---------|---------|
| SAFE | 0 - 0.3 | 正常通过 |
| WARNING | 0.3 - 0.6 | 标记审核 |
| DANGEROUS | 0.6 - 0.8 | 拦截+人工复审 |
| CRITICAL | 0.8 - 1.0 | 立即拦截+告警 |

**复杂度分析**:
- 构建: O(m)，m为所有模式串总长度
- 匹配: O(n + z)，n为文本长度，z为匹配数量
- 空间: O(m * sigma)，sigma为字符集大小

---

## 5. 五因子心理风险评估

基于心理学研究的五因子模型，对用户文本进行多维度心理风险评估，用于识别潜在的心理危机。

**代码位置**: `backend/src/utils/PsychologicalRiskAssessment.cpp`

### 5.1 五因子模型

| 因子 | 权重 | 说明 |
|-----|------|------|
| selfHarmIntent | 0.9 | 自伤意图检测，权重最高 |
| hopelessness | 0.6 | 绝望感评估 |
| temporalUrgency | 0.5 | 时间紧迫性（"现在就"、"马上"） |
| linguisticMarkers | 0.3 | 语言学标记（第一人称过度使用等） |
| socialIsolation | 0.1 | 社交孤立信号（"没人理解"、"独自一人"） |

### 5.2 评估算法

```
function assessPsychologicalRisk(text):
    factors = {}
    // 各因子独立评估
    factors.selfHarmIntent = detectSelfHarmKeywords(text)      // 中文关键词匹配
    factors.hopelessness = detectHopelessnessPatterns(text)     // 绝望表达模式
    factors.temporalUrgency = detectTemporalUrgency(text)      // 时间紧迫词汇
    factors.linguisticMarkers = analyzeLinguisticPatterns(text) // 语言学特征
    factors.socialIsolation = detectIsolationSignals(text)      // 社交孤立信号

    // 加权求和
    weights = [0.9, 0.6, 0.5, 0.3, 0.1]
    totalRisk = sum(weight_i * factor_i) / sum(weight_i)

    // 风险等级映射
    level = mapRiskLevel(totalRisk)
    return {totalRisk, level, factors}
```

### 5.3 风险等级

| 等级 | 分数范围 | 响应策略 |
|-----|---------|---------|
| NONE | 0 | 无风险 |
| LOW | 0 - 0.3 | 正常服务 |
| MEDIUM | 0.3 - 0.6 | 温和关怀提示 |
| HIGH | 0.6 - 0.8 | 推送专业资源 |
| CRITICAL | 0.8 - 1.0 | 紧急干预+危机热线 |

### 5.4 关键词检测

每个因子维护独立的中文关键词集合，例如：

- **selfHarmIntent**: 包含自伤、自杀相关的中文表达模式
- **hopelessness**: "没有希望"、"活着没意思"等绝望表达
- **temporalUrgency**: "现在就"、"今晚"、"马上"等时间紧迫词
- **socialIsolation**: "没人理解我"、"独自一人"、"被抛弃"等孤立表达

**复杂度**: O(n * k)，n为文本长度，k为关键词总数

---

## 6. HNSW向量搜索

HNSW（Hierarchical Navigable Small World）是一种基于分层可导航小世界图的近似最近邻搜索算法，用于高维向量的快速相似度检索。

**代码位置**: `backend/src/infrastructure/ai/EdgeAIEngine.cpp:100-393`

### 6.1 核心参数

| 参数 | 值 | 说明 |
|-----|---|------|
| M | 16 | 每层最大连接数 |
| MMax0 | 32 | 第0层最大连接数（底层加倍） |
| efConstruction | 200 | 构建时搜索宽度 |
| efSearch | 50 | 查询时搜索宽度 |
| levelMult | 1/ln(M) = 1/ln(16) | 层级概率乘数 |

### 6.2 层级分配

节点的最大层级通过随机概率分配：

```
function randomLevel():
    level = 0
    while random() < 1.0/ln(M) and level < MAX_LEVEL:
        level++
    return level
```

这保证了层级呈指数递减分布，高层节点稀疏，底层节点密集。

### 6.3 插入算法

```
function insert(node, embedding):
    level = randomLevel()
    node.level = level
    node.embedding = embedding

    if graph is empty:
        entryPoint = node
        return

    current = entryPoint
    // 从最高层贪心下降到 level+1
    for l = maxLevel downto level+1:
        current = greedySearch(current, embedding, l)

    // 从 level 层到第0层，搜索并建立连接
    for l = level downto 0:
        neighbors = searchLayer(current, embedding, efConstruction, l)
        // 选择最近的M个邻居（第0层选MMax0个）
        maxConn = (l == 0) ? MMax0 : M
        selected = selectNeighbors(neighbors, maxConn)
        // 双向连接
        for neighbor in selected:
            connect(node, neighbor, l)
            connect(neighbor, node, l)
            // 如果邻居连接数超限，裁剪最远的
            if degree(neighbor, l) > maxConn:
                pruneConnections(neighbor, maxConn, l)
```

### 6.4 搜索算法

```
function search(query, K, efSearch):
    current = entryPoint
    // 从最高层贪心下降到第1层
    for l = maxLevel downto 1:
        current = greedySearch(current, query, l)
    // 在第0层进行beam search
    candidates = searchLayer(current, query, efSearch, 0)
    return topK(candidates, K)
```

### 6.5 复杂度分析

| 操作 | 时间复杂度 | 说明 |
|-----|-----------|------|
| 插入 | O(log n * M * efConstruction) | 每层搜索+连接 |
| 搜索 | O(log n * efSearch) | 分层贪心+底层beam |
| 空间 | O(n * M * L) | n节点，M连接，L平均层数 |

其中 L = O(log n)，因此搜索复杂度近似 O(log^2 n)。

### 6.6 关键参数

| 参数 | 默认值 | 含义 |
|-----|-------|------|
| M | 16 | 每层最大连接数 |
| MMax0 | 32 | 第0层最大连接数（2*M） |
| efConstruction | 200 | 构建时搜索宽度 |
| efSearch | 50 | 查询时搜索宽度 |
| levelMult | 1/ln(M) ≈ 0.3607 | 层级概率乘数 |
| 距离度量 | 余弦距离 | 1 - cosineSimilarity |

代码位置：`backend/src/infrastructure/ai/EdgeAIEngine.cpp:100-393`，`backend/include/infrastructure/ai/EdgeAIEngine.h`（HNSWNode, VectorSearchResult 结构体）

---

## 7. 差分隐私Laplace机制

### 7.1 设计动机

联邦学习中，即使只上传模型梯度而非原始数据，攻击者仍可通过梯度反演（gradient inversion）推断训练数据。差分隐私通过向梯度添加校准噪声，提供数学可证明的隐私保障。

### 7.2 核心算法

Laplace机制通过逆CDF采样生成噪声：

```
X = -b * sign(U - 0.5) * ln(1 - 2|U - 0.5|)
```

其中 b = sensitivity / epsilon，U ~ Uniform(0, 1)。

### 7.3 实现伪代码

```
function sampleLaplace(sensitivity, epsilon, currentBudget, maxBudget):
    // 原子CAS预算检查
    loop:
        current = atomicLoad(currentBudget)
        if current + epsilon > maxBudget:
            return 0.0  // 预算耗尽，返回零噪声
        if atomicCAS(currentBudget, current, current + epsilon):
            break

    // Laplace采样（逆CDF方法）
    b = sensitivity / epsilon
    U = uniformRandom(0, 1)
    noise = -b * sign(U - 0.5) * ln(1 - 2 * |U - 0.5|)
    return noise
```

### 7.4 预算管理

采用原子CAS（Compare-And-Swap）操作实现无锁预算追踪：

- `privacyBudgetUsed_`：原子变量，记录已消耗的隐私预算
- `maxBudget`：DPConfig中配置的最大预算上限
- 每次采样消耗 epsilon 预算，预算耗尽后返回零噪声（静默降级）

### 7.5 关键参数

| 参数 | 默认值 | 含义 |
|-----|-------|------|
| epsilon | 1.0 | 隐私预算单次消耗 |
| maxBudget | 10.0 | 总隐私预算上限 |
| sensitivity | 1.0 | 查询敏感度 |
| noiseScale | sensitivity/epsilon | Laplace分布尺度参数 |

### 7.6 复杂度分析

| 操作 | 复杂度 | 说明 |
|-----|-------|------|
| 单次采样 | O(1) | 逆CDF计算 |
| 预算检查 | O(1) 均摊 | 原子CAS，极少重试 |
| 空间 | O(1) | 仅存储预算计数器 |

代码位置：`backend/src/infrastructure/ai/EdgeAIEngine.cpp:1057-1099`

---

## 8. 联邦学习FedAvg

### 8.1 设计动机

用户情感模型需要持续学习以适应个体差异，但原始情感数据不能离开设备。联邦学习允许多个设备协作训练共享模型，仅交换模型参数（梯度），原始数据始终留在本地。

### 8.2 核心算法

FedAvg（Federated Averaging）加权聚合公式：

```
w_global = Sum(n_k / n) * w_k
```

其中 n_k 为第k个客户端的样本数，n = Sum(n_k) 为总样本数。

### 8.3 实现伪代码

```
function aggregateFedAvg(clientUpdates[]):
    if clientUpdates is empty:
        return currentGlobalModel

    // 计算总样本数
    totalSamples = 0
    for each update in clientUpdates:
        totalSamples += update.sampleCount

    // 加权平均聚合
    globalWeights = zeros(modelDimension)
    for each update in clientUpdates:
        weight = update.sampleCount / totalSamples
        for i in 0..modelDimension:
            globalWeights[i] += weight * update.weights[i]

    // 可选：添加差分隐私噪声
    if dpEnabled:
        for i in 0..modelDimension:
            globalWeights[i] += sampleLaplace(sensitivity, epsilon)

    // 更新全局模型版本号
    globalModelVersion++
    return globalWeights
```

### 8.4 关键参数

| 参数 | 含义 |
|-----|------|
| clientUpdates | 各客户端上传的模型参数和样本数 |
| sampleCount | 单客户端本地训练样本数 |
| modelVersion | 全局模型版本号，单调递增 |
| dpEnabled | 是否在聚合后添加差分隐私噪声 |

### 8.5 复杂度分析

| 操作 | 复杂度 | 说明 |
|-----|-------|------|
| 聚合 | O(K * d) | K个客户端，d维模型参数 |
| 空间 | O(d) | 全局模型参数存储 |

代码位置：`backend/src/infrastructure/ai/EdgeAIEngine.cpp:955-1019`

---

## 9. 情绪脉搏追踪

### 9.1 设计动机

单次情感分析只能捕捉瞬时情绪，无法反映用户情绪的变化趋势。情绪脉搏系统通过滑动窗口持续追踪情绪时序数据，计算趋势、波动性和稳定性指标，为心理健康预警提供数据支撑。

### 9.2 核心机制

- 滑动窗口：固定300秒时间窗口，最多保留100条历史记录
- 趋势计算：线性回归斜率（最小二乘法）
- 波动性：窗口内情绪分数的标准差
- 快照机制：每累积10个样本自动生成一次快照

### 9.3 实现伪代码

```
struct EmotionPulse:
    currentScore: float       // 当前情绪分数
    trend: float             // 趋势斜率（正=好转，负=恶化）
    volatility: float        // 波动性（标准差）
    stability: float         // 稳定性 = 1 / (1 + volatility)
    dominantMood: string     // 窗口内主导情绪
    windowSize: int          // 当前窗口样本数
    snapshotCount: int       // 已生成快照数

function updatePulse(newScore, timestamp):
    // 1. 清理过期数据（超出300秒窗口）
    removeExpired(history, timestamp - 300s)

    // 2. 添加新数据点
    history.push({score: newScore, time: timestamp})
    if history.size > 100:
        history.removeOldest()

    // 3. 计算趋势（线性回归斜率）
    if history.size >= 3:
        trend = linearRegressionSlope(history)

    // 4. 计算波动性（标准差）
    volatility = stddev(history.scores)
    stability = 1.0 / (1.0 + volatility)

    // 5. 统计主导情绪
    dominantMood = mostFrequent(history.moods)

    // 6. 快照机制
    samplesSinceSnapshot++
    if samplesSinceSnapshot >= 10:
        saveSnapshot(currentPulse)
        samplesSinceSnapshot = 0

function linearRegressionSlope(points):
    n = points.size
    sumX = sumY = sumXY = sumX2 = 0
    for i, point in enumerate(points):
        x = i  // 使用索引作为时间序列
        y = point.score
        sumX += x; sumY += y
        sumXY += x * y; sumX2 += x * x
    denominator = n * sumX2 - sumX * sumX
    if denominator == 0: return 0
    return (n * sumXY - sumX * sumY) / denominator
```

### 9.4 情绪分数到情绪类型映射

```
function scoresToMood(score):
    if score > 0.6:  return "joy"
    if score > 0.2:  return "surprise"
    if score > -0.2: return "neutral"
    if score > -0.5: return "sadness"
    if score > -0.7: return "fear"
    return "anger"
```

### 9.5 关键参数

| 参数 | 默认值 | 含义 |
|-----|-------|------|
| windowDuration | 300秒 | 滑动窗口时间跨度 |
| maxHistory | 100 | 最大历史记录数 |
| snapshotInterval | 10 | 每N个样本生成快照 |
| minSamplesForTrend | 3 | 计算趋势所需最少样本数 |

### 9.6 复杂度分析

| 操作 | 复杂度 | 说明 |
|-----|-------|------|
| 更新 | O(w) | w为窗口大小，需遍历计算统计量 |
| 快照 | O(1) | 固定大小数据拷贝 |
| 空间 | O(w) | 滑动窗口存储 |

代码位置：`backend/src/infrastructure/ai/EdgeAIEngine.cpp:824-941`

---

## 10. INT8量化推理

### 10.1 设计动机

边缘设备内存和算力有限，FP32模型体积大、推理慢。INT8对称量化将模型体积压缩至1/4，同时利用整数运算加速推理，在精度损失可控的前提下大幅提升端侧性能。

### 10.2 核心算法

对称量化公式：

```
scale = max(|x|) / 127
x_quantized = round(x / scale)        // FP32 -> INT8
x_dequantized = x_quantized * scale   // INT8 -> FP32
```

量化矩阵乘法使用INT32累加防止溢出：

```
C_int32[i][j] = Sum(A_int8[i][k] * B_int8[k][j])  // INT32累加
C_fp32[i][j] = C_int32[i][j] * scaleA * scaleB     // 反量化
```

### 10.3 实现伪代码

```
function quantize(floatData[]):
    // 计算对称量化scale
    maxAbs = 0
    for x in floatData:
        maxAbs = max(maxAbs, |x|)
    scale = maxAbs / 127.0
    if scale == 0: scale = 1.0

    // 量化为INT8
    quantized = new int8[floatData.size]
    for i in 0..floatData.size:
        val = round(floatData[i] / scale)
        quantized[i] = clamp(val, -128, 127)
    return QuantizedTensor{quantized, scale}

function quantizedDotProduct(a: QuantizedTensor, b: QuantizedTensor):
    // INT32累加，4路展开优化
    acc = 0  // int32
    i = 0
    // 4-way unrolling
    while i + 4 <= length:
        acc += a.data[i]   * b.data[i]
              + a.data[i+1] * b.data[i+1]
              + a.data[i+2] * b.data[i+2]
              + a.data[i+3] * b.data[i+3]
        i += 4
    // 处理剩余元素
    while i < length:
        acc += a.data[i] * b.data[i]
        i++
    // 反量化回FP32
    return acc * a.scale * b.scale
```

### 10.4 关键参数

| 参数 | 值 | 含义 |
|-----|---|------|
| 量化位宽 | 8-bit | INT8对称量化 |
| 量化范围 | [-128, 127] | 有符号8位整数 |
| 累加精度 | INT32 | 防止中间结果溢出 |
| 展开因子 | 4 | 循环展开优化 |

### 10.5 复杂度分析

| 操作 | 复杂度 | 说明 |
|-----|-------|------|
| 量化 | O(n) | 遍历所有元素 |
| 点积 | O(n) | 4路展开常数优化 |
| 空间 | O(n) | INT8存储，为FP32的1/4 |

代码位置：`backend/src/infrastructure/ai/EdgeAIEngine.cpp:1100+`，`backend/include/infrastructure/ai/EdgeAIEngine.h`（QuantizedTensor 结构体）

---

## 11. 熔断器状态机（CircuitBreaker）

### 11.1 设计动机

边缘AI推理依赖多个子系统（embedding生成、向量搜索、情绪分析等），任何子系统的持续故障都可能导致级联失败。熔断器模式通过快速失败（fail-fast）机制，在检测到下游服务异常时主动切断调用链，保护系统整体可用性，并在故障恢复后自动重新接入。

### 11.2 状态机模型

```
                    失败次数 >= threshold
    CLOSED ──────────────────────────────> OPEN
      ^                                     |
      |                                     |
      | 探测成功                    超时(resetTimeoutMs)
      |                                     |
      └──────────── HALF_OPEN <─────────────┘
                       |
                       | 探测失败
                       └──────> OPEN
```

三态转换规则：

| 当前状态 | 事件 | 目标状态 | 动作 |
|---------|------|---------|------|
| CLOSED | 失败次数 >= failureThreshold | OPEN | 拒绝所有请求 |
| OPEN | 经过 resetTimeoutMs 毫秒 | HALF_OPEN | 允许一个探测请求 |
| HALF_OPEN | 探测请求成功 | CLOSED | 重置失败计数，恢复正常 |
| HALF_OPEN | 探测请求失败 | OPEN | 重新计时 |

### 11.3 实现伪代码

```
class CircuitBreaker:
    state = CLOSED
    failureCount = 0
    failureThreshold = 5
    resetTimeoutMs = 30000
    lastFailureTime = 0

    function execute(func):
        if not allowRequest():
            throw "Circuit breaker is OPEN"

        try:
            result = func()
            onSuccess()
            return result
        catch:
            onFailure()
            rethrow

    function allowRequest():
        lock(mutex)
        if state == CLOSED:
            return true
        if state == OPEN:
            if now() - lastFailureTime > resetTimeoutMs:
                state = HALF_OPEN
                return true
            return false
        // HALF_OPEN: 允许探测
        return true

    function onSuccess():
        lock(mutex)
        if state == HALF_OPEN:
            state = CLOSED
            failureCount = 0

    function onFailure():
        lock(mutex)
        failureCount++
        lastFailureTime = now()
        if failureCount >= failureThreshold:
            state = OPEN
```

### 11.4 关键参数

| 参数 | 默认值 | 含义 |
|-----|-------|------|
| failureThreshold | 5 | 触发熔断的连续失败次数 |
| resetTimeoutMs | 30000 | OPEN状态超时时间（毫秒） |
| 并发控制 | std::mutex | 保护状态转换的线程安全 |

### 11.5 设计要点

- 使用 `mutable std::mutex` 保证 `getState()` const方法也能加锁
- `execute()` 模板方法通过 `if constexpr` 区分 void 和非 void 返回类型
- 失败计数采用简单累加策略（非滑动窗口），适合边缘设备低开销场景
- `reset()` 方法提供手动恢复能力，用于运维干预

代码位置：`backend/include/utils/CircuitBreaker.h`

---

## 12. 情绪感知时序共鸣引擎（EmotionResonanceEngine）

### 12.1 设计动机

传统内容推荐基于内容相似度或协同过滤，忽略了用户的情绪状态变化。HeartLake的"石头"（匿名心事）具有强烈的情绪属性，因此推荐算法需要理解用户的情绪轨迹，将处于相似情绪旅程中的用户连接起来，同时避免"回音室效应"（只推荐相同情绪的内容）。

### 12.2 四维共鸣公式

```
ResonanceScore = alpha * SemanticSim + beta * EmotionTrajectorySim
               + gamma * TemporalDecay + delta * DiversityBonus
```

| 维度 | 权重 | 含义 |
|-----|------|------|
| SemanticSim (alpha) | 0.30 | 文本语义相似度（余弦距离） |
| EmotionTrajectorySim (beta) | 0.35 | 情绪轨迹DTW相似度 |
| TemporalDecay (gamma) | 0.20 | 时间新鲜度衰减 |
| DiversityBonus (delta) | 0.15 | 多样性奖励（反回音室） |

权重约束：alpha + beta + gamma + delta = 1.0

### 12.3 DTW情绪轨迹相似度

Dynamic Time Warping（动态时间规整）用于比较两个不等长的情绪分数序列，找到最优对齐路径。

```
function trajectorySimDTW(traj1[1..n], traj2[1..m]):
    // 初始化DTW距离矩阵
    dtw[0..n][0..m] = INF
    dtw[0][0] = 0

    for i = 1 to n:
        for j = 1 to m:
            cost = |traj1[i] - traj2[j]|
            dtw[i][j] = cost + min(dtw[i-1][j],
                                    dtw[i][j-1],
                                    dtw[i-1][j-1])

    // 归一化 + 高斯核转换
    normalizedDist = dtw[n][m] / max(n, m)
    similarity = exp(-normalizedDist^2 / 2)
    return similarity
```

高斯核 `exp(-d^2/2)` 将距离映射到 [0, 1] 相似度空间，距离越小相似度越高。

复杂度：O(n * m) 时间，O(n * m) 空间。

### 12.4 时间衰减

指数衰减函数，使新发布的石头获得更高权重：

```
temporalDecay(timestamp) = exp(-lambda * deltaHours)
```

其中 lambda = 0.1（默认），deltaHours 为石头发布距今的小时数。

| deltaHours | 衰减值 |
|-----------|-------|
| 0 | 1.000 |
| 1 | 0.905 |
| 6 | 0.549 |
| 24 | 0.091 |
| 72 | 0.001 |

支持两种时间戳格式解析：`%Y-%m-%d %H:%M:%S` 和 `%Y-%m-%dT%H:%M:%S`。

### 12.5 多样性奖励

多样性奖励机制通过三层策略避免推荐结果的情绪单一化：

```
function diversityBonus(currentMood, candidateMood, alreadyRecommended):
    // 层1: 基础分 - 不同情绪类型获得更高基础分
    bonus = (currentMood != candidateMood) ? 0.6 : 0.3

    // 层2: 重复衰减 - 已推荐列表中相同情绪越多，奖励越低
    sameCount = count(candidateMood in alreadyRecommended)
    bonus *= 0.8 ^ sameCount

    // 层3: 互补情绪额外奖励（心理学治愈效果）
    complementaryPairs = {
        sad <-> hopeful,
        anxious <-> calm,
        angry <-> grateful,
        confused -> hopeful,
        lonely -> happy
    }
    if (currentMood, candidateMood) in complementaryPairs:
        bonus += 0.3

    return min(1.0, bonus)
```

### 12.6 共鸣原因生成

根据四维分数中的主导维度，生成人类可读的共鸣原因文本：

| 主导维度 | 条件 | 示例文本 |
|---------|------|---------|
| trajectoryScore > 0.6 | 相同情绪 | "你们正经历着相似的情绪旅程，心灵在同一频率上共振" |
| semanticScore > 0.7 | - | "你们的心声如此相似，仿佛来自同一片星空" |
| diversityScore > 0.5 | 互补情绪 | 根据currentMood查表（如sad: "这份温暖也许能照亮你心中的阴霾"） |
| temporalScore > 0.8 | - | "此刻，有人和你一样在湖边驻足" |
| totalScore > 0.7 | 综合 | "冥冥之中，你们的心灵产生了深深的共鸣" |

### 12.7 推荐流程

```
function findResonance(userId, stoneId, limit=10):
    // 1. 获取源石头信息
    source = db.query("SELECT ... FROM stones WHERE stone_id = ?", stoneId)

    // 2. 加载当前用户近7天情绪轨迹
    userTraj = loadTrajectory(userId, days=7)

    // 3. 生成源石头的embedding向量
    sourceEmb = embeddingEngine.generateEmbedding(source.content)

    // 4. 获取候选石头（排除自己的、近30天、limit*5条）
    candidates = db.query("SELECT ... FROM stones WHERE ...")

    // 5. 对每个候选计算四维分数
    for each candidate in candidates:
        semantic  = cosineSimilarity(sourceEmb, candidateEmb)
        trajectory = trajectorySimDTW(userTraj.scores, candTraj.scores)
        temporal  = temporalDecay(candidate.created_at)
        diversity = diversityBonus(sourceMood, candMood, recommendedMoods)

        total = alpha*semantic + beta*trajectory
              + gamma*temporal + delta*diversity

        result.resonanceReason = generateResonanceReason(...)

    // 6. 按总分降序排序，截取top-K
    sort(results, by=totalScore, desc)
    return results[0..limit]
```

### 12.8 复杂度分析

| 操作 | 复杂度 | 说明 |
|-----|-------|------|
| DTW计算 | O(n * m) | n, m为两用户的情绪序列长度 |
| Embedding生成 | O(d) | d为embedding维度 |
| 余弦相似度 | O(d) | 向量点积 |
| 单次推荐 | O(C * (n*m + d)) | C为候选数量 |

代码位置：`backend/src/infrastructure/ai/EmotionResonanceEngine.cpp`，`backend/include/infrastructure/ai/EmotionResonanceEngine.h`

---

## 13. 节点健康监控

### 13.1 设计动机

边缘AI引擎运行在资源受限的环境中，需要实时监控各子系统的健康状态，以便在异常时触发降级策略或告警。监控系统采集CPU、内存、推理延迟等指标，并通过滑动窗口聚合提供趋势判断能力。

### 13.2 监控指标

EdgeAIEngine通过 `getHealthStatus()` 方法汇报以下指标：

| 指标 | 类型 | 含义 |
|-----|------|------|
| initialized | bool | 引擎是否完成初始化 |
| model_loaded | bool | 模型文件是否加载成功 |
| embedding_dimension | int | 当前embedding维度 |
| vocab_size | int | 词汇表大小 |
| hnsw_node_count | int | HNSW索引中的节点数量 |
| privacy_budget_used | float | 已消耗的差分隐私预算 |
| privacy_budget_max | float | 最大隐私预算 |
| emotion_pulse.window_size | int | 情绪脉搏窗口当前样本数 |
| emotion_pulse.current_avg | float | 当前窗口平均情绪分 |
| federated.total_rounds | int | 联邦学习已完成轮次 |
| federated.active_nodes | int | 活跃参与节点数 |

### 13.3 健康检查流程

```
function getHealthStatus():
    status = {}
    status.initialized = isInitialized()
    status.model_loaded = (embeddingDim > 0)
    status.embedding_dimension = embeddingDim
    status.vocab_size = vocabulary.size()

    // HNSW索引状态
    lock(hnswMutex)
    status.hnsw_node_count = hnswNodes.size()

    // 隐私预算状态
    status.privacy_budget_used = privacyBudgetUsed.load()
    status.privacy_budget_max = dpConfig.maxBudget

    // 情绪脉搏状态
    lock(pulseMutex)
    status.emotion_pulse.window_size = pulseWindow.size()
    status.emotion_pulse.current_avg = computeWindowAverage()

    // 联邦学习状态
    status.federated.total_rounds = federatedRounds
    status.federated.active_nodes = activeNodes.size()

    return status
```

### 13.4 降级策略

当监控检测到异常时，系统按以下优先级降级：

| 级别 | 条件 | 降级动作 |
|-----|------|---------|
| L1 | 模型未加载 | 跳过embedding生成，使用纯规则引擎 |
| L2 | HNSW索引为空 | 向量搜索退化为全量扫描 |
| L3 | 隐私预算耗尽 | 停止添加噪声，静默降级 |
| L4 | 熔断器OPEN | 快速失败，返回缓存结果 |

代码位置：`backend/src/infrastructure/ai/EdgeAIEngine.cpp`（getHealthStatus方法）

---

## 14. 云端同步协议

### 14.1 设计动机

边缘AI引擎在本地完成推理和学习后，需要将联邦学习的模型更新同步到云端进行聚合，同时从云端拉取全局模型更新。同步协议需要处理网络不稳定、断点续传、冲突解决等问题。

### 14.2 同步流程

```
边缘节点                              云端服务器
   |                                     |
   |  1. 上传本地模型更新(加噪声)         |
   |  ─────────────────────────────────> |
   |                                     |
   |                          2. FedAvg聚合
   |                                     |
   |  3. 下发全局模型更新                 |
   | <───────────────────────────────── |
   |                                     |
   |  4. 本地模型替换                     |
   |                                     |
```

### 14.3 同步策略

| 策略 | 说明 |
|-----|------|
| 增量同步 | 仅上传模型参数的差值（delta），减少带宽消耗 |
| 压缩传输 | 对梯度进行稀疏化和量化后传输 |
| 断点续传 | 记录同步进度，网络恢复后从断点继续 |
| 冲突解决 | 云端以FedAvg加权平均方式合并多节点更新 |

### 14.4 隐私保护

同步过程中的隐私保护措施：

- 上传前对模型更新添加Laplace噪声（见第7节）
- 传输层使用TLS加密
- 云端仅接收聚合后的模型参数，无法反推单个用户数据
- 差分隐私预算在本地严格管控，云端无法要求超额消耗

---

## 15. 未来演进方向

### 15.1 算法优化

| 方向 | 当前状态 | 演进目标 |
|-----|---------|---------|
| DTW加速 | O(n*m) 全矩阵 | FastDTW O(n) 近似算法 |
| HNSW | 内存索引 | 支持磁盘映射（mmap），突破内存限制 |
| 量化 | INT8对称 | INT4/混合精度量化 |
| 隐私 | Laplace机制 | Gaussian机制 + Renyi DP组合定理 |

### 15.2 架构演进

| 方向 | 说明 |
|-----|------|
| ONNX Runtime集成 | 替代手工推理，支持更多模型格式 |
| WebAssembly部署 | 将AI引擎编译为WASM，支持浏览器端推理 |
| 模型热更新 | 支持不停机更新模型文件 |
| 多模态扩展 | 支持图片情绪识别（表情、色彩分析） |

### 15.3 性能目标

| 指标 | 当前 | 目标 |
|-----|------|------|
| 单次情绪分析延迟 | < 10ms | < 5ms |
| Embedding生成 | < 50ms | < 20ms |
| HNSW搜索（10K节点） | < 5ms | < 2ms |
| 内存占用 | ~50MB | < 30MB |

---

## 附录A：系统架构总览

```
+─────────────────────────────────────────────────────────────────+
|                     HeartLake EdgeAI Engine                      |
+─────────────────────────────────────────────────────────────────+
|                                                                  |
|  +──────────────+  +──────────────+  +───────────────────────+  |
|  | 三层情感分析  |  | AC自动机     |  | 五因子心理风险评估    |  |
|  | (规则+词典    |  | 内容审核     |  | (selfHarmIntent      |  |
|  |  +统计融合)   |  | (4类模式)    |  |  +hopelessness+...)  |  |
|  +──────────────+  +──────────────+  +───────────────────────+  |
|                                                                  |
|  +──────────────+  +──────────────+  +───────────────────────+  |
|  | HNSW向量搜索 |  | INT8量化推理 |  | 情绪脉搏追踪          |  |
|  | (M=16,       |  | (对称量化    |  | (滑动窗口300s        |  |
|  |  多层图)     |  |  +4路展开)   |  |  +快照机制)          |  |
|  +──────────────+  +──────────────+  +───────────────────────+  |
|                                                                  |
|  +──────────────+  +──────────────+  +───────────────────────+  |
|  | Laplace      |  | FedAvg       |  | 情绪共鸣引擎          |  |
|  | 差分隐私     |  | 联邦学习     |  | (DTW+语义+时间+多样性)|  |
|  | (原子CAS)    |  | (加权聚合)   |  |                       |  |
|  +──────────────+  +──────────────+  +───────────────────────+  |
|                                                                  |
|  +──────────────────────────────────────────────────────────+   |
|  |              CircuitBreaker 熔断器保护层                  |   |
|  |         (CLOSED / OPEN / HALF_OPEN 三态自愈)             |   |
|  +──────────────────────────────────────────────────────────+   |
|                                                                  |
+─────────────────────────────────────────────────────────────────+
```

## 附录B：代码文件索引

| 文件路径 | 内容 |
|---------|------|
| `backend/include/infrastructure/ai/EdgeAIEngine.h` | 引擎头文件，所有结构体定义 |
| `backend/src/infrastructure/ai/EdgeAIEngine.cpp` | 引擎实现（8大子系统） |
| `backend/include/infrastructure/ai/EmotionResonanceEngine.h` | 共鸣引擎头文件 |
| `backend/src/infrastructure/ai/EmotionResonanceEngine.cpp` | 共鸣引擎实现（DTW、时间衰减、多样性） |
| `backend/include/infrastructure/ai/AdvancedEmbeddingEngine.h` | Embedding引擎头文件 |
| `backend/src/infrastructure/ai/AdvancedEmbeddingEngine.cpp` | Embedding生成与余弦相似度 |
| `backend/include/utils/CircuitBreaker.h` | 熔断器（header-only） |
| `backend/src/utils/PsychologicalRiskAssessment.cpp` | 五因子心理风险评估 |
| `backend/include/utils/PsychologicalRiskAssessment.h` | 风险评估头文件 |
| `backend/include/infrastructure/services/ResonanceSearchService.h` | 共鸣搜索服务 |

---

*本文档基于 HeartLake 项目实际代码编写，所有算法描述、参数值、复杂度分析均与源码一致。最后更新：2026-02-21*