/**
 * @file EnvUtils.h
 * @brief 环境变量解析工具
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace heartlake::utils {

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

inline int parsePositiveIntEnv(const char* envName, int defaultValue) {
    return parsePositiveInt(std::getenv(envName), defaultValue);
}

}  // namespace heartlake::utils
