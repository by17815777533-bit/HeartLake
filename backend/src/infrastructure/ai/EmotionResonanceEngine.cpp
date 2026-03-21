/**
 * @file EmotionResonanceEngine.cpp
 * @brief 情绪感知时序共鸣引擎 —— 四维加权共鸣推荐
 *
 * 核心公式：
 *   ResonanceScore = α·SemanticSim + β·EmotionTrajectorySim + γ·TemporalDecay + δ·DiversityBonus
 *
 * 关键算法：
 *   - DTW (Dynamic Time Warping) 情绪轨迹相似度，Sakoe-Chiba band O(n*w)
 *   - LB_Keogh / LB_Improved 下界剪枝，O(n) 快速排除差异过大的轨迹
 *   - Early Abandoning DTW：以当前最优分数为阈值提前终止
 *   - 互补情绪多样性奖励（心理学治愈模型）
 *   - EMA 在线权重自适应，根据用户隐式反馈动态调整四维权重
 */

#include "infrastructure/ai/EmotionResonanceEngine.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <chrono>
#include <deque>
#include <sstream>
#include <limits>
#include <unordered_map>

using namespace drogon;

namespace heartlake::ai {

namespace {

std::vector<float> parsePgFloatArray(const std::string& raw) {
    std::vector<float> values;
    if (raw.size() < 2 || raw.front() != '{' || raw.back() != '}') {
        return values;
    }

    std::string token;
    token.reserve(raw.size());
    for (size_t i = 1; i + 1 < raw.size(); ++i) {
        const char ch = raw[i];
        if (ch == ',') {
            if (!token.empty() && token != "NULL") {
                try {
                    values.push_back(std::stof(token));
                } catch (const std::exception&) {
                }
            }
            token.clear();
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            token.push_back(ch);
        }
    }

    if (!token.empty() && token != "NULL") {
        try {
            values.push_back(std::stof(token));
        } catch (const std::exception&) {
        }
    }

    return values;
}

struct EnvelopeBounds {
    std::vector<float> lower;
    std::vector<float> upper;
};

EnvelopeBounds buildEnvelopeBounds(const std::vector<float>& series, int bandWidth) {
    EnvelopeBounds bounds;
    if (series.empty()) {
        return bounds;
    }

    const size_t n = series.size();
    const size_t radius = static_cast<size_t>(std::max(0, bandWidth));
    bounds.lower.resize(n);
    bounds.upper.resize(n);

    std::deque<size_t> minDeque;
    std::deque<size_t> maxDeque;
    size_t nextIndex = 0;

    auto pushIndex = [&](size_t idx) {
        while (!minDeque.empty() && series[minDeque.back()] >= series[idx]) {
            minDeque.pop_back();
        }
        minDeque.push_back(idx);

        while (!maxDeque.empty() && series[maxDeque.back()] <= series[idx]) {
            maxDeque.pop_back();
        }
        maxDeque.push_back(idx);
    };

    for (size_t i = 0; i < n; ++i) {
        const size_t lo = (i > radius) ? (i - radius) : 0;
        const size_t hi = std::min(i + radius, n - 1);

        while (nextIndex <= hi) {
            pushIndex(nextIndex++);
        }

        while (!minDeque.empty() && minDeque.front() < lo) {
            minDeque.pop_front();
        }
        while (!maxDeque.empty() && maxDeque.front() < lo) {
            maxDeque.pop_front();
        }

        bounds.lower[i] = series[minDeque.front()];
        bounds.upper[i] = series[maxDeque.front()];
    }

    return bounds;
}

float lbKeoghFromBounds(
    const std::vector<float>& query,
    const EnvelopeBounds& bounds
) {
    const size_t len = std::min(query.size(), bounds.lower.size());
    float sumSq = 0.0f;
    for (size_t i = 0; i < len; ++i) {
        if (query[i] > bounds.upper[i]) {
            const float d = query[i] - bounds.upper[i];
            sumSq += d * d;
        } else if (query[i] < bounds.lower[i]) {
            const float d = bounds.lower[i] - query[i];
            sumSq += d * d;
        }
    }
    return std::sqrt(sumSq);
}

}  // namespace

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

    return lbKeoghFromBounds(query, buildEnvelopeBounds(candidate, bandWidth));
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
    const auto candidateBounds = buildEnvelopeBounds(candidate, bandWidth);

    // Pass 1: 正向 LB_Keogh(query, candidate_envelope)
    // 同时构建投影序列 projected — query 超出 candidate 包络的位置被钳位到包络边界
    std::vector<float> projected(len);

