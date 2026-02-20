/**
 * @file PasswordUtil.h
 * @brief PasswordUtil 模块接口定义
 * Created by 林子怡
 */

#pragma once

#include <string>

namespace heartlake {
namespace utils {

/**
 * 密码工具类
 * 提供安全的密码哈希和验证功能
 * 使用 PBKDF2 (SHA256) 算法，带盐值和多次迭代
 */
/**
 * @brief 密码工具类，用于密码处理
 *
 * 详细说明
 *
 * @note 注意事项
 */
class PasswordUtil {
public:
    /**
     * 生成随机盐值
     * @return 32字节的十六进制盐值字符串
     */
    static std::string generateSalt();

    /**
     * 使用盐值对密码进行哈希
     * @param password 原始密码
     * @param salt 盐值
     * @return 哈希后的密码（十六进制字符串）
     */
    static std::string hashPassword(const std::string& password, const std::string& salt);

    /**
     * 验证密码是否正确
     * @param password 用户输入的密码
     * @param salt 存储的盐值
     * @param hashedPassword 存储的哈希密码
     * @return 密码是否匹配
     */
    static bool verifyPassword(const std::string& password,
                              const std::string& salt,
                              const std::string& hashedPassword);

    /**
     * 生成密码哈希和盐值（一次性生成）
     * @param password 原始密码
     * @param outSalt 输出参数：生成的盐值
     * @param outHash 输出参数：生成的哈希值
     */
    static void generatePasswordHash(const std::string& password,
                                     std::string& outSalt,
                                     std::string& outHash);

private:
    static constexpr int ITERATIONS = 100000;  // PBKDF2 迭代次数
    static constexpr int SALT_LENGTH = 32;     // 盐值长度（字节）
    static constexpr int HASH_LENGTH = 64;     // 哈希长度（字节）

    /**
     * PBKDF2 密钥派生函数
     */
    static std::string pbkdf2(const std::string& password,
                             const std::string& salt,
                             int iterations,
                             int keyLength);

    /**
     * 字节数组转十六进制字符串
     */
    static std::string bytesToHex(const unsigned char* bytes, size_t length);

    /**
     * 十六进制字符串转字节数组
     */
    static std::string hexToBytes(const std::string& hex);
};

} // namespace utils
} // namespace heartlake
