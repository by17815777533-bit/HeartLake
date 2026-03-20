/**
 * @brief 可复用业务规则工具
 */
#pragma once

#include <algorithm>
#include <string>
#include <utility>

namespace heartlake::utils {

inline std::string escapeLike(const std::string& input) {
    std::string result;
    result.reserve(input.size() * 2);
    for (char c : input) {
        if (c == '%' || c == '_' || c == '\\') {
            result += '\\';
        }
        result += c;
    }
    return result;
}

inline double interactionWeightForType(const std::string& interactionType) {
    if (interactionType == "ripple") return 1.0;
    if (interactionType == "boat") return 2.0;
    if (interactionType == "share") return 3.0;
    return 0.1;
}

inline std::pair<std::string, std::string> canonicalizeUserPair(
    const std::string& left,
    const std::string& right
) {
    if (left <= right) {
        return {left, right};
    }
    return {right, left};
}

inline int clampRemainingSeconds(int rawSeconds) {
    return std::max(0, rawSeconds);
}

inline int remainingHoursFromSeconds(int rawSeconds) {
    return clampRemainingSeconds(rawSeconds) / 3600;
}

inline bool constantTimeEqual(const std::string& a, const std::string& b) {
    volatile unsigned char result = static_cast<unsigned char>(a.size() ^ b.size());
    const size_t len = std::max(a.size(), b.size());
    for (size_t i = 0; i < len; ++i) {
        const unsigned char ca = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
        const unsigned char cb = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
        result |= static_cast<unsigned char>(ca ^ cb);
    }
    return result == 0;
}

} // namespace heartlake::utils
