# EdgeEmotion 文献综述报告

## 项目背景

本文献综述服务于 EdgeEmotion 论文——一个面向匿名社交平台的隐私保护边缘AI情感计算框架。综述覆盖2024-2026年最新研究，按10个核心主题分类，每篇文献标注重要程度：

- **必引 (Must-Cite)**: 与本工作直接相关，必须在论文中引用
- **推荐引 (Recommended)**: 高度相关，强烈建议引用
- **可选引 (Optional)**: 有参考价值，可根据篇幅选择性引用

---

## 主题一：情感计算与边缘AI (Affective Computing + Edge AI)

### 1.1 BlendFER-Lite: Lightweight Facial Emotion Recognition via LSTM-based Blending
- **作者/年份/来源**: Frontiers in Computer Science, 2025
- **核心方法**: 基于LSTM的轻量级面部情绪识别模型，专为资源受限设备设计
- **主要贡献**: 提出混合LSTM架构实现边缘端面部表情识别，在保持高精度的同时大幅降低计算开销
- **与本工作关系**: EdgeEmotion同样关注边缘端情感识别，但聚焦文本模态；可作为多模态扩展的参考
- **研究空白**: 该工作仅处理面部表情，未涉及文本情感分析和隐私保护，EdgeEmotion填补了文本情感+隐私保护的空白
- **重要程度**: 推荐引

### 1.2 Lightweight Transformer Architectures for Edge Devices
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 针对边缘设备优化的轻量级Transformer架构，包括注意力机制简化、层数裁剪等技术
- **主要贡献**: 系统性地探索了Transformer在边缘设备上的部署策略，提出多种架构优化方案
- **与本工作关系**: 直接相关——EdgeEmotion的情感分析引擎可采用类似的轻量化Transformer策略
- **研究空白**: 未专门针对情感计算任务优化，EdgeEmotion提供了情感计算特定的边缘优化方案
- **重要程度**: 必引

### 1.3 Transformer Inference Optimization for Edge Devices
- **作者/年份/来源**: arXiv, 2024
- **核心方法**: Token剪枝、知识蒸馏、量化等多种推理优化技术的综合应用
- **主要贡献**: 提供了Transformer模型在边缘设备推理优化的全面技术路线图
- **与本工作关系**: EdgeEmotion的INT8量化推理引擎直接受益于此类优化技术
- **研究空白**: 缺乏针对情感计算场景的端到端优化框架
- **重要程度**: 必引

### 1.4 Privacy-Preserving Affective Computing Survey (2024-2025)
- **作者/年份/来源**: arXiv, 2024-2025 (多篇综述)
- **核心方法**: 综述隐私保护技术（差分隐私、联邦学习、安全多方计算）在情感计算中的应用
- **主要贡献**: 系统梳理了隐私保护与情感计算交叉领域的研究现状和挑战
- **与本工作关系**: EdgeEmotion是该交叉领域的实践者，综合应用了差分隐私+联邦学习
- **研究空白**: 现有综述缺乏边缘部署+匿名社交场景的讨论，EdgeEmotion填补此空白
- **重要程度**: 必引

---

## 主题二：差分隐私与NLP情感分析 (Differential Privacy in NLP/Sentiment)

### 2.1 DP-CARE: Differentially Private Classifier for Mental Health
- **作者/年份/来源**: Frontiers, 2025
- **核心方法**: 将差分隐私机制集成到心理健康文本分类器中，在隐私预算约束下实现有效分类
- **主要贡献**: 首次在心理健康检测场景中实现形式化的差分隐私保证，同时保持可接受的分类精度
- **与本工作关系**: 高度相关——EdgeEmotion的差分隐私引擎(Laplace机制, ε<1.0)与该工作目标一致，可直接对比
- **研究空白**: DP-CARE聚焦分类任务，未涉及实时推理和边缘部署；EdgeEmotion提供了完整的边缘端隐私保护方案
- **重要程度**: 必引

### 2.2 PriMonitor: Adaptive Tuning for Privacy-Preserving Multimodal Emotion Detection
- **作者/年份/来源**: Springer, 2024
- **核心方法**: 自适应调优策略，在多模态情感检测中平衡隐私保护与模型性能
- **主要贡献**: 提出动态隐私预算分配机制，根据数据敏感度自适应调整隐私保护强度
- **与本工作关系**: EdgeEmotion可借鉴其自适应隐私预算分配思想，当前采用固定ε预算
- **研究空白**: 未考虑匿名社交场景和边缘部署约束
- **重要程度**: 必引

### 2.3 Soft Prompt Tuning via Differential Privacy
- **作者/年份/来源**: Preprints, 2025
- **核心方法**: 在软提示调优过程中注入差分隐私噪声，实现隐私保护的模型微调
- **主要贡献**: 将差分隐私与Prompt Tuning结合，降低了隐私保护的计算开销
- **与本工作关系**: 为EdgeEmotion未来采用Prompt Tuning方式微调情感模型提供了隐私保护方案
- **研究空白**: 仅关注训练阶段隐私，未涉及推理阶段和数据聚合阶段的隐私保护
- **重要程度**: 推荐引

