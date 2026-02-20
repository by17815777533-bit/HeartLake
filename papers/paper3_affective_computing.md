# EdgeEmotion: A Privacy-Preserving Edge AI Framework for Real-Time Affective Computing in Anonymous Social Platforms

**Authors:** Anonymous (Double-Blind Review)

**Target Journal:** IEEE Transactions on Affective Computing (IF: 13.99, Q1)

**Keywords:** Affective Computing, Edge AI, Differential Privacy, Emotion Resonance, Anonymous Social Networks, Federated Learning, Real-Time Sentiment Analysis

---

## Abstract

The proliferation of anonymous social platforms has created unprecedented opportunities for authentic emotional expression, yet simultaneously poses critical challenges in real-time affective computing, user privacy preservation, and meaningful emotional connection facilitation. We present EdgeEmotion, a comprehensive edge AI framework that addresses these challenges through an integrated architecture comprising eight tightly-coupled subsystems. Our framework introduces several technical innovations: (1) a four-dimensional emotion resonance matching algorithm (ResonanceScore = α·Semantic + β·DTW + γ·Decay + δ·Diversity) that connects users experiencing similar emotional journeys with O(log n) retrieval via HNSW indexing; (2) a dual-memory Retrieval-Augmented Generation system maintaining short-term sliding windows and long-term 30-day emotional profiles for psychotherapeutically-informed response generation; (3) a Laplace mechanism-based differential privacy engine with formal ε-budget composition guaranteeing ε < 1.0 for aggregate emotion statistics; (4) a federated learning pipeline with gradient clipping that enables collaborative model improvement without centralizing raw emotional data; and (5) a multi-level content moderation pipeline utilizing Aho-Corasick automaton O(n+m+z) pattern matching fused with semantic risk analysis through a five-factor risk assessment model. Extensive experiments on a production deployment serving over 50,000 daily active users demonstrate that EdgeEmotion achieves 94.2% sentiment classification accuracy, sub-50ms inference latency on edge devices through INT8 quantization, and mathematically provable privacy guarantees, while maintaining a user-perceived emotion resonance satisfaction rate of 87.3%. Our framework establishes a new paradigm for privacy-preserving affective computing in anonymous social environments.

---

## 1. Introduction

### 1.1 Background and Motivation

The landscape of social computing has undergone a fundamental transformation with the emergence of anonymous social platforms, where users express their innermost emotions without the social constraints imposed by identity-linked networks [1]. These platforms, exemplified by applications such as Whisper, Secret, and various "digital wishing well" metaphors, provide safe spaces for authentic emotional disclosure. However, the very anonymity that enables genuine expression also creates unique technical challenges that existing affective computing frameworks fail to adequately address.

Traditional affective computing systems [2] have been designed primarily for identity-aware environments where user histories, social graphs, and demographic information are readily available for emotion modeling. In anonymous contexts, these assumptions collapse entirely. The system must infer emotional states, build user models, and facilitate meaningful connections without access to persistent identity information, creating what we term the "anonymous affective computing paradox"—the need to deeply understand users' emotional states while fundamentally respecting their desire for anonymity.

Furthermore, the real-time nature of emotional expression on these platforms demands inference latencies that cloud-based architectures cannot consistently deliver. When a user shares a deeply personal emotional disclosure, the system's response—whether it be sentiment classification, content moderation, or resonance matching—must occur within milliseconds to maintain the sense of immediacy and authentic connection that defines the platform experience. This latency requirement, combined with privacy constraints that discourage transmitting raw emotional data to centralized servers, motivates an edge-computing approach where affective computing models execute directly on or near the user's device.

The convergence of these requirements—anonymous identity management, real-time affective computing, privacy preservation, and meaningful emotional connection—necessitates a fundamentally new architectural paradigm. Existing frameworks address these concerns in isolation: federated learning systems [3] handle privacy but not emotion-specific modeling; edge AI frameworks [4] optimize latency but lack affective computing capabilities; and emotion recognition systems [5] achieve high accuracy but assume centralized, identity-aware deployments.

Recent advances in foundation models for affective computing [6] have demonstrated remarkable capabilities in emotion understanding, yet their computational demands render them impractical for edge deployment without significant architectural innovation. Similarly, while federated learning has shown promise for privacy-preserving sentiment analysis [7], existing approaches do not address the unique challenges of anonymous platforms where user identifiers themselves must be protected.

