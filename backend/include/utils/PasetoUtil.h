/**
 * PASETO Token工具 - 替代JWT的更安全认证方案（前沿技术）
 *
 * PASETO vs JWT 优势：
 * 1. 消除alg:none攻击漏洞
 * 2. 强制使用安全算法，无需担心配置错误
 * 3. 更简洁的设计，减少实现错误
 *
 * 实现：PASETO v4.local (对称加密，适合单服务场景)
 */

#pragma once

#include <string>
#include <chrono>
#include <drogon/HttpRequest.h>

namespace heartlake {
namespace utils {

class PasetoUtil {
public:
    static constexpr const char* HEADER = "v4.local.";
    static constexpr int KEY_SIZE = 32;
    static constexpr int NONCE_SIZE = 12;  // ChaCha20-Poly1305 requires 12-byte nonce
    static constexpr int TAG_SIZE = 16;

    // Base64URL编码（无填充）
    static std::string base64urlEncode(const std::string& input);
    static std::string base64urlDecode(const std::string& input);

    static std::string getKey();

    /**
     * 生成PASETO v4.local token
     */
    static std::string generateToken(const std::string& userId, const std::string& key, int expireHours = 24);

    /**
     * 验证并解析PASETO token
     * @return userId if valid
     */
    static std::string verifyToken(const std::string& token, const std::string& key);

    static std::string extractToken(const drogon::HttpRequestPtr& req);

    static std::string getAdminKey();

    static std::string generateAdminToken(const std::string& adminId, const std::string& role,
                                          const std::string& key, int expireHours = 24);

    static std::string verifyAdminToken(const std::string& token, const std::string& key);

    static bool verifyAdminToken(const std::string& token, const std::string& key,
                                 std::string& adminId, std::string& role);

private:
    static std::string encrypt(const std::string& payload, const std::string& key);
    static std::string decrypt(const std::string& encoded, const std::string& key);
    static std::string formatTime(std::chrono::system_clock::time_point tp);
    static std::chrono::system_clock::time_point parseTime(const std::string& s);
    static std::string extractJsonField(const std::string& json, const std::string& field);
};

} // namespace utils
} // namespace heartlake
