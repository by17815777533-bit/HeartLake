/**
 * @file PasetoUtil.h
 * @brief PASETO Token工具 - 替代JWT的更安全认证方案（前沿技术）
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
#include <cstdlib>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>
#include <mutex>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <drogon/HttpRequest.h>
#include <json/json.h>

namespace heartlake {
namespace utils {

class PasetoUtil {
public:
    static constexpr const char* HEADER = "v4.local.";
    static constexpr int KEY_SIZE = 32;
    static constexpr int NONCE_SIZE = 12;  // ChaCha20-Poly1305 requires 12-byte nonce
    static constexpr int TAG_SIZE = 16;

    // Base64URL编码（无填充）
    static std::string base64urlEncode(const std::string& input) {
        static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        std::string result;
        result.reserve((input.size() + 2) / 3 * 4);
        for (size_t i = 0; i < input.size(); i += 3) {
            uint32_t n = (uint8_t)input[i] << 16;
            if (i + 1 < input.size()) n |= (uint8_t)input[i + 1] << 8;
            if (i + 2 < input.size()) n |= (uint8_t)input[i + 2];
            result += table[(n >> 18) & 0x3F];
            result += table[(n >> 12) & 0x3F];
            if (i + 1 < input.size()) result += table[(n >> 6) & 0x3F];
            if (i + 2 < input.size()) result += table[n & 0x3F];
        }
        return result;
    }

    static std::string base64urlDecode(const std::string& input) {
        static const int table[256] = {
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
            -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,63,
            -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        };
        std::string result;
        uint32_t buf = 0; int bits = 0;
        for (char c : input) {
            int v = table[(uint8_t)c];
            if (v < 0) continue;
            buf = (buf << 6) | v;
            bits += 6;
            if (bits >= 8) { bits -= 8; result += (char)(buf >> bits); buf &= (1 << bits) - 1; }
        }
        return result;
    }

    static std::string getKey() {
        const char* env_key = std::getenv("PASETO_KEY");
        if (!env_key || std::string(env_key).length() < KEY_SIZE) {
            throw std::runtime_error("PASETO_KEY must be at least 32 bytes");
        }
        return std::string(env_key).substr(0, KEY_SIZE);
    }

    /**
     * @brief 生成PASETO v4.local token
     */
    static std::string generateToken(const std::string& userId, const std::string& key, int expireHours = 24) {
        auto now = std::chrono::system_clock::now();
        auto exp = now + std::chrono::hours(expireHours);

        // 使用Json::Value安全构建payload，防止JSON注入
        Json::Value payloadJson;
        payloadJson["sub"] = userId;
        payloadJson["iss"] = "heart_lake";
        payloadJson["iat"] = formatTime(now);
        payloadJson["exp"] = formatTime(exp);

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string payload = Json::writeString(writer, payloadJson);

        return encrypt(payload, key);
    }

    /**
     * @brief 验证并解析PASETO token
     * @return userId if valid
     */
    static std::string verifyToken(const std::string& token, const std::string& key) {
        if (token.find(HEADER) != 0) {
            throw std::runtime_error("Invalid PASETO header");
        }

        std::string payload = decrypt(token.substr(strlen(HEADER)), key);

        // 解析JSON获取userId和过期时间
        auto sub = extractJsonField(payload, "sub");
        auto exp = extractJsonField(payload, "exp");

        // 检查过期
        auto expTime = parseTime(exp);
        if (std::chrono::system_clock::now() > expTime) {
            throw std::runtime_error("Token expired");
        }

        return sub;
    }

    static std::string extractToken(const drogon::HttpRequestPtr& req) {
        auto auth = req->getHeader("Authorization");
        if (auth.empty()) throw std::runtime_error("No authorization header");
        if (auth.find("Bearer ") == 0) return auth.substr(7);
        throw std::runtime_error("Invalid authorization format");
    }

    static std::string getAdminKey() {
        const char* env_key = std::getenv("ADMIN_PASETO_KEY");
        if (!env_key || std::string(env_key).length() < KEY_SIZE) {
            throw std::runtime_error("ADMIN_PASETO_KEY must be at least 32 bytes");
        }
        return std::string(env_key).substr(0, KEY_SIZE);
    }

    static std::string generateAdminToken(const std::string& adminId, const std::string& role,
                                          const std::string& key, int expireHours = 24) {
        auto now = std::chrono::system_clock::now();
        auto exp = now + std::chrono::hours(expireHours);

        // 使用Json::Value安全构建payload，防止JSON注入
        Json::Value payloadJson;
        payloadJson["sub"] = adminId;
        payloadJson["role"] = role;
        payloadJson["iss"] = "heart_lake_admin";
        payloadJson["iat"] = formatTime(now);
        payloadJson["exp"] = formatTime(exp);

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string payload = Json::writeString(writer, payloadJson);

        return encrypt(payload, key);
    }

    static bool verifyAdminToken(const std::string& token, const std::string& key,
                                 std::string& adminId, std::string& role) {
        try {
            if (token.find(HEADER) != 0) return false;
            std::string payload = decrypt(token.substr(strlen(HEADER)), key);
            adminId = extractJsonField(payload, "sub");
            role = extractJsonField(payload, "role");
            auto exp = extractJsonField(payload, "exp");
            if (std::chrono::system_clock::now() > parseTime(exp)) return false;
            return true;
        } catch (...) {
            return false;
        }
    }