    for (size_t i = 0; i < len; ++i) {
        const float envMin = candidateBounds.lower[i];
        const float envMax = candidateBounds.upper[i];
        if (query[i] > envMax) {
            projected[i] = envMax;  // 钳位到上包络
        } else if (query[i] < envMin) {
            projected[i] = envMin;  // 钳位到下包络
        } else {
            projected[i] = query[i]; // 在包络内，保持原值
        }
    }

    // Pass 2: 反向 LB_Keogh(candidate, projected_envelope)
    // 用投影后的 query 构建包络，计算 candidate 超出该包络的距离
    const auto projectedBounds = buildEnvelopeBounds(projected, bandWidth);
    const float fwdLb = lbKeoghFromBounds(query, candidateBounds);
    const float revLb = lbKeoghFromBounds(candidate, projectedBounds);

    // 取两个方向的最大值 — 更紧的下界
    return std::max(fwdLb, revLb);
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
        auto t1 = std::vector<float>(traj1.end() - static_cast<ptrdiff_t>(std::min(n, MAX_DTW_LEN)), traj1.end());
        auto t2 = std::vector<float>(traj2.end() - static_cast<ptrdiff_t>(std::min(m, MAX_DTW_LEN)), traj2.end());
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

float EmotionResonanceEngine::trajectorySimDTW_EA(
    const std::vector<float>& traj1,
    const std::vector<float>& traj2,
    float bestSoFar
) {
    if (traj1.empty() || traj2.empty()) return 0.0f;

    const size_t n = traj1.size();
    const size_t m = traj2.size();
    const int w = std::max(10, static_cast<int>(std::max(n, m)) / 10);
    constexpr double INF = std::numeric_limits<double>::max();

    // bestSoFar 是相似度，映射到距离阈值后用于 early abandoning。
    // sim = exp(-d^2 / 2) => d = sqrt(-2 * ln(sim))
    double abandonThreshold = INF;
    if (bestSoFar > 1e-6f && bestSoFar < 0.999999f) {
        const double normalizedDistThreshold =
            std::sqrt(-2.0 * std::log(static_cast<double>(bestSoFar)));
        abandonThreshold =
            normalizedDistThreshold * static_cast<double>(std::max(n, m));
    }

    thread_local std::vector<double> prev_buf_ea, curr_buf_ea;
    prev_buf_ea.assign(m + 1, INF);
    curr_buf_ea.resize(m + 1);
    prev_buf_ea[0] = 0.0;

    for (size_t i = 1; i <= n; ++i) {
        curr_buf_ea[0] = INF;

        size_t j_lo = (i > static_cast<size_t>(w)) ? (i - static_cast<size_t>(w)) : 1;
        size_t j_hi = std::min(i + static_cast<size_t>(w), m);
        if (j_lo > 1) curr_buf_ea[j_lo - 1] = INF;

        double rowMin = INF;
        for (size_t j = j_lo; j <= j_hi; ++j) {
            const double cost = std::abs(
                static_cast<double>(traj1[i - 1]) - static_cast<double>(traj2[j - 1]));
            double minPrev = prev_buf_ea[j - 1];
            if (prev_buf_ea[j] < minPrev) minPrev = prev_buf_ea[j];
            if (curr_buf_ea[j - 1] < minPrev) minPrev = curr_buf_ea[j - 1];
            curr_buf_ea[j] = cost + minPrev;
            if (curr_buf_ea[j] < rowMin) rowMin = curr_buf_ea[j];
        }

        // 若当前行最小累计代价已超阈值，可提前终止。
        if (rowMin > abandonThreshold) {
            return 0.0f;
        }

        std::swap(prev_buf_ea, curr_buf_ea);
    }

    constexpr size_t SHRINK_THRESHOLD = 2048;
    if (prev_buf_ea.capacity() > SHRINK_THRESHOLD) {
        prev_buf_ea.shrink_to_fit();
        curr_buf_ea.shrink_to_fit();
    }

    const double normalizedDist = prev_buf_ea[m] / static_cast<double>(std::max(n, m));
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

        float latestProfileScore = 0.0f;
        for (const auto& row : moodResult) {
            traj.moods.push_back(row["mood_type"].as<std::string>());
            latestProfileScore = row["avg_emotion_score"].isNull()
                ? latestProfileScore
                : row["avg_emotion_score"].as<float>();
        }

        // 设置当前情绪
        if (!traj.scores.empty()) {
            traj.currentScore = traj.scores.back();
        } else if (!moodResult.empty()) {
            traj.currentScore = latestProfileScore;
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
    limit = std::max(1, limit);

    try {
        auto db = app().getDbClient("default");

        // 1. 获取源石头信息
        auto stoneRow = db->execSqlSync(
            "SELECT content, mood_type, emotion_score, created_at FROM stones "
            "WHERE stone_id = $1 AND status = 'published' AND deleted_at IS NULL",
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

        // 4. 获取候选石头，并批量带出作者近 7 天轨迹，避免按候选逐条查库。
        auto candidates = db->execSqlSync(
            "WITH candidate_stones AS ("
            "  SELECT s.stone_id, s.user_id, s.content, "
            "         COALESCE(s.mood_type, 'neutral') AS mood_type, "
            "         COALESCE(s.emotion_score, 0.0) AS emotion_score, "
            "         COALESCE(s.ripple_count, 0) AS ripple_count, "
            "         s.created_at, "
            "         EXP(-0.1 * EXTRACT(EPOCH FROM (NOW() - s.created_at)) / 3600.0) AS temporal_score "
            "  FROM stones s "
            "  WHERE s.stone_id != $1 AND s.user_id != $2 "
            "    AND s.status = 'published' "
            "    AND s.deleted_at IS NULL "
            "    AND s.created_at > NOW() - INTERVAL '30 days' "
            "    AND NOT EXISTS ("
            "      SELECT 1 FROM user_interaction_history h "
            "      WHERE h.user_id = $2 AND h.stone_id = s.stone_id"
            "    ) "
            "  ORDER BY s.created_at DESC "
            "  LIMIT $3"
            "), candidate_users AS ("
            "  SELECT DISTINCT user_id FROM candidate_stones"
            "), trajectory_scores AS ("
            "  SELECT et.user_id, ARRAY_AGG(et.score ORDER BY et.created_at ASC) AS scores "
            "  FROM emotion_tracking et "
            "  JOIN candidate_users cu ON cu.user_id = et.user_id "
            "  WHERE et.created_at > NOW() - INTERVAL '7 days' "
            "  GROUP BY et.user_id"
            "), latest_moods AS ("
            "  SELECT DISTINCT ON (uep.user_id) "
            "         uep.user_id, "
            "         COALESCE(uep.mood_type, 'neutral') AS current_mood, "
            "         COALESCE(uep.avg_emotion_score, 0.0) AS current_score "
            "  FROM user_emotion_profile uep "
            "  JOIN candidate_users cu ON cu.user_id = uep.user_id "
            "  WHERE uep.date >= CURRENT_DATE - INTERVAL '7 days' "
            "  ORDER BY uep.user_id, uep.date DESC"
            ") "
            "SELECT cs.stone_id, cs.user_id, cs.content, cs.mood_type, "
            "       cs.emotion_score, cs.ripple_count, cs.created_at, "
            "       cs.temporal_score, "
            "       u.nickname AS author_name, "
            "       COALESCE(ts.scores::text, '{}') AS trajectory_scores, "
            "       COALESCE(lm.current_mood, 'neutral') AS current_mood, "
            "       COALESCE(lm.current_score, 0.0) AS current_score "
            "FROM candidate_stones cs "
            "JOIN users u ON u.user_id = cs.user_id "
            "LEFT JOIN trajectory_scores ts ON ts.user_id = cs.user_id "
            "LEFT JOIN latest_moods lm ON lm.user_id = cs.user_id "
            "ORDER BY cs.created_at DESC",
            stoneId, userId, limit * 5
        );
        // 已推荐的情绪列表（用于多样性计算）
        std::vector<std::string> recommendedMoods;
        recommendedMoods.reserve(candidates.size());
        results.reserve(std::min(candidates.size(), static_cast<size_t>(limit)));
        float bestTrajectoryScore = 0.0f;
        std::unordered_map<std::string, EmotionTrajectory> trajectoryCache;
        trajectoryCache.reserve(candidates.size());
        const auto weights = getWeights();

        // 5. 对每个候选计算四维共鸣分数
        for (const auto& row : candidates) {
            std::string candStoneId = row["stone_id"].as<std::string>();
            std::string candUserId = row["user_id"].as<std::string>();
            std::string candContent = row["content"].as<std::string>();
            std::string candMood = row["mood_type"].as<std::string>();
            std::string candTimestamp = row["created_at"].as<std::string>();

            ResonanceResult res;
            res.stoneId = candStoneId;
            res.userId = candUserId;
            res.content = candContent;
            res.moodType = candMood;
            res.emotionScore = row["emotion_score"].as<float>();
            res.authorName = row["author_name"].as<std::string>();
            res.rippleCount = row["ripple_count"].as<int>();
            res.createdAt = candTimestamp;

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
            auto trajIt = trajectoryCache.find(candUserId);
            if (trajIt == trajectoryCache.end()) {
                EmotionTrajectory candTraj;
                candTraj.userId = candUserId;
                candTraj.scores = parsePgFloatArray(row["trajectory_scores"].as<std::string>());
                candTraj.currentMood = row["current_mood"].as<std::string>();
                candTraj.currentScore = row["current_score"].as<float>();
                trajIt = trajectoryCache.emplace(candUserId, std::move(candTraj)).first;
            }
            const auto& candTraj = trajIt->second;
            if (!userTraj.scores.empty() && !candTraj.scores.empty()) {
                const float lbDist = lbImproved(userTraj.scores, candTraj.scores);
                // 将下界距离转换为相似度（与 DTW 输出同尺度高斯核映射）
                const float lbSim = std::exp(-lbDist * lbDist / 2.0f);
                if (lbSim < 0.1f) {
                    // 下界已表明轨迹差异极大，直接用下界估计值，跳过昂贵的 DTW
                    res.trajectoryScore = lbSim;
                } else {
                    // 以当前最优轨迹分数作为 early-abandon 参考阈值，降低 tail-latency
                    const float eaFloor = std::max(0.08f, bestTrajectoryScore * 0.85f);
                    const float dtwEaSim = trajectorySimDTW_EA(
                        userTraj.scores, candTraj.scores, eaFloor);
                    res.trajectoryScore = std::max(lbSim, dtwEaSim);
                }
            } else {
                // 无轨迹数据时退化为情绪分数差异
                float scoreDiff = std::abs(userTraj.currentScore - candTraj.currentScore);
                res.trajectoryScore = std::exp(-scoreDiff);
            }

            // 维度3: 时间衰减
            res.temporalScore = row["temporal_score"].isNull()
                ? temporalDecay(candTimestamp)
                : row["temporal_score"].as<float>();

            // 维度4: 多样性奖励
            res.diversityScore = diversityBonus(sourceMood, candMood, recommendedMoods);
            res.totalScore = weights.a * res.semanticScore
                           + weights.b * res.trajectoryScore
                           + weights.g * res.temporalScore
                           + weights.d * res.diversityScore;

            // 生成共鸣原因
            res.resonanceReason = generateResonanceReason(res, sourceMood, candMood);

            results.push_back(res);
            recommendedMoods.push_back(candMood);
            if (res.trajectoryScore > bestTrajectoryScore) {
                bestTrajectoryScore = res.trajectoryScore;
            }
        }

        // 6. 按总分降序排序 + 截取top-K
        if (static_cast<int>(results.size()) > limit) {
            std::partial_sort(results.begin(), results.begin() + static_cast<ptrdiff_t>(limit), results.end(),
                [](const ResonanceResult& a, const ResonanceResult& b) {
                    return a.totalScore > b.totalScore;
                });
            results.resize(static_cast<size_t>(limit));
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

// EMA 在线权重自适应 — 根据用户隐式反馈动态调整四维权重
// 参考: "MultiSentimentArcs" (Frontiers 2024) 时序情绪自适应建模
void EmotionResonanceEngine::updateWeightsEMA(const ResonanceWeights& feedback,
                                               float learningRate) {
    auto cur = getWeights();

    // EMA 混合: w_new = (1 - lr) * w_cur + lr * feedback
    float a = (1.0f - learningRate) * cur.a + learningRate * feedback.a;
    float b = (1.0f - learningRate) * cur.b + learningRate * feedback.b;
    float g = (1.0f - learningRate) * cur.g + learningRate * feedback.g;
    float d = (1.0f - learningRate) * cur.d + learningRate * feedback.d;

    // 归一化保证 α+β+γ+δ = 1.0
    float sum = a + b + g + d;
    if (sum > 1e-6f) {
        a /= sum;
        b /= sum;
        g /= sum;
        d /= sum;
    } else {
        // 退化情况：回退到默认权重
        a = 0.30f; b = 0.35f; g = 0.20f; d = 0.15f;
    }

    setWeights(a, b, g, d);
}

} // namespace heartlake::ai