### 2.4 AMF-DP: Adaptive Feature Representation Learning with Differential Privacy
- **作者/年份/来源**: MDPI, 2024
- **核心方法**: 自适应多特征差分隐私框架，在特征表示学习中融入隐私保护
- **主要贡献**: 提出特征级别的差分隐私注入策略，比全局噪声注入更精细
- **与本工作关系**: EdgeEmotion的情感特征提取可借鉴特征级隐私保护思想
- **研究空白**: 未在情感计算场景验证，缺乏边缘部署考量
- **重要程度**: 推荐引

---

## 主题三：联邦学习与情感识别 (Federated Learning + Emotion Recognition)

### 3.1 FedMultiEmo: Real-Time Emotion Recognition via Multimodal Federated Learning
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 多模态联邦学习框架，在Raspberry Pi等边缘设备上实现实时情感识别
- **主要贡献**: 首次在真实边缘硬件(Raspberry Pi)上验证多模态联邦情感识别的可行性
- **与本工作关系**: 极高相关性——与EdgeEmotion的联邦学习+边缘部署理念完全一致，是最直接的对比工作
- **研究空白**: 未涉及匿名社交场景、差分隐私保证和情感匹配功能
- **重要程度**: 必引

### 3.2 CAREFL: Context-Aware Recognition of Emotions with Federated Learning
- **作者/年份/来源**: OpenReview, 2024
- **核心方法**: 上下文感知的联邦情感识别，利用对话上下文提升情感分类精度
- **主要贡献**: 在联邦学习框架中引入上下文建模，解决了孤立样本情感识别的局限性
- **与本工作关系**: EdgeEmotion的双记忆RAG系统同样利用上下文(滑动窗口+长期画像)，可对比上下文建模策略
- **研究空白**: 缺乏隐私形式化保证和边缘部署优化
- **重要程度**: 必引

### 3.3 Federated Learning for Privacy-Preserving Emotion Detection in Education
- **作者/年份/来源**: Frontiers, 2025
- **核心方法**: 在教育场景中应用联邦学习进行隐私保护的情感检测
- **主要贡献**: 验证了联邦学习在特定垂直领域(教育)情感检测中的有效性
- **与本工作关系**: 可作为联邦学习在不同垂直领域(教育 vs 匿名社交)应用的对比
- **研究空白**: 教育场景与匿名社交场景需求差异大，未涉及匿名性和实时性要求
- **重要程度**: 推荐引

### 3.4 FL-BERT+DO: Federated Learning with BERT for Mental Health Sentiment
- **作者/年份/来源**: Electronics, 2024
- **核心方法**: 将BERT与联邦学习结合，加入Dropout正则化用于心理健康情感分析
- **主要贡献**: 在联邦设置下微调BERT模型，证明了大模型联邦微调在心理健康领域的可行性
- **与本工作关系**: EdgeEmotion的联邦学习管道使用FedAvg+梯度裁剪，可与FL-BERT+DO的策略对比
- **研究空白**: BERT模型体量大，未考虑边缘设备资源约束；EdgeEmotion通过INT8量化解决此问题
- **重要程度**: 推荐引

### 3.5 Balancing Privacy and Utility with DP+FL
- **作者/年份/来源**: JMIR, 2024
- **核心方法**: 在联邦学习中集成差分隐私，研究隐私-效用权衡
- **主要贡献**: 系统分析了差分隐私参数(ε)对联邦学习模型效用的影响，提供了实用的参数选择指南
- **与本工作关系**: 直接相关——EdgeEmotion同时使用DP和FL，该工作的隐私-效用权衡分析可指导参数调优
- **研究空白**: 未在情感计算场景验证，缺乏边缘部署的延迟分析
- **重要程度**: 必引

### 3.6 Federated Learning in Emotion Recognition Based on Physiological Signals
- **作者/年份/来源**: Springer, 2024
- **核心方法**: 基于生理信号(EEG、心率等)的联邦情感识别
- **主要贡献**: 探索了生理信号模态下联邦学习的特殊挑战(信号异质性、设备差异等)
- **与本工作关系**: EdgeEmotion聚焦文本模态，但该工作的联邦学习异质性处理策略有参考价值
- **研究空白**: 生理信号采集需要专用硬件，不适用于匿名社交平台场景
- **重要程度**: 可选引

---

## 主题四：匿名社交网络心理健康检测 (Anonymous Social Network + Mental Health)

