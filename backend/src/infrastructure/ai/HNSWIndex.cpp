/**
 * @file HNSWIndex.cpp
 * @brief HNSW 向量检索引擎实现
 *
 * 从 EdgeAIEngine 拆分的独立子系统。
 * 支持多层图结构、Ada-EF 自适应搜索、Matryoshka 融合 cosine 重排序。
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
    entryPoint_ = 0;
    maxLevel_ = 0;
}

// ============================================================================
// 内部算法
// ============================================================================

int HNSWIndex::randomLevel() {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r = dist(rng_);
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

std::vector<std::pair<float, size_t>> HNSWIndex::searchLayer(
    const std::vector<float>& query, size_t entryPoint, int ef, int level) const {

    using DistIdx = std::pair<float, size_t>;

    // 结果集：max-heap（距离最大的在顶部）
    std::priority_queue<DistIdx> results;
    // 候选集：min-heap（距离最小的在顶部）
    std::priority_queue<DistIdx, std::vector<DistIdx>, std::greater<DistIdx>> candidates;

    // 位图替代 unordered_set，避免哈希开销
    std::vector<bool> visited(nodes_.size(), false);

    float entryDist = vectorDistance(query, nodes_[entryPoint].vector);
    results.push({entryDist, entryPoint});
    candidates.push({entryDist, entryPoint});
    visited[entryPoint] = true;

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

            // prefetch 邻居向量数据到 L1 cache
            for (size_t ni = 0; ni < neighborList.size() && ni < 8; ++ni) {
                size_t nIdx = neighborList[ni];
                if (!visited[nIdx]) {
                    __builtin_prefetch(nodes_[nIdx].vector.data(), 0, 1);
                }
            }

            worstResult = results.top().first;
            const bool resultsFull = static_cast<int>(results.size()) >= ef;

            for (size_t neighborIdx : neighborList) {
                if (visited[neighborIdx]) continue;
                visited[neighborIdx] = true;

                float neighborDist = vectorDistance(query, nodes_[neighborIdx].vector);

                if (!resultsFull || neighborDist < worstResult) {
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

            // 裁剪：如果反向邻居超过 maxM，保留最近的
            if (static_cast<int>(revNeighbors.size()) > maxM) {
                std::vector<std::pair<float, size_t>> distPairs;
                distPairs.reserve(revNeighbors.size());
                for (size_t nIdx : revNeighbors) {
                    float d = vectorDistance(nodes_[neighborIdx].vector,
                                             nodes_[nIdx].vector);
                    distPairs.push_back({d, nIdx});
                }
                std::partial_sort(distPairs.begin(), distPairs.begin() + maxM, distPairs.end());

                revNeighbors.clear();
                for (int i = 0; i < maxM; ++i) {
                    revNeighbors.push_back(distPairs[i].second);
                }
            }
        }
    }

    // 裁剪正向邻居
    if (static_cast<int>(nodeNeighbors.size()) > maxM) {
        std::vector<std::pair<float, size_t>> distPairs;
        distPairs.reserve(nodeNeighbors.size());
        for (size_t nIdx : nodeNeighbors) {
            float d = vectorDistance(nodes_[nodeIdx].vector,
                                     nodes_[nIdx].vector);
            distPairs.push_back({d, nIdx});
        }
        std::partial_sort(distPairs.begin(), distPairs.begin() + maxM, distPairs.end());

        nodeNeighbors.clear();
        for (int i = 0; i < maxM; ++i) {
            nodeNeighbors.push_back(distPairs[i].second);
        }
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
        int numConnect = std::min(maxM, static_cast<int>(neighborPairs.size()));

        std::vector<size_t> selectedNeighbors;
        selectedNeighbors.reserve(numConnect);
        for (int i = 0; i < numConnect; ++i) {
            selectedNeighbors.push_back(neighborPairs[i].second);
        }

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

    // 在第0层执行自适应搜索（Ada-EF思路）
    int baseEf = std::max(requestedK, efSearch_);
    int adaptiveEf = baseEf;
    if (nodes_.size() >= 256 && baseEf >= 24) {
        const int pilotLo = 12;
        const int pilotHi = std::max(pilotLo, std::min(baseEf, 32));
        const int pilotEf = std::clamp(std::max(requestedK * 2, 12), pilotLo, pilotHi);
        auto pilotCandidates = searchLayer(query, currentEntry, pilotEf, 0);
        if (pilotCandidates.size() >= 2) {
            const float best = pilotCandidates[0].first;
            const float secondBest = pilotCandidates[1].first;
            const float marginRatio = (secondBest - best) / (std::abs(best) + 1e-6f);

            if (marginRatio < 0.08f) {
                adaptiveEf = static_cast<int>(std::ceil(baseEf * 1.6f));
            } else if (marginRatio > 0.20f) {
                adaptiveEf = static_cast<int>(std::floor(baseEf * 0.75f));
            }

            const int entryDegree = (currentEntry < nodes_.size() &&
                                     !nodes_[currentEntry].neighbors.empty())
                ? static_cast<int>(nodes_[currentEntry].neighbors[0].size())
                : 0;
            if (entryDegree >= mMax0_ && marginRatio < 0.12f) {
                adaptiveEf = static_cast<int>(std::ceil(adaptiveEf * 1.2f));
            }
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
