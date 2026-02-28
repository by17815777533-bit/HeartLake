/**
 * @brief 各类业务实体的唯一ID生成器
 *
 * 为用户、Stone、涟漪、漂流瓶、通知等实体生成带业务前缀的唯一标识。
 * 安全敏感场景（如会话ID）使用 CSPRNG 保证不可预测性。
 */

#pragma once

#include <string>
#include <cstddef>

namespace heartlake {
namespace utils {

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
    static int generateRandomNumber(int min, int max);
};

} // namespace utils
} // namespace heartlake