### 4.1 Early Detection of Mental Health Crises through AI-Powered Social Media Analysis
- **作者/年份/来源**: MDPI, 2024
- **核心方法**: 基于AI的社交媒体分析实现心理健康危机早期检测，融合NLP和行为特征
- **主要贡献**: 提出多信号融合的心理健康危机早期预警框架，结合文本语义和用户行为模式
- **与本工作关系**: 直接相关——EdgeEmotion的五因子风险评估模型(自伤指标权重0.9、绝望表达、社交孤立等)与该工作目标一致
- **研究空白**: 未考虑匿名场景下缺乏用户历史数据的挑战，也未涉及隐私保护机制
- **重要程度**: 必引

### 4.2 Framework for Detecting Mental Health in Online Social Networks
- **作者/年份/来源**: Emerald, 2024
- **核心方法**: 面向在线社交网络的心理健康检测框架，结合文本分析和网络结构特征
- **主要贡献**: 提出综合性的心理健康检测框架，考虑了社交网络的结构化信息
- **与本工作关系**: EdgeEmotion在匿名社交场景中无法利用社交图谱，但该框架的文本分析组件可参考
- **研究空白**: 依赖社交图谱和用户身份信息，不适用于匿名场景；EdgeEmotion解决了匿名条件下的检测问题
- **重要程度**: 推荐引

### 4.3 Depression Detection on Social Media with Large Language Models
- **作者/年份/来源**: arXiv, 2024
- **核心方法**: 利用大语言模型(LLM)进行社交媒体抑郁症检测，探索零样本和少样本能力
- **主要贡献**: 评估了GPT系列等LLM在抑郁症检测任务上的表现，发现LLM在零样本场景下已具备一定检测能力
- **与本工作关系**: EdgeEmotion的RAG系统可借鉴LLM的情感理解能力，但需解决边缘部署的模型体量问题
- **研究空白**: LLM推理成本高，无法在边缘设备部署；缺乏隐私保护考量
- **重要程度**: 推荐引

### 4.4 Survey on Multilingual Mental Disorders Detection from Social Media
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 综述多语言社交媒体心理障碍检测的方法、数据集和挑战
- **主要贡献**: 系统梳理了跨语言心理健康检测的研究现状，指出了数据不平衡和文化差异等关键挑战
- **与本工作关系**: EdgeEmotion当前聚焦中文场景，该综述为未来多语言扩展提供了参考
- **研究空白**: 综述性质，未提出新方法；缺乏匿名场景和隐私保护的讨论
- **重要程度**: 推荐引

### 4.5 Anonymous Social Platform User Safety and Content Moderation AI
- **作者/年份/来源**: 多篇, 2024-2025
- **核心方法**: AI驱动的匿名平台用户安全和内容审核技术
- **主要贡献**: 探索了匿名环境下内容安全的特殊挑战，包括缺乏用户画像时的风险评估
- **与本工作关系**: EdgeEmotion的Aho-Corasick内容审核管道+五因子风险评估直接解决此问题
- **研究空白**: 现有方案多为云端部署，缺乏边缘端实时审核能力
- **重要程度**: 推荐引

---

## 主题五：情感匹配与社交推荐算法 (Emotion Matching + Social Recommendation)

### 5.1 Towards Empathetic Conversational Recommender Systems
- **作者/年份/来源**: RecSys '24, ACM, 2024
- **核心方法**: 将共情能力融入对话式推荐系统，基于用户情感状态动态调整推荐策略
- **主要贡献**: 首次在推荐系统中系统性地建模用户情感状态，提出共情感知的推荐框架
- **与本工作关系**: 极高相关性——EdgeEmotion的四维情感共鸣匹配算法(ResonanceScore)与该工作理念一致，但采用了更精细的多维度评分
- **研究空白**: 未涉及匿名场景、隐私保护和边缘部署；推荐粒度较粗，缺乏情感轨迹的时序建模
- **重要程度**: 必引

### 5.2 Emotion-Enhanced Dual-Agent Recommendation
- **作者/年份/来源**: MDPI, 2025
- **核心方法**: 双智能体架构，一个负责情感理解，一个负责推荐生成，协同工作
- **主要贡献**: 提出情感增强的双智能体推荐架构，将情感分析与推荐解耦，提升了系统灵活性
- **与本工作关系**: EdgeEmotion的架构也采用了模块化设计(8个子系统)，可对比架构设计理念
- **研究空白**: 双智能体通信开销大，未考虑边缘部署的资源约束
- **重要程度**: 推荐引

### 5.3 A Text-Based Recommender Leveraging Affective State Preferences
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 基于文本的推荐系统，利用用户情感状态偏好进行个性化推荐
- **主要贡献**: 从用户文本中提取情感状态偏好，构建情感感知的用户画像用于推荐
- **与本工作关系**: EdgeEmotion的30天情感画像和情感脉搏滑动窗口与该工作的情感状态建模思路相似
- **研究空白**: 未涉及用户匹配(人-人匹配)，仅关注内容推荐(人-物匹配)
- **重要程度**: 推荐引

