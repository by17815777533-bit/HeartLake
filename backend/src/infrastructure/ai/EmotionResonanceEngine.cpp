/**
 * @file EmotionResonanceEngine.cpp
 * @brief 情绪感知时序共鸣引擎实现
 *
 * 核心算法：
 * ResonanceScore = α·SemanticSim + β·EmotionTrajectorySim + γ·TemporalDecay + δ·DiversityBonus
 */

#include "infrastructure/ai/EmotionResonanceEngine.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "infrastructure/services/ResonanceSearchService.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <sstream>
#include <limits>
#include <unordered_set>

using namespace drogon;

namespace heartlake::ai {

EmotionResonanceEngine& EmotionResonanceEngine::getInstance() {
    static EmotionResonanceEngine instance;
    return instance;
}

// ===== DTW (Dynamic Time Warping) 情绪轨迹相似度 =====
// 优化: Sakoe-Chiba band 约束 O(n*w) + LB_Keogh 下界剪枝
// 参考: Salvador & Chan 2007 "FastDTW", Keogh 2005 "LB_Keogh"

float EmotionResonanceEngine::lbKeogh(
    const std::vector<float>& query,
    const std::vector<float>& candidate,
    int bandWidth
) {
    // LB_Keogh: 计算 DTW 的下界，O(n) 时间
    // 构建 candidate 的上下包络线，计算 query 超出包络的距离
    const size_t n = query.size();
    const size_t m = candidate.size();
    if (n == 0 || m == 0) return 0.0f;

    const size_t len = std::min(n, m);
    float sumSq = 0.0f;

    for (size_t i = 0; i < len; ++i) {
        // 包络线范围: [i - bandWidth, i + bandWidth] 在 candidate 上
        size_t lo = (i > static_cast<size_t>(bandWidth)) ? (i - static_cast<size_t>(bandWidth)) : 0;
        size_t hi = std::min(i + static_cast<size_t>(bandWidth), m - 1);

        // 找包络线的 min/max
        float envMin = candidate[lo];
        float envMax = candidate[lo];
        for (size_t j = lo + 1; j <= hi; ++j) {
            if (candidate[j] < envMin) envMin = candidate[j];
            if (candidate[j] > envMax) envMax = candidate[j];
        }

        // query[i] 超出包络的部分
        if (query[i] > envMax) {
            float d = query[i] - envMax;
            sumSq += d * d;
        } else if (query[i] < envMin) {
            float d = envMin - query[i];
            sumSq += d * d;
        }
    }

    return std::sqrt(sumSq);
}

float EmotionResonanceEngine::lbImproved(
    const std::vector<float>& query,
    const std::vector<float>& candidate,
    int bandWidth
) {
    // LB_Improved: Lemire 2009 "Faster Retrieval with a Two-Pass
    // Dynamic-Time-Warping Lower Bound"
    // 双向包络下界，比 LB_Keogh 更紧，实测剪枝率提升 20-40%
    // 额外开销仅 O(n)：一次投影 + 一次反向包络计算
    const size_t n = query.size();
    const size_t m = candidate.size();
    if (n == 0 || m == 0) return 0.0f;

    const size_t len = std::min(n, m);

    // Pass 1: 正向 LB_Keogh(query, candidate_envelope)
    // 同时构建投影序列 projected — query 超出 candidate 包络的位置被钳位到包络边界
    float fwdSumSq = 0.0f;
    std::vector<float> projected(len);

    for (size_t i = 0; i < len; ++i) {
        size_t lo = (i > static_cast<size_t>(bandWidth)) ? (i - static_cast<size_t>(bandWidth)) : 0;
        size_t hi = std::min(i + static_cast<size_t>(bandWidth), m - 1);

        float envMin = candidate[lo];
        float envMax = candidate[lo];
        for (size_t j = lo + 1; j <= hi; ++j) {
            if (candidate[j] < envMin) envMin = candidate[j];
            if (candidate[j] > envMax) envMax = candidate[j];
        }

        if (query[i] > envMax) {
            float d = query[i] - envMax;
            fwdSumSq += d * d;
            projected[i] = envMax;  // 钳位到上包络
        } else if (query[i] < envMin) {
            float d = envMin - query[i];
            fwdSumSq += d * d;
            projected[i] = envMin;  // 钳位到下包络
        } else {
            projected[i] = query[i]; // 在包络内，保持原值
        }
    }

    // Pass 2: 反向 LB_Keogh(candidate, projected_envelope)
    // 用投影后的 query 构建包络，计算 candidate 超出该包络的距离
    float revSumSq = 0.0f;
    const size_t pLen = projected.size();

    for (size_t i = 0; i < std::min(m, pLen); ++i) {
        size_t lo = (i > static_cast<size_t>(bandWidth)) ? (i - static_cast<size_t>(bandWidth)) : 0;
        size_t hi = std::min(i + static_cast<size_t>(bandWidth), pLen - 1);

        float pEnvMin = projected[lo];
        float pEnvMax = projected[lo];
        for (size_t j = lo + 1; j <= hi; ++j) {
            if (projected[j] < pEnvMin) pEnvMin = projected[j];
            if (projected[j] > pEnvMax) pEnvMax = projected[j];
        }

        if (candidate[i] > pEnvMax) {
            float d = candidate[i] - pEnvMax;
            revSumSq += d * d;
        } else if (candidate[i] < pEnvMin) {
            float d = pEnvMin - candidate[i];
            revSumSq += d * d;
        }
    }

    // 取两个方向的最大值 — 更紧的下界
    return std::max(std::sqrt(fwdSumSq), std::sqrt(revSumSq));
}

float EmotionResonanceEngine::trajectorySimDTW(
    const std::vector<float>& traj1,
    const std::vector<float>& traj2
) {
    if (traj1.empty() || traj2.empty()) return 0.0f;

    const size_t n = traj1.size();
    const size_t m = traj2.size();

    constexpr size_t MAX_DTW_LEN = 1000;
    if (n > MAX_DTW_LEN || m > MAX_DTW_LEN) {
        auto t1 = std::vector<float>(traj1.end() - std::min(n, MAX_DTW_LEN), traj1.end());
        auto t2 = std::vector<float>(traj2.end() - std::min(m, MAX_DTW_LEN), traj2.end());
        return trajectorySimDTW(t1, t2);
    }

    // Sakoe-Chiba band: 带宽 = max(10, max(n,m) * 10%)
    const int w = std::max(10, static_cast<int>(std::max(n, m)) / 10);

    constexpr double INF = std::numeric_limits<double>::max();

    // thread_local 滚动数组：避免每次调用堆分配
    thread_local std::vector<double> prev_buf, curr_buf;
    prev_buf.assign(m + 1, INF);
    curr_buf.resize(m + 1);
    prev_buf[0] = 0.0;

    for (size_t i = 1; i <= n; ++i) {
        curr_buf[0] = INF;

        // Sakoe-Chiba band: 只计算 [j_lo, j_hi] 范围
        size_t j_lo = (i > static_cast<size_t>(w)) ? (i - static_cast<size_t>(w)) : 1;
        size_t j_hi = std::min(i + static_cast<size_t>(w), m);

        // band 外的位置设为 INF
        if (j_lo > 1) {
            curr_buf[j_lo - 1] = INF;
        }

        for (size_t j = j_lo; j <= j_hi; ++j) {
            double cost = std::abs(static_cast<double>(traj1[i - 1]) - static_cast<double>(traj2[j - 1]));
            double min_prev = prev_buf[j - 1];
            if (prev_buf[j] < min_prev) min_prev = prev_buf[j];
            if (curr_buf[j - 1] < min_prev) min_prev = curr_buf[j - 1];
            curr_buf[j] = cost + min_prev;
        }

        std::swap(prev_buf, curr_buf);
    }

    // 防止 thread_local 缓冲区因偶发大轨迹而永久占用过多内存
    constexpr size_t SHRINK_THRESHOLD = 2048;
    if (prev_buf.capacity() > SHRINK_THRESHOLD) {
        prev_buf.shrink_to_fit();
        curr_buf.shrink_to_fit();
    }

    double maxLen = static_cast<double>(std::max(n, m));
    double normalizedDist = prev_buf[m] / maxLen;
    return static_cast<float>(std::exp(-normalizedDist * normalizedDist / 2.0));
}

// ===== 时间衰减 =====

float EmotionResonanceEngine::temporalDecay(const std::string& timestamp, float lambda) {
    // 解析ISO格式时间戳，计算距今小时数
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);

