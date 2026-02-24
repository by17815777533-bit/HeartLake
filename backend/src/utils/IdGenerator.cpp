/**
 * @file IdGenerator.cpp
 * @brief ID生成器实现
 */

#include "utils/IdGenerator.h"
#include <random>
#include <openssl/rand.h>

namespace heartlake {
namespace utils {

std::string IdGenerator::generateUserId() {
    return "user_" + generateRandomId(16);
}

std::string IdGenerator::generateAnonymousId() {
    return "anonymous_" + generateRandomId(12);
}

std::string IdGenerator::generateStoneId() {
    return "stone_" + generateRandomId(16);
}

std::string IdGenerator::generateRippleId() {
    return "ripple_" + generateRandomId(16);
}

std::string IdGenerator::generateBoatId() {
    return "boat_" + generateRandomId(16);
}

std::string IdGenerator::generateNotificationId() {
    return "notif_" + generateRandomId(16);
}

std::string IdGenerator::generateConnectionId() {
    return "conn_" + generateRandomId(16);
}

std::string IdGenerator::generateMessageId() {
    return "msg_" + generateRandomId(16);
}

std::string IdGenerator::generateReportId() {
    return "report_" + generateRandomId(16);
}

std::string IdGenerator::generateSessionId() {
    // 会话 ID 是安全敏感的，使用 CSPRNG 生成
    return "session_" + generateSecureRandomId(32);
}

std::string IdGenerator::generateUUID() {
    return generateRandomId(32);
}

std::string IdGenerator::generateNickname() {
    int number = generateRandomNumber(1000, 9999);
    return "旅人#" + std::to_string(number);
}

std::string IdGenerator::generateRandomId(size_t length) {
    // 统一使用 CSPRNG，避免 mt19937 熵不足的风险
    return generateSecureRandomId(length);
}

int IdGenerator::generateRandomNumber(int min, int max) {
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

std::string IdGenerator::generateSecureRandomId(size_t length) {
    static const char hex_chars[] = "0123456789abcdef";
    std::vector<unsigned char> buf((length + 1) / 2);
    RAND_bytes(buf.data(), static_cast<int>(buf.size()));

    std::string id;
    id.reserve(length);
    for (size_t i = 0; i < buf.size() && id.size() < length; ++i) {
        id += hex_chars[(buf[i] >> 4) & 0x0F];
        if (id.size() < length) {
            id += hex_chars[buf[i] & 0x0F];
        }
    }
    return id;
}

} // namespace utils
} // namespace heartlake
