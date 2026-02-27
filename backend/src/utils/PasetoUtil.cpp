/**
 * PASETO v4 令牌工具实现
 */
#include "utils/PasetoUtil.h"

#include <cstdlib>
#include <stdexcept>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>
#include <memory>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <json/json.h>

namespace heartlake {
namespace utils {

std::string PasetoUtil::base64urlEncode(const std::string& input) {
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

std::string PasetoUtil::base64urlDecode(const std::string& input) {
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

std::string PasetoUtil::getKey() {
    const char* env_key = std::getenv("PASETO_KEY");
    if (!env_key || std::string(env_key).length() < KEY_SIZE) {
        throw std::runtime_error("PASETO_KEY must be at least 32 bytes");
    }
    return std::string(env_key).substr(0, KEY_SIZE);
}

std::string PasetoUtil::generateToken(const std::string& userId, const std::string& key, int expireHours) {
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

std::string PasetoUtil::verifyToken(const std::string& token, const std::string& key) {
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

std::string PasetoUtil::extractToken(const drogon::HttpRequestPtr& req) {
    auto auth = req->getHeader("Authorization");
    if (auth.empty()) throw std::runtime_error("No authorization header");
    if (auth.find("Bearer ") == 0) return auth.substr(7);
    throw std::runtime_error("Invalid authorization format");
}

std::string PasetoUtil::getAdminKey() {
    const char* env_key = std::getenv("ADMIN_PASETO_KEY");
    if (!env_key || std::string(env_key).length() < KEY_SIZE) {
        throw std::runtime_error("ADMIN_PASETO_KEY must be at least 32 bytes");
    }
    return std::string(env_key).substr(0, KEY_SIZE);
}

std::string PasetoUtil::generateAdminToken(const std::string& adminId, const std::string& role,
                                           const std::string& key, int expireHours) {
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

std::string PasetoUtil::verifyAdminToken(const std::string& token, const std::string& key) {
    if (token.find(HEADER) != 0) {
        throw std::runtime_error("Invalid PASETO header");
    }

    std::string payload = decrypt(token.substr(strlen(HEADER)), key);

    auto sub = extractJsonField(payload, "sub");
    auto role = extractJsonField(payload, "role");
    auto exp = extractJsonField(payload, "exp");

    auto expTime = parseTime(exp);
    if (std::chrono::system_clock::now() > expTime) {
        throw std::runtime_error("Admin token expired");
    }

    return sub;
}

std::string PasetoUtil::encrypt(const std::string& payload, const std::string& key) {
    std::vector<unsigned char> nonce(NONCE_SIZE);
    if (RAND_bytes(nonce.data(), NONCE_SIZE) != 1) {
        throw std::runtime_error("RAND_bytes failed: CSPRNG不可用，无法安全生成nonce");
    }

    struct CtxDeleter { void operator()(EVP_CIPHER_CTX* p) { EVP_CIPHER_CTX_free(p); } };
    std::unique_ptr<EVP_CIPHER_CTX, CtxDeleter> ctx(EVP_CIPHER_CTX_new());
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    if (!EVP_EncryptInit_ex(ctx.get(), EVP_chacha20_poly1305(), nullptr,
                      reinterpret_cast<const unsigned char*>(key.data()), nonce.data()))
        throw std::runtime_error("EncryptInit failed");

    std::vector<unsigned char> ciphertext(payload.size());
    int len;
    if (!EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &len,
                      reinterpret_cast<const unsigned char*>(payload.data()), payload.size()))
        throw std::runtime_error("EncryptUpdate failed");

    int ciphertext_len = len;
    if (!EVP_EncryptFinal_ex(ctx.get(), ciphertext.data() + len, &len))
        throw std::runtime_error("EncryptFinal failed");
    ciphertext_len += len;

    std::vector<unsigned char> tag(TAG_SIZE);
    EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_GET_TAG, TAG_SIZE, tag.data());

    std::string result(reinterpret_cast<char*>(nonce.data()), NONCE_SIZE);
    result.append(reinterpret_cast<char*>(ciphertext.data()), ciphertext_len);
    result.append(reinterpret_cast<char*>(tag.data()), TAG_SIZE);

    return std::string(HEADER) + base64urlEncode(result);
}

std::string PasetoUtil::decrypt(const std::string& encoded, const std::string& key) {
    std::string data = base64urlDecode(encoded);
    if (data.size() < NONCE_SIZE + TAG_SIZE) {
        throw std::runtime_error("Invalid token format");
    }

    const unsigned char* nonce = reinterpret_cast<const unsigned char*>(data.data());
    size_t ciphertext_len = data.size() - NONCE_SIZE - TAG_SIZE;
    const unsigned char* ciphertext = reinterpret_cast<const unsigned char*>(data.data() + NONCE_SIZE);
    const unsigned char* tag = reinterpret_cast<const unsigned char*>(data.data() + data.size() - TAG_SIZE);

    struct CtxDeleter { void operator()(EVP_CIPHER_CTX* p) { EVP_CIPHER_CTX_free(p); } };
    std::unique_ptr<EVP_CIPHER_CTX, CtxDeleter> ctx(EVP_CIPHER_CTX_new());
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    std::vector<unsigned char> plaintext(ciphertext_len);
    int len, plaintext_len;

    if (!EVP_DecryptInit_ex(ctx.get(), EVP_chacha20_poly1305(), nullptr,
                      reinterpret_cast<const unsigned char*>(key.data()), nonce))
        throw std::runtime_error("DecryptInit failed");
    if (!EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len, ciphertext, ciphertext_len))
        throw std::runtime_error("DecryptUpdate failed");
    plaintext_len = len;

    EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_TAG, TAG_SIZE, const_cast<unsigned char*>(tag));

    if (EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + len, &len) <= 0) {
        throw std::runtime_error("Token verification failed");
    }
    plaintext_len += len;

    return std::string(reinterpret_cast<char*>(plaintext.data()), plaintext_len);
}

std::string PasetoUtil::formatTime(std::chrono::system_clock::time_point tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf{};
    gmtime_r(&t, &tm_buf);
    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::chrono::system_clock::time_point PasetoUtil::parseTime(const std::string& s) {
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return std::chrono::system_clock::from_time_t(timegm(&tm));
}

std::string PasetoUtil::extractJsonField(const std::string& json, const std::string& field) {
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

bool PasetoUtil::verifyAdminToken(const std::string& token, const std::string& key,
                                  std::string& adminId, std::string& role) {
    try {
        if (token.find(HEADER) != 0) return false;
        std::string payload = decrypt(token.substr(strlen(HEADER)), key);
        adminId = extractJsonField(payload, "sub");
        role = extractJsonField(payload, "role");
        auto exp = extractJsonField(payload, "exp");
        if (std::chrono::system_clock::now() > parseTime(exp)) return false;
        return true;
    } catch (const std::exception& e) {
        LOG_WARN << "PASETO admin token verification failed: " << e.what();
        return false;
    }
}

} // namespace utils
} // namespace heartlake
