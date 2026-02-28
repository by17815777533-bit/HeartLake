/**
 * @brief HNSW 向量检索引擎 —— 多层导航图 + Ada-EF 自适应搜索 + Matryoshka 重排序
 *
 * 从 EdgeAIEngine 拆分的独立子系统。
 *
 * 核心算法：
 *   - 插入：随机层级分配 + RND 多样性邻居选择（Vamana robust pruning）
 *   - 搜索：贪心逐层下降 → Distribution-Aware Ada-EF 自适应 ef 调节
 *     - pilot search 采样距离分布 → 高斯拟合 → z-score 量化查询难度
 *     - 难查询扩大 ef 保召回，易查询缩小 ef 降延迟
 *   - 距离计算：8路展开平方欧氏 + ADSampling 早期终止优化
 *   - 重排序：Matryoshka 融合 cosine（60% full-dim + 40% coarse-dim）
 *
 * 并发模型：shared_mutex 读写分离，搜索持读锁，插入/删除持写锁。
 * visited 标记使用 thread_local + epoch 法实现 O(1) 初始化。
 *
 * @note 参考 Malkov & Yashunin 2018, Vamana (NeurIPS 2019),
 *       ADSampling (VLDB 2024), arxiv 2502.05575 (2025)
 */

#include "infrastructure/ai/HNSWIndex.h"

#include <trantor/utils/Logger.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <mutex>
#include <cstdlib>
#include <limits>
#include <queue>
#include <set>

