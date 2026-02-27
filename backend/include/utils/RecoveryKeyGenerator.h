/**
 * 恢复关键词生成器 - 用于匿名用户账号恢复
 *
 * 生成6个中文词组合的恢复关键词，格式如 "星辰-湖畔-微风-暖阳-碧海-飞鸟"
 * 词库256个词，6词组合提供 256^6 = 2^48 种可能（48 bit 熵）
 * 使用 OpenSSL RAND_bytes 作为 CSPRNG 选词
 * 使用 SHA256 哈希存储，明文仅在首次生成时返回给用户
 */

#pragma once
#include <string>

namespace heartlake {
namespace utils {

class RecoveryKeyGenerator {
public:
    /// 生成6个中文词组合的恢复关键词，格式如 "星辰-湖畔-微风-暖阳-碧海-飞鸟"
    static std::string generate();

    /// 对关键词进行 PBKDF2-HMAC-SHA256 加盐哈希，返回 "salt:hash" 格式
    static std::string hash(const std::string& key);

    /// 验证明文关键词是否匹配已存储的 "salt:hash"
    static bool verify(const std::string& key, const std::string& storedHash);
};

} // namespace utils
} // namespace heartlake
