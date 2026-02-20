/**
 * @file IdentityShadowMap.cpp
 * @brief 身份影子映射实现
 * Created by engineer-4
 */

#include "utils/IdentityShadowMap.h"
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <chrono>
#include <mutex>
#include <sstream>
#include <iomanip>

namespace heartlake {
namespace utils {

IdentityShadowMap& IdentityShadowMap::getInstance() {
    static IdentityShadowMap instance;
    return instance;
}

std::string IdentityShadowMap::getOrCreateShadowId(const std::string& userId) {
    std::unique_lock lock(mutex_);
    auto it = shadowMap_.find(userId);
    if (it != shadowMap_.end()) {
        return it->second.shadowId;
    }

    ShadowIdentity shadow;
    shadow.shadowId = generateShadowId();
    shadow.createdAt = std::chrono::system_clock::now().time_since_epoch().count();
    shadow.lastRotatedAt = shadow.createdAt;
    shadowMap_[userId] = shadow;
    return shadow.shadowId;
}

std::string IdentityShadowMap::anonymizeIp(const std::string& ipAddress) {
    // 使用固定盐值进行单向哈希，确保同一IP始终映射到同一匿名ID
    static const std::string salt = "heartlake_ip_salt_v1";
    auto hash = sha256Hash(ipAddress, salt);
    // 只返回前16字符作为匿名标识
    return "ip_" + hash.substr(0, 16);
}

std::string IdentityShadowMap::anonymizeFingerprint(const std::string& fingerprint) {
    static const std::string salt = "heartlake_fp_salt_v1";
    auto hash = sha256Hash(fingerprint, salt);
    return "fp_" + hash.substr(0, 16);
}

std::string IdentityShadowMap::rotateShadowId(const std::string& userId) {
    std::unique_lock lock(mutex_);
    auto newShadowId = generateShadowId();
    auto now = std::chrono::system_clock::now().time_since_epoch().count();

    auto it = shadowMap_.find(userId);
    if (it != shadowMap_.end()) {
        it->second.shadowId = newShadowId;
        it->second.lastRotatedAt = now;
    } else {
        ShadowIdentity shadow;
        shadow.shadowId = newShadowId;
        shadow.createdAt = now;
        shadow.lastRotatedAt = now;
        shadowMap_[userId] = shadow;
    }
    return newShadowId;
}

std::string IdentityShadowMap::desensitize(const std::string& value, const std::string& type) {
    if (value.empty()) return value;

    if (type == "email") {
        auto at = value.find('@');
        if (at != std::string::npos && at > 1) {
            return value.substr(0, 1) + std::string(at - 1, '*') + value.substr(at);
        }
    } else if (type == "phone") {
        if (value.length() >= 7) {
            return value.substr(0, 3) + std::string(value.length() - 7, '*') + value.substr(value.length() - 4);
        }
    } else if (type == "name") {
        if (value.length() > 1) {
            return value.substr(0, 1) + std::string(value.length() - 1, '*');
        }
    }
    return value;
}

std::string IdentityShadowMap::sha256Hash(const std::string& input, const std::string& salt) {
    std::string combined = salt + input;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.size(), hash);

    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

std::string IdentityShadowMap::generateShadowId() {
    unsigned char bytes[16];
    RAND_bytes(bytes, 16);

    std::ostringstream oss;
    oss << "shd_";
    for (int i = 0; i < 16; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    return oss.str();
}

} // namespace utils
} // namespace heartlake