namespace heartlake::ai {

// ============================================================================
// 构造 & 配置
// ============================================================================

HNSWIndex::HNSWIndex() {
    rng_.seed(std::random_device{}());
}

void HNSWIndex::configure(int m, int mMax0, int efConstruction, int efSearch) {
    std::unique_lock lock(mutex_);
    m_ = m;
    mMax0_ = mMax0;
    efConstruction_ = efConstruction;
    efSearch_ = efSearch;
    levelMult_ = 1.0f / std::log(std::max(2.0f, static_cast<float>(m_)));
}

void HNSWIndex::clear() {
    std::unique_lock lock(mutex_);
    nodes_.clear();
    idMap_.clear();
    // visited 标记现在是 thread_local，无需在此清理
    entryPoint_ = 0;
    maxLevel_ = 0;
}

// ============================================================================
// 内部算法
// ============================================================================

int HNSWIndex::randomLevel() {
    // thread_local 消除多线程并发插入时对 rng_ 的数据竞争
    thread_local static std::mt19937 tlRng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r = dist(tlRng);
    return static_cast<int>(-std::log(r) * levelMult_);
}

float HNSWIndex::vectorDistance(const std::vector<float>& a,
                                const std::vector<float>& b) const {
    // 平方欧氏距离，8路展开 + 双累加器减少数据依赖链
    const size_t dim = std::min(a.size(), b.size());
    const float* __restrict__ pa = a.data();
    const float* __restrict__ pb = b.data();
    float sum0 = 0.0f, sum1 = 0.0f;
    size_t i = 0;
    for (; i + 7 < dim; i += 8) {
        float d0 = pa[i]     - pb[i];
        float d1 = pa[i + 1] - pb[i + 1];
        float d2 = pa[i + 2] - pb[i + 2];
        float d3 = pa[i + 3] - pb[i + 3];
        float d4 = pa[i + 4] - pb[i + 4];
        float d5 = pa[i + 5] - pb[i + 5];
        float d6 = pa[i + 6] - pb[i + 6];
        float d7 = pa[i + 7] - pb[i + 7];
        sum0 += d0 * d0 + d1 * d1 + d2 * d2 + d3 * d3;
        sum1 += d4 * d4 + d5 * d5 + d6 * d6 + d7 * d7;
    }
    for (; i < dim; ++i) {
        float d = pa[i] - pb[i];
        sum0 += d * d;
    }
    return sum0 + sum1;
}

float HNSWIndex::vectorDistanceThreshold(const std::vector<float>& a,
                                          const std::vector<float>& b,
                                          float threshold) const {
    // 带阈值的平方欧氏距离：每处理 16 维检查一次部分和，
    // 若已超过 threshold 则提前终止返回 +inf，避免无效的全维度计算。
    // 参考: ADSampling (Gao et al., VLDB 2024) 的 early-termination 思路
    const size_t dim = std::min(a.size(), b.size());
    const float* __restrict__ pa = a.data();
    const float* __restrict__ pb = b.data();
    float sum = 0.0f;
    size_t i = 0;

    // 每 16 维做一次阈值检查（平衡检查开销与跳过收益）
    for (; i + 15 < dim; i += 16) {
        float d0  = pa[i]      - pb[i];
        float d1  = pa[i + 1]  - pb[i + 1];
        float d2  = pa[i + 2]  - pb[i + 2];
        float d3  = pa[i + 3]  - pb[i + 3];
        float d4  = pa[i + 4]  - pb[i + 4];
        float d5  = pa[i + 5]  - pb[i + 5];
        float d6  = pa[i + 6]  - pb[i + 6];
        float d7  = pa[i + 7]  - pb[i + 7];
        float d8  = pa[i + 8]  - pb[i + 8];
        float d9  = pa[i + 9]  - pb[i + 9];
        float d10 = pa[i + 10] - pb[i + 10];
        float d11 = pa[i + 11] - pb[i + 11];
        float d12 = pa[i + 12] - pb[i + 12];
        float d13 = pa[i + 13] - pb[i + 13];
        float d14 = pa[i + 14] - pb[i + 14];
        float d15 = pa[i + 15] - pb[i + 15];
        sum += d0*d0 + d1*d1 + d2*d2 + d3*d3
             + d4*d4 + d5*d5 + d6*d6 + d7*d7
             + d8*d8 + d9*d9 + d10*d10 + d11*d11
             + d12*d12 + d13*d13 + d14*d14 + d15*d15;
        if (sum > threshold) {
            return std::numeric_limits<float>::infinity();
        }
    }
    // 处理剩余维度
    for (; i < dim; ++i) {
        float d = pa[i] - pb[i];
        sum += d * d;
    }
    return sum;
}

std::vector<size_t> HNSWIndex::selectDiverseNeighbors(
    size_t baseIdx,
    const std::vector<std::pair<float, size_t>>& candidates,
    int maxM) const {
    // RND (Relative Neighborhood Diversification) 邻居选择
    // 参考: Vamana (Subramanya et al., NeurIPS 2019) 的 robust pruning
    //       "A Practitioner's Guide to ANN Algorithms" (arxiv 2502.05575, 2025)
    //       评估表明 RND 在保持召回率的同时显著提升搜索效率
    //
    // 核心思想：按距离排序遍历候选，对每个候选 c 检查是否被已选邻居"覆盖"。
    // 如果存在已选邻居 n 使得 dist(c, n) < dist(c, base)，
    // 说明从 n 出发就能到达 c，c 的方向已被覆盖，跳过以保持方向多样性。
    // 这比纯距离排序裁剪能构建更高质量的导航图。

    std::vector<size_t> selected;
    selected.reserve(static_cast<size_t>(std::min(static_cast<int>(candidates.size()), maxM)));

    for (const auto& [distToBase, candIdx] : candidates) {
        if (static_cast<int>(selected.size()) >= maxM) break;
        if (candIdx == baseIdx) continue;

        // 检查是否被已选邻居覆盖
        bool covered = false;
        for (size_t selIdx : selected) {
            float distCandToSel = vectorDistance(nodes_[candIdx].vector,
                                                  nodes_[selIdx].vector);
            if (distCandToSel < distToBase) {
                covered = true;
                break;
            }
        }

        if (!covered) {
            selected.push_back(candIdx);
        }
    }

    // 如果多样性剪枝过于激进导致邻居不足，回填距离最近的候选
    // 保证连通性不被破坏
    if (static_cast<int>(selected.size()) < maxM) {
        for (const auto& candidate : candidates) {
            if (static_cast<int>(selected.size()) >= maxM) break;
            const size_t candIdx = candidate.second;
            if (candIdx == baseIdx) continue;
            if (std::find(selected.begin(), selected.end(), candIdx) == selected.end()) {
                selected.push_back(candIdx);
            }
        }
    }

    return selected;
}

std::vector<std::pair<float, size_t>> HNSWIndex::searchLayer(
    const std::vector<float>& query, size_t entryPoint, int ef, int level) const {

    using DistIdx = std::pair<float, size_t>;

    // 结果集：max-heap（距离最大的在顶部）
    std::priority_queue<DistIdx> results;
    // 候选集：min-heap（距离最小的在顶部）
    std::priority_queue<DistIdx, std::vector<DistIdx>, std::greater<DistIdx>> candidates;

    // thread_local visited 标记：每个线程独立维护，避免 shared_lock 下的数据竞争。
    // epoch 标记法：O(1) 初始化替代 O(n) 清零。
    // 参考: hnswlib (Malkov & Yashunin, 2018) visited_list_pool 优化
    thread_local std::vector<uint32_t> tlVisitedMarker;
    thread_local uint32_t tlVisitedEpoch = 0;

    ++tlVisitedEpoch;
    if (tlVisitedEpoch == 0) {
        // epoch 溢出（极罕见，约 42 亿次搜索后），重置所有标记
        std::fill(tlVisitedMarker.begin(), tlVisitedMarker.end(), 0);
        tlVisitedEpoch = 1;
    }
    if (tlVisitedMarker.size() < nodes_.size()) {
        tlVisitedMarker.resize(nodes_.size(), 0);
    }

    float entryDist = vectorDistance(query, nodes_[entryPoint].vector);
    results.push({entryDist, entryPoint});
    candidates.push({entryDist, entryPoint});
    tlVisitedMarker[entryPoint] = tlVisitedEpoch;

    while (!candidates.empty()) {
        auto [candDist, candIdx] = candidates.top();
        float worstResult = results.top().first;

        // 如果最近的候选比结果集中最远的还远，停止
        if (candDist > worstResult && static_cast<int>(results.size()) >= ef) {
            break;
        }
        candidates.pop();

        // 扩展该候选的邻居
        if (level < static_cast<int>(nodes_[candIdx].neighbors.size())) {
            const auto& neighborList = nodes_[candIdx].neighbors[level];
            const size_t nSize = neighborList.size();

            // 两阶段 prefetch 流水线：先预取前 8 个，处理时再预取下一批
            // 更深的流水线让 CPU 有更多时间隐藏内存延迟
            const size_t prefetchBatch = 8;
            for (size_t ni = 0; ni < std::min(nSize, prefetchBatch); ++ni) {
                size_t nIdx = neighborList[ni];
                if (tlVisitedMarker[nIdx] != tlVisitedEpoch) {
                    __builtin_prefetch(nodes_[nIdx].vector.data(), 0, 1);
                }
            }

            for (size_t ni = 0; ni < nSize; ++ni) {
                // 提前预取下一批邻居向量（流水线第二阶段）
                if (ni + prefetchBatch < nSize) {
                    size_t futureIdx = neighborList[ni + prefetchBatch];
                    if (tlVisitedMarker[futureIdx] != tlVisitedEpoch) {
                        __builtin_prefetch(nodes_[futureIdx].vector.data(), 0, 1);
                    }
                }

                size_t neighborIdx = neighborList[ni];
                if (tlVisitedMarker[neighborIdx] == tlVisitedEpoch) continue;
                tlVisitedMarker[neighborIdx] = tlVisitedEpoch;

                const bool resultsFullNow = static_cast<int>(results.size()) >= ef;
                worstResult = resultsFullNow
                    ? results.top().first
                    : std::numeric_limits<float>::infinity();

                // 当结果集已满时，使用 ADSampling 早期终止距离计算
                // 参考: ADSampling (Gao et al., VLDB 2024)
                // 如果部分维度的累积距离已超过 worstResult，跳过剩余维度
                float neighborDist;
                if (resultsFullNow) {
                    neighborDist = vectorDistanceThreshold(
                        query, nodes_[neighborIdx].vector, worstResult);
                } else {
                    neighborDist = vectorDistance(query, nodes_[neighborIdx].vector);
                }

                if (!resultsFullNow || neighborDist < worstResult) {
                    results.push({neighborDist, neighborIdx});
                    candidates.push({neighborDist, neighborIdx});

                    // 保持结果集大小不超过 ef
                    if (static_cast<int>(results.size()) > ef) {
                        results.pop();
                    }
                    worstResult = results.top().first;
                }
            }
        }
    }

    // 提取结果（按距离从小到大排序）
    std::vector<DistIdx> sorted;
    sorted.reserve(results.size());
    while (!results.empty()) {
        sorted.push_back(results.top());
        results.pop();
    }
    std::sort(sorted.begin(), sorted.end());

    return sorted;
}

void HNSWIndex::connectNeighbors(size_t nodeIdx,
                                  const std::vector<size_t>& neighbors,
                                  int level, int maxM) {
    if (nodeIdx >= nodes_.size() ||
        level < 0 ||
        level >= static_cast<int>(nodes_[nodeIdx].neighbors.size())) {
        return;
    }

    auto& nodeNeighbors = nodes_[nodeIdx].neighbors[level];
    for (size_t neighborIdx : neighbors) {
        // 添加正向连接
        if (std::find(nodeNeighbors.begin(), nodeNeighbors.end(), neighborIdx)
            == nodeNeighbors.end()) {
            nodeNeighbors.push_back(neighborIdx);
        }

        // 添加反向连接
        if (level < static_cast<int>(nodes_[neighborIdx].neighbors.size())) {
            auto& revNeighbors = nodes_[neighborIdx].neighbors[level];
            if (std::find(revNeighbors.begin(), revNeighbors.end(), nodeIdx)
                == revNeighbors.end()) {
                revNeighbors.push_back(nodeIdx);
            }

            // 裁剪反向邻居：使用 RND 多样性选择替代纯距离排序
            // 参考: Vamana robust pruning (NeurIPS 2019) + arxiv 2502.05575 (2025)
            // RND 保持方向多样性，构建更高质量的导航图，提升搜索效率
            if (static_cast<int>(revNeighbors.size()) > maxM) {
                std::vector<std::pair<float, size_t>> distPairs;
                distPairs.reserve(revNeighbors.size());
                for (size_t nIdx : revNeighbors) {
                    float d = vectorDistance(nodes_[neighborIdx].vector,
                                             nodes_[nIdx].vector);
                    distPairs.push_back({d, nIdx});
                }
                std::sort(distPairs.begin(), distPairs.end());

                auto diverseNeighbors = selectDiverseNeighbors(neighborIdx, distPairs, maxM);
                revNeighbors = std::move(diverseNeighbors);
            }
        }
    }

    // 裁剪正向邻居：同样使用 RND 多样性选择
    // 正向和反向都用 RND 才能保证图的整体导航质量一致
    if (static_cast<int>(nodeNeighbors.size()) > maxM) {
        std::vector<std::pair<float, size_t>> distPairs;
        distPairs.reserve(nodeNeighbors.size());
        for (size_t nIdx : nodeNeighbors) {
            float d = vectorDistance(nodes_[nodeIdx].vector,
                                     nodes_[nIdx].vector);
            distPairs.push_back({d, nIdx});
        }
        std::sort(distPairs.begin(), distPairs.end());

        auto diverseNeighbors = selectDiverseNeighbors(nodeIdx, distPairs, maxM);
        nodeNeighbors = std::move(diverseNeighbors);
    }
}

// ============================================================================
// 公开接口
// ============================================================================

void HNSWIndex::addVector(const std::string& id, const std::vector<float>& vec) {
    if (vec.empty()) return;

    std::unique_lock lock(mutex_);

    // 检查重复
    if (idMap_.count(id)) {
        LOG_WARN << "[HNSW] Duplicate vector id: " << id;
        return;
    }

    // 维度检查
    if (!nodes_.empty() && vec.size() != nodes_.front().vector.size()) {
        LOG_ERROR << "[HNSW] Dimension mismatch: expected="
                  << nodes_.front().vector.size() << ", got=" << vec.size();
        return;
    }

    int nodeLevel = randomLevel();
    size_t nodeIdx = nodes_.size();

    HNSWNode node;
    node.id = id;
    node.vector = vec;
    node.maxLevel = nodeLevel;
    node.neighbors.resize(nodeLevel + 1);
    nodes_.push_back(std::move(node));
    idMap_[id] = nodeIdx;

    // 第一个节点
    if (nodes_.size() == 1) {
        entryPoint_ = 0;
        maxLevel_ = nodeLevel;
        return;
    }

    // 从最高层贪心搜索到 nodeLevel+1 层
    size_t currentEntry = entryPoint_;
    for (int lv = maxLevel_; lv > nodeLevel; --lv) {
        auto neighborPairs = searchLayer(vec, currentEntry, 1, lv);
        if (!neighborPairs.empty()) {
            currentEntry = neighborPairs[0].second;
        }
    }

    // 在 nodeLevel 到第0层建立连接
    for (int lv = std::min(nodeLevel, maxLevel_); lv >= 0; --lv) {
        int efC = std::max(efConstruction_, m_);
        auto neighborPairs = searchLayer(vec, currentEntry, efC, lv);

        int maxM = (lv == 0) ? mMax0_ : m_;

        // 使用 RND 多样性选择初始邻居，而非简单取前 N 个最近邻
        // 构建阶段的邻居质量直接决定搜索时的图导航效率
        // 参考: Vamana (NeurIPS 2019) + arxiv 2502.05575 (2025)
        auto selectedNeighbors = selectDiverseNeighbors(nodeIdx, neighborPairs, maxM);

        connectNeighbors(nodeIdx, selectedNeighbors, lv, maxM);

        if (!neighborPairs.empty()) {
            currentEntry = neighborPairs[0].second;
        }
    }

    // 更新入口点
    if (nodeLevel > maxLevel_) {
        maxLevel_ = nodeLevel;
        entryPoint_ = nodeIdx;
    }
}

std::vector<VectorSearchResult> HNSWIndex::searchKNN(
    const std::vector<float>& query, int k) {
    if (query.empty()) return {};

    totalSearches_.fetch_add(1);

    std::shared_lock lock(mutex_);

    if (nodes_.empty()) return {};
    const size_t expectedDim = nodes_.front().vector.size();
    if (expectedDim == 0 || query.size() != expectedDim) {
        LOG_WARN << "[HNSW] Search dimension mismatch: expected="
                 << expectedDim << ", got=" << query.size();
        return {};
    }
    const int requestedK = std::max(1, k);

    // 从最高层贪心搜索到第1层
    size_t currentEntry = entryPoint_;
    for (int lv = maxLevel_; lv > 0; --lv) {
        auto nearest = searchLayer(query, currentEntry, 1, lv);
        if (!nearest.empty()) {
            currentEntry = nearest[0].second;
        }
    }

    // Distribution-Aware Ada-EF 自适应搜索宽度
    // 参考: "Distribution-Aware Adaptive Exploration" (arxiv 2512.06636, 2025)
    // 核心思想：用 pilot search 的距离分布拟合高斯模型，通过 z-score 量化查询难度，
    //          替代简单的 margin ratio 硬阈值，实现更精准的 per-query ef 调节。
    // 难查询（距离分布紧密、z-score 低）→ 增大 ef 保证召回率
    // 易查询（距离分布分散、z-score 高）→ 缩小 ef 降低延迟
    int baseEf = std::max(requestedK, efSearch_);
    int adaptiveEf = baseEf;
    if (nodes_.size() >= 256 && baseEf >= 24) {
        // 阶段1: 轻量 pilot search 采样距离分布
        const int pilotLo = 12;
        const int pilotHi = std::max(pilotLo, std::min(baseEf, 32));
        const int pilotEf = std::clamp(std::max(requestedK * 2, 12), pilotLo, pilotHi);
        auto pilotCandidates = searchLayer(query, currentEntry, pilotEf, 0);

        if (pilotCandidates.size() >= 4) {
            // 阶段2: 拟合距离分布的高斯参数（均值 μ、标准差 σ）
            const size_t n = pilotCandidates.size();
            float sumDist = 0.0f;
            float sumDistSq = 0.0f;
            for (const auto& [dist, _] : pilotCandidates) {
                sumDist += dist;
                sumDistSq += dist * dist;
            }
            const float mean = sumDist / static_cast<float>(n);
            const float variance = (sumDistSq / static_cast<float>(n)) - (mean * mean);
            const float stddev = std::sqrt(std::max(0.0f, variance));

            // 阶段3: 计算最近邻的 z-score = (mean - best) / σ
            // z-score 高 → best 远离分布中心 → 查询容易（最近邻很突出）
            // z-score 低 → best 接近分布中心 → 查询困难（候选距离相近，需更多探索）
            const float best = pilotCandidates[0].first;
            const float zScore = (stddev > 1e-8f)
                ? (mean - best) / stddev
                : 0.0f;

            // 阶段4: z-score → ef 缩放因子（分段线性映射）
            // z < 0.5: 极难查询，扩大 1.8×
            // 0.5 ≤ z < 1.5: 中等查询，线性插值 1.8× → 1.0×
            // 1.5 ≤ z < 3.0: 较易查询，线性插值 1.0× → 0.6×
            // z ≥ 3.0: 极易查询，缩小到 0.6×
            float scaleFactor;
            if (zScore < 0.5f) {
                scaleFactor = 1.8f;
            } else if (zScore < 1.5f) {
                // 线性插值: 0.5→1.8, 1.5→1.0
                scaleFactor = 1.8f - (zScore - 0.5f) * 0.8f;
            } else if (zScore < 3.0f) {
                // 线性插值: 1.5→1.0, 3.0→0.6
                scaleFactor = 1.0f - (zScore - 1.5f) * (0.4f / 1.5f);
            } else {
                scaleFactor = 0.6f;
            }

            // Hub 节点加成：高度数入口点在困难查询时需要更多探索
            // 参考: "Hub Highway Hypothesis" (arxiv 2412.01940, 2024)
            const int entryDegree = (currentEntry < nodes_.size() &&
                                     !nodes_[currentEntry].neighbors.empty())
                ? static_cast<int>(nodes_[currentEntry].neighbors[0].size())
                : 0;
            if (entryDegree >= mMax0_ && zScore < 1.0f) {
                scaleFactor *= 1.15f;
            }

            adaptiveEf = static_cast<int>(std::round(
                static_cast<float>(baseEf) * scaleFactor));
        }
        const int efCap = std::max(96, efSearch_ * 6);
        adaptiveEf = std::clamp(adaptiveEf, requestedK, std::max(requestedK, efCap));
    }
    auto candidates = searchLayer(query, currentEntry, adaptiveEf, 0);

    // 取 top-k
    if (static_cast<int>(candidates.size()) > requestedK) {
        candidates.resize(requestedK);
    }

    // 构建结果
    std::vector<VectorSearchResult> results;
    results.reserve(candidates.size());

    const float dim = std::max(1.0f, static_cast<float>(query.size()));
    for (const auto& [dist, idx] : candidates) {
        VectorSearchResult r;
        r.id = nodes_[idx].id;
        r.distance = std::sqrt(dist);
        r.similarity = std::exp(-dist / (2.0f * dim));
        results.push_back(std::move(r));
    }

    return results;
}

bool HNSWIndex::removeVector(const std::string& id) {
    std::unique_lock lock(mutex_);

    auto it = idMap_.find(id);
    if (it == idMap_.end()) return false;

    size_t idx = it->second;

    // 断开所有邻居连接
    for (int lv = 0; lv < static_cast<int>(nodes_[idx].neighbors.size()); ++lv) {
        for (size_t neighborIdx : nodes_[idx].neighbors[lv]) {
            if (lv < static_cast<int>(nodes_[neighborIdx].neighbors.size())) {
                auto& revNeighbors = nodes_[neighborIdx].neighbors[lv];
                revNeighbors.erase(
                    std::remove(revNeighbors.begin(), revNeighbors.end(), idx),
                    revNeighbors.end());
            }
        }
        nodes_[idx].neighbors[lv].clear();
    }

    // 清空向量数据（逻辑删除）
    nodes_[idx].vector.clear();
    nodes_[idx].id.clear();
    idMap_.erase(it);

    // 如果删除的是入口点，选择新的入口点
    if (idx == entryPoint_ && !idMap_.empty()) {
        entryPoint_ = idMap_.begin()->second;
    }

    return true;
}

std::vector<VectorSearchResult> HNSWIndex::rerankCandidates(
    const std::vector<float>& query,
    const std::vector<VectorSearchResult>& candidates,
    int topK) const {
    if (query.empty() || candidates.empty()) return {};

    std::shared_lock lock(mutex_);
    if (nodes_.empty()) return {};

    const size_t expectedDim = nodes_.front().vector.size();
    if (expectedDim == 0 || query.size() != expectedDim) {
        return {};
    }

    const int requestedTopK = std::max(1, topK);

    // Matryoshka 融合 cosine 重排序
    const size_t coarseDim = std::clamp<size_t>(64, 16, expectedDim);

    auto cosinePrefixFused = [](const std::vector<float>& a,
                                const std::vector<float>& b,
                                size_t fullDim, size_t cDim,
                                float& outFull, float& outCoarse) {
        float dot = 0.0f, normA = 0.0f, normB = 0.0f;
        float dotC = 0.0f, normAC = 0.0f, normBC = 0.0f;
        bool coarseDone = false;
        for (size_t i = 0; i < fullDim; ++i) {
            float ai = a[i], bi = b[i];
            dot += ai * bi;
            normA += ai * ai;
            normB += bi * bi;
            if (!coarseDone) {
                dotC += ai * bi;
                normAC += ai * ai;
                normBC += bi * bi;
                if (i + 1 == cDim) {
                    coarseDone = true;
                    float denomC = std::sqrt(normAC) * std::sqrt(normBC);
                    outCoarse = (denomC > 1e-12f) ? (dotC / denomC) : 0.0f;
                }
            }
        }
        float denom = std::sqrt(normA) * std::sqrt(normB);
        outFull = (denom > 1e-12f) ? (dot / denom) : 0.0f;
        if (!coarseDone) {
            outCoarse = outFull;
        }
    };

    struct RerankEntry {
        size_t origIdx;
        float fusedScore;
    };
    std::vector<RerankEntry> entries;
    entries.reserve(candidates.size());

    for (size_t ci = 0; ci < candidates.size(); ++ci) {
        auto idIt = idMap_.find(candidates[ci].id);
        if (idIt == idMap_.end()) continue;
        size_t nodeIdx = idIt->second;
        if (nodes_[nodeIdx].vector.empty()) continue;

        float fullCos = 0.0f, coarseCos = 0.0f;
        cosinePrefixFused(query, nodes_[nodeIdx].vector,
                          expectedDim, coarseDim, fullCos, coarseCos);
        float fused = 0.6f * fullCos + 0.4f * coarseCos;
        entries.push_back({ci, fused});
    }

    const int n = std::min(requestedTopK, static_cast<int>(entries.size()));
    std::partial_sort(entries.begin(), entries.begin() + n, entries.end(),
                      [](const RerankEntry& a, const RerankEntry& b) {
                          return a.fusedScore > b.fusedScore;
                      });

    std::vector<VectorSearchResult> result;
    result.reserve(n);
    for (int i = 0; i < n; ++i) {
        auto r = candidates[entries[i].origIdx];
        r.similarity = entries[i].fusedScore;
        result.push_back(std::move(r));
    }
    return result;
}

size_t HNSWIndex::getVectorCount() const {
    std::shared_lock lock(mutex_);
    return idMap_.size();
}

Json::Value HNSWIndex::getHNSWStats() const {
    std::shared_lock lock(mutex_);

    Json::Value stats;
    stats["total_vectors"] = static_cast<Json::UInt64>(nodes_.size());
    stats["active_vectors"] = static_cast<Json::UInt64>(idMap_.size());
    stats["max_level"] = maxLevel_;
    stats["m"] = m_;
    stats["m_max0"] = mMax0_;
    stats["ef_construction"] = efConstruction_;
    stats["ef_search"] = efSearch_;
    stats["total_searches"] = static_cast<Json::UInt64>(totalSearches_.load());

    // 每层节点数
    std::vector<int> counts(static_cast<size_t>(maxLevel_ + 1), 0);
    for (const auto& node : nodes_) {
        for (int lv = 0; lv <= node.maxLevel && lv <= maxLevel_; ++lv) {
            counts[static_cast<size_t>(lv)]++;
        }
    }
    Json::Value levelCounts(Json::arrayValue);
    for (int c : counts) {
        levelCounts.append(c);
    }
    stats["level_node_counts"] = levelCounts;

    // 平均邻居数（第0层）
    if (!nodes_.empty()) {
        double totalNeighbors = 0;
        for (const auto& node : nodes_) {
            if (!node.neighbors.empty()) {
                totalNeighbors += static_cast<double>(node.neighbors[0].size());
            }
        }
        stats["avg_neighbors_level0"] = totalNeighbors / static_cast<double>(nodes_.size());
    }

    return stats;
}

size_t HNSWIndex::getHNSWVectorDimension() const {
    std::shared_lock lock(mutex_);
    if (nodes_.empty()) return 0;
    return nodes_.front().vector.size();
}

}  // namespace heartlake::ai