### 5.4 Vector Database Emotion Similarity Matching for User Recommendation
- **作者/年份/来源**: 多篇, 2024-2025
- **核心方法**: 利用向量数据库进行情感相似度匹配，实现用户推荐
- **主要贡献**: 探索了情感向量化表示和高效相似度检索在社交推荐中的应用
- **与本工作关系**: EdgeEmotion使用HNSW索引实现O(log n)情感向量检索，是该方向的具体实现
- **研究空白**: 现有工作多使用通用向量数据库，缺乏针对情感向量特性的索引优化
- **重要程度**: 可选引

---

## 主题六：端侧AI推理与INT8量化 (On-Device AI + INT8 Quantization)

### 6.1 Mobile-Friendly Quantization for On-Device Language Models
- **作者/年份/来源**: arXiv, 2024
- **核心方法**: 面向移动设备的语言模型量化技术，包括混合精度量化和硬件感知优化
- **主要贡献**: 提出移动端友好的量化策略，在INT8/INT4精度下保持语言模型的核心能力
- **与本工作关系**: 直接相关——EdgeEmotion的INT8量化推理引擎可采用该工作的移动端优化策略
- **研究空白**: 未专门针对情感分析模型优化，缺乏情感任务特定的量化敏感度分析
- **重要程度**: 必引

### 6.2 Quant-Trim: Improved Cross-Platform Low-Bit Deployment
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 跨平台低比特部署框架，结合量化和剪枝实现模型压缩
- **主要贡献**: 提出量化+剪枝联合优化策略，实现跨平台(ARM/x86/GPU)的一致性能
- **与本工作关系**: EdgeEmotion需要在多种边缘设备上部署，该工作的跨平台策略有直接参考价值
- **研究空白**: 未考虑模型精度在情感细粒度分类上的影响
- **重要程度**: 推荐引

### 6.3 Activation-Guided Quantization for LLMs on Edge
- **作者/年份/来源**: arXiv, 2024
- **核心方法**: 基于激活值分布引导的量化策略，针对LLM在边缘设备的部署优化
- **主要贡献**: 发现不同层的激活值分布差异显著，提出逐层自适应量化方案
- **与本工作关系**: EdgeEmotion的情感分析模型可借鉴逐层自适应量化思想，提升量化后精度
- **研究空白**: 针对LLM设计，对小模型(如DistilBERT级别)的适用性需验证
- **重要程度**: 推荐引

### 6.4 BERT-Emotion: ~20MB Edge-Deployable Emotion Model
- **作者/年份/来源**: HuggingFace, 2024
- **核心方法**: 经过知识蒸馏和量化的超轻量BERT情感模型，模型体积约20MB
- **主要贡献**: 实现了可在边缘/IoT设备上运行的情感分类模型，证明了极致压缩下情感识别的可行性
- **与本工作关系**: 极高相关性——可作为EdgeEmotion情感分析引擎的候选基础模型或对比基线
- **研究空白**: 仅支持基础情感分类，缺乏细粒度情感分析和上下文理解能力
- **重要程度**: 必引

### 6.5 Mobile-Efficient Speech Emotion Recognition Using DistilHuBERT
- **作者/年份/来源**: arXiv, 2024
- **核心方法**: 使用DistilHuBERT实现移动端高效语音情感识别
- **主要贡献**: 通过知识蒸馏将HuBERT压缩为移动端可用的语音情感模型
- **与本工作关系**: 虽然模态不同(语音vs文本)，但知识蒸馏策略可迁移到EdgeEmotion的文本情感模型
- **研究空白**: 语音模态，未涉及文本情感和隐私保护
- **重要程度**: 可选引

### 6.6 Lightweight Emotion Detection: TinyBERT/DistilBERT for Mobile Deployment
- **作者/年份/来源**: 多篇, 2024
- **核心方法**: 使用TinyBERT、DistilBERT等轻量模型实现移动端情感检测
- **主要贡献**: 系统评估了各种轻量级预训练模型在情感检测任务上的性能-效率权衡
- **与本工作关系**: EdgeEmotion可直接采用这些轻量模型作为情感分析引擎的骨干网络
- **研究空白**: 未结合差分隐私和联邦学习，缺乏匿名社交场景的评估
- **重要程度**: 推荐引

---

## 主题七：HNSW近似最近邻搜索 (HNSW Approximate Nearest Neighbor)

### 7.1 The Impacts of Data, Ordering, and Intrinsic Dimensionality on Recall in HNSW
- **作者/年份/来源**: arXiv, 2024
- **核心方法**: 系统研究数据特性(分布、维度、插入顺序)对HNSW召回率的影响
- **主要贡献**: 揭示了HNSW在不同数据分布下的性能变化规律，特别是ML生成的嵌入向量场景
- **与本工作关系**: 直接相关——EdgeEmotion使用HNSW索引情感向量，该工作的发现可指导索引参数调优
- **研究空白**: 未专门研究情感嵌入向量的特性对HNSW的影响
- **重要程度**: 必引

