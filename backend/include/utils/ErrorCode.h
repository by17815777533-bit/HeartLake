/**
 * @brief 全局统一错误码定义
 *
 * 按模块分段编号：100xxx通用、200xxx认证、300xxx用户、400xxx内容、
 * 410xxx好友、500xxx智能引擎、600xxx数据库、700xxx网络。
 * 配套 ErrorCodeHelper 提供HTTP状态码映射和中英文消息。
 */

#pragma once
#include <string>
#include <unordered_map>

namespace heartlake {
namespace utils {

/// 全局统一错误码，按模块分段编号
enum class ErrorCode {
    SUCCESS = 200,
    
    INVALID_REQUEST = 100001,          // 无效请求
    INVALID_PARAMETER = 100002,        // 无效参数
    MISSING_PARAMETER = 100003,        // 缺少参数
    INTERNAL_ERROR = 100500,           // 内部错误
    SERVICE_UNAVAILABLE = 100503,      // 服务不可用
    
    UNAUTHORIZED = 200001,             // 未授权
    TOKEN_EXPIRED = 200002,            // Token过期
    TOKEN_INVALID = 200003,            // Token无效
    PERMISSION_DENIED = 200004,        // 权限不足
    LOGIN_REQUIRED = 200005,           // 需要登录
    
    USER_NOT_FOUND = 300001,           // 用户不存在
    USER_ALREADY_EXISTS = 300002,      // 用户已存在
    EMAIL_ALREADY_EXISTS = 300003,     // 邮箱已存在
    INVALID_EMAIL = 300004,            // 无效邮箱
    INVALID_PASSWORD = 300005,         // 无效密码
    PASSWORD_TOO_WEAK = 300006,        // 密码过弱
    VERIFICATION_CODE_INVALID = 300007,// 验证码无效
    VERIFICATION_CODE_EXPIRED = 300008,// 验证码过期
    EMAIL_NOT_VERIFIED = 300009,       // 邮箱未验证
    USER_SUSPENDED = 300010,           // 账户已停用
    USER_DELETED = 300011,             // 账户已删除
    
    CONTENT_NOT_FOUND = 400001,        // 内容不存在
    CONTENT_DELETED = 400002,          // 内容已删除
    CONTENT_TOO_LONG = 400003,         // 内容过长
    CONTENT_EMPTY = 400004,            // 内容为空
    CONTENT_SENSITIVE = 400005,        // 内容包含敏感词
    CONTENT_MODERATION_FAILED = 400006,// 内容审核失败
    RATE_LIMIT_EXCEEDED = 400007,      // 超过速率限制
    DUPLICATE_CONTENT = 400008,        // 重复内容
    
    FRIEND_NOT_FOUND = 410001,         // 好友不存在
    ALREADY_FRIENDS = 410002,          // 已经是好友
    FRIEND_REQUEST_EXISTS = 410003,    // 好友请求已存在
    CANNOT_ADD_SELF = 410004,          // 不能添加自己
    FRIEND_LIMIT_EXCEEDED = 410005,    // 好友数量超限
    
    AI_SERVICE_ERROR = 500001,         // AI服务错误
    AI_API_ERROR = 500002,             // AI API错误
    AI_TIMEOUT = 500003,               // AI超时
    AI_QUOTA_EXCEEDED = 500004,        // AI配额超限
    AI_INVALID_RESPONSE = 500005,      // AI返回无效
    
    DATABASE_ERROR = 600001,           // 数据库错误
    DATABASE_CONNECTION_FAILED = 600002,// 数据库连接失败
    DUPLICATE_KEY = 600003,            // 主键冲突
    FOREIGN_KEY_VIOLATION = 600004,    // 外键约束
    
    NETWORK_ERROR = 700001,            // 网络错误
    REQUEST_TIMEOUT = 700002,          // 请求超时
    UPSTREAM_ERROR = 700003,           // 上游服务错误
};

/// 多语言标识
enum class Language { ZH, EN };

/**
 * @brief 错误码辅助工具，提供HTTP状态码映射和多语言错误消息
 */
class ErrorCodeHelper {
public:
    /// 获取错误码对应的 HTTP 状态码
    static int getHttpStatus(ErrorCode code);
    /// 获取中文错误消息
    static std::string getMessageZh(ErrorCode code);
    /// 获取英文错误消息
    static std::string getMessageEn(ErrorCode code);
    /// 根据语言获取错误消息
    static std::string getMessage(ErrorCode code, Language lang);
    /// 获取错误码的整型值
    static int getCodeValue(ErrorCode code);
    /// 判断是否为成功码
    static bool isSuccess(ErrorCode code);
    /// 判断是否为客户端错误（4xx 类）
    static bool isClientError(ErrorCode code);
    /// 判断是否为服务端错误（5xx 类）
    static bool isServerError(ErrorCode code);

private:
    static const std::unordered_map<ErrorCode, int> httpStatusMap;           ///< 错误码 -> HTTP 状态码
    static const std::unordered_map<ErrorCode, std::string> messageZhMap;   ///< 错误码 -> 中文消息
    static const std::unordered_map<ErrorCode, std::string> messageEnMap;   ///< 错误码 -> 英文消息
};

} // namespace utils
} // namespace heartlake
