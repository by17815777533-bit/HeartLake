/**
 * @file ErrorCode.cpp
 * @brief ErrorCode 模块实现
 * Created by 林子怡
 */
#include "utils/ErrorCode.h"

namespace heartlake {
namespace utils {

const std::unordered_map<ErrorCode, int> ErrorCodeHelper::httpStatusMap = {
    {ErrorCode::SUCCESS, 200},
    {ErrorCode::INVALID_REQUEST, 400},
    {ErrorCode::INVALID_PARAMETER, 400},
    {ErrorCode::MISSING_PARAMETER, 400},
    {ErrorCode::INTERNAL_ERROR, 500},
    {ErrorCode::SERVICE_UNAVAILABLE, 503},
    
    {ErrorCode::UNAUTHORIZED, 401},
    {ErrorCode::TOKEN_EXPIRED, 401},
    {ErrorCode::TOKEN_INVALID, 401},
    {ErrorCode::PERMISSION_DENIED, 403},
    {ErrorCode::LOGIN_REQUIRED, 401},
    
    {ErrorCode::USER_NOT_FOUND, 404},
    {ErrorCode::USER_ALREADY_EXISTS, 409},
    {ErrorCode::EMAIL_ALREADY_EXISTS, 409},
    {ErrorCode::INVALID_EMAIL, 400},
    {ErrorCode::INVALID_PASSWORD, 400},
    {ErrorCode::PASSWORD_TOO_WEAK, 400},
    {ErrorCode::VERIFICATION_CODE_INVALID, 400},
    {ErrorCode::VERIFICATION_CODE_EXPIRED, 400},
    {ErrorCode::EMAIL_NOT_VERIFIED, 403},
    {ErrorCode::USER_SUSPENDED, 403},
    {ErrorCode::USER_DELETED, 410},
    
    {ErrorCode::CONTENT_NOT_FOUND, 404},
    {ErrorCode::CONTENT_DELETED, 410},
    {ErrorCode::CONTENT_TOO_LONG, 400},
    {ErrorCode::CONTENT_EMPTY, 400},
    {ErrorCode::CONTENT_SENSITIVE, 400},
    {ErrorCode::CONTENT_MODERATION_FAILED, 400},
    {ErrorCode::RATE_LIMIT_EXCEEDED, 429},
    {ErrorCode::DUPLICATE_CONTENT, 409},
    
    {ErrorCode::FRIEND_NOT_FOUND, 404},
    {ErrorCode::ALREADY_FRIENDS, 409},
    {ErrorCode::FRIEND_REQUEST_EXISTS, 409},
    {ErrorCode::CANNOT_ADD_SELF, 400},
    {ErrorCode::FRIEND_LIMIT_EXCEEDED, 400},
    
    {ErrorCode::AI_SERVICE_ERROR, 500},
    {ErrorCode::AI_API_ERROR, 502},
    {ErrorCode::AI_TIMEOUT, 504},
    {ErrorCode::AI_QUOTA_EXCEEDED, 429},
    {ErrorCode::AI_INVALID_RESPONSE, 502},
    
    {ErrorCode::DATABASE_ERROR, 500},
    {ErrorCode::DATABASE_CONNECTION_FAILED, 503},
    {ErrorCode::DUPLICATE_KEY, 409},
    {ErrorCode::FOREIGN_KEY_VIOLATION, 400},
    
    {ErrorCode::NETWORK_ERROR, 502},
    {ErrorCode::REQUEST_TIMEOUT, 504},
    {ErrorCode::UPSTREAM_ERROR, 502},
};

const std::unordered_map<ErrorCode, std::string> ErrorCodeHelper::messageZhMap = {
    {ErrorCode::SUCCESS, "成功"},
    {ErrorCode::INVALID_REQUEST, "无效的请求"},
    {ErrorCode::INVALID_PARAMETER, "参数无效"},
    {ErrorCode::MISSING_PARAMETER, "缺少必需参数"},
    {ErrorCode::INTERNAL_ERROR, "服务器内部错误"},
    {ErrorCode::SERVICE_UNAVAILABLE, "服务暂时不可用"},
    
    {ErrorCode::UNAUTHORIZED, "未授权"},
    {ErrorCode::TOKEN_EXPIRED, "登录已过期，请重新登录"},
    {ErrorCode::TOKEN_INVALID, "无效的登录凭证"},
    {ErrorCode::PERMISSION_DENIED, "权限不足"},
    {ErrorCode::LOGIN_REQUIRED, "请先登录"},
    
    {ErrorCode::USER_NOT_FOUND, "用户不存在"},
    {ErrorCode::USER_ALREADY_EXISTS, "用户已存在"},
    {ErrorCode::EMAIL_ALREADY_EXISTS, "邮箱已被注册"},
    {ErrorCode::INVALID_EMAIL, "邮箱格式不正确"},
    {ErrorCode::INVALID_PASSWORD, "密码不正确"},
    {ErrorCode::PASSWORD_TOO_WEAK, "密码强度不够"},
    {ErrorCode::VERIFICATION_CODE_INVALID, "验证码错误"},
    {ErrorCode::VERIFICATION_CODE_EXPIRED, "验证码已过期"},
    {ErrorCode::EMAIL_NOT_VERIFIED, "邮箱未验证"},
    {ErrorCode::USER_SUSPENDED, "账户已被停用"},
    {ErrorCode::USER_DELETED, "账户已删除"},
    
    {ErrorCode::CONTENT_NOT_FOUND, "内容不存在"},
    {ErrorCode::CONTENT_DELETED, "内容已被删除"},
    {ErrorCode::CONTENT_TOO_LONG, "内容过长"},
    {ErrorCode::CONTENT_EMPTY, "内容不能为空"},
    {ErrorCode::CONTENT_SENSITIVE, "内容包含敏感信息"},
    {ErrorCode::CONTENT_MODERATION_FAILED, "内容审核未通过"},
    {ErrorCode::RATE_LIMIT_EXCEEDED, "操作过于频繁，请稍后再试"},
    {ErrorCode::DUPLICATE_CONTENT, "内容重复"},
    
    {ErrorCode::FRIEND_NOT_FOUND, "好友不存在"},
    {ErrorCode::ALREADY_FRIENDS, "已经是好友了"},
    {ErrorCode::FRIEND_REQUEST_EXISTS, "好友请求已存在"},
    {ErrorCode::CANNOT_ADD_SELF, "不能添加自己为好友"},
    {ErrorCode::FRIEND_LIMIT_EXCEEDED, "好友数量已达上限"},
    
    {ErrorCode::AI_SERVICE_ERROR, "AI服务暂时不可用"},
    {ErrorCode::AI_API_ERROR, "AI服务响应异常"},
    {ErrorCode::AI_TIMEOUT, "AI服务响应超时"},
    {ErrorCode::AI_QUOTA_EXCEEDED, "AI服务配额已用完"},
    {ErrorCode::AI_INVALID_RESPONSE, "AI服务返回无效"},
    
    {ErrorCode::DATABASE_ERROR, "数据库错误"},
    {ErrorCode::DATABASE_CONNECTION_FAILED, "数据库连接失败"},
    {ErrorCode::DUPLICATE_KEY, "数据已存在"},
    {ErrorCode::FOREIGN_KEY_VIOLATION, "数据关联错误"},
    
    {ErrorCode::NETWORK_ERROR, "网络错误"},
    {ErrorCode::REQUEST_TIMEOUT, "请求超时"},
    {ErrorCode::UPSTREAM_ERROR, "上游服务错误"},
};

const std::unordered_map<ErrorCode, std::string> ErrorCodeHelper::messageEnMap = {
    {ErrorCode::SUCCESS, "Success"},
    {ErrorCode::INVALID_REQUEST, "Invalid request"},
    {ErrorCode::INVALID_PARAMETER, "Invalid parameter"},
    {ErrorCode::MISSING_PARAMETER, "Missing required parameter"},
    {ErrorCode::INTERNAL_ERROR, "Internal server error"},
    {ErrorCode::SERVICE_UNAVAILABLE, "Service unavailable"},
    
    {ErrorCode::UNAUTHORIZED, "Unauthorized"},
    {ErrorCode::TOKEN_EXPIRED, "Token expired"},
    {ErrorCode::TOKEN_INVALID, "Invalid token"},
    {ErrorCode::PERMISSION_DENIED, "Permission denied"},
    {ErrorCode::LOGIN_REQUIRED, "Login required"},
    
    {ErrorCode::USER_NOT_FOUND, "User not found"},
    {ErrorCode::USER_ALREADY_EXISTS, "User already exists"},
    {ErrorCode::EMAIL_ALREADY_EXISTS, "Email already registered"},
    {ErrorCode::INVALID_EMAIL, "Invalid email format"},
    {ErrorCode::INVALID_PASSWORD, "Invalid password"},
    {ErrorCode::PASSWORD_TOO_WEAK, "Password too weak"},
    {ErrorCode::VERIFICATION_CODE_INVALID, "Invalid verification code"},
    {ErrorCode::VERIFICATION_CODE_EXPIRED, "Verification code expired"},
    {ErrorCode::EMAIL_NOT_VERIFIED, "Email not verified"},
    {ErrorCode::USER_SUSPENDED, "Account suspended"},
    {ErrorCode::USER_DELETED, "Account deleted"},
    
    {ErrorCode::CONTENT_NOT_FOUND, "Content not found"},
    {ErrorCode::CONTENT_DELETED, "Content deleted"},
    {ErrorCode::CONTENT_TOO_LONG, "Content too long"},
    {ErrorCode::CONTENT_EMPTY, "Content cannot be empty"},
    {ErrorCode::CONTENT_SENSITIVE, "Content contains sensitive information"},
    {ErrorCode::CONTENT_MODERATION_FAILED, "Content moderation failed"},
    {ErrorCode::RATE_LIMIT_EXCEEDED, "Rate limit exceeded"},
    {ErrorCode::DUPLICATE_CONTENT, "Duplicate content"},
    
    {ErrorCode::FRIEND_NOT_FOUND, "Friend not found"},
    {ErrorCode::ALREADY_FRIENDS, "Already friends"},
    {ErrorCode::FRIEND_REQUEST_EXISTS, "Friend request already exists"},
    {ErrorCode::CANNOT_ADD_SELF, "Cannot add yourself as friend"},
    {ErrorCode::FRIEND_LIMIT_EXCEEDED, "Friend limit exceeded"},
    
    {ErrorCode::AI_SERVICE_ERROR, "AI service error"},
    {ErrorCode::AI_API_ERROR, "AI API error"},
    {ErrorCode::AI_TIMEOUT, "AI timeout"},
    {ErrorCode::AI_QUOTA_EXCEEDED, "AI quota exceeded"},
    {ErrorCode::AI_INVALID_RESPONSE, "Invalid AI response"},
    
    {ErrorCode::DATABASE_ERROR, "Database error"},
    {ErrorCode::DATABASE_CONNECTION_FAILED, "Database connection failed"},
    {ErrorCode::DUPLICATE_KEY, "Duplicate key"},
    {ErrorCode::FOREIGN_KEY_VIOLATION, "Foreign key violation"},
    
    {ErrorCode::NETWORK_ERROR, "Network error"},
    {ErrorCode::REQUEST_TIMEOUT, "Request timeout"},
    {ErrorCode::UPSTREAM_ERROR, "Upstream error"},
};

int ErrorCodeHelper::getHttpStatus(ErrorCode code) {
    auto it = httpStatusMap.find(code);
    return (it != httpStatusMap.end()) ? it->second : 500;
}

std::string ErrorCodeHelper::getMessageZh(ErrorCode code) {
    auto it = messageZhMap.find(code);
    return (it != messageZhMap.end()) ? it->second : "未知错误";
}

std::string ErrorCodeHelper::getMessageEn(ErrorCode code) {
    auto it = messageEnMap.find(code);
    return (it != messageEnMap.end()) ? it->second : "Unknown error";
}

bool ErrorCodeHelper::isClientError(ErrorCode code) {
    int httpStatus = getHttpStatus(code);
    return httpStatus >= 400 && httpStatus < 500;
}

bool ErrorCodeHelper::isServerError(ErrorCode code) {
    int httpStatus = getHttpStatus(code);
    return httpStatus >= 500 && httpStatus < 600;
}

std::string ErrorCodeHelper::getMessage(ErrorCode code, Language lang) {
    return (lang == Language::ZH) ? getMessageZh(code) : getMessageEn(code);
}

int ErrorCodeHelper::getCodeValue(ErrorCode code) {
    return static_cast<int>(code);
}

bool ErrorCodeHelper::isSuccess(ErrorCode code) {
    return code == ErrorCode::SUCCESS;
}

} // namespace utils
} // namespace heartlake
