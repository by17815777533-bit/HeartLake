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

namespace {

const std::string& cachedUserKey() {
    static const std::string key = []() {
        const char* envKey = std::getenv("PASETO_KEY");
        if (!envKey || std::string(envKey).length() < PasetoUtil::KEY_SIZE) {
            throw std::runtime_error("PASETO_KEY must be at least 32 bytes");
        }
        return std::string(envKey).substr(0, PasetoUtil::KEY_SIZE);
    }();
    return key;
}

const std::string& cachedAdminKey() {
    static const std::string key = []() {
        const char* envKey = std::getenv("ADMIN_PASETO_KEY");
        if (!envKey || std::string(envKey).length() < PasetoUtil::KEY_SIZE) {
            throw std::runtime_error("ADMIN_PASETO_KEY must be at least 32 bytes");
        }
        return std::string(envKey).substr(0, PasetoUtil::KEY_SIZE);
    }();
    return key;
}

Json::Value parsePayloadObject(const std::string& payload) {
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream stream(payload);
    if (!Json::parseFromStream(builder, stream, &root, &errs) || !root.isObject()) {
        throw std::runtime_error("Invalid JSON payload: " + errs);
    }
    return root;
}

std::string requireStringField(const Json::Value& root, const char* field) {
    if (!root.isMember(field) || !root[field].isString()) {
        throw std::runtime_error(std::string("Field not found: ") + field);
    }
    return root[field].asString();
}

std::chrono::system_clock::time_point parseTimeClaim(const Json::Value& value) {
    if (value.isInt64() || value.isUInt64() || value.isInt() || value.isUInt()) {
        const auto seconds = value.asLargestInt();
        return std::chrono::system_clock::time_point{std::chrono::seconds(seconds)};
    }
    if (value.isString()) {
        std::tm tm = {};
        std::istringstream ss(value.asString());
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        if (ss.fail()) {
            throw std::runtime_error("Invalid exp claim time format");
        }
        return std::chrono::system_clock::from_time_t(timegm(&tm));
    }
    throw std::runtime_error("Invalid exp claim type");
}

} // namespace

std::string PasetoUtil::base64urlEncode(const std::string& input) {
    static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string result;
    result.reserve((input.size() + 2) / 3 * 4);
    for (size_t i = 0; i < input.size(); i += 3) {
        uint32_t n = static_cast<uint8_t>(input[i]) << 16;
        if (i + 1 < input.size()) n |= static_cast<uint8_t>(input[i + 1]) << 8;
        if (i + 2 < input.size()) n |= static_cast<uint8_t>(input[i + 2]);
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
        int v = table[static_cast<uint8_t>(c)];
        if (v < 0) continue;
        buf = (buf << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            result += static_cast<char>(buf >> bits);
            buf &= (1U << bits) - 1U;
        }
    }
    return result;
}

const std::string& PasetoUtil::getKey() {
    return cachedUserKey();
}

std::string PasetoUtil::generateToken(const std::string& userId, const std::string& key, int expireHours) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(expireHours);

    // 使用Json::Value安全构建payload，防止JSON注入
    Json::Value payloadJson;
    payloadJson["sub"] = userId;
    payloadJson["iss"] = "heart_lake";
    payloadJson["iat"] = static_cast<Json::Int64>(
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
    payloadJson["exp"] = static_cast<Json::Int64>(
        std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count());

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
    const Json::Value root = parsePayloadObject(payload);
    const std::string sub = requireStringField(root, "sub");

    // 检查过期
    if (!root.isMember("exp")) {
        throw std::runtime_error("Field not found: exp");
    }
    auto expTime = parseTimeClaim(root["exp"]);
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

const std::string& PasetoUtil::getAdminKey() {
    return cachedAdminKey();
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
    payloadJson["iat"] = static_cast<Json::Int64>(
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
    payloadJson["exp"] = static_cast<Json::Int64>(
        std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count());

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
    const Json::Value root = parsePayloadObject(payload);
    const std::string sub = requireStringField(root, "sub");

    if (!root.isMember("exp")) {
        throw std::runtime_error("Field not found: exp");
    }
    auto expTime = parseTimeClaim(root["exp"]);
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

bool PasetoUtil::verifyAdminToken(const std::string& token, const std::string& key,
                                  std::string& adminId, std::string& role) {
    try {
        if (token.find(HEADER) != 0) return false;
        std::string payload = decrypt(token.substr(strlen(HEADER)), key);
        const Json::Value root = parsePayloadObject(payload);
        adminId = requireStringField(root, "sub");
        role = requireStringField(root, "role");
        if (!root.isMember("exp")) return false;
        if (std::chrono::system_clock::now() > parseTimeClaim(root["exp"])) return false;
        return true;
    } catch (const std::exception& e) {
        LOG_WARN << "PASETO admin token verification failed: " << e.what();
        return false;
    }
}

} // namespace utils
} // namespace heartlake
