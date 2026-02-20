/**
 * @file IdGenerator.h
 * @brief IdGenerator 模块接口定义
 * Created by 林子怡
 */

#pragma once

#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace heartlake {
namespace utils {

/**
 * @brief ID生成器，用于生成唯一ID
 *
 * 详细说明
 *
 * @note 注意事项
 */
class IdGenerator {
public:
    static std::string generateUserId() {
        return "user_" + generateRandomId(16);
    }

    static std::string generateAnonymousId() {
        return "anonymous_" + generateRandomId(12);
    }

    static std::string generateStoneId() {
        return "stone_" + generateRandomId(16);
    }

    static std::string generateRippleId() {
        return "ripple_" + generateRandomId(16);
    }

    static std::string generateBoatId() {
        return "boat_" + generateRandomId(16);
    }

    static std::string generateNotificationId() {
        return "notif_" + generateRandomId(16);
    }

    static std::string generateConnectionId() {
        return "conn_" + generateRandomId(16);
    }

    static std::string generateMessageId() {
        return "msg_" + generateRandomId(16);
    }

    static std::string generateReportId() {
        return "report_" + generateRandomId(16);
    }

    static std::string generateSessionId() {
        return "session_" + generateRandomId(16);
    }

    static std::string generateUUID() {
        return generateRandomId(32);
    }

    static std::string generateNickname() {
        int number = generateRandomNumber(1000, 9999);
        return "旅人#" + std::to_string(number);
    }

private:
    static std::string generateRandomId(size_t length) {
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

    /**
     * @brief generateRandomNumber方法
     *
     * @param min 参数说明
     * @param max 参数说明
     * @return 返回值说明
     */
    static int generateRandomNumber(int min, int max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }
};

} // namespace utils
} // namespace heartlake
