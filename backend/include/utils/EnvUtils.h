/**
 * @brief 环境变量解析工具函数集
 *
 * 提供类型安全的环境变量读取，支持布尔值和正整数两种类型。
 * 所有函数在解析失败时返回调用方指定的默认值，不会抛出异常。
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace heartlake::utils {

/**
 * @brief 将字符串解析为布尔值
 * @param rawValue 原始字符串，nullptr 视为缺失
 * @param defaultValue 解析失败或缺失时的回退值
 * @return 识别 "1"/"true"/"yes"/"on" 为 true，"0"/"false"/"no"/"off" 为 false（大小写不敏感）
 */
inline bool parseBoolEnv(const char* rawValue, bool defaultValue) {
    if (!rawValue) {
        return defaultValue;
    }

    std::string value(rawValue);
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (value == "1" || value == "true" || value == "yes" || value == "on") {
        return true;
    }
    if (value == "0" || value == "false" || value == "no" || value == "off") {
        return false;
    }
    return defaultValue;
}

/**
 * @brief 将字符串解析为正整数
 * @param rawValue 原始字符串，nullptr 视为缺失
 * @param defaultValue 解析失败、非正整数或有尾随字符时的回退值
 * @return 解析成功返回正整数值，否则返回 defaultValue
 */
inline int parsePositiveInt(const char* rawValue, int defaultValue) {
    if (!rawValue) {
        return defaultValue;
    }
    char* end = nullptr;
    long parsed = std::strtol(rawValue, &end, 10);
    if (end == rawValue || *end != '\0' || parsed <= 0) {
        return defaultValue;
    }
    return static_cast<int>(parsed);
}

/**
 * @brief 从环境变量读取正整数
 * @param envName 环境变量名
 * @param defaultValue 环境变量不存在或解析失败时的回退值
 * @return 解析后的正整数，或 defaultValue
 */
inline int parsePositiveIntEnv(const char* envName, int defaultValue) {
    return parsePositiveInt(std::getenv(envName), defaultValue);
}

}  // namespace heartlake::utils
