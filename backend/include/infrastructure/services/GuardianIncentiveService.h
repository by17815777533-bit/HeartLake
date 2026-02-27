/**
 * 守望者激励系统 - 共鸣积分与成就
 */

#pragma once

#include <string>
#include <vector>

namespace heartlake::infrastructure {

struct GuardianStats {
    std::string userId;
    int totalResonancePoints;
    int qualityRipples;
    int warmBoats;
    bool isGuardian;
    bool canTransferLamp;
};

class GuardianIncentiveService {
public:
    static GuardianIncentiveService& getInstance();

    // 记录高质量涟漪
    void recordQualityRipple(const std::string& userId, const std::string& stoneId);

    // 记录温暖纸船
    void recordWarmBoat(const std::string& userId, const std::string& boatId, float warmthScore);

    // 获取用户守望者状态
    GuardianStats getGuardianStats(const std::string& userId);

    // 检查并授予守望者成就
    bool checkAndGrantGuardian(const std::string& userId);

    // 转赠灯火
    bool transferLamp(const std::string& fromUserId, const std::string& toUserId);

private:
    GuardianIncentiveService() = default;
    ~GuardianIncentiveService() = default;
    GuardianIncentiveService(const GuardianIncentiveService&) = delete;
    GuardianIncentiveService& operator=(const GuardianIncentiveService&) = delete;

    void addResonancePoints(const std::string& userId, int points, const std::string& reason);

    static constexpr int GUARDIAN_THRESHOLD = 100;
    static constexpr int RIPPLE_POINTS = 2;
    static constexpr int WARM_BOAT_POINTS = 5;
};

} // namespace heartlake::infrastructure
