/**
 * VIPService 模块接口定义
 */

#pragma once

#include <string>
#include <vector>
#include <drogon/orm/DbClient.h>

namespace heartlake {
namespace services {

/**
 * VIP服务类
 *
 * 处理VIP相关的业务逻辑，包括：
 * - 情绪检测自动升级VIP
 * - VIP特权管理
 * - VIP状态检查
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
