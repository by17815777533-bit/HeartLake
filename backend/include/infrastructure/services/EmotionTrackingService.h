/**
 * 负重者监测系统 - 追踪用户72小时情绪状态
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

struct EmotionTrackingResult {
    std::string userId;
    int postCount72h;
    float avgNegativeScore;
    bool isExtremelyBurdened;
    std::string triggerReason;
};

class EmotionTrackingService {
public:
    static EmotionTrackingService& getInstance();

    void start();
    void stop();

    // 记录用户情绪数据点
    void recordEmotion(const std::string& userId, float score, const std::string& content);

    // 检查单个用户是否触发极度负重阈值
    EmotionTrackingResult checkUserBurden(const std::string& userId);

    // 手动触发一次全量扫描
    void scanOnce();

    // 设置触发回调
    void setTriggerCallback(std::function<void(const EmotionTrackingResult&)> callback);

private:
    EmotionTrackingService() = default;
    ~EmotionTrackingService();
    EmotionTrackingService(const EmotionTrackingService&) = delete;
    EmotionTrackingService& operator=(const EmotionTrackingService&) = delete;

    void scanLoop();
    void processUser(const std::string& userId);
    void triggerIntervention(const EmotionTrackingResult& result);

    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> scanThread_;
    std::recursive_mutex mutex_;
    std::mutex cvMutex_;
    std::condition_variable cv_;
    std::function<void(const EmotionTrackingResult&)> triggerCallback_;

    static constexpr int SCAN_INTERVAL_MINUTES = 30;
    static constexpr int TRACKING_HOURS = 72;
    static constexpr int MIN_POST_COUNT = 3;
    static constexpr float EXTREME_BURDEN_THRESHOLD = -0.6f;
};

} // namespace heartlake::infrastructure