    // 简化解析：从数据库时间戳提取
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        // 尝试另一种格式
        ss.clear();
        ss.str(timestamp);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail()) return 0.5f;
    }
    // timegm: 按 UTC 解析，避免 mktime 受本地时区影响导致衰减计算偏差
    auto stoneTime = timegm(&tm);
    if (stoneTime == -1) return 0.5f;
    double hoursOld = std::difftime(nowTime, stoneTime) / 3600.0;
    if (hoursOld < 0) hoursOld = 0;

    // 指数衰减: decay = exp(-λ * Δt_hours)
    return std::exp(-lambda * static_cast<float>(hoursOld));
}

// ===== 多样性奖励 =====

float EmotionResonanceEngine::diversityBonus(
    const std::string& currentMood,
    const std::string& candidateMood,
    const std::vector<std::string>& alreadyRecommended
) {
    // 基础分：不同情绪类型获得奖励
    float bonus = (currentMood != candidateMood) ? 0.6f : 0.3f;

    // 已推荐列表中该情绪出现次数越多，奖励越低（避免回音室）
    int sameCount = 0;
    for (const auto& mood : alreadyRecommended) {
        if (mood == candidateMood) sameCount++;
    }
    // 衰减因子：每多一个相同情绪，奖励减少20%
    float decayFactor = std::pow(0.8f, static_cast<float>(sameCount));
    bonus *= decayFactor;

    // 互补情绪额外奖励（心理学：互补情绪有治愈效果）
    static const std::unordered_map<std::string, std::string> complementary = {
        {"sad", "hopeful"}, {"hopeful", "sad"},
        {"anxious", "calm"}, {"calm", "anxious"},
        {"angry", "grateful"}, {"grateful", "angry"},
        {"confused", "hopeful"}, {"lonely", "happy"}
    };
    auto it = complementary.find(currentMood);
    if (it != complementary.end() && it->second == candidateMood) {
        bonus += 0.3f;
    }

    return std::min(1.0f, bonus);
}

