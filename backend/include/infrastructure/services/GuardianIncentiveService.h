/**
 * @brief 守望者激励系统 -- 共鸣积分与成就体系
 *
 * @details
 * 鼓励用户积极参与社区互动的正向激励机制。当用户发出高质量涟漪
 * 或温暖纸船时，系统自动累加共鸣积分；积分达到阈值（100 分）后
 * 授予"守望者"成就，解锁"转赠灯火"等特殊能力。
 *
 * 积分规则：
 * - 高质量涟漪：+2 分（由内容审核引擎判定质量）
 * - 温暖纸船：+5 分（warmthScore > 0.7 时触发）
 *
 * 守望者可以将自己的"灯火"转赠给其他用户，
 * 被赠予灯火的用户会收到特殊的暖心通知。
 */

#pragma once

#include <string>
#include <vector>

namespace heartlake::infrastructure {

/// 用户的守望者状态快照
struct GuardianStats {
    std::string userId;
    int totalResonancePoints;  ///< 累计共鸣积分
    int qualityRipples;        ///< 高质量涟漪数
    int warmBoats;             ///< 温暖纸船数
    bool isGuardian;           ///< 是否已达成守望者成就
    bool canTransferLamp;      ///< 是否可以转赠灯火
};

class GuardianIncentiveService {
public:
    static GuardianIncentiveService& getInstance();

    /**
     * @brief 记录一次高质量涟漪，累加积分
     * @param userId 涟漪发送者
     * @param stoneId 涟漪所属的石头
     */
    void recordQualityRipple(const std::string& userId, const std::string& stoneId);

    /**
     * @brief 记录一次温暖纸船，累加积分
     * @param userId 纸船发送者
     * @param boatId 纸船 ID
     * @param warmthScore 温暖度评分 [0, 1]，超过 0.7 才计入
     */
    void recordWarmBoat(const std::string& userId, const std::string& boatId, float warmthScore);

    /// 获取用户当前的守望者状态
    GuardianStats getGuardianStats(const std::string& userId);

    /// 检查积分是否达标，达标则授予守望者成就
    bool checkAndGrantGuardian(const std::string& userId);

    /**
     * @brief 转赠灯火给另一位用户
     * @param fromUserId 赠予者（必须是守望者）
     * @param toUserId 接收者
     * @return 转赠是否成功
     */
    bool transferLamp(const std::string& fromUserId, const std::string& toUserId);

private:
    GuardianIncentiveService() = default;
    ~GuardianIncentiveService() = default;
    GuardianIncentiveService(const GuardianIncentiveService&) = delete;
    GuardianIncentiveService& operator=(const GuardianIncentiveService&) = delete;

    /// 内部积分累加，记录原因用于审计
    void addResonancePoints(const std::string& userId, int points, const std::string& reason);

    static constexpr int GUARDIAN_THRESHOLD = 100;  ///< 守望者成就所需积分
    static constexpr int RIPPLE_POINTS = 2;         ///< 每次高质量涟漪的积分
    static constexpr int WARM_BOAT_POINTS = 5;      ///< 每次温暖纸船的积分
};

} // namespace heartlake::infrastructure
