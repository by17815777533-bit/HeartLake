# EdgeEmotion: Expanded Sections 5–8 and References

## 5. Privacy-Preserving Mechanisms

Protecting user privacy in affective computing systems presents unique challenges: emotion data is inherently sensitive, and even aggregate statistics can reveal individual emotional states through inference attacks [36, 37]. EdgeEmotion addresses these challenges through a layered privacy architecture comprising four complementary mechanisms: differential privacy for statistical queries, formal privacy budget composition, federated learning with gradient privacy, and cryptographic identity protection.

### 5.1 Differential Privacy Framework

We adopt the standard $(\varepsilon, \delta)$-differential privacy framework [1] as the foundational privacy guarantee for all aggregate emotion analytics.

**Definition 1 (Differential Privacy).** A randomized mechanism $\mathcal{M}: \mathcal{D} \rightarrow \mathcal{R}$ satisfies $(\varepsilon, \delta)$-differential privacy if for all adjacent datasets $D, D' \in \mathcal{D}$ differing in at most one record, and for all measurable subsets $S \subseteq \mathcal{R}$:

$$\Pr[\mathcal{M}(D) \in S] \leq e^{\varepsilon} \cdot \Pr[\mathcal{M}(D') \in S] + \delta$$

In EdgeEmotion, we implement the pure $\varepsilon$-differential privacy variant ($\delta = 0$) using the Laplace mechanism for all real-valued aggregate queries. The Laplace mechanism adds noise drawn from the Laplace distribution $\text{Lap}(0, b)$ where $b = \Delta f / \varepsilon$, with $\Delta f$ denoting the $\ell_1$-sensitivity of the query function $f$.

**Laplace Sampling Implementation.** Our implementation employs the inverse CDF method for generating Laplace random variates. Given a uniform random variable $U \sim \text{Uniform}(-0.5, 0.5)$, the Laplace sample is computed as:

$$X = -b \cdot \text{sign}(U) \cdot \ln(1 - 2|U|)$$

where $b = \Delta f / \varepsilon$ is the scale parameter. This approach avoids the numerical instability of the standard two-exponential method and provides exact sampling from the Laplace distribution. The implementation includes boundary protection against $\log(0)$ by clamping $|U|$ to $[0, 0.5 - \epsilon_{\text{machine}})$.

**Sensitivity Analysis.** We perform rigorous sensitivity analysis for each aggregate query in the emotion analytics pipeline:

1. **Average Emotion Score** ($f_{\text{avg}}$): The emotion score is bounded to $[-1, 1]$. For a dataset of $N$ users, adding or removing one user changes the average by at most $2/N$. However, since $N$ is itself a private quantity, we use the worst-case global sensitivity $\Delta f_{\text{avg}} = 2.0$.

2. **User Count** ($f_{\text{count}}$): The count query has sensitivity $\Delta f_{\text{count}} = 1$, as adding or removing one user changes the count by exactly one.

3. **Mood Distribution Counts** ($f_{\text{mood}_i}$): Each mood category count has sensitivity $\Delta f_{\text{mood}_i} = 1$, since one user contributes to exactly one mood category.

**Post-Processing.** Following the post-processing immunity property of differential privacy [1], we apply deterministic transformations to noisy outputs without additional privacy cost:
- Average scores are clamped to $[-1, 1]$ via $\text{clamp}(\tilde{f}_{\text{avg}}, -1, 1)$
- User counts are rounded and floored to zero: $\max(0, \lfloor \tilde{f}_{\text{count}} + 0.5 \rfloor)$
- Mood distributions are normalized to form valid probability distributions: $\tilde{p}_i = \max(0, \tilde{f}_{\text{mood}_i}) / \sum_j \max(0, \tilde{f}_{\text{mood}_j})$

**Theorem 1 (Privacy Guarantee).** The `aggregateWithPrivacy` mechanism satisfies $\varepsilon$-differential privacy for any $\varepsilon > 0$.

*Proof.* The mechanism composes three groups of Laplace mechanisms: (1) average score with budget $\varepsilon_{\text{avg}} = 0.3\varepsilon$ and sensitivity 2.0, yielding scale $b_{\text{avg}} = 2.0 / (0.3\varepsilon)$; (2) user count with budget $\varepsilon_{\text{count}} = 0.2\varepsilon$ and sensitivity 1.0, yielding scale $b_{\text{count}} = 1.0 / (0.2\varepsilon)$; (3) seven mood counts, each with budget $\varepsilon_{\text{mood}} = 0.5\varepsilon / 7$ and sensitivity 1.0. By the sequential composition theorem (Theorem 2), the total privacy cost is $\varepsilon_{\text{avg}} + \varepsilon_{\text{count}} + 7 \cdot \varepsilon_{\text{mood}} = 0.3\varepsilon + 0.2\varepsilon + 0.5\varepsilon = \varepsilon$. Post-processing operations (clamping, rounding, normalization) do not consume additional privacy budget. $\square$

### 5.2 Privacy Budget Composition

Managing the cumulative privacy loss across multiple queries is critical for maintaining meaningful privacy guarantees over time. EdgeEmotion implements a multi-level composition framework.

**Theorem 2 (Sequential Composition [1]).** If mechanisms $\mathcal{M}_1, \ldots, \mathcal{M}_k$ satisfy $\varepsilon_1, \ldots, \varepsilon_k$-differential privacy respectively, then their sequential composition $(\mathcal{M}_1, \ldots, \mathcal{M}_k)$ satisfies $(\sum_{i=1}^{k} \varepsilon_i)$-differential privacy.

**Theorem 3 (Parallel Composition [1]).** If mechanisms $\mathcal{M}_1, \ldots, \mathcal{M}_k$ are applied to disjoint subsets of the dataset and each satisfies $\varepsilon_i$-differential privacy, then the combined mechanism satisfies $(\max_i \varepsilon_i)$-differential privacy.

**Intra-Query Budget Allocation.** Within a single `aggregateWithPrivacy` invocation, the total budget $\varepsilon$ is allocated across query components using a weighted split optimized for the relative importance and sensitivity of each statistic:

| Component | Budget Fraction | Allocated $\varepsilon_i$ | Sensitivity | Scale $b_i$ |
|-----------|----------------|--------------------------|-------------|-------------|
| Average Score | 30% | $0.3\varepsilon$ | 2.0 | $6.67/\varepsilon$ |
| User Count | 20% | $0.2\varepsilon$ | 1.0 | $5.0/\varepsilon$ |
| Mood Counts ($\times 7$) | 50% | $0.5\varepsilon/7$ each | 1.0 | $14.0/\varepsilon$ each |

This allocation prioritizes the mood distribution (50% of budget) as it drives the primary user-facing feature (lake weather visualization), while allocating sufficient budget to the average score (30%) for trend analysis.

**Inter-Query Budget Tracking.** EdgeEmotion maintains a cumulative privacy ledger that tracks total epsilon consumed across all queries:

$$\varepsilon_{\text{total}} = \sum_{t=1}^{T} \varepsilon_t$$

The system classifies the cumulative privacy state into three levels:
- **Strong** ($\varepsilon_{\text{total}} < 10$): Full utility with robust privacy guarantees
- **Moderate** ($10 \leq \varepsilon_{\text{total}} < 50$): Acceptable for most applications
- **Weak** ($\varepsilon_{\text{total}} \geq 50$): System recommends budget reset or reduced query frequency

**Advanced Composition via Renyi Differential Privacy.** For tighter composition bounds under repeated queries, we additionally support Renyi Differential Privacy (RDP) [48] analysis. The Laplace mechanism with parameter $b$ satisfies $(\alpha, \alpha / (2b^2(\alpha - 1)))$-RDP for all $\alpha > 1$. Converting back to $(\varepsilon, \delta)$-DP via the relation $\varepsilon = \rho + \ln(1/\delta) / (\alpha - 1)$ yields substantially tighter bounds than naive sequential composition for large numbers of queries.

### 5.3 Federated Learning with Gradient Clipping

EdgeEmotion employs Federated Averaging (FedAvg) [38] for collaborative model training without centralizing raw emotion data. To prevent gradient-based privacy leakage [39], we integrate DP-SGD [40] with per-sample gradient clipping.

**Gradient Clipping.** Before aggregation, each client clips its gradient update to a maximum $\ell_2$-norm $C$:

$$\tilde{g} = g \cdot \min\left(1, \frac{C}{\|g\|_2}\right)$$

This ensures bounded sensitivity $\Delta g = C$ for the gradient aggregation mechanism, enabling calibrated Gaussian noise addition:

$$\bar{g} = \frac{1}{K} \sum_{k=1}^{K} \tilde{g}_k + \mathcal{N}\left(0, \sigma^2 C^2 \mathbf{I}\right)$$

where $\sigma$ is calibrated to achieve the target $(\varepsilon, \delta)$-DP guarantee per round via the moments accountant [40].

**Federated Emotion Model Update Protocol:**

1. Server broadcasts global model $\theta_t$ to selected clients
2. Each client $k$ computes local update $g_k = \nabla_\theta \mathcal{L}_k(\theta_t)$ on private emotion data
3. Client clips gradient: $\tilde{g}_k = g_k \cdot \min(1, C / \|g_k\|_2)$
4. Server aggregates with noise: $\theta_{t+1} = \theta_t - \eta \left(\frac{1}{K}\sum_k \tilde{g}_k + \mathcal{N}(0, \sigma^2 C^2 \mathbf{I})\right)$
5. Privacy accountant updates cumulative $(\varepsilon, \delta)$ via RDP composition

**Convergence Guarantee.** Under standard assumptions (L-smooth, $\mu$-strongly convex loss), the federated DP-SGD protocol converges at rate $O(1/\sqrt{T} + d\sigma^2 C^2 / (K \cdot T))$, where $d$ is the model dimension, $T$ is the number of rounds, and $K$ is the number of participating clients per round.

### 5.4 Identity Shadow Mapping

To enable consistent user interaction tracking without storing personally identifiable information, EdgeEmotion implements a cryptographic identity shadow mapping scheme.

**Shadow ID Generation.** Each user is assigned a cryptographically random shadow identifier using a CSPRNG (Cryptographically Secure Pseudo-Random Number Generator) backed by OpenSSL's `RAND_bytes`:

$$\text{ShadowID} = \texttt{"shd\_"} \| \text{Hex}(\text{RAND\_bytes}(16))$$

This produces 128-bit random identifiers with $2^{128}$ possible values, making collision probability negligible ($< 2^{-64}$ for up to $2^{32}$ users by the birthday bound).

**Derived Identifiers.** For auxiliary tracking (e.g., rate limiting, abuse detection), we derive deterministic but unlinkable identifiers using keyed SHA-256 hashing:

$$\text{IP\_Shadow} = \texttt{"ip\_"} \| \text{Hex}(\text{SHA256}(\text{salt}_{\text{ip}} \| \text{IP}))[:16]$$
$$\text{FP\_Shadow} = \texttt{"fp\_"} \| \text{Hex}(\text{SHA256}(\text{salt}_{\text{fp}} \| \text{fingerprint}))[:16]$$

where $\text{salt}_{\text{ip}}$ and $\text{salt}_{\text{fp}}$ are server-side secrets. The truncation to 64 bits provides sufficient collision resistance for operational purposes while limiting the information content of stored identifiers.

**Security Properties:**
- **One-wayness**: SHA-256 preimage resistance ($2^{256}$ operations) prevents recovering original identifiers from shadows
- **Unlinkability**: Different salts ensure IP shadows and fingerprint shadows cannot be correlated without server-side secrets
- **Forward secrecy**: Periodic salt rotation invalidates historical shadow-to-identity mappings

**End-to-End Encryption.** All emotion content in transit is protected by a hybrid encryption scheme combining X25519 key exchange, HKDF key derivation, and AES-256-GCM authenticated encryption, providing IND-CCA2 security and forward secrecy through ephemeral key pairs.

## 6. Experimental Evaluation

We conduct comprehensive experiments to evaluate EdgeEmotion across five dimensions: sentiment analysis accuracy, emotion resonance recommendation quality, privacy-utility tradeoffs, end-to-end system latency, and content moderation effectiveness. All experiments are designed to answer the following research questions:

- **RQ1**: How does EdgeEmotion's on-device sentiment analysis compare to cloud-based and edge-optimized baselines in accuracy and latency?
- **RQ2**: Does the four-dimensional emotion resonance algorithm improve recommendation quality over traditional content-based and collaborative filtering approaches?
- **RQ3**: What is the empirical privacy-utility tradeoff under varying differential privacy budgets?
- **RQ4**: Can the full EdgeEmotion pipeline meet real-time latency requirements ($< 100$ ms) for interactive social applications?
- **RQ5**: How effective is the multi-layer content moderation system in detecting harmful content while minimizing false positives?

### 6.1 Experimental Setup

**Datasets.** We evaluate on three datasets: (1) **GoEmotions** [41], a corpus of 58K Reddit comments annotated with 27 emotion categories, which we map to our 7-category taxonomy (happy, sad, angry, fearful, surprised, neutral, calm) following the hierarchical grouping of Demszky et al.; (2) **HeartLake-Live**, a production dataset comprising 12,847 anonymous emotion stones collected over a 90-day deployment period, with ground-truth mood labels provided by users at submission time; and (3) **SemEval-2018 Task 1** [42], used for cross-domain sentiment analysis evaluation. For the resonance recommendation experiments, we use HeartLake-Live with implicit feedback signals (stone pickups, replies, and resonance ratings) as ground truth.

**Baselines.** We compare against the following baselines across different evaluation dimensions:

- *Sentiment Analysis*: BERT-base [43], DistilBERT [44], TinyBERT [45], MobileBERT [46], and a cloud-hosted GPT-3.5 API baseline.
- *Recommendation*: Content-based filtering (TF-IDF + cosine similarity), collaborative filtering (matrix factorization), and a hybrid neural approach (NCF [47]).
- *Privacy*: Non-private aggregation (no noise), Gaussian mechanism [1], and the RAPPOR protocol [49].
- *Moderation*: Perspective API (Google), keyword matching, and a fine-tuned BERT classifier.

**Metrics.** We report accuracy, macro-F1, and weighted-F1 for classification tasks; Precision@K, Recall@K, NDCG@K, and Mean Reciprocal Rank (MRR) for recommendation; Mean Absolute Error (MAE) and relative error for privacy-utility analysis; end-to-end latency (P50, P95, P99) for system performance; and F1, precision, recall for content moderation.

**Hardware and Deployment.** Edge inference runs on a Raspberry Pi 4 Model B (4GB RAM, ARM Cortex-A72) to simulate resource-constrained edge devices. The backend server runs on a single-node deployment with an Intel Xeon E5-2680 v4 (28 cores), 64GB RAM, and PostgreSQL 15. All latency measurements are averaged over 10,000 requests after a 1,000-request warm-up period.

### 6.2 Sentiment Analysis Performance (RQ1)

Table 2 presents the sentiment analysis results on the GoEmotions test set mapped to our 7-category taxonomy.

**Table 2.** Sentiment analysis performance comparison on GoEmotions (7-category).

| Model | Accuracy (%) | Macro-F1 | Weighted-F1 | Latency (ms) | Model Size (MB) |
|-------|-------------|----------|-------------|---------------|-----------------|
| BERT-base (cloud) | 95.1 | 0.938 | 0.949 | 142 | 440 |
| GPT-3.5 API | 96.3 | 0.951 | 0.961 | 890 | — |
| DistilBERT | 93.4 | 0.917 | 0.931 | 68 | 265 |
| TinyBERT | 91.8 | 0.899 | 0.914 | 34 | 56 |
| MobileBERT | 92.6 | 0.908 | 0.922 | 28 | 100 |
| **EdgeEmotion** | **94.2** | **0.927** | **0.940** | **12** | **18** |

EdgeEmotion achieves 94.2% accuracy with only 12 ms inference latency, representing a 91.5% latency reduction compared to cloud-based BERT while retaining 99.1% of its accuracy. The model size of 18 MB is 24.4$\times$ smaller than BERT-base, enabling deployment on memory-constrained edge devices. Compared to MobileBERT, EdgeEmotion achieves 1.6 percentage points higher accuracy at 2.3$\times$ lower latency, demonstrating the effectiveness of our emotion-specific knowledge distillation and INT8 quantization pipeline.

On the HeartLake-Live dataset, EdgeEmotion achieves 92.8% accuracy (macro-F1 = 0.911), with the slight decrease attributable to the informal, emoji-rich language characteristic of anonymous social platforms. Cross-domain evaluation on SemEval-2018 yields 89.4% accuracy, confirming reasonable generalization without domain-specific fine-tuning.

**Error Analysis.** The primary confusion patterns involve the calm-neutral boundary (38% of errors) and the sad-fearful overlap (24% of errors). These confusions are linguistically motivated: calm and neutral share low-arousal characteristics, while sadness and fear frequently co-occur in anonymous emotional expressions. We note that these confusions have minimal impact on the downstream resonance algorithm, as both confused categories occupy adjacent regions in the emotion embedding space.

### 6.3 Emotion Resonance Recommendation Quality (RQ2)

Table 3 evaluates the four-dimensional emotion resonance algorithm against standard recommendation baselines on the HeartLake-Live dataset with implicit feedback signals (stone pickups, replies, and resonance ratings).

**Table 3.** Recommendation quality comparison on HeartLake-Live.

| Method | P@10 | R@10 | NDCG@10 | MRR | Diversity |
|--------|------|------|---------|-----|-----------|
| Content-Based (TF-IDF) | 0.612 | 0.384 | 0.651 | 0.573 | 0.42 |
| Collaborative Filtering (MF) | 0.589 | 0.361 | 0.628 | 0.551 | 0.38 |
| Neural CF (NCF) [47] | 0.694 | 0.442 | 0.738 | 0.672 | 0.45 |
| **EdgeEmotion Resonance** | **0.823** | **0.537** | **0.856** | **0.791** | **0.71** |

EdgeEmotion's resonance algorithm achieves substantial improvements across all metrics: 18.6% higher P@10 and 16.0% higher NDCG@10 compared to the strongest baseline (NCF). Notably, the diversity score of 0.71 represents a 57.8% improvement over NCF (0.45), confirming that the diversity bonus mechanism effectively prevents echo chamber effects in emotion-driven recommendations.

The superior performance stems from the algorithm's unique ability to capture emotional dynamics rather than static content features. Traditional content-based methods rely on surface-level textual similarity, which fails to capture the nuanced emotional resonance between users experiencing similar emotional trajectories. Collaborative filtering suffers from severe cold-start problems in anonymous platforms where user interaction histories are sparse by design.

**Ablation Study.** To quantify the contribution of each dimension, we conduct an ablation study by removing one component at a time while redistributing its weight proportionally among the remaining dimensions.

**Table 3a.** Ablation study on resonance dimensions.

| Configuration | NDCG@10 | $\Delta$ NDCG | MRR | $\Delta$ MRR |
|--------------|---------|---------------|-----|-------------|
| Full Model ($\alpha$=0.30, $\beta$=0.35, $\gamma$=0.20, $\delta$=0.15) | 0.856 | — | 0.791 | — |
| w/o Semantic Similarity ($\alpha$=0) | 0.756 | −11.7% | 0.694 | −12.3% |
| w/o Trajectory Similarity ($\beta$=0) | 0.786 | −8.2% | 0.723 | −8.6% |
| w/o Diversity Bonus ($\delta$=0) | 0.812 | −5.1% | 0.751 | −5.1% |
| w/o Temporal Decay ($\gamma$=0) | 0.827 | −3.4% | 0.764 | −3.4% |

The ablation results reveal that semantic similarity ($\alpha$) is the most critical dimension, with its removal causing an 11.7% NDCG drop. This is expected, as textual content remains the primary signal for emotional resonance. The emotion trajectory dimension ($\beta$) contributes the second-largest effect (−8.2%), validating our hypothesis that users experiencing similar emotional journeys form stronger connections. The diversity bonus ($\delta$) accounts for a 5.1% improvement, confirming its role in preventing recommendation homogeneity. Temporal decay ($\gamma$) has the smallest individual contribution (−3.4%), but its removal leads to noticeably stale recommendations in qualitative evaluation.

**Weight Sensitivity Analysis.** We perform a grid search over the weight space $\{\alpha, \beta, \gamma, \delta\}$ with step size 0.05, subject to the constraint $\alpha + \beta + \gamma + \delta = 1$. The optimal configuration ($\alpha$=0.30, $\beta$=0.35, $\gamma$=0.20, $\delta$=0.15) is robust: all configurations within a Euclidean distance of 0.1 from the optimum achieve NDCG@10 $\geq$ 0.840, indicating low sensitivity to small weight perturbations.
