/**
 * @file E2EEncryption.cpp
 * @brief 端到端加密模块实现
 *
 * X25519 密钥交换 + HKDF-SHA256 密钥派生 + AES-256-GCM 认证加密
 * 提供 IND-CCA2 安全性，通过临时密钥对实现前向保密。
 *
 * Created by engineer-4
 */

#include "utils/E2EEncryption.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>
#include <cstring>
#include <stdexcept>
#include <memory>

namespace heartlake {
namespace utils {

// RAII wrapper for EVP_PKEY
struct EvpPkeyDeleter {
    void operator()(EVP_PKEY* p) const { if (p) EVP_PKEY_free(p); }
};
using EvpPkeyPtr = std::unique_ptr<EVP_PKEY, EvpPkeyDeleter>;

// RAII wrapper for EVP_PKEY_CTX
struct EvpPkeyCtxDeleter {
    void operator()(EVP_PKEY_CTX* p) const { if (p) EVP_PKEY_CTX_free(p); }
};
using EvpPkeyCtxPtr = std::unique_ptr<EVP_PKEY_CTX, EvpPkeyCtxDeleter>;

std::string E2EEncryption::generateKey() {
    auto bytes = generateRandomBytes(KEY_SIZE);
    return base64Encode(bytes);
}

std::optional<X25519KeyPair> E2EEncryption::generateX25519KeyPair() {
    EvpPkeyCtxPtr ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr));
    if (!ctx) return std::nullopt;

    if (EVP_PKEY_keygen_init(ctx.get()) <= 0) return std::nullopt;

    EVP_PKEY* rawKey = nullptr;
    if (EVP_PKEY_keygen(ctx.get(), &rawKey) <= 0) return std::nullopt;
    EvpPkeyPtr pkey(rawKey);

    // Extract raw private key
    size_t privLen = X25519_KEY_SIZE;
    std::vector<unsigned char> privBytes(privLen);
    if (EVP_PKEY_get_raw_private_key(pkey.get(), privBytes.data(), &privLen) <= 0)
        return std::nullopt;
    privBytes.resize(privLen);

    // Extract raw public key
    size_t pubLen = X25519_KEY_SIZE;
    std::vector<unsigned char> pubBytes(pubLen);
    if (EVP_PKEY_get_raw_public_key(pkey.get(), pubBytes.data(), &pubLen) <= 0)
        return std::nullopt;
    pubBytes.resize(pubLen);

    return X25519KeyPair{base64Encode(pubBytes), base64Encode(privBytes)};
}

std::optional<std::string> E2EEncryption::computeSharedSecret(const std::string& myPrivateKey,
                                                               const std::string& peerPublicKey) {
    auto privBytes = base64Decode(myPrivateKey);
    auto pubBytes = base64Decode(peerPublicKey);

    if (privBytes.size() != X25519_KEY_SIZE || pubBytes.size() != X25519_KEY_SIZE)
        return std::nullopt;

    // Create EVP_PKEY from raw private key
    EvpPkeyPtr privKey(EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr,
                                                     privBytes.data(), privBytes.size()));
    if (!privKey) return std::nullopt;

    // Create EVP_PKEY from raw public key
    EvpPkeyPtr pubKey(EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                                    pubBytes.data(), pubBytes.size()));
    if (!pubKey) return std::nullopt;

    // Perform ECDH key agreement
    EvpPkeyCtxPtr ctx(EVP_PKEY_CTX_new(privKey.get(), nullptr));
    if (!ctx) return std::nullopt;

    if (EVP_PKEY_derive_init(ctx.get()) <= 0) return std::nullopt;
    if (EVP_PKEY_derive_set_peer(ctx.get(), pubKey.get()) <= 0) return std::nullopt;

    // Determine shared secret length
    size_t secretLen = 0;
    if (EVP_PKEY_derive(ctx.get(), nullptr, &secretLen) <= 0) return std::nullopt;

    std::vector<unsigned char> secret(secretLen);
    if (EVP_PKEY_derive(ctx.get(), secret.data(), &secretLen) <= 0) return std::nullopt;
    secret.resize(secretLen);

    return base64Encode(secret);
}