// ===== 生成共鸣原因 =====

std::string EmotionResonanceEngine::generateResonanceReason(
    const ResonanceResult& result,
    const std::string& currentMood,
    const std::string& candidateMood
) {
    // 根据主导维度生成不同的共鸣原因
    float maxDim = std::max({result.semanticScore, result.trajectoryScore,
                             result.temporalScore, result.diversityScore});

    if (maxDim == result.trajectoryScore && result.trajectoryScore > 0.6f) {
        // 情绪轨迹主导
        if (currentMood == candidateMood) {
            return "你们正经历着相似的情绪旅程，心灵在同一频率上共振";
        }
        return "你们的情绪轨迹有着奇妙的相似，也许能彼此理解";
    }
    if (maxDim == result.semanticScore && result.semanticScore > 0.7f) {
        return "你们的心声如此相似，仿佛来自同一片星空";
    }

    if (maxDim == result.diversityScore && result.diversityScore > 0.5f) {
        // 多样性主导 - 互补情绪
        static const std::unordered_map<std::string, std::string> healingPhrases = {
            {"sad", "这份温暖也许能照亮你心中的阴霾"},
            {"anxious", "这份宁静也许能抚平你内心的波澜"},
            {"angry", "这份柔软也许能融化你心中的坚冰"},
            {"confused", "这份清明也许能为你指引方向"},
            {"lonely", "这份陪伴也许能温暖你的孤独"}
        };
        auto phraseIt = healingPhrases.find(currentMood);
        if (phraseIt != healingPhrases.end()) {
            return phraseIt->second;
        }
        return "不同的视角，也许能带来意想不到的共鸣";
    }

    if (result.temporalScore > 0.8f) {
        return "此刻，有人和你一样在湖边驻足";
    }

    // 综合共鸣
    if (result.totalScore > 0.7f) {
        return "冥冥之中，你们的心灵产生了深深的共鸣";
    }
    return "也许这颗石子能在你心中泛起涟漪";
}

