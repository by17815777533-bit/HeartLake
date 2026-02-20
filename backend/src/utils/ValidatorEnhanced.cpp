/**
 * @file ValidatorEnhanced.cpp
 * @brief 增强的验证器实现 - 安全验证功能
 * @author 白洋
 * @date 2025-02-08
 */

#include "utils/Validator.h"
#include <regex>
#include <algorithm>
#include <drogon/drogon.h>

namespace heartlake {
namespace utils {

// ==================== 增强的密码验证 ====================

ValidationResult Validator::passwordStrong(const std::string& value) {
    if (value.empty()) {
        return ValidationResult::invalid("密码不能为空");
    }
    if (value.length() < 8) {
        return ValidationResult::invalid("密码至少需要 8 个字符");
    }
    if (value.length() > 32) {
        return ValidationResult::invalid("密码不能超过 32 个字符");
    }

    int strength = calculatePasswordStrength(value);
    if (strength < 3) {
        return ValidationResult::invalid("密码强度不足，请包含大小写字母、数字、特殊字符中的至少3种");
    }

    return ValidationResult::valid();
}

int Validator::calculatePasswordStrength(const std::string& value) {
    int score = 0;

    // 检查长度
    if (value.length() >= 8) score++;
    if (value.length() >= 12) score++;

    // 检查字符类型
    bool hasLower = false, hasUpper = false, hasDigit = false, hasSpecial = false;

    for (char c : value) {
        if (std::islower(c)) hasLower = true;
        else if (std::isupper(c)) hasUpper = true;
        else if (std::isdigit(c)) hasDigit = true;
        else hasSpecial = true;
    }

    if (hasLower) score++;
    if (hasUpper) score++;
    if (hasDigit) score++;
    if (hasSpecial) score++;

    return score;
}

// ==================== XSS 防护 ====================

std::string Validator::sanitizeHtml(const std::string& input) {
    std::string result = input;

    // 替换HTML特殊字符
    std::vector<std::pair<std::string, std::string>> replacements = {
        {"&", "&amp;"},
        {"<", "&lt;"},
        {">", "&gt;"},
        {"\"", "&quot;"},
        {"'", "&#x27;"},
        {"/", "&#x2F;"}
    };

    for (const auto& [from, to] : replacements) {
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    return result;
}

// ==================== SQL 注入防护 ====================

ValidationResult Validator::checkSqlInjection(const std::string& input, const std::string& fieldName) {
    // 检测常见的SQL注入关键字
    std::vector<std::string> sqlKeywords = {
        "SELECT", "INSERT", "UPDATE", "DELETE", "DROP", "CREATE", "ALTER",
        "EXEC", "EXECUTE", "UNION", "DECLARE", "CAST", "CONVERT",
        "--", "/*", "*/", "xp_", "sp_", "0x"
    };

    std::string upperInput = input;
    std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);

    for (const auto& keyword : sqlKeywords) {
        if (upperInput.find(keyword) != std::string::npos) {
            LOG_WARN << "Potential SQL injection detected in " << fieldName << ": " << keyword;
            return ValidationResult::invalid(fieldName + " 包含非法字符");
        }
    }

    // 检测
    std::regex dangerousPattern(R"((--|;|'|\"|\*|\/\*|\*\/|xp_|sp_))");
    if (std::regex_search(input, dangerousPattern)) {
        LOG_WARN << "Dangerous pattern detected in " << fieldName;
        return ValidationResult::invalid(fieldName + " 包含非法字符");
    }

    return ValidationResult::valid();
}

// ==================== 路径遍历防护 ====================

ValidationResult Validator::checkPathTraversal(const std::string& path, const std::string& fieldName) {
    // 检测路径遍历攻击
    if (path.find("..") != std::string::npos ||
        path.find("./") != std::string::npos ||
        path.find("\\") != std::string::npos) {
        LOG_WARN << "Path traversal attempt detected in " << fieldName << ": " << path;
        return ValidationResult::invalid(fieldName + " 路径格式不正确");
    }

    return ValidationResult::valid();
}

// ==================== URL 验证 ====================

ValidationResult Validator::url(const std::string& value, const std::string& fieldName) {
    if (value.empty()) {
        return ValidationResult::invalid(fieldName + " 不能为空");
    }

    // URL格式验证
    std::regex urlRegex(
        R"(^(https?:\/\/)?([\da-z\.-]+)\.([a-z\.]{2,6})([\/\w \.-]*)*\/?$)",
        std::regex::icase
    );

    if (!std::regex_match(value, urlRegex)) {
        return ValidationResult::invalid(fieldName + " 格式不正确");
    }

    // 只允许 http 和 https 协议
    if (value.find("http://") != 0 && value.find("https://") != 0) {
        return ValidationResult::invalid(fieldName + " 必须使用 http 或 https 协议");
    }

    return ValidationResult::valid();
}

// ==================== 手机号验证 ====================

ValidationResult Validator::phoneNumber(const std::string& value) {
    if (value.empty()) {
        return ValidationResult::invalid("手机号不能为空");
    }

    // 中国大陆手机号验证：1开头，第二位是3-9，共11位
    std::regex phoneRegex(R"(^1[3-9]\d{9}$)");

    if (!std::regex_match(value, phoneRegex)) {
        return ValidationResult::invalid("手机号格式不正确");
    }

    return ValidationResult::valid();
}

// ==================== 验证码验证 ====================

ValidationResult Validator::verificationCode(const std::string& code) {
    if (code.empty()) {
        return ValidationResult::invalid("验证码不能为空");
    }

    // 验证码通常是6位数字
    std::regex codeRegex(R"(^\d{6}$)");

    if (!std::regex_match(code, codeRegex)) {
        return ValidationResult::invalid("验证码格式不正确");
    }

    return ValidationResult::valid();
}

// ========= 文件扩展名验证 ====================

ValidationResult Validator::fileExtension(const std::string& filename,
                                         const std::vector<std::string>& allowedExtensions,
                                         const std::string& fieldName) {
    if (filename.empty()) {
        return ValidationResult::invalid(fieldName + " 不能为空");
    }

    // 获取文件扩展名
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return ValidationResult::invalid(fieldName + " 缺少文件扩展名");
    }

    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // 检查是否在允许列表中
    bool found = false;
    for (const auto& allowedExt : allowedExtensions) {
        std::string lowerAllowed = allowedExt;
        std::transform(lowerAllowed.begin(), lowerAllowed.end(), lowerAllowed.begin(), ::tolower);
        if (ext == lowerAllowed) {
            found = true;
            break;
        }
    }

    if (!found) {
        return ValidationResult::invalid(fieldName + " 类型不支持");
    }

    return ValidationResult::valid();
}

// ==================== JSON 字段验证 ====================

ValidationResult Validator::hasField(const Json::Value& json, const std::string& fieldName) {
    if (!json.isMember(fieldName)) {
        return ValidationResult::invalid("缺少必需字段: " + fieldName);
    }

    if (json[fieldName].isNull()) {
        return ValidationResult::invalid(fieldName + " 不能为空");
    }

    return ValidationResult::valid();
}

// ==================== 枚举值验证 ====================

ValidationResult Validator::inEnum(const std::string& value,
                                   const std::vector<std::string>& allowedValues,
                                   const std::string& fieldName) {
    if (value.empty()) {
        return ValidationResult::invalid(fieldName + " 不能为空");
    }

    bool found = false;
    for (const auto& allowed : allowedValues) {
        if (value == allowed) {
            found = true;
            break;
        }
    }

    if (!found) {
        return ValidationResult::invalid(fieldName + " 的值不在允许范围内");
    }

    return ValidationResult::valid();
}

} // namespace utils
} // namespace heartlake
