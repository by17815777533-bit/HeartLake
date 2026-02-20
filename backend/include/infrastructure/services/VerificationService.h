/**
 * @file VerificationService.h
 * @brief VerificationService 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <string>
#include <chrono>

namespace heartlake {
namespace services {

/**
 * @brief 验证码服务类
 *
 * 提供安全的验证码生成、存储和验证功能
 * 使用Redis进行持久化存储，支持TTL和速率限制
 */
class VerificationService {
public:
    /**
     * 生成并存储验证码
     * @param email 邮箱地址
     * @param purpose 验证码用途 ("registration", "password_reset", "email_change")
     * @return 生成的6位数字验证码
     * @throws std::runtime_error 如果速率限制触发或Redis错误
     */
    static std::string generateAndStoreCode(const std::string& email,
                                          const std::string& purpose = "registration");

    /**
     * 验证验证码
     * @param email 邮箱地址
     * @param code 用户输入的验证码
     * @param purpose 验证码用途
     * @return 验证是否成功
     */
    static bool verifyCode(const std::string& email,
                          const std::string& code,
                          const std::string& purpose = "registration");

    /**
     * 检查发送速率限制
     * @param email 邮箱地址
     * @return 是否可以发送（true=可以发送，false=需要等待）
     */
    static bool checkRateLimit(const std::string& email);

    /**
     * 获取剩余冷却时间
     * @param email 邮箱地址
     * @return 剩余秒数，0表示可以立即发送
     */
    static int getRemainingCooldown(const std::string& email);

    /**
     * 清理过期的验证码（定期清理任务）
     */
    static void cleanupExpiredCodes();

    /**
     * 删除验证码
     * @param email 邮箱地址
     * @param purpose 验证码用途
     */
    static void deleteCode(const std::string& email, const std::string& purpose);

private:
    // Redis键前缀
    static constexpr const char* CODE_PREFIX = "verification_code:";
    static constexpr const char* RATE_LIMIT_PREFIX = "verification_rate:";
    static constexpr const char* COUNT_PREFIX = "verification_count:";

    // 配置常量
    static constexpr int CODE_LENGTH = 6;           // 验证码长度
    static constexpr int CODE_TTL_SECONDS = 600;    // 验证码有效期（10分钟）
    static constexpr int RATE_LIMIT_SECONDS = 60;   // 发送间隔（60秒）
    static constexpr int MAX_DAILY_COUNT = 10;      // 每日最大发送次数
    static constexpr int DAILY_TTL_SECONDS = 86400; // 24小时

    /**
     * 生成随机验证码
     * @return 6位数字验证码
     */
    static std::string generateCode();

    /**
     * 构建Redis键
     * @param email 邮箱地址
     * @param purpose 用途
     * @param prefix 键前缀
     * @return Redis键
     */
    static std::string buildRedisKey(const std::string& email,
                                   const std::string& purpose,
                                   const std::string& prefix);

    /**
     * 检查每日发送次数限制
     * @param email 邮箱地址
     * @return 是否超过限制
     */
    static bool checkDailyLimit(const std::string& email);

    /**
     * 增加每日发送计数
     * @param email 邮箱地址
     */
    static void incrementDailyCount(const std::string& email);
};

} // namespace services
} // namespace heartlake