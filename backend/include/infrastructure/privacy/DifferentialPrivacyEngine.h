/**
 * 差分隐私情绪聚合引擎
 *
 * 创新点：基于2025年FedMultiEmo/FED-PsyAU论文的隐私保护机制
 * - 对情绪数据添加拉普拉斯噪声，保护个体隐私
 * - 支持ε-差分隐私保证
 * - 聚合统计不泄露任何单个用户的情绪状态
 *
 * 参考论文：FedMultiEmo (arXiv:2507.15470, July 2025)
 */
#pragma once

#include <string>
#include <vector>
#include <random>
#include <json/json.h>
#include <mutex>

namespace heartlake::privacy {

struct PrivateEmotionStats {
    // 差分隐私保护后的聚合统计
    double avgScore;           // 加噪后的平均情绪分数
    double moodDistribution[7]; // 7种情绪的加噪分布 (happy, sad, angry, fearful, surprised, neutral, calm)
    int totalUsers;            // 加噪后的参与用户数
    double privacyBudget;      // 已消耗的隐私预算 ε
    std::string timestamp;
};

class DifferentialPrivacyEngine {
public:
    static DifferentialPrivacyEngine& getInstance();

    /**
     * 使用差分隐私聚合情绪统计
     * @param epsilon 隐私预算参数（越小越隐私，推荐1.0-3.0）
     * @param timeWindowHours 时间窗口（小时）
     * @return 加噪后的聚合统计
     */
    PrivateEmotionStats aggregateWithPrivacy(double epsilon = 1.0, int timeWindowHours = 24);

    /**
     * 对单个值添加拉普拉斯噪声
     * @param value 原始值
     * @param sensitivity 查询敏感度
     * @param epsilon 隐私预算
     * @return 加噪后的值
     */
    double addLaplaceNoise(double value, double sensitivity, double epsilon);

    /**
     * 获取湖面情绪天气（隐私保护版）
     * 替代原有的直接统计，使用差分隐私
     */
    Json::Value getPrivateLakeWeather(double epsilon = 2.0);

    /**
     * 获取隐私预算消耗报告
     */
    Json::Value getPrivacyReport();

private:
    DifferentialPrivacyEngine();
    std::mt19937 rng_;
    std::mutex mutex_;
    double totalEpsilonConsumed_ = 0.0;
    int queryCount_ = 0;

    double laplaceSample(double scale);
};

} // namespace heartlake::privacy