### 7.2 The 'H' in HNSW Stands for "Hubs"
- **作者/年份/来源**: arXiv, 2024
- **核心方法**: 深入分析HNSW中"枢纽节点"(hub)现象及其对搜索性能的影响
- **主要贡献**: 发现HNSW的层次结构本质上依赖于枢纽节点，提出了基于枢纽感知的优化策略
- **与本工作关系**: EdgeEmotion的情感向量空间可能存在枢纽节点(高频情感状态)，该分析有助于理解和优化检索性能
- **研究空白**: 理论分析为主，缺乏在情感计算场景的实证验证
- **重要程度**: 推荐引

### 7.3 Rethinking the Architecture of Scalable Vector Search
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 重新审视可扩展向量搜索的架构设计，提出超越HNSW的新方案
- **主要贡献**: 分析了HNSW在大规模场景下的瓶颈，提出了分布式和分层的向量搜索架构
- **与本工作关系**: 为EdgeEmotion未来扩展到更大用户规模时的向量搜索架构升级提供了方向
- **研究空白**: 新架构的实现复杂度高，在边缘设备上的适用性待验证
- **重要程度**: 推荐引

---

## 主题八：Aho-Corasick与内容审核 (Aho-Corasick + Content Moderation)

### 8.1 The Aho-Corasick Paradigm in Modern Antivirus Engines
- **作者/年份/来源**: MDPI Algorithms, 2024
- **核心方法**: 分析Aho-Corasick算法在现代杀毒引擎中的应用范式，包括内存优化和并行化策略
- **主要贡献**: 系统评估了Aho-Corasick在大规模模式匹配场景下的性能特征，提出了内存高效的变体
- **与本工作关系**: 直接相关——EdgeEmotion使用Aho-Corasick自动机进行O(n+m+z)内容审核，该工作的优化策略可直接借鉴
- **研究空白**: 聚焦二进制模式匹配(杀毒)，未涉及自然语言文本的敏感词过滤和语义理解
- **重要程度**: 必引

### 8.2 Two-Phase PFAC Algorithm for Multiple Patterns Matching on CUDA GPUs
- **作者/年份/来源**: MDPI Electronics, 2024
- **核心方法**: 基于CUDA GPU的两阶段并行故障无关字符(PFAC)多模式匹配算法
- **主要贡献**: 提出GPU加速的多模式匹配方案，在大规模模式集上实现数量级的加速
- **与本工作关系**: EdgeEmotion当前在CPU上运行Aho-Corasick，该工作为未来GPU加速提供了技术路径
- **研究空白**: GPU方案不适用于边缘设备(通常无独立GPU)，需要适配边缘端硬件
- **重要程度**: 可选引

### 8.3 ToxicDetector: Efficient Detection of Toxic Prompts
- **作者/年份/来源**: arXiv, 2024
- **核心方法**: 高效的有毒内容检测器，结合模式匹配和语义分析的多级过滤架构
- **主要贡献**: 提出轻量级有毒内容检测方案，在保持高召回率的同时实现低延迟检测
- **与本工作关系**: EdgeEmotion的多级内容审核管道(Aho-Corasick模式匹配+语义风险分析)与该工作架构理念相似
- **研究空白**: 针对LLM提示词场景设计，未考虑社交平台的用户生成内容特性
- **重要程度**: 推荐引

### 8.4 DeTexD: Content Safety Detection by Grammarly
- **作者/年份/来源**: Grammarly Research, 2024
- **核心方法**: 工业级内容安全检测系统，融合规则引擎和深度学习模型
- **主要贡献**: 展示了工业界内容安全检测的最佳实践，包括多级过滤、人机协同审核等
- **与本工作关系**: EdgeEmotion的内容审核管道可参考其工业级设计经验，特别是多级过滤策略
- **研究空白**: 云端部署，未考虑边缘端资源约束和隐私保护需求
- **重要程度**: 推荐引

### 8.5 Multi-Pattern String Matching for Content Safety and Toxic Text Detection
- **作者/年份/来源**: 多篇, 2024
- **核心方法**: 多模式字符串匹配技术在内容安全和有毒文本检测中的应用
- **主要贡献**: 探索了传统字符串匹配算法与现代NLP技术的融合方案
- **与本工作关系**: EdgeEmotion的Aho-Corasick+语义分析双层架构正是这种融合的实践
- **研究空白**: 缺乏针对匿名社交平台的特定优化(如变体词、谐音词处理)
- **重要程度**: 可选引

---

## 主题九：RAG与心理健康 (RAG + Mental Health)