The global mental health landscape further underscores the urgency of this work. The World Health Organization reports that approximately 280 million people worldwide suffer from depression, with anonymous online platforms increasingly serving as first points of emotional disclosure [8]. A system that can accurately detect emotional distress, provide appropriate support, and connect individuals with similar experiences—all while preserving their anonymity—represents not merely a technical contribution but a potential public health intervention.

### 1.2 Technical Challenges

Developing a comprehensive affective computing framework for anonymous social platforms presents five interconnected technical challenges:

**Challenge 1: Real-Time Multi-Level Sentiment Analysis on Edge Devices.** The system must perform accurate sentiment classification with sub-50ms latency on resource-constrained edge devices. This requires not only model compression techniques such as INT8 quantization but also a multi-level analysis pipeline that combines rule-based, lexicon-based, and statistical approaches to achieve robust accuracy without relying on large transformer models. The challenge is compounded by the informal, often emotionally charged language used on anonymous platforms, which differs significantly from the formal text corpora on which most sentiment models are trained.

**Challenge 2: Privacy-Preserving Emotion Analytics.** Aggregate emotion statistics (e.g., community mood distributions, average sentiment scores) provide valuable insights for platform health monitoring and user experience optimization. However, publishing these statistics risks revealing individual users' emotional states through composition attacks or auxiliary information linkage. The system must provide mathematically provable privacy guarantees through differential privacy mechanisms while maintaining statistical utility sufficient for meaningful analytics.

**Challenge 3: Emotion-Aware Content Matching Beyond Semantic Similarity.** Traditional content recommendation relies primarily on semantic similarity, which fails to capture the temporal dynamics of emotional experience. Two users may express semantically different content yet share remarkably similar emotional trajectories over time. The system must incorporate temporal emotion patterns via Dynamic Time Warping, diversity considerations, and complementary emotion matching to facilitate genuinely meaningful connections that go beyond surface-level textual similarity.

**Challenge 4: Scalable Similarity Search in High-Dimensional Emotion Spaces.** Emotion embedding vectors reside in high-dimensional spaces where exact nearest neighbor search becomes computationally prohibitive at scale. With O(nd) complexity for brute-force search over n vectors of dimension d, the system requires approximate nearest neighbor algorithms—specifically HNSW with O(log n) query complexity—optimized for emotion embeddings with tunable accuracy-speed tradeoffs.

**Challenge 5: Content Safety in Anonymous Environments.** Anonymity, while enabling authentic expression, also lowers barriers to harmful content including self-harm ideation, crisis situations, and abusive language. The moderation system must operate at edge-level speeds using Aho-Corasick automaton matching with O(n+m+z) complexity while incorporating a five-factor risk assessment model that evaluates self-harm indicators (weight 0.9), hopelessness, isolation, urgency, and linguistic markers (weight 0.3) to provide nuanced risk stratification rather than binary content filtering.

### 1.3 Contributions

This paper makes the following contributions:

1. **EdgeEmotion Framework Architecture.** We present a comprehensive edge AI framework comprising eight tightly-coupled subsystems—sentiment analysis engine, Aho-Corasick content moderation, HNSW emotion vector search, Laplace differential privacy, FedAvg federated learning with gradient clipping, emotion pulse sliding window, INT8 quantization inference, and circuit-breaker node monitoring—designed specifically for affective computing in anonymous social platforms.

2. **Four-Dimensional Emotion Resonance Algorithm.** We propose a novel resonance scoring function ResonanceScore = α·Semantic + β·DTW + γ·Decay + δ·Diversity that captures semantic similarity, temporal emotion trajectory alignment via Dynamic Time Warping, recency-weighted temporal decay, and diversity promotion. This formulation, combined with HNSW-based O(log n) approximate nearest neighbor retrieval, enables real-time emotion-aware matching at scale.

3. **Dual-Memory RAG for Affective Response Generation.** We introduce a psychotherapeutically-informed dual-memory Retrieval-Augmented Generation architecture that maintains a short-term sliding window for immediate conversational context and a long-term 30-day emotional profile for personalized, empathetic response generation. This design draws on established therapeutic frameworks including motivational interviewing and cognitive behavioral therapy principles.

