/**
 * @file Validator.cpp
 * @brief Validator 模块实现
 * Created by 林子怡
 */
#include "utils/Validator.h"
#include <regex>
#include <drogon/drogon.h>

namespace heartlake {
namespace utils {

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
    if (value.length() < min) {
        return ValidationResult::invalid(fieldName + " 至少需要 " + std::to_string(min) + " 个字符");
    }
    if (value.length() > max) {
        return ValidationResult::invalid(fieldName + " 不能超过 " + std::to_string(max) + " 个字符");
    }
    return ValidationResult::valid();
}

ValidationResult Validator::email(const std::string& value) {
    if (value.empty()) {
        return ValidationResult::invalid("邮箱不能为空");
    }
    
    std::regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
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

ValidationResult Validator::userId(const std::string& value) {
    if (value.empty()) {
        return ValidationResult::invalid("用户ID不能为空");
    }
    // UUID格式验证
    std::regex uuidRegex(R"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})");
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
    try {
        auto userId = req->getAttributes()->get<std::string>("user_id");
        if (userId.empty()) {
            return std::nullopt;
        }
        return userId;
    } catch (...) {
        return std::nullopt;
    }
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