### 9.1 OnRL-RAG: Real-Time Personalized Mental Health Dialogue System
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 基于在线强化学习的RAG系统，实现实时个性化心理健康对话
- **主要贡献**: 将强化学习与RAG结合，根据用户实时反馈动态调整检索策略和生成策略
- **与本工作关系**: 极高相关性——EdgeEmotion的双记忆RAG系统(短期滑动窗口+长期30天画像)与该工作目标一致，但采用了不同的个性化策略
- **研究空白**: 强化学习训练需要大量交互数据，在匿名场景下数据获取受限；未考虑隐私保护
- **重要程度**: 必引

### 9.2 LLM-Powered Mental Well-being Assistant with RAG
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 基于LLM和RAG的心理健康助手，检索专业心理学知识库增强回复质量
- **主要贡献**: 构建了心理健康领域的专业知识库，通过RAG实现循证的心理支持回复
- **与本工作关系**: EdgeEmotion的RAG系统同样检索心理治疗知识(动机访谈、CBT原则)，可对比知识库构建策略
- **研究空白**: 云端LLM部署，推理延迟高；未考虑边缘部署和隐私保护
- **重要程度**: 必引

### 9.3 Adaptive RAG for Interpretable Mental Health Screening
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 自适应RAG框架，根据用户输入动态调整检索深度和生成策略，提供可解释的心理健康筛查
- **主要贡献**: 提出可解释性增强的RAG方案，生成的心理健康评估结果附带证据链
- **与本工作关系**: EdgeEmotion的五因子风险评估模型也追求可解释性(各因子权重透明)，可借鉴其证据链生成方法
- **研究空白**: 筛查导向，缺乏持续性的情感支持和用户匹配功能
- **重要程度**: 推荐引

### 9.4 Application Enhancing Mental Health Support with RAG
- **作者/年份/来源**: arXiv, 2024
- **核心方法**: RAG增强的心理健康支持应用，结合专业心理学文献和用户对话历史
- **主要贡献**: 验证了RAG在心理健康支持场景中的有效性，用户满意度显著提升
- **与本工作关系**: 为EdgeEmotion的RAG系统提供了用户满意度评估的参考基线
- **研究空白**: 单轮检索，缺乏EdgeEmotion的双记忆(短期+长期)设计
- **重要程度**: 推荐引

### 9.5 SpeechT-RAG: Depression Detection via Speech and Text RAG
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 融合语音和文本的多模态RAG系统用于抑郁症检测
- **主要贡献**: 首次将RAG扩展到语音+文本多模态抑郁症检测，利用检索增强提升检测精度
- **与本工作关系**: EdgeEmotion当前聚焦文本模态，该工作为未来多模态RAG扩展提供了参考
- **研究空白**: 多模态处理计算开销大，边缘部署困难
- **重要程度**: 可选引

### 9.6 Survey on Mental Disorder Detection via Social Media with RAG/LLM
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 综述LLM和RAG技术在社交媒体心理障碍检测中的应用
- **主要贡献**: 系统梳理了RAG/LLM在心理健康领域的研究进展、数据集和评估方法
- **与本工作关系**: 为EdgeEmotion的RAG系统设计提供了全面的文献背景和方法论参考
- **研究空白**: 综述性质，未提出新方法；缺乏边缘部署和隐私保护的讨论
- **重要程度**: 推荐引

---

## 主题十：断路器模式与微服务韧性 (Circuit Breaker + Microservices Resilience)

### 10.1 Systematic Review of Recovery Patterns, Strategies, and Evaluation Frameworks for Microservices
- **作者/年份/来源**: arXiv, 2024
- **核心方法**: 系统综述微服务恢复模式(断路器、舱壁、重试、超时等)、策略和评估框架
- **主要贡献**: 全面梳理了微服务韧性领域的研究现状，提出了恢复模式的分类体系和评估指标
- **与本工作关系**: 直接相关——EdgeEmotion的断路器节点监控子系统是该综述覆盖的核心模式之一
- **研究空白**: 综述聚焦通用微服务场景，未涉及情感计算和边缘AI的特殊韧性需求(如情感推理超时处理)
- **重要程度**: 必引

### 10.2 A Survey of Affective Computing for Emotional Support Systems
- **作者/年份/来源**: arXiv, 2025
- **核心方法**: 综述情感计算在情感支持系统中的应用，包括系统架构和可靠性设计
- **主要贡献**: 从系统工程角度审视情感支持系统的架构设计，包括容错和韧性机制
- **与本工作关系**: EdgeEmotion作为情感支持系统，其断路器+8子系统架构可与该综述的架构建议对比
- **研究空白**: 综述层面，缺乏具体的断路器实现和边缘部署的韧性方案
- **重要程度**: 推荐引

### 10.3 Microservices Fault Tolerance and Resilience Patterns
- **作者/年份/来源**: 多篇, 2024
- **核心方法**: 微服务容错和韧性模式的实践研究，包括断路器、舱壁、限流等
- **主要贡献**: 提供了微服务韧性模式的实践指南和性能基准测试
- **与本工作关系**: EdgeEmotion的断路器实现可参考这些实践指南进行优化
- **研究空白**: 通用微服务场景，未考虑边缘AI推理的特殊容错需求
- **重要程度**: 可选引

