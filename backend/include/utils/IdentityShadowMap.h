/**
 * @file IdentityShadowMap.h
 * @brief 身份影子映射 - 隔离物理标识与匿名ID
 * Created by engineer-4
 */

#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <shared_mutex>

namespace heartlake {
namespace utils {

/**
 * @brief 影子身份数据
 */
struct ShadowIdentity {
    std::string shadowId;        // 影子ID（对外展示）
    std::string hashedFingerprint; // 哈希后的设备指纹
    int64_t createdAt;
    int64_t lastRotatedAt;
};

/**
 * @brief 身份影子映射管理器
 *
 * 实现物理标识与匿名ID的完全隔离：
 * - 物理IP、设备指纹经过单向哈希后存储
 * - 用户ID与影子ID通过加密映射关联
 * - 即使数据库泄露也无法溯源真实身份
 */
class IdentityShadowMap {
public:
    static IdentityShadowMap& getInstance();

    /**
     * 为用户生成或获取影子ID
     * @param userId 真实用户ID
     * @return 影子ID
     */
    std::string getOrCreateShadowId(const std::string& userId);

    /**
     * 匿名化IP地址
     * @param ipAddress 原始IP
     * @return 匿名化后的标识（不可逆）
     */
    static std::string anonymizeIp(const std::string& ipAddress);

    /**
     * 匿名化设备指纹
     * @param fingerprint 原始指纹
     * @return 匿名化后的标识（不可逆）
     */
    static std::string anonymizeFingerprint(const std::string& fingerprint);

    /**
     * 轮换用户的影子ID（增强隐私）
     * @param userId 用户ID
     * @return 新的影子ID
     */
    std::string rotateShadowId(const std::string& userId);

    /**
     * 脱敏敏感字段
     * @param value 原始值
     * @param type 字段类型（email, phone, name）
     * @return 脱敏后的值
     */
    static std::string desensitize(const std::string& value, const std::string& type);

private:
    IdentityShadowMap() = default;
    ~IdentityShadowMap() = default;
    IdentityShadowMap(const IdentityShadowMap&) = delete;
    IdentityShadowMap& operator=(const IdentityShadowMap&) = delete;

    static std::string sha256Hash(const std::string& input, const std::string& salt);
    static std::string generateShadowId();

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, ShadowIdentity> shadowMap_;
};

} // namespace utils
} // namespace heartlake
