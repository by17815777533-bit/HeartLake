/**
 * @brief 差分隐私情绪聚合引擎
 *
 * @details
 * 基于 2025 年 FedMultiEmo / FED-PsyAU 论文的隐私保护机制，
 * 对用户情绪数据进行差分隐私保护后的聚合统计。核心保证：
 *
 * - 对情绪数据添加 Laplace 噪声，满足 epsilon-差分隐私定义
 * - 聚合统计不泄露任何单个用户的情绪状态
 * - 隐私预算 (epsilon) 累计追踪，防止无限查询导致隐私泄露
 *
 * 噪声量由 sensitivity / epsilon 决定：epsilon 越小隐私保护越强，
 * 但统计精度越低。推荐 epsilon 范围 1.0~3.0。
 *
 * @note 参考论文：FedMultiEmo (arXiv:2507.15470, July 2025)
 */
#pragma once

#include <string>
#include <vector>
#include <random>
#include <json/json.h>
#include <mutex>

namespace heartlake::privacy {

/// 差分隐私保护后的情绪聚合统计结果
struct PrivateEmotionStats {
    double avgScore;            ///< 加噪后的平均情绪分数
    double moodDistribution[7]; ///< 7 种情绪的加噪分布 (happy, sad, angry, fearful, surprised, neutral, calm)
    int totalUsers;             ///< 加噪后的参与用户数
    double privacyBudget;       ///< 本次查询消耗的隐私预算 epsilon
    std::string timestamp;      ///< 统计时间戳
};

/**
 * @brief 差分隐私引擎，全局单例
 *
 * @details
 * 所有公开方法均线程安全（内部 mutex 保护）。
 * 每次查询都会消耗隐私预算，通过 getPrivacyReport() 可查看累计消耗。
 */
class DifferentialPrivacyEngine {
public:
    static DifferentialPrivacyEngine& getInstance();

    /**
     * @brief 使用差分隐私聚合情绪统计
     * @param epsilon 隐私预算参数（越小越隐私，推荐 1.0~3.0）
     * @param timeWindowHours 统计的时间窗口（小时）
     * @return 加噪后的聚合统计
     */
    PrivateEmotionStats aggregateWithPrivacy(double epsilon = 1.0, int timeWindowHours = 24);

    /**
     * @brief 对单个数值添加 Laplace 噪声
     * @param value 原始值
     * @param sensitivity 查询的全局敏感度 (L1 sensitivity)
     * @param epsilon 隐私预算
     * @return 加噪后的值
     */
    double addLaplaceNoise(double value, double sensitivity, double epsilon);

    /**
     * @brief 获取湖面情绪天气（隐私保护版）
     * @param epsilon 隐私预算，默认 2.0
     * @return 包含天气描述和情绪分布的 JSON
     * @note 替代原有的直接统计接口，所有数据均经过差分隐私处理
     */
    Json::Value getPrivateLakeWeather(double epsilon = 2.0);

    /**
     * @brief 获取隐私预算消耗报告
     * @return 包含累计 epsilon 消耗、查询次数等信息的 JSON
     */
    Json::Value getPrivacyReport();

private:
    DifferentialPrivacyEngine();
    std::mt19937 rng_;                    ///< Mersenne Twister 随机数生成器
    std::mutex mutex_;                    ///< 保护 rng_ 和预算计数器的并发访问
    double totalEpsilonConsumed_ = 0.0;   ///< 累计消耗的隐私预算
    int queryCount_ = 0;                  ///< 累计查询次数

    /// 从 Laplace(0, scale) 分布中采样
    double laplaceSample(double scale);
};

} // namespace heartlake::privacy