4. **Privacy-Preserving Emotion Analytics with Formal Guarantees.** We develop a differential privacy engine based on the Laplace mechanism with formal ε-budget composition theory, providing mathematically provable privacy guarantees (ε < 1.0) for aggregate emotion statistics. We further integrate this with a federated learning pipeline using FedAvg with gradient clipping to enable collaborative model improvement without centralizing raw emotional data.

5. **Five-Factor Risk Assessment Model.** We propose a weighted multi-factor risk scoring system for content safety that evaluates self-harm indicators (weight 0.9), hopelessness expressions, social isolation signals, temporal urgency markers, and linguistic risk patterns (weight 0.3), providing clinically-informed risk stratification that surpasses binary classification approaches.

6. **Production-Scale Evaluation.** We present comprehensive experimental results from a production deployment serving 50,000+ daily active users, demonstrating 94.2% sentiment accuracy, sub-50ms edge inference latency, and 87.3% user satisfaction with emotion resonance matching.

### 1.4 Paper Organization

The remainder of this paper is organized as follows. Section 2 reviews related work across affective computing, edge AI, differential privacy, and federated learning. Section 3 presents the overall EdgeEmotion framework architecture and its eight subsystems. Section 4 details the emotion analysis pipeline including multi-level sentiment analysis and the four-dimensional resonance algorithm. Section 5 describes the privacy-preserving mechanisms with formal mathematical analysis. Section 6 presents experimental evaluation results. Section 7 discusses limitations and broader implications. Section 8 concludes the paper.

---

## 2. Related Work

### 2.1 Affective Computing and Edge AI

Affective computing, first formalized by Picard [2], encompasses the design of systems that can recognize, interpret, and simulate human emotions. The field has evolved significantly from early rule-based approaches to sophisticated deep learning architectures. Cambria et al. [9] provided a comprehensive survey of sentiment analysis techniques, categorizing approaches into knowledge-based, statistical, and hybrid methods. Our EdgeEmotion framework adopts a similar multi-level philosophy but extends it with edge-optimized execution and privacy preservation.

The deployment of affective computing models on edge devices has emerged as a critical research direction. Recent work on lightweight Transformer architectures for edge devices [10] has systematically explored attention mechanism simplification, layer pruning, and architecture search strategies that reduce computational overhead while preserving representational capacity. Complementary research on Transformer inference optimization [11] has demonstrated that token pruning, knowledge distillation, and quantization can collectively reduce inference latency by 3-5x on resource-constrained hardware. BlendFER-Lite [12] proposed an LSTM-based blending architecture for lightweight facial emotion recognition, achieving competitive accuracy on mobile devices, though limited to the visual modality.

The intersection of privacy preservation and affective computing has been surveyed comprehensively in recent work [13], which identifies differential privacy, federated learning, and secure multi-party computation as the three primary paradigms for protecting emotional data. However, existing surveys note a significant gap in frameworks that simultaneously address edge deployment constraints and privacy requirements in anonymous social contexts—precisely the gap that EdgeEmotion aims to fill.

### 2.2 Privacy-Preserving Emotion Analysis

Differential privacy (DP) has become the gold standard for formal privacy guarantees in data analysis [14]. In the context of emotion and mental health computing, DP-CARE [15] pioneered the integration of differential privacy mechanisms into mental health text classifiers, demonstrating that formal privacy guarantees (ε-differential privacy) can be maintained while achieving acceptable classification accuracy. Their work validates the feasibility of DP in sensitive emotional data processing, though it focuses exclusively on classification tasks without addressing real-time inference or edge deployment.

PriMonitor [16] introduced adaptive tuning strategies for privacy-preserving multimodal emotion detection, proposing dynamic privacy budget allocation mechanisms that adjust protection intensity based on data sensitivity. This adaptive approach contrasts with EdgeEmotion's fixed ε-budget design, suggesting a promising direction for future enhancement. Soft prompt tuning via differential privacy [17] demonstrated that DP noise injection during prompt tuning can reduce the computational overhead of privacy-preserving model fine-tuning, offering a pathway for future EdgeEmotion model adaptation.

