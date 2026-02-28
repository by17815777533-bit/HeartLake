/**
 * @brief 负重者监测系统 -- 追踪用户 72 小时情绪状态，识别极度负重用户
 *
 * @details
 * 后台守护线程，每 30 分钟全量扫描一次用户的近 72 小时情绪数据。
 * 当某用户在窗口期内发布 >= 3 条石头且平均负面分数低于 -0.6 时，
 * 判定为"极度负重"状态，触发干预流程：
 *
 * 1. 通过 triggerCallback_ 通知上层（通常是 SafeHarborService）
 * 2. SafeHarborService 向用户推送暖心提示和危机资源
 * 3. EmotionTrackingService 记录干预事件，供 UserFollowUpService 后续回访
 *
 * 也支持实时模式：每次用户发布石头时调用 recordEmotion()，
 * 立即检查该用户是否触发阈值，无需等待定时扫描。
 *
 * 线程安全：scanThread_ 独立运行，通过 condition_variable 实现
 * 可中断的定时等待，stop() 时能快速唤醒并退出。
 */

#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace heartlake::infrastructure {

/// 单个用户的情绪负重评估结果
struct EmotionTrackingResult {
    std::string userId;
    int postCount72h;            ///< 72 小时内的发帖数
    float avgNegativeScore;      ///< 平均负面情绪分数
    bool isExtremelyBurdened;    ///< 是否达到极度负重阈值
    std::string triggerReason;   ///< 触发原因描述
};

class EmotionTrackingService {
public:
    static EmotionTrackingService& getInstance();

    /// 启动后台扫描线程
    void start();
    /// 停止扫描线程（通过 condition_variable 快速唤醒）
    void stop();

    /**
     * @brief 记录用户情绪数据点（实时模式入口）
     * @param userId 用户 ID
     * @param score 情绪分数 [-1, 1]
     * @param content 石头内容（用于日志和分析）
     */
    void recordEmotion(const std::string& userId, float score, const std::string& content);

    /**
     * @brief 检查单个用户是否触发极度负重阈值
     * @param userId 用户 ID
     * @return 评估结果，包含是否触发和触发原因
     */
    EmotionTrackingResult checkUserBurden(const std::string& userId);

    /// 手动触发一次全量扫描（用于测试）
    void scanOnce();

    /**
     * @brief 设置极度负重触发时的回调
     * @param callback 回调函数，参数为评估结果
     */
    void setTriggerCallback(std::function<void(const EmotionTrackingResult&)> callback);

private:
    EmotionTrackingService() = default;
    ~EmotionTrackingService();
    EmotionTrackingService(const EmotionTrackingService&) = delete;
    EmotionTrackingService& operator=(const EmotionTrackingService&) = delete;

    /// 扫描主循环
    void scanLoop();
    /// 处理单个用户的情绪评估
    void processUser(const std::string& userId);
    /// 触发干预流程
    void triggerIntervention(const EmotionTrackingResult& result);

    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> scanThread_;
    std::recursive_mutex mutex_;
    std::mutex cvMutex_;            ///< 配合 condition_variable 使用
    std::condition_variable cv_;    ///< 用于可中断的定时等待
    std::function<void(const EmotionTrackingResult&)> triggerCallback_;

    static constexpr int SCAN_INTERVAL_MINUTES = 30;          ///< 扫描间隔（分钟）
    static constexpr int TRACKING_HOURS = 72;                 ///< 情绪追踪窗口（小时）
    static constexpr int MIN_POST_COUNT = 3;                  ///< 最少发帖数才触发评估
    static constexpr float EXTREME_BURDEN_THRESHOLD = -0.6f;  ///< 极度负重的平均分阈值
};

} // namespace heartlake::infrastructure
