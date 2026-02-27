/**
 * 湖神守护服务 - 自动为零互动石头派送暖心纸船
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>

namespace heartlake::infrastructure {

class LakeGodGuardianService {
public:
    static LakeGodGuardianService& getInstance();

    void start();
    void stop();
    bool isRunning() const { return running_; }

    // 手动触发一次扫描（用于测试）
    void scanOnce();

private:
    LakeGodGuardianService() = default;
    ~LakeGodGuardianService();
    LakeGodGuardianService(const LakeGodGuardianService&) = delete;
    LakeGodGuardianService& operator=(const LakeGodGuardianService&) = delete;

    void scanLoop();
    void cleanStalePendingBoats();
    void processZeroInteractionStones();
    void sendAutoBoat(const std::string& stoneId, const std::string& stoneContent, const std::string& mood, const std::string& boatId);

    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> scanThread_;
    std::mutex mutex_;

    static constexpr int SCAN_INTERVAL_MINUTES = 15;
    static constexpr int ZERO_INTERACTION_THRESHOLD_HOURS = 2;
};

} // namespace heartlake::infrastructure
