/**
 * VIP 会员服务 -- 情绪驱动的会员权益管理
 *
 * HeartLake 的 VIP 机制与传统付费会员不同：当系统检测到用户持续处于
 * 低落情绪状态时，会自动赠送限时 VIP 权益（免费心理咨询、AI 陪伴加速等），
 * 体现"越需要帮助的人越应该获得更多资源"的产品理念。
 *
 * 核心功能：
 *   - 情绪驱动升级：基于 emotionScore 阈值自动触发 VIP 赠送
 *   - 权益管理：按 privilegeKey 粒度控制各项特权的使用与配额
 *   - 批量扫描：定时任务批量检测用户情绪状态，触发自动升级
 *   - 咨询预约：VIP 用户可使用免费额度预约心理咨询
 *
 * @note 全部方法为 static，无状态设计，数据持久化依赖 Drogon ORM。
 */

#pragma once

#include <string>
#include <vector>
#include <drogon/orm/DbClient.h>

namespace heartlake {
namespace services {

/**
 * @brief VIP 会员服务（静态工具类）
 *
 * @details 提供情绪驱动的 VIP 自动升级、权益查询与使用记录、
 *          心理咨询预约等功能。所有方法均为 static，
 *          通过 Drogon ORM 直接操作数据库。
 */
class VIPService {
public:
    /**
     * 检查用户情绪并可能自动赠送VIP
     *
     * @param userId 用户ID
     * @param emotionScore 情绪分数 (-1到1，负数表示负面情绪)
     * @param emotionTags 情绪标签数组
     * @return 是否成功赠送VIP
     */
    static bool checkEmotionAndGrantVIP(
        const std::string& userId,
        float emotionScore,
        const std::vector<std::string>& emotionTags
    );

    /**
     * 检查用户是否有特定VIP特权
     *
     * @param userId 用户ID
     * @param privilegeKey 特权键名
     * @return 是否拥有该特权
     */
    static bool hasPrivilege(
        const std::string& userId,
        const std::string& privilegeKey
    );

    /**
     * 获取用户的所有VIP特权
     *
     * @param userId 用户ID
     * @return 特权列表
     */
    static std::vector<std::string> getUserPrivileges(const std::string& userId);

    /**
     * 检查VIP是否过期
     *
     * @param userId 用户ID
     * @return 是否过期
     */
    static bool isVIPExpired(const std::string& userId);

    /**
     * 手动升级用户VIP
     *
     * @param userId 用户ID
     * @param vipLevel VIP等级
     * @param durationDays 有效期（天）
     * @param reason 升级原因
     * @return 是否成功
     */
    static bool upgradeVIP(
        const std::string& userId,
        int vipLevel,
        int durationDays,
        const std::string& reason
    );

    /**
     * 批量检查用户情绪并自动赠送VIP
     *
     * @return 成功赠送的用户数量
     */
    static int batchCheckEmotionsForVIP();

    /**
     * 获取用户VIP状态信息
     *
     * @param userId 用户ID
     * @return VIP状态JSON对象
     */
    static Json::Value getVIPStatus(const std::string& userId);

    /**
     * 检查并记录VIP权益使用
     *
     * @param userId 用户ID
     * @param privilegeKey 权益键名
     * @param metadata 元数据（可选）
     * @return 是否可以使用该权益
     */
    static bool checkAndRecordPrivilegeUsage(
        const std::string& userId,
        const std::string& privilegeKey,
        const std::string& metadata = ""
    );

    /**
     * 获取AI评论频率（小时）
     *
     * @param userId 用户ID
     * @return 评论频率（小时），VIP用户返回更短的时间
     */
    static float getAICommentFrequency(const std::string& userId);

    /**
     * 检查用户是否有免费心理咨询额度
     *
     * @param userId 用户ID
     * @return 是否有免费额度
     */
    static bool hasFreeCounselingQuota(const std::string& userId);

    /**
     * 预约心理咨询
     *
     * @param userId 用户ID
     * @param appointmentTime 预约时间
     * @param isFreeVIP 是否使用VIP免费额度
     * @return 预约ID，失败返回空字符串
     */
    static std::string bookCounseling(
        const std::string& userId,
        const std::string& appointmentTime,
        bool isFreeVIP = false
    );

private:
    /**
     * 判断是否为负面情绪
     *
     * @param emotionScore 情绪分数
     * @param emotionTags 情绪标签
     * @return 是否为负面情绪
     */
    static bool isLowEmotion(
        float emotionScore,
        const std::vector<std::string>& emotionTags
    );

    /**
     * 发送VIP升级通知
     *
     * @param userId 用户ID
     * @param vipLevel VIP等级
     * @param reason 升级原因
     */
    static void sendVIPNotification(
        const std::string& userId,
        int vipLevel,
        const std::string& reason
    );
};

} // namespace services
} // namespace heartlake
