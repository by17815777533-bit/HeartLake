/**
 * @file E2EEncryption.h
 * @brief 端到端加密模块 - X25519 + HKDF + AES-256-GCM
 *
 * 提供 IND-CCA2 安全性，通过临时密钥对实现前向保密。
 * 密钥交换: X25519 ECDH
 * 密钥派生: HKDF-SHA256
 * 认证加密: AES-256-GCM
 *
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
 * @brief X25519 密钥对
 */
struct X25519KeyPair {
    std::string publicKey;   // Base64编码的公钥 (32 bytes)
    std::string privateKey;  // Base64编码的私钥 (32 bytes)
};

/**
 * @brief E2E加密工具类
 * 使用 X25519 密钥交换 + HKDF-SHA256 密钥派生 + AES-256-GCM 认证加密
 */
class E2EEncryption {
public:
    static constexpr size_t KEY_SIZE = 32;       // 256 bits
    static constexpr size_t IV_SIZE = 12;        // 96 bits for GCM
    static constexpr size_t TAG_SIZE = 16;       // 128 bits
    static constexpr size_t X25519_KEY_SIZE = 32; // 256 bits

    /**
     * 生成随机密钥
     * @return Base64编码的密钥
     */
    static std::string generateKey();

    /**
     * 生成 X25519 密钥对（用于 ECDH 密钥交换）
     * @return 包含 Base64 编码公钥和私钥的 KeyPair，失败返回 nullopt
     */
    static std::optional<X25519KeyPair> generateX25519KeyPair();

    /**
     * 计算 X25519 共享密钥（ECDH）
     * @param myPrivateKey Base64编码的本方私钥
     * @param peerPublicKey Base64编码的对方公钥
     * @return Base64编码的共享密钥，失败返回 nullopt
     */
    static std::optional<std::string> computeSharedSecret(const std::string& myPrivateKey,
                                                           const std::string& peerPublicKey);

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
     * 派生会话密钥（HKDF-SHA256）
     * @param sharedSecret 共享密钥（来自 X25519 ECDH 或其他来源）
     * @param salt 盐值
     * @return Base64编码的派生会话密钥
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
