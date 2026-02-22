/**
 * @file IdGenerator.cpp
 * @brief ID生成器实现
 */

#include "utils/IdGenerator.h"
#include <random>

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
    return "session_" + generateRandomId(16);
}

std::string IdGenerator::generateUUID() {
    return generateRandomId(32);
}

std::string IdGenerator::generateNickname() {
    int number = generateRandomNumber(1000, 9999);
    return "旅人#" + std::to_string(number);
}

std::string IdGenerator::generateRandomId(size_t length) {
    static const char hex_chars[] = "0123456789abcdef";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::string id;
    id.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        id += hex_chars[dis(gen)];
    }

    return id;
}

int IdGenerator::generateRandomNumber(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

} // namespace utils
} // namespace heartlake
