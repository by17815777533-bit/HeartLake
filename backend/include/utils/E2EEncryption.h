/**
 * @file E2EEncryption.h
 * @brief 端到端加密模块 - AES-256-GCM
 * Created by engineer-4
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace heartlake {
namespace utils {

/**
 * @brief 加密消息结构
 */
struct EncryptedMessage {
    std::string ciphertext;  // Base64编码的密文
    std::string iv;          // Base64编码的初始化向量
    std::string tag;         // Base64编码的认证标签
};

/**
 * @brief E2E加密工具类
 * 使用AES-256-GCM提供认证加密
 */
class E2EEncryption {
public:
    static constexpr size_t KEY_SIZE = 32;   // 256 bits
    static constexpr size_t IV_SIZE = 12;    // 96 bits for GCM
    static constexpr size_t TAG_SIZE = 16;   // 128 bits

    /**
     * 生成随机密钥
     * @return Base64编码的密钥
     */
    static std::string generateKey();

    /**
     * 加密消息
     * @param plaintext 明文
     * @param key Base64编码的密钥
     * @return 加密消息结构，失败返回nullopt
     */
    static std::optional<EncryptedMessage> encrypt(const std::string& plaintext,
                                                    const std::string& key);

    /**
     * 解密消息
     * @param encrypted 加密消息结构
     * @param key Base64编码的密钥
     * @return 明文，失败返回nullopt
     */
    static std::optional<std::string> decrypt(const EncryptedMessage& encrypted,
                                               const std::string& key);

    /**
     * 派生会话密钥（用于双方协商）
     * @param sharedSecret 共享密钥
     * @param salt 盐值
     * @return 派生的会话密钥
     */
    static std::string deriveSessionKey(const std::string& sharedSecret,
                                         const std::string& salt);

private:
    static std::string base64Encode(const std::vector<unsigned char>& data);
    static std::vector<unsigned char> base64Decode(const std::string& encoded);
    static std::vector<unsigned char> generateRandomBytes(size_t length);
};

} // namespace utils
} // namespace heartlake
