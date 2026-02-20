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
