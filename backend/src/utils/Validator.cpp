/**
 * @file Validator.cpp
 * @brief Validator 模块实现
 * Created by 林子怡
 */
#include "utils/Validator.h"
#include <algorithm>
#include <regex>
#include <drogon/drogon.h>

namespace heartlake {
namespace utils {

// UTF-8 字符计数：统计非续字节（不以 10xxxxxx 开头）的个数
static size_t utf8Length(const std::string& str) {
    size_t count = 0;
    for (unsigned char c : str) {
        if ((c & 0xC0) != 0x80) {
            ++count;
        }
    }
    return count;
}

ValidationResult Validator::validateJson(const drogon::HttpRequestPtr& req) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        return ValidationResult::invalid("请求体必须是 JSON 格式");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::required(const Json::Value& value, const std::string& fieldName) {
    if (value.isNull() || !value.isString() || value.asString().empty()) {
        return ValidationResult::invalid(fieldName + " 不能为空");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::requiredString(const std::string& value, const std::string& fieldName) {
    if (value.empty()) {
        return ValidationResult::invalid(fieldName + " 不能为空");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::length(const std::string& value, size_t min, size_t max,
                                   const std::string& fieldName) {
    // 按 UTF-8 字符计数，对中文等多字节字符更准确
    size_t len = utf8Length(value);
    if (len < min) {
        return ValidationResult::invalid(fieldName + " 至少需要 " + std::to_string(min) + " 个字符");
    }
    if (len > max) {
        return ValidationResult::invalid(fieldName + " 不能超过 " + std::to_string(max) + " 个字符");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::email(const std::string& value) {
    if (value.empty()) {
        return ValidationResult::invalid("邮箱不能为空");
    }

    // 预编译正则，避免每次调用重新构造
    thread_local static const std::regex emailRegex(
        R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    if (!std::regex_match(value, emailRegex)) {
        return ValidationResult::invalid("邮箱格式不正确");
    }

    return ValidationResult::valid();
}

ValidationResult Validator::password(const std::string& value) {
    if (value.empty()) {
        return ValidationResult::invalid("密码不能为空");
    }
    if (value.length() < 6) {
        return ValidationResult::invalid("密码至少需要 6 个字符");
    }
    if (value.length() > 20) {
        return ValidationResult::invalid("密码不能超过 20 个字符");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::passwordStrong(const std::string& value) {
    if (value.empty()) {
        return ValidationResult::invalid("密码不能为空");
    }
    if (value.length() < 8) {
        return ValidationResult::invalid("密码至少需要 8 个字符");
    }
    if (value.length() > 64) {
        return ValidationResult::invalid("密码不能超过 64 个字符");
    }
    // 至少包含大小写字母、数字、特殊字符中的3种
    int categories = 0;
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    for (char c : value) {
        if (std::isupper(static_cast<unsigned char>(c))) hasUpper = true;
        else if (std::islower(static_cast<unsigned char>(c))) hasLower = true;
        else if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
        else hasSpecial = true;
    }
    if (hasUpper) categories++;
    if (hasLower) categories++;
    if (hasDigit) categories++;
    if (hasSpecial) categories++;
    if (categories < 3) {
        return ValidationResult::invalid("密码需要包含大小写字母、数字、特殊字符中的至少3种");
    }
    return ValidationResult::valid();
}

int Validator::calculatePasswordStrength(const std::string& value) {
    if (value.empty()) return 0;
    int score = 0;
    // 长度加分
    if (value.length() >= 8) score++;
    if (value.length() >= 12) score++;
    // 字符种类加分
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    for (char c : value) {
        if (std::isupper(static_cast<unsigned char>(c))) hasUpper = true;
        else if (std::islower(static_cast<unsigned char>(c))) hasLower = true;
        else if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
        else hasSpecial = true;
    }
    if (hasUpper) score++;
    if (hasLower) score++;
    if (hasDigit) score++;
    if (hasSpecial) score++;
    return score;
}

std::string Validator::sanitizeHtml(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c; break;
        }
    }
    return result;
}

ValidationResult Validator::checkSqlInjection(const std::string& input, const std::string& fieldName) {
    // 转小写检测
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    static const std::vector<std::string> dangerousKeywords = {
        "drop ", "delete ", "insert ", "update ", "union ", "select ",
        "--", ";--", "/*", "*/", "xp_", "exec ", "execute "
    };
    for (const auto& keyword : dangerousKeywords) {
        if (lower.find(keyword) != std::string::npos) {
            return ValidationResult::invalid(fieldName + " 包含不安全的内容");
        }
    }
    return ValidationResult::valid();
}

ValidationResult Validator::checkPathTraversal(const std::string& path, const std::string& fieldName) {
    if (path.find("..") != std::string::npos) {
        return ValidationResult::invalid(fieldName + " 包含不安全的路径");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::url(const std::string& value, const std::string& fieldName) {
    if (value.empty()) {
        return ValidationResult::invalid(fieldName + " 不能为空");
    }
    thread_local static const std::regex urlRegex(
        R"(https?://[a-zA-Z0-9\-._~:/?#\[\]@!$&'()*+,;=%]+)");
    if (!std::regex_match(value, urlRegex)) {
        return ValidationResult::invalid(fieldName + " 格式不正确");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::phoneNumber(const std::string& value) {
    if (value.empty()) {
        return ValidationResult::invalid("手机号不能为空");
    }
    thread_local static const std::regex phoneRegex(R"(1[3-9]\d{9})");
    if (!std::regex_match(value, phoneRegex)) {
        return ValidationResult::invalid("手机号格式不正确");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::verificationCode(const std::string& code) {
    if (code.empty()) {
        return ValidationResult::invalid("验证码不能为空");
    }
    thread_local static const std::regex codeRegex(R"(\d{6})");
    if (!std::regex_match(code, codeRegex)) {
        return ValidationResult::invalid("验证码格式不正确");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::fileExtension(const std::string& filename,
                                          const std::vector<std::string>& allowedExtensions,
                                          const std::string& fieldName) {
    auto dotPos = filename.rfind('.');
    if (dotPos == std::string::npos) {
        return ValidationResult::invalid(fieldName + " 缺少扩展名");
    }
    std::string ext = filename.substr(dotPos + 1);
    // 转小写
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    for (const auto& allowed : allowedExtensions) {
        std::string lowerAllowed = allowed;
        // 去掉前导点号（允许 ".jpg" 和 "jpg" 两种格式）
        if (!lowerAllowed.empty() && lowerAllowed[0] == '.') {
            lowerAllowed = lowerAllowed.substr(1);
        }
        std::transform(lowerAllowed.begin(), lowerAllowed.end(), lowerAllowed.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (ext == lowerAllowed) {
            return ValidationResult::valid();
        }
    }
    return ValidationResult::invalid(fieldName + " 扩展名不允许");
}

ValidationResult Validator::hasField(const Json::Value& json, const std::string& fieldName) {
    if (!json.isMember(fieldName)) {
        return ValidationResult::invalid("缺少字段: " + fieldName);
    }
    return ValidationResult::valid();
}

ValidationResult Validator::inEnum(const std::string& value,
                                   const std::vector<std::string>& allowedValues,
                                   const std::string& fieldName) {
    for (const auto& allowed : allowedValues) {
        if (value == allowed) {
            return ValidationResult::valid();
        }
    }
    return ValidationResult::invalid(fieldName + " 的值不在允许范围内");
}

ValidationResult Validator::userId(const std::string& value) {
    if (value.empty()) {
        return ValidationResult::invalid("用户ID不能为空");
    }
    // 预编译正则
    thread_local static const std::regex uuidRegex(
        R"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})");
    if (!std::regex_match(value, uuidRegex)) {
        return ValidationResult::invalid("用户ID格式不正确");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::arrayLength(const Json::Value& array, size_t max,
                                       const std::string& fieldName) {
    if (!array.isArray()) {
        return ValidationResult::valid(); // 不是数组时跳过验证
    }
    if (array.size() > max) {
        return ValidationResult::invalid(fieldName + " 最多只能包含 " + std::to_string(max) + " 个元素");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::numberRange(int value, int min, int max,
                                       const std::string& fieldName) {
    if (value < min || value > max) {
        return ValidationResult::invalid(fieldName + " 必须在 " +
                                       std::to_string(min) + " 到 " +
                                       std::to_string(max) + " 之间");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::paginationParams(int page, int pageSize) {
    auto pageResult = numberRange(page, 1, 1000, "页码");
    if (!pageResult) return pageResult;

    auto sizeResult = numberRange(pageSize, 1, 100, "每页数量");
    if (!sizeResult) return sizeResult;

    return ValidationResult::valid();
}

ValidationResult Validator::combine(const std::vector<ValidationResult>& results) {
    for (const auto& result : results) {
        if (!result.isValid) {
            return result;
        }
    }
    return ValidationResult::valid();
}

std::optional<std::string> Validator::getUserId(const drogon::HttpRequestPtr& req) {
    // Drogon Attributes::get<T>() 不抛异常，key 不存在时返回 T() 即空字符串
    const auto& userId = req->getAttributes()->get<std::string>("user_id");
    if (userId.empty()) {
        return std::nullopt;
    }
    return userId;
}


ValidationResult ValidationRules::content(const std::string& value) {
    return Validator::combine({
        Validator::requiredString(value, "内容"),
        Validator::length(value, 1, 5000, "内容")
    });
}

ValidationResult ValidationRules::nickname(const std::string& value) {
    return Validator::combine({
        Validator::requiredString(value, "昵称"),
        Validator::length(value, 2, 20, "昵称")
    });
}

ValidationResult ValidationRules::tags(const Json::Value& tags) {
    return Validator::arrayLength(tags, 5, "标签");
}

ValidationResult ValidationRules::mediaIds(const Json::Value& mediaIds) {
    return Validator::arrayLength(mediaIds, 9, "媒体文件");
}

} // namespace utils
} // namespace heartlake
