/**
 * @brief PASETO v4.local Token 工具 — 替代 JWT 的现代认证方案
 *
 * 选择 PASETO 而非 JWT 的安全考量：
 * 1. 消除 alg:none 攻击面 — 不存在算法协商，杜绝降级攻击
 * 2. 强制安全算法 — v4.local 固定使用 XChaCha20-Poly1305 + BLAKE2b
 * 3. 设计更简洁 — 减少实现层面的配置错误风险
 *
 * @details 本实现采用 v4.local（对称加密）模式，适用于单服务部署场景。
 *          加密使用 ChaCha20-Poly1305（12 字节 nonce，16 字节 tag），
 *          密钥从环境变量 PASETO_KEY / ADMIN_PASETO_KEY 读取（32 字节）。
 *          普通用户 token 和管理员 token 使用独立密钥，互不干扰。
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
    static constexpr int KEY_SIZE = 32;       ///< 对称密钥长度（字节）
    static constexpr int NONCE_SIZE = 12;     ///< ChaCha20-Poly1305 nonce 长度
    static constexpr int TAG_SIZE = 16;       ///< Poly1305 认证标签长度

    /// Base64URL 编码（无填充，RFC 4648 Section 5）
    static std::string base64urlEncode(const std::string& input);
    /// Base64URL 解码
    static std::string base64urlDecode(const std::string& input);

    /// 从环境变量 PASETO_KEY 获取普通用户 token 密钥
    static const std::string& getKey();

    /**
     * @brief 生成普通用户的 PASETO v4.local token
     * @param userId 用户唯一标识，写入 token payload 的 sub 字段
     * @param key 32 字节对称密钥
     * @param expireHours token 有效期（小时），默认 24h
     * @return 完整的 "v4.local.xxx" token 字符串
     */
    static std::string generateToken(const std::string& userId, const std::string& key, int expireHours = 24);

    /**
     * @brief 验证并解析普通用户 token
     * @param token 完整 token 字符串
     * @param key 对称密钥
     * @return 解析出的 userId；验证失败返回空字符串
     */
    static std::string verifyToken(const std::string& token, const std::string& key);

    /// 从 HTTP 请求的 Authorization 头提取 Bearer token
    static std::string extractToken(const drogon::HttpRequestPtr& req);

    /// 从环境变量 ADMIN_PASETO_KEY 获取管理员 token 密钥
    static const std::string& getAdminKey();

    /**
     * @brief 生成管理员 token，payload 中额外携带 role 字段
     * @param adminId 管理员ID
     * @param role 角色标识（如 "super_admin"、"admin"）
     * @param key 管理员专用密钥
     * @param expireHours 有效期（小时）
     */
    static std::string generateAdminToken(const std::string& adminId, const std::string& role,
                                          const std::string& key, int expireHours = 24);

    /// 验证管理员 token，返回 adminId；失败返回空字符串
    static std::string verifyAdminToken(const std::string& token, const std::string& key);

    /**
     * @brief 验证管理员 token 并同时提取 adminId 和 role
     * @param token 完整 token 字符串
     * @param key 管理员密钥
     * @param[out] adminId 解析出的管理员ID
     * @param[out] role 解析出的角色
     * @return 验证成功返回 true
     */
    static bool verifyAdminToken(const std::string& token, const std::string& key,
                                 std::string& adminId, std::string& role);

private:
    /// ChaCha20-Poly1305 加密，返回 Base64URL 编码的密文
    static std::string encrypt(const std::string& payload, const std::string& key);
    /// 解密 Base64URL 编码的密文，返回明文 JSON
    static std::string decrypt(const std::string& encoded, const std::string& key);
};

} // namespace utils
} // namespace heartlake