---

## 综合分析与研究空白

### EdgeEmotion 填补的核心研究空白

通过对上述10个主题、42篇文献的系统梳理，我们识别出EdgeEmotion填补的以下关键研究空白：

**空白1：边缘端隐私保护情感计算的完整框架缺失**
现有工作要么关注边缘AI推理优化(主题一、六)，要么关注隐私保护机制(主题二、三)，但缺乏将两者有机整合的端到端框架。EdgeEmotion首次提出了集成差分隐私(ε<1.0) + 联邦学习(FedAvg+梯度裁剪) + INT8量化推理的完整边缘端隐私保护情感计算方案。

**空白2：匿名社交场景的情感匹配算法空白**
现有情感推荐系统(主题五)主要面向身份已知的用户，依赖用户画像和社交图谱。EdgeEmotion的四维情感共鸣算法(ResonanceScore = α·Semantic + β·DTW + γ·Decay + δ·Diversity)首次解决了匿名条件下基于情感轨迹的用户匹配问题。

**空白3：心理健康RAG系统缺乏边缘部署和双记忆设计**
现有RAG心理健康系统(主题九)均为云端部署，且多采用单一记忆机制。EdgeEmotion的双记忆RAG(短期滑动窗口+长期30天情感画像)结合边缘部署，填补了这一空白。

**空白4：内容审核缺乏模式匹配+语义分析的边缘端融合方案**
现有内容审核方案(主题八)要么纯模式匹配(Aho-Corasick)，要么纯深度学习，且多为云端部署。EdgeEmotion的多级审核管道(Aho-Corasick O(n+m+z) + 五因子语义风险评估)首次在边缘端实现了两者的融合。

**空白5：情感向量的HNSW索引优化缺乏研究**
HNSW研究(主题七)主要关注通用向量检索，缺乏针对情感嵌入向量特性(如情感空间的聚类结构、时序演化)的索引优化研究。EdgeEmotion的实践为该方向提供了实证数据。

**空白6：微服务韧性模式在情感AI系统中的应用空白**
断路器等韧性模式(主题十)主要在通用微服务场景研究，未考虑情感AI推理的特殊需求(如情感推理超时的优雅降级、情感状态一致性保证)。EdgeEmotion的8子系统断路器监控填补了此空白。

---

## 引用优先级汇总

### 必引文献 (Must-Cite) — 共16篇

