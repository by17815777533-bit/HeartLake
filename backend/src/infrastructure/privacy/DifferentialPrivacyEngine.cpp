/**
 * 差分隐私情绪聚合引擎实现
 *
 * 基于拉普拉斯机制实现 ε-差分隐私保证。
 * 核心思路：对情绪统计的每个维度（均值、用户数、情绪分布）分别注入
 * 校准噪声，使得单个用户的加入或移除不会显著改变输出分布。
 * epsilon 预算按组合定理分配：30% 给均值、20% 给用户数、50% 给 7 类情绪分布。
 *
 * 参考：FedMultiEmo (arXiv:2507.15470, July 2025)
 */

#include "infrastructure/privacy/DifferentialPrivacyEngine.h"
#include <drogon/drogon.h>
#include <cmath>
#include <chrono>
#include <algorithm>

namespace heartlake::privacy {

// 情绪标签，与 PrivateEmotionStats::moodDistribution 下标对应
static const std::vector<std::string> kMoodLabels = {
    "happy", "sad", "angry", "fearful", "surprised", "neutral", "calm"
};

DifferentialPrivacyEngine::DifferentialPrivacyEngine()
    : rng_(std::random_device{}()) {}

DifferentialPrivacyEngine& DifferentialPrivacyEngine::getInstance() {
    static DifferentialPrivacyEngine instance;
    return instance;
}

// ==================== 拉普拉斯采样 ====================

double DifferentialPrivacyEngine::laplaceSample(double scale) {
    // 标准拉普拉斯分布采样：Lap(0, scale)
    // 使用逆CDF方法：X = -scale * sign(U) * ln(1 - 2|U|), U ~ Uniform(-0.5, 0.5)
    std::uniform_real_distribution<double> dist(-0.5, 0.5);
    double u = dist(rng_);
    // 避免 log(0)
    double absU = std::abs(u);
    if (absU >= 0.5) absU = std::nextafter(0.5, 0.0);
    return -scale * (u > 0 ? 1.0 : -1.0) * std::log(1.0 - 2.0 * absU);
}

double DifferentialPrivacyEngine::addLaplaceNoise(double value, double sensitivity, double epsilon) {
    if (epsilon <= 0) epsilon = 1.0;
    double scale = sensitivity / epsilon;
    std::lock_guard<std::mutex> lock(mutex_);
    return value + laplaceSample(scale);
}

// ==================== 差分隐私聚合 ====================

PrivateEmotionStats DifferentialPrivacyEngine::aggregateWithPrivacy(double epsilon, int timeWindowHours) {
    PrivateEmotionStats stats{};
    stats.privacyBudget = epsilon;

    // 时间戳
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    stats.timestamp = buf;

    try {
        auto db = drogon::app().getDbClient("default");
        if (!db) {
            // 无数据库连接，返回空统计
            return stats;
        }
        // 1) 查询时间窗口内的平均情绪分数和用户数
        auto avgResult = db->execSqlSync(
            "SELECT COUNT(DISTINCT user_id) AS user_count, "
            "       COALESCE(AVG(emotion_score), 0) AS avg_score "
            "FROM stones "
            "WHERE status = 'published' "
            "  AND created_at > NOW() - make_interval(hours => $1)",
            timeWindowHours
        );

        double rawAvgScore = 0.0;
        int rawUserCount = 0;
        if (!avgResult.empty()) {
            rawUserCount = avgResult[0]["user_count"].as<int>();
            rawAvgScore = avgResult[0]["avg_score"].isNull()
                ? 0.0 : avgResult[0]["avg_score"].as<double>();
        }

        // 2) 查询各情绪类型分布
        auto moodResult = db->execSqlSync(
            "SELECT mood_type, COUNT(*) AS cnt "
            "FROM stones "
            "WHERE status = 'published' "
            "  AND created_at > NOW() - make_interval(hours => $1) "
            "GROUP BY mood_type",
            timeWindowHours
        );

        double rawDistribution[7] = {0};
        int totalStones = 0;
        for (const auto& row : moodResult) {
            std::string mood = row["mood_type"].as<std::string>();
            int cnt = row["cnt"].as<int>();
            totalStones += cnt;
            for (size_t i = 0; i < kMoodLabels.size(); ++i) {
                if (kMoodLabels[i] == mood) {
                    rawDistribution[i] = static_cast<double>(cnt);
                    break;
                }
            }
        }

        // 3) 添加拉普拉斯噪声
        // 敏感度分析：
        //   - avgScore: 单用户对平均值的影响 ≤ 2/N (分数范围[-1,1])，取 sensitivity=2.0
        //   - userCount: 单用户影响 = 1
        //   - 每个mood计数: 单用户影响 = 1
        // 将 epsilon 按组合定理分配给各查询
        double epsAvg = epsilon * 0.3;
        double epsCount = epsilon * 0.2;
        double epsMood = epsilon * 0.5 / 7.0; // 7个情绪类型均分

        {
            std::lock_guard<std::mutex> lock(mutex_);

            stats.avgScore = rawAvgScore + laplaceSample(2.0 / epsAvg);
            stats.avgScore = std::clamp(stats.avgScore, -1.0, 1.0);

            double noisyCount = rawUserCount + laplaceSample(1.0 / epsCount);
            stats.totalUsers = std::max(0, static_cast<int>(std::round(noisyCount)));

            for (int i = 0; i < 7; ++i) {
                double noisy = rawDistribution[i] + laplaceSample(1.0 / epsMood);
                stats.moodDistribution[i] = std::max(0.0, noisy);
            }

            // 归一化分布为比例
            double sum = 0;
            for (int i = 0; i < 7; ++i) sum += stats.moodDistribution[i];
            if (sum > 0) {
                for (int i = 0; i < 7; ++i) stats.moodDistribution[i] /= sum;
            }

            // 记录隐私预算消耗
            totalEpsilonConsumed_ += epsilon;
            queryCount_++;
        }

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "DifferentialPrivacyEngine aggregation failed: " << e.base().what();
    }

    return stats;
}

// ==================== 隐私保护湖面天气 ====================

Json::Value DifferentialPrivacyEngine::getPrivateLakeWeather(double epsilon) {
    auto stats = aggregateWithPrivacy(epsilon, 1); // 1小时窗口，与原始getLakeWeather一致

    Json::Value weather;
    weather["privacy_protected"] = true;
    weather["epsilon"] = epsilon;
    weather["timestamp"] = stats.timestamp;
    weather["total_users_approx"] = stats.totalUsers;

    // 找到加噪后占比最高的情绪
    int dominantIdx = 0;
    for (int i = 1; i < 7; ++i) {
        if (stats.moodDistribution[i] > stats.moodDistribution[dominantIdx]) {
            dominantIdx = i;
        }
    }
    std::string dominant = kMoodLabels[dominantIdx];

    // 映射情绪到天气
    if (dominant == "happy") {
        weather["weather"] = "sunny";
        weather["description"] = "湖面晴朗，大家心情愉悦（隐私保护统计）";
        weather["emoji"] = "☀️";
    } else if (dominant == "sad") {
        weather["weather"] = "rainy";
        weather["description"] = "湖面微雨，有人心情忧郁（隐私保护统计）";
        weather["emoji"] = "🌧️";
    } else if (dominant == "angry") {
        weather["weather"] = "stormy";
        weather["description"] = "湖面风暴，情绪激烈（隐私保护统计）";
        weather["emoji"] = "⛈️";
    } else if (dominant == "fearful") {
        weather["weather"] = "foggy";
        weather["description"] = "湖面起雾，有人感到不安（隐私保护统计）";
        weather["emoji"] = "🌫️";
    } else if (dominant == "surprised") {
        weather["weather"] = "rainbow";
        weather["description"] = "湖面彩虹，充满惊喜（隐私保护统计）";
        weather["emoji"] = "🌈";
    } else if (dominant == "calm") {
        weather["weather"] = "calm";
        weather["description"] = "湖面平静，岁月静好（隐私保护统计）";
        weather["emoji"] = "🌊";
    } else {
        weather["weather"] = "mixed";
        weather["description"] = "湖面波光粼粼，情绪交织（隐私保护统计）";
        weather["emoji"] = "✨";
    }

    // 加噪后的情绪分布
    Json::Value dist(Json::objectValue);
    for (int i = 0; i < 7; ++i) {
        dist[kMoodLabels[i]] = std::round(stats.moodDistribution[i] * 1000) / 1000.0;
    }
    weather["mood_distribution"] = dist;
    weather["avg_emotion_score"] = std::round(stats.avgScore * 1000) / 1000.0;

    return weather;
}

// ==================== 隐私预算报告 ====================

Json::Value DifferentialPrivacyEngine::getPrivacyReport() {
    std::lock_guard<std::mutex> lock(mutex_);

    Json::Value report;
    report["total_epsilon_consumed"] = totalEpsilonConsumed_;
    report["query_count"] = queryCount_;
    report["avg_epsilon_per_query"] = queryCount_ > 0
        ? totalEpsilonConsumed_ / queryCount_ : 0.0;
    report["privacy_level"] = totalEpsilonConsumed_ < 10.0
        ? "strong" : (totalEpsilonConsumed_ < 50.0 ? "moderate" : "weak");
    report["recommendation"] = totalEpsilonConsumed_ > 50.0
        ? "建议重置隐私预算或降低查询频率" : "隐私预算充足";

    return report;
}

} // namespace heartlake::privacy