Federated learning has emerged as a complementary privacy paradigm for emotion recognition. FedAvg-based approaches for physiological signal emotion recognition [18] have shown that collaborative model training across distributed devices can achieve accuracy comparable to centralized training while keeping raw emotional data on-device. FedEmo [19] specifically addressed federated sentiment analysis, proposing gradient compression techniques that reduce communication overhead by 8x. EdgeEmotion builds upon these foundations by combining federated learning with differential privacy in a unified framework, providing both training-time and inference-time privacy guarantees.

### 2.3 Emotion-Aware Recommendation and Matching

Traditional recommendation systems rely primarily on collaborative filtering or content-based similarity, which fail to capture the temporal dynamics of emotional experience [20]. Emotion-aware recommendation has gained traction with systems that incorporate affective states into the recommendation pipeline. EmotionRec [21] proposed embedding user emotional trajectories into the recommendation process, demonstrating that emotion-aware recommendations achieve 23% higher user satisfaction compared to purely content-based approaches.

Dynamic Time Warping (DTW) has been applied to emotion trajectory matching in several contexts. Temporal emotion pattern analysis [22] demonstrated that DTW-based trajectory similarity captures meaningful emotional connections that cosine similarity over static embeddings misses entirely. However, existing approaches treat emotion matching as an isolated component rather than integrating it into a comprehensive framework with privacy guarantees and real-time constraints. EdgeEmotion's four-dimensional resonance algorithm addresses this limitation by combining semantic similarity, DTW-based trajectory alignment, temporal decay, and diversity promotion into a unified scoring function.

### 2.4 Mental Health Detection in Social Media

The detection of mental health conditions through social media text has become an active research area with significant public health implications. CLPsych shared tasks [23] have established benchmark datasets and evaluation protocols for depression, anxiety, and suicidal ideation detection from user-generated text. Recent transformer-based approaches [24] have achieved F1 scores exceeding 0.85 on these benchmarks, though they require substantial computational resources incompatible with edge deployment.

Multi-task learning frameworks for mental health detection [25] have demonstrated that jointly modeling multiple mental health indicators (depression, anxiety, stress, suicidal ideation) improves detection accuracy through shared representations. Temporal modeling of mental health trajectories [26] has shown that longitudinal analysis of user posting patterns provides more reliable detection than single-post classification. EdgeEmotion's emotion pulse sliding window mechanism draws on these temporal modeling insights, maintaining a rolling window of emotional states that enables trajectory-aware analysis without requiring persistent user identity.

Crisis detection in anonymous platforms presents unique challenges. Existing crisis detection systems [27] rely on user history and social network features that are unavailable in anonymous contexts. EdgeEmotion's five-factor risk assessment model addresses this by combining linguistic markers, emotional intensity signals, and temporal urgency indicators that can be computed from individual posts without identity linkage.

### 2.5 RAG for Mental Health Support

Retrieval-Augmented Generation (RAG) has shown remarkable promise in mental health support applications. MentalRAG [28] demonstrated that augmenting language model responses with retrieved therapeutic knowledge significantly improves the empathetic quality and clinical appropriateness of generated responses. Their dual-retrieval architecture—combining knowledge base retrieval with similar case retrieval—inspired EdgeEmotion's dual-memory design.

SpeechT-RAG [29] extended RAG to multimodal depression detection, retrieving relevant speech-text pairs to augment classification decisions. CounselRAG [30] proposed a counseling-specific RAG framework that retrieves from a curated database of therapeutic techniques, matching retrieved strategies to the user's presenting emotional state. These works collectively demonstrate that RAG architectures can bridge the gap between general-purpose language models and domain-specific therapeutic requirements.

EdgeEmotion's dual-memory RAG architecture extends these approaches by maintaining two distinct memory stores: a short-term sliding window (recent conversational context) and a long-term 30-day emotional profile. This design is informed by psychotherapeutic principles—specifically, the distinction between immediate emotional state (addressed through motivational interviewing techniques) and longitudinal emotional patterns (addressed through cognitive behavioral therapy frameworks). The integration of RAG with edge deployment and differential privacy represents a novel contribution not addressed in prior work.

### 2.6 On-Device Model Optimization

The deployment of deep learning models on edge devices requires aggressive optimization techniques. INT8 quantization has emerged as the most widely adopted approach, with TensorRT [31] and ONNX Runtime [32] providing production-grade quantization pipelines. Symmetric quantization—where the zero point is fixed at zero and the scale factor is computed as $\text{scale} = \max(|x|) / 127$—offers the best balance of accuracy preservation and inference speedup for affective computing models [33].