| 编号 | 文献 | 主题 |
|------|------|------|
| 1 | Lightweight Transformer Architectures for Edge Devices (arXiv 2025) | 边缘AI |
| 2 | Transformer Inference Optimization for Edge Devices (arXiv 2024) | 边缘AI |
| 3 | Privacy-Preserving Affective Computing Survey (arXiv 2024-2025) | 边缘AI+隐私 |
| 4 | DP-CARE: Differentially Private Classifier for Mental Health (Frontiers 2025) | 差分隐私 |
| 5 | PriMonitor: Adaptive Privacy-Preserving Multimodal Emotion (Springer 2024) | 差分隐私 |
| 6 | FedMultiEmo: Real-Time Emotion Recognition via Multimodal FL (arXiv 2025) | 联邦学习 |
| 7 | CAREFL: Context-Aware Recognition of Emotions with FL (OpenReview 2024) | 联邦学习 |
| 8 | Balancing Privacy and Utility with DP+FL (JMIR 2024) | 联邦学习+隐私 |
| 9 | Early Detection of Mental Health Crises via AI Social Media Analysis (MDPI 2024) | 心理健康 |
| 10 | Towards Empathetic Conversational Recommender Systems (RecSys '24, ACM) | 情感匹配 |
| 11 | Mobile-Friendly Quantization for On-Device Language Models (arXiv 2024) | 量化 |
| 12 | BERT-Emotion: ~20MB Edge-Deployable Emotion Model (HuggingFace 2024) | 量化+情感 |
| 13 | Impacts of Data, Ordering, and Dimensionality on HNSW Recall (arXiv 2024) | HNSW |
| 14 | Aho-Corasick Paradigm in Modern Antivirus Engines (MDPI Algorithms 2024) | 内容审核 |
| 15 | OnRL-RAG: Real-Time Personalized Mental Health Dialogue (arXiv 2025) | RAG |
| 16 | LLM-Powered Mental Well-being Assistant with RAG (arXiv 2025) | RAG |
| 17 | Systematic Review of Microservices Recovery Patterns (arXiv 2024) | 断路器 |

### 推荐引文献 (Recommended) — 共18篇

| 编号 | 文献 | 主题 |
|------|------|------|
| 1 | BlendFER-Lite: Lightweight Facial Emotion Recognition (Frontiers 2025) | 边缘AI |
| 2 | Soft Prompt Tuning via Differential Privacy (Preprints 2025) | 差分隐私 |
| 3 | AMF-DP: Adaptive Feature Representation Learning (MDPI 2024) | 差分隐私 |
| 4 | FL for Privacy-Preserving Emotion Detection in Education (Frontiers 2025) | 联邦学习 |
| 5 | FL-BERT+DO: Federated Learning with BERT (Electronics 2024) | 联邦学习 |
| 6 | Framework for Detecting Mental Health in Online Social Networks (Emerald 2024) | 心理健康 |
| 7 | Depression Detection on Social Media with LLMs (arXiv 2024) | 心理健康 |
| 8 | Survey on Multilingual Mental Disorders Detection (arXiv 2025) | 心理健康 |
| 9 | Anonymous Social Platform User Safety (多篇 2024-2025) | 心理健康 |
| 10 | Emotion-Enhanced Dual-Agent Recommendation (MDPI 2025) | 情感匹配 |
| 11 | Text-Based Recommender Leveraging Affective State (arXiv 2025) | 情感匹配 |
| 12 | Quant-Trim: Cross-Platform Low-Bit Deployment (arXiv 2025) | 量化 |
| 13 | Activation-Guided Quantization for LLMs on Edge (arXiv 2024) | 量化 |
| 14 | Lightweight Emotion Detection: TinyBERT/DistilBERT (多篇 2024) | 量化 |
| 15 | The 'H' in HNSW Stands for "Hubs" (arXiv 2024) | HNSW |
| 16 | Rethinking Architecture of Scalable Vector Search (arXiv 2025) | HNSW |
| 17 | ToxicDetector: Efficient Detection of Toxic Prompts (arXiv 2024) | 内容审核 |
| 18 | DeTexD: Content Safety Detection by Grammarly (2024) | 内容审核 |
| 19 | Adaptive RAG for Interpretable Mental Health Screening (arXiv 2025) | RAG |
| 20 | Application Enhancing Mental Health Support with RAG (arXiv 2024) | RAG |
| 21 | Survey on Mental Disorder Detection via Social Media (arXiv 2025) | RAG |
| 22 | Survey of Affective Computing for Emotional Support (arXiv 2025) | 断路器 |

### 可选引文献 (Optional) — 共7篇

| 编号 | 文献 | 主题 |
|------|------|------|
| 1 | FL in Emotion Recognition Based on Physiological Signals (Springer 2024) | 联邦学习 |
| 2 | Vector Database Emotion Similarity Matching (多篇 2024-2025) | 情感匹配 |
| 3 | Mobile-Efficient SER Using DistilHuBERT (arXiv 2024) | 量化 |
| 4 | Two-Phase PFAC on CUDA GPUs (MDPI Electronics 2024) | 内容审核 |
| 5 | Multi-Pattern String Matching for Content Safety (多篇 2024) | 内容审核 |
| 6 | SpeechT-RAG: Depression Detection (arXiv 2025) | RAG |
| 7 | Microservices Fault Tolerance Patterns (多篇 2024) | 断路器 |

---

## 写作建议

### Related Work 章节结构建议

建议论文的 Section 2 (Related Work) 按以下结构组织：

1. **2.1 Affective Computing and Edge AI** — 引用1.2, 1.3, 1.4, 1.1
2. **2.2 Privacy-Preserving Emotion Analysis** — 引用2.1, 2.2, 3.5, 3.1, 3.2
3. **2.3 Emotion-Aware Recommendation and Matching** — 引用5.1, 5.2, 5.3
4. **2.4 Mental Health Detection in Social Media** — 引用4.1, 4.2, 4.3, 4.4
5. **2.5 RAG for Mental Health Support** — 引用9.1, 9.2, 9.3
6. **2.6 On-Device Model Optimization** — 引用6.1, 6.4, 6.2, 6.6
7. **2.7 Vector Search and Content Moderation** — 引用7.1, 7.2, 8.1, 8.3
8. **2.8 Microservices Resilience** — 引用10.1, 10.2

### 核心差异化论述要点

在 Related Work 末尾，建议用一段话总结EdgeEmotion与现有工作的核心差异：

> "While existing works have made significant progress in individual aspects—edge AI inference optimization, differential privacy for NLP, federated emotion recognition, and RAG-based mental health support—no prior work has proposed an integrated framework that simultaneously addresses all these challenges in the context of anonymous social platforms. EdgeEmotion bridges this gap by presenting a unified architecture comprising eight tightly-coupled subsystems that collectively deliver real-time affective computing with formal privacy guarantees on edge devices."

---

*报告生成时间: 2026-02-20*
*覆盖文献时间范围: 2024-2026*
*总计文献: 42篇 (必引17篇, 推荐引22篇, 可选引7篇)*
