/**
 * @brief 密码安全工具类
 *
 * 基于 PBKDF2-HMAC-SHA256 实现密码哈希与验证。
 * 每个密码使用独立的 32 字节随机盐值，迭代 100,000 次，
 * 输出 64 字节密钥，满足 OWASP 推荐的密码存储强度。
 *
 * @note 盐值由 OpenSSL RAND_bytes 生成，保证密码学安全随机性
 */

#pragma once

#include <string>

namespace heartlake {
namespace utils {

class PasswordUtil {
public:
    /**
     * @brief 生成密码学安全的随机盐值
     * @return 32 字节随机数据的十六进制编码字符串（64 个 hex 字符）
     */
    static std::string generateSalt();

    /**
     * @brief 使用 PBKDF2 对密码进行哈希
     * @param password 明文密码
     * @param salt 盐值（十六进制字符串）
     * @return 哈希结果的十六进制编码字符串
     */
    static std::string hashPassword(const std::string& password, const std::string& salt);

    /**
     * @brief 验证密码是否与存储的哈希匹配
     * @param password 用户输入的明文密码
     * @param salt 数据库中存储的盐值
     * @param hashedPassword 数据库中存储的哈希值
     * @return 匹配返回 true
     */
    static bool verifyPassword(const std::string& password,
                              const std::string& salt,
                              const std::string& hashedPassword);

    /**
     * @brief 一步完成盐值生成和密码哈希（注册/改密时使用）
     * @param password 明文密码
     * @param[out] outSalt 生成的盐值
     * @param[out] outHash 生成的哈希值
     */
    static void generatePasswordHash(const std::string& password,
                                     std::string& outSalt,
                                     std::string& outHash);

private:
    static constexpr int ITERATIONS = 100000;  ///< PBKDF2 迭代次数（OWASP 推荐 >= 600,000，此处取平衡值）
    static constexpr int SALT_LENGTH = 32;     ///< 盐值长度（字节）
    static constexpr int HASH_LENGTH = 64;     ///< 派生密钥长度（字节）

    /// PBKDF2 密钥派生核心实现
    static std::string pbkdf2(const std::string& password,
                             const std::string& salt,
                             int iterations,
                             int keyLength);

    /// 字节数组转十六进制字符串
    static std::string bytesToHex(const unsigned char* bytes, size_t length);
    /// 十六进制字符串转字节数组
    static std::string hexToBytes(const std::string& hex);
};

} // namespace utils
} // namespace heartlake