private:
    static std::string encrypt(const std::string& plaintext, const std::string& key) {
        unsigned char nonce[NONCE_SIZE];
        RAND_bytes(nonce, NONCE_SIZE);

        // ChaCha20-Poly1305 (AEAD加密)
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        std::vector<unsigned char> ciphertext(plaintext.size() + TAG_SIZE);
        int len, ciphertext_len;

        EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr,
                          reinterpret_cast<const unsigned char*>(key.data()), nonce);
        EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                         reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size());
        ciphertext_len = len;
        EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
        ciphertext_len += len;

        unsigned char tag[TAG_SIZE];
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, TAG_SIZE, tag);
        EVP_CIPHER_CTX_free(ctx);

        // 组合: nonce + ciphertext + tag
        std::string result(reinterpret_cast<char*>(nonce), NONCE_SIZE);
        result.append(reinterpret_cast<char*>(ciphertext.data()), ciphertext_len);
        result.append(reinterpret_cast<char*>(tag), TAG_SIZE);

        return std::string(HEADER) + base64urlEncode(result);
    }

    static std::string decrypt(const std::string& encoded, const std::string& key) {
        std::string data = base64urlDecode(encoded);
        if (data.size() < NONCE_SIZE + TAG_SIZE) {
            throw std::runtime_error("Invalid token format");
        }

        const unsigned char* nonce = reinterpret_cast<const unsigned char*>(data.data());
        size_t ciphertext_len = data.size() - NONCE_SIZE - TAG_SIZE;
        const unsigned char* ciphertext = reinterpret_cast<const unsigned char*>(data.data() + NONCE_SIZE);
        const unsigned char* tag = reinterpret_cast<const unsigned char*>(data.data() + data.size() - TAG_SIZE);

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        std::vector<unsigned char> plaintext(ciphertext_len);
        int len, plaintext_len;

        EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr,
                          reinterpret_cast<const unsigned char*>(key.data()), nonce);
        EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, ciphertext_len);
        plaintext_len = len;

        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, TAG_SIZE, const_cast<unsigned char*>(tag));

        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) <= 0) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Token verification failed");
        }
        plaintext_len += len;
        EVP_CIPHER_CTX_free(ctx);

        return std::string(reinterpret_cast<char*>(plaintext.data()), plaintext_len);
    }

    static std::string formatTime(std::chrono::system_clock::time_point tp) {
        auto t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm_buf{};
        gmtime_r(&t, &tm_buf);
        std::ostringstream ss;
        ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
        return ss.str();
    }

    static std::chrono::system_clock::time_point parseTime(const std::string& s) {
        std::tm tm = {};
        std::istringstream ss(s);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        return std::chrono::system_clock::from_time_t(timegm(&tm));
    }

    static std::string extractJsonField(const std::string& json, const std::string& field) {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;
        std::istringstream stream(json);
        if (!Json::parseFromStream(builder, stream, &root, &errs)) {
            throw std::runtime_error("Invalid JSON payload: " + errs);
        }
        if (!root.isMember(field) || !root[field].isString()) {
            throw std::runtime_error("Field not found: " + field);
        }
        return root[field].asString();
    }
};

} // namespace utils
} // namespace heartlake
