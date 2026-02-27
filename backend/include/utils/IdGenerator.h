/**
 * IdGenerator 模块接口定义
 */

#pragma once

#include <string>
#include <cstddef>

namespace heartlake {
namespace utils {

/**
 * ID生成器，用于生成唯一ID
 *
 * 详细说明
 *
 * @note 注意事项
 */
class IdGenerator {
public:
    static std::string generateUserId();
    static std::string generateAnonymousId();
    static std::string generateStoneId();
    static std::string generateRippleId();
    static std::string generateBoatId();
    static std::string generateNotificationId();
    static std::string generateConnectionId();
    static std::string generateMessageId();
    static std::string generateReportId();
    static std::string generateSessionId();
    static std::string generateUUID();
    static std::string generateNickname();

private:
    static std::string generateRandomId(size_t length);
    static std::string generateSecureRandomId(size_t length);  // CSPRNG，用于安全敏感的 ID

    /**
     * generateRandomNumber方法
     *
     * @param min 参数说明
     * @param max 参数说明
     * @return 返回值说明
     */
    static int generateRandomNumber(int min, int max);
};

} // namespace utils
} // namespace heartlake