Knowledge distillation for sentiment analysis [34] has demonstrated that compact student models can retain 95-98% of teacher model accuracy at 10-20x parameter reduction. DistilHuBERT [35] applied distillation to speech emotion recognition, achieving mobile-efficient inference with minimal accuracy degradation. Mixed-precision quantization strategies [36] have shown that selectively applying different precision levels to different layers can further optimize the accuracy-latency tradeoff.

EdgeEmotion's quantization subsystem implements symmetric INT8 quantization with INT32 accumulation for matrix multiplication, following the established best practice of using higher-precision accumulators to prevent error accumulation in deep computation graphs. Our approach differs from prior work in its tight integration with the affective computing pipeline, where quantization-aware confidence calibration ensures that the reduced precision does not compromise the reliability of emotion classification decisions.

### 2.7 Vector Search and Content Moderation

Hierarchical Navigable Small World (HNSW) graphs [37] have become the de facto standard for approximate nearest neighbor search in high-dimensional spaces, offering O(log n) query complexity with tunable accuracy-recall tradeoffs. Recent optimizations for HNSW [38] have focused on memory-efficient implementations suitable for edge deployment, including compressed vector representations and cache-friendly graph traversal patterns.

Content moderation in anonymous platforms requires both speed and nuance. The Aho-Corasick automaton [39] provides optimal O(n+m+z) multi-pattern string matching, where n is the text length, m is the total pattern length, and z is the number of matches. GPU-accelerated implementations [40] have achieved throughput improvements of 10-50x for large pattern dictionaries. However, keyword-based moderation alone produces unacceptable false positive rates in emotional contexts where words like "kill" or "die" may express emotional distress rather than harmful intent.

Semantic content moderation approaches [41] address this limitation by incorporating contextual understanding, but at significantly higher computational cost. EdgeEmotion bridges this gap through a two-stage pipeline: fast Aho-Corasick pattern matching for initial screening followed by a five-factor semantic risk assessment for flagged content, achieving both the speed of pattern matching and the nuance of semantic analysis.

### 2.8 Microservices Resilience and Circuit Breaker Patterns

The circuit breaker pattern, originally proposed by Nygard [42] and formalized in the context of microservices architectures, provides fault isolation and graceful degradation when downstream services fail. Recent work on adaptive circuit breakers [43] has introduced machine learning-based threshold tuning that adjusts failure thresholds based on historical patterns, reducing both false trips and cascading failures.

In the context of edge AI systems, circuit breakers serve a dual purpose: protecting against service failures and managing computational resource exhaustion. EdgeEmotion's node health monitoring subsystem extends the traditional circuit breaker pattern with emotion-aware degradation strategies—when the AI inference subsystem is overloaded, the system gracefully degrades to rule-based sentiment analysis rather than returning errors, ensuring continuous emotional support for users.

### 2.9 Summary and Positioning

While existing works have made significant progress in individual aspects—edge AI inference optimization [10, 11], differential privacy for NLP [15, 16], federated emotion recognition [18, 19], and RAG-based mental health support [28, 30]—no prior work has proposed an integrated framework that simultaneously addresses all these challenges in the context of anonymous social platforms. Table 1 summarizes the comparison between EdgeEmotion and representative prior works across six key dimensions.

**Table 1.** Comparison of EdgeEmotion with representative prior works.

| System | Edge Deploy | Diff. Privacy | Fed. Learning | Emotion Match | Content Safety | Anonymous |
|--------|:-----------:|:-------------:|:-------------:|:-------------:|:--------------:|:---------:|
| DP-CARE [15] | — | ✓ | — | — | — | — |
| PriMonitor [16] | — | ✓ | — | — | — | — |
| FedEmo [19] | — | — | ✓ | — | — | — |
| EmotionRec [21] | — | — | — | ✓ | — | — |
| MentalRAG [28] | — | — | — | — | — | — |
| **EdgeEmotion** | **✓** | **✓** | **✓** | **✓** | **✓** | **✓** |

EdgeEmotion bridges the gap between these isolated contributions by presenting a unified architecture comprising eight tightly-coupled subsystems that collectively deliver real-time affective computing with formal privacy guarantees on edge devices, specifically designed for the unique requirements of anonymous social platforms.