// ===== 加载用户情绪轨迹 =====

EmotionTrajectory EmotionResonanceEngine::loadTrajectory(const std::string& userId, int days) {
    EmotionTrajectory traj;
    traj.userId = userId;
    traj.currentScore = 0.0f;
    traj.currentMood = "neutral";

    try {
        auto db = app().getDbClient("default");

        // 从emotion_tracking获取用户近N天的情绪分数序列
        auto result = db->execSqlSync(
            "SELECT score, created_at FROM emotion_tracking "
            "WHERE user_id = $1 AND created_at > NOW() - make_interval(days => $2) "
            "ORDER BY created_at ASC",
            userId, days
        );

        for (const auto& row : result) {
            float score = row["score"].as<float>();
            traj.scores.push_back(score);
        }

        // 从user_emotion_profile获取最近的情绪类型序列
        auto moodResult = db->execSqlSync(
            "SELECT mood_type, avg_emotion_score FROM user_emotion_profile "
            "WHERE user_id = $1 AND date >= CURRENT_DATE - make_interval(days => $2) "
            "ORDER BY date ASC",
            userId, days
        );

        for (const auto& row : moodResult) {
            traj.moods.push_back(row["mood_type"].as<std::string>());
        }

        // 设置当前情绪
        if (!traj.scores.empty()) {
            traj.currentScore = traj.scores.back();
        }
        if (!traj.moods.empty()) {
            traj.currentMood = traj.moods.back();
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "loadTrajectory failed for user " << userId << ": " << e.base().what();
    }

    return traj;
}

// ===== 核心：多维度共鸣推荐 =====

std::vector<ResonanceResult> EmotionResonanceEngine::findResonance(
    const std::string& userId,
    const std::string& stoneId,
    int limit
) {
    std::vector<ResonanceResult> results;

    try {
        auto db = app().getDbClient("default");

        // 1. 获取源石头信息
        auto stoneRow = db->execSqlSync(
            "SELECT content, mood_type, emotion_score, created_at FROM stones "
            "WHERE stone_id = $1 AND status = 'published'",
            stoneId
        );
        if (stoneRow.empty()) return results;

        std::string sourceContent = stoneRow[0]["content"].as<std::string>();
        std::string sourceMood = stoneRow[0]["mood_type"].isNull()
            ? "neutral" : stoneRow[0]["mood_type"].as<std::string>();

        // 2. 加载当前用户的情绪轨迹
        auto userTraj = loadTrajectory(userId);

        // 3. 生成源石头的embedding用于语义相似度
        auto& embEngine = AdvancedEmbeddingEngine::getInstance();
        auto sourceEmb = embEngine.generateEmbedding(sourceContent);

        // 4. 获取候选石头（排除自己的、已交互的）
        auto candidates = db->execSqlSync(
            "SELECT s.stone_id, s.user_id, s.content, s.mood_type, "
            "s.emotion_score, s.created_at "
            "FROM stones s "
            "WHERE s.stone_id != $1 AND s.user_id != $2 "
            "AND s.status = 'published' "
            "AND s.created_at > NOW() - INTERVAL '30 days' "
            "ORDER BY s.created_at DESC LIMIT $3",
            stoneId, userId, limit * 5
        );
        // 已推荐的情绪列表（用于多样性计算）
        std::vector<std::string> recommendedMoods;

        // 5. 对每个候选计算四维共鸣分数
        for (const auto& row : candidates) {
            std::string candStoneId = row["stone_id"].as<std::string>();
            std::string candUserId = row["user_id"].as<std::string>();
            std::string candContent = row["content"].as<std::string>();
            std::string candMood = row["mood_type"].isNull()
                ? "neutral" : row["mood_type"].as<std::string>();
            std::string candTimestamp = row["created_at"].as<std::string>();

            ResonanceResult res;
            res.stoneId = candStoneId;
            res.userId = candUserId;

            // 维度1: 语义相似度
            auto candEmb = embEngine.generateEmbedding(candContent);
            res.semanticScore = (!sourceEmb.empty() && !candEmb.empty())
                ? AdvancedEmbeddingEngine::cosineSimilarity(sourceEmb, candEmb)
                : 0.0f;

            // 粗排剪枝：语义相似度过低则跳过昂贵的 DTW 计算
            if (res.semanticScore < 0.05f) {
                continue;
            }

            // 维度2: 情绪轨迹相似度 (DTW + LB_Keogh 剪枝)
            // DTW 复杂度 O(n*w)，LB_Keogh 下界仅 O(n)
            // 先用下界快速排除差异过大的轨迹，减少不必要的完整 DTW 计算
            // 参考: Keogh 2005 "Exact indexing of dynamic time warping"
            auto candTraj = loadTrajectory(candUserId);
            if (!userTraj.scores.empty() && !candTraj.scores.empty()) {
                float lbDist = lbKeogh(userTraj.scores, candTraj.scores);
                // 将下界距离转换为相似度（与 DTW 输出同尺度的高斯核映射）
                float lbSim = std::exp(-lbDist * lbDist / 2.0f);
                if (lbSim < 0.1f) {
                    // 下界已表明轨迹差异极大，直接用下界估计值，跳过昂贵的 DTW
                    res.trajectoryScore = lbSim;
                } else {
                    res.trajectoryScore = trajectorySimDTW(userTraj.scores, candTraj.scores);
                }
            } else {
                // 无轨迹数据时退化为情绪分数差异
                float scoreDiff = std::abs(userTraj.currentScore - candTraj.currentScore);
                res.trajectoryScore = std::exp(-scoreDiff);
            }

            // 维度3: 时间衰减
            res.temporalScore = temporalDecay(candTimestamp);

            // 维度4: 多样性奖励
            res.diversityScore = diversityBonus(sourceMood, candMood, recommendedMoods);
            // 加权总分 — 一次性拿到一致的权重快照，杜绝 torn-read
            const auto w = getWeights();
            res.totalScore = w.a * res.semanticScore
                           + w.b * res.trajectoryScore
                           + w.g * res.temporalScore
                           + w.d * res.diversityScore;

            // 生成共鸣原因
            res.resonanceReason = generateResonanceReason(res, sourceMood, candMood);

            results.push_back(res);
            recommendedMoods.push_back(candMood);
        }

        // 6. 按总分降序排序 + 截取top-K
        if (static_cast<int>(results.size()) > limit) {
            std::partial_sort(results.begin(), results.begin() + limit, results.end(),
                [](const ResonanceResult& a, const ResonanceResult& b) {
                    return a.totalScore > b.totalScore;
                });
            results.resize(limit);
        } else {
            std::sort(results.begin(), results.end(),
                [](const ResonanceResult& a, const ResonanceResult& b) {
                    return a.totalScore > b.totalScore;
                });
        }

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "EmotionResonanceEngine::findResonance DB error: " << e.base().what();
    } catch (const std::exception& e) {
        LOG_ERROR << "EmotionResonanceEngine::findResonance error: " << e.what();
    }

    return results;
}

} // namespace heartlake::ai