std::optional<EncryptedMessage> E2EEncryption::encrypt(const std::string& plaintext,
                                                        const std::string& key) {
    auto keyBytes = base64Decode(key);
    if (keyBytes.size() != KEY_SIZE) return std::nullopt;

    auto iv = generateRandomBytes(IV_SIZE);
    std::vector<unsigned char> ciphertext(plaintext.size() + 16);
    std::vector<unsigned char> tag(TAG_SIZE);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return std::nullopt;

    int len = 0, ciphertextLen = 0;
    bool success = true;

    success = success && EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    success = success && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr);
    success = success && EVP_EncryptInit_ex(ctx, nullptr, nullptr, keyBytes.data(), iv.data());
    success = success && EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                                            reinterpret_cast<const unsigned char*>(plaintext.data()),
                                            static_cast<int>(plaintext.size()));
    ciphertextLen = len;
    success = success && EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    ciphertextLen += len;
    success = success && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data());

    EVP_CIPHER_CTX_free(ctx);

    if (!success) return std::nullopt;

    ciphertext.resize(ciphertextLen);
    return EncryptedMessage{base64Encode(ciphertext), base64Encode(iv), base64Encode(tag)};
}

std::optional<std::string> E2EEncryption::decrypt(const EncryptedMessage& encrypted,
                                                   const std::string& key) {
    auto keyBytes = base64Decode(key);
    auto ciphertext = base64Decode(encrypted.ciphertext);
    auto iv = base64Decode(encrypted.iv);
    auto tag = base64Decode(encrypted.tag);

    if (keyBytes.size() != KEY_SIZE || iv.size() != IV_SIZE || tag.size() != TAG_SIZE)
        return std::nullopt;

    std::vector<unsigned char> plaintext(ciphertext.size());
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return std::nullopt;

    int len = 0, plaintextLen = 0;
    bool success = true;

    success = success && EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    success = success && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr);
    success = success && EVP_DecryptInit_ex(ctx, nullptr, nullptr, keyBytes.data(), iv.data());
    success = success && EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(),
                                            static_cast<int>(ciphertext.size()));
    plaintextLen = len;
    success = success && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE,
                                              const_cast<unsigned char*>(tag.data()));
    success = success && EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
    plaintextLen += len;

    EVP_CIPHER_CTX_free(ctx);

    if (!success) return std::nullopt;

    return std::string(reinterpret_cast<char*>(plaintext.data()), plaintextLen);
}

std::string E2EEncryption::deriveSessionKey(const std::string& sharedSecret,
                                             const std::string& salt) {
    std::vector<unsigned char> derived(KEY_SIZE);
    auto secretBytes = base64Decode(sharedSecret);
    auto saltBytes = base64Decode(salt);

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
    if (ctx) {
        EVP_PKEY_derive_init(ctx);
        EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_sha256());
        EVP_PKEY_CTX_set1_hkdf_salt(ctx, saltBytes.data(), saltBytes.size());
        EVP_PKEY_CTX_set1_hkdf_key(ctx, secretBytes.data(), secretBytes.size());
        size_t outLen = KEY_SIZE;
        EVP_PKEY_derive(ctx, derived.data(), &outLen);
        EVP_PKEY_CTX_free(ctx);
    }
    return base64Encode(derived);
}

std::string E2EEncryption::base64Encode(const std::vector<unsigned char>& data) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve((data.size() + 2) / 3 * 4);

    for (size_t i = 0; i < data.size(); i += 3) {
        unsigned int n = data[i] << 16;
        if (i + 1 < data.size()) n |= data[i + 1] << 8;
        if (i + 2 < data.size()) n |= data[i + 2];

        result += table[(n >> 18) & 0x3F];
        result += table[(n >> 12) & 0x3F];
        result += (i + 1 < data.size()) ? table[(n >> 6) & 0x3F] : '=';
        result += (i + 2 < data.size()) ? table[n & 0x3F] : '=';
    }
    return result;
}

std::vector<unsigned char> E2EEncryption::base64Decode(const std::string& encoded) {
    static const int table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };

    std::vector<unsigned char> result;
    result.reserve(encoded.size() * 3 / 4);

    unsigned int buf = 0;
    int bits = 0;
    for (char c : encoded) {
        if (c == '=') break;
        int val = table[static_cast<unsigned char>(c)];
        if (val < 0) continue;
        buf = (buf << 6) | val;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            result.push_back(static_cast<unsigned char>((buf >> bits) & 0xFF));
        }
    }
    return result;
}

std::vector<unsigned char> E2EEncryption::generateRandomBytes(size_t length) {
    std::vector<unsigned char> bytes(length);
    RAND_bytes(bytes.data(), static_cast<int>(length));
    return bytes;
}

} // namespace utils
} // namespace heartlake
