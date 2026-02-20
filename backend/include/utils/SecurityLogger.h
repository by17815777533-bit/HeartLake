/**
 * @file SecurityLogger.h
 * @brief SecurityLogger 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <string>
#include <drogon/HttpRequest.h>

namespace heartlake {
namespace utils {

/**
 * @brief 安全事件类型枚举
 */
enum class SecurityEventType {
    LOGIN_SUCCESS,           // 登录成功
    LOGIN_FAILED,           // 登录失败
    LOGIN_SUSPICIOUS,       // 可疑登录（新IP、新设备等）
    PASSWORD_CHANGED,       // 密码修改
    PASSWORD_RESET_REQUEST, // 密码重置请求
    PASSWORD_RESET_SUCCESS, // 密码重置成功
    ACCOUNT_LOCKED,         // 账号被锁定
    ACCOUNT_UNLOCKED,       // 账号解锁
    ACCOUNT_CREATED,        // 账号创建
    ACCOUNT_DELETED,        // 账号删除
    EMAIL_VERIFIED,         // 邮箱验证成功
    EMAIL_CHANGED,          // 邮箱修改
    VERIFICATION_CODE_SENT, // 验证码发送
    VERIFICATION_CODE_FAILED, // 验证码验证失败
    RATE_LIMIT_EXCEEDED,    // 速率限制触发
    UNAUTHORIZED_ACCESS,    // 未授权访问
    PERMISSION_DENIED,      // 权限拒绝
    TOKEN_EXPIRED,          // Token过期
    TOKEN_INVALID,          // Token无效
    SESSION_CREATED,        // 会话创建
    SESSION_TERMINATED,     // 会话终止
    SUSPICIOUS_ACTIVITY,    // 可疑活动
    DATA_EXPORT_REQUEST,    // 数据导出请求
    PRIVACY_SETTINGS_CHANGED, // 隐私设置修改
    TWO_FACTOR_ENABLED,     // 双因素认证启用
    TWO_FACTOR_DISABLED     // 双因素认证禁用
};

/**
 * @brief 安全事件严重程度
 */
enum class SecuritySeverity {
    LOW,      // 低：正常操作
    MEDIUM,   // 中：需要关注的操作
    HIGH,     // 高：可疑操作
    CRITICAL  // 严重：安全威胁
};

/**
 * @brief 安全日志记录器
 *
 * 提供统一的安全事件日志记录功能
 * 记录到数据库的security_events表
 */
class SecurityLogger {
public:
    /**
     * 记录安全事件
     * @param userId 用户ID（可选，某些事件可能没有用户ID）
     * @param eventType 事件类型
     * @param severity 严重程度
     * @param description 事件描述
     * @param ipAddress IP地址
     * @param userAgent User-Agent字符串
     * @param metadata 额外的元数据（JSON格式）
     */
    static void logEvent(const std::string& userId,
                        SecurityEventType eventType,
                        SecuritySeverity severity,
                        const std::string& description,
                        const std::string& ipAddress = "",
                        const std::string& userAgent = "",
                   const std::string& metadata = "{}");

    /**
     * 从HTTP请求记录安全事件
     * @param req HTTP请求对象
     * @param userId 用户ID
     * @param eventType 事件类型
     * @param severity 严重程度
     * @param description 事件描述
     * @param metadata 额外的元数据
     */
    static void logEventFromRequest(const drogon::HttpRequestPtr& req,
                                    const std::string& userId,
                                    SecurityEventType eventType,
                                    SecuritySeverity severity,
                                    const std::string& description,
                                    const std::string& metadata = "{}");

    /**
     * 记录登录成功事件
     */
    static void logLoginSuccess(const std::string& userId,
                               const std::string& ipAddress,
                               const std::string& userAgent);

    /**
     * 记录登录失败事件
     */
    static void logLoginFailed(const std::string& username,
                              const std::string& reason,
                              const std::string& ipAddress,
                              const std::string& userAgent);

    /**
     * 记录可疑登录事件
     */
    static void logSuspiciousLogin(const std::string& userId,
                                  const std::string& reason,
                                  const std::string& ipAddress,
                                  const std::string& userAgent);

    /**
     * 记录密码修改事件
     */
    static void logPasswordChanged(const std::string& userId,
                                  const std::string& ipAddress);

    /**
     * 记录账号锁定事件
     */
    static void logAccountLocked(const std::string& userId,
                                const std::string& reason,
                                const std::string& ipAddress);

    /**
     * 记录速率限制触发事件
     */
    static void logRateLimitExceeded(const std::string& endpoint,
                                    const std::string& ipAddress,
                                    const std::string& userId = "");

    /**
     * 记录未授权访问事件
     */
    static void logUnauthorizedAccess(const std::string& endpoint,
                                     const std::string& ipAddress,
                                     const std::string& reason);

    /**
     * 获取用户最近的安全事件
     * @param userId 用户ID
     * @param limit 返回数量限制
     * @return JSON格式的事件列表
     */
    static std::string getUserSecurityEvents(const std::string& userId, int limit = 10);

    /**
     * 检查用户是否有可疑活动
     * @param userId 用户ID
     * @param timeWindowMinutes 时间窗口（分钟）
     * @return 是否存在可疑活动
     */
    static bool hasSuspiciousActivity(const std::string& userId, int timeWindowMinutes = 60);

private:
    /**
     * 将事件类型转换为字符串
     */
    static std::string eventTypeToString(SecurityEventType type);

    /**
     * 将严重程度转换为字符串
     */
    static std::string severityToString(SecuritySeverity severity);

    /**
     * 从HTTP请求提取IP地址
     */
    static std::string extractIpAddress(const drogon::HttpRequestPtr& req);

    /**
     * 从HTTP请求提取User-Agent
     */
    static std::string extractUserAgent(const drogon::HttpRequestPtr& req);
};

} // namespace utils
} // namespace heartlake