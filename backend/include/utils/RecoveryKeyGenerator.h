/**
 * @file RecoveryKeyGenerator.h
 * @brief 恢复关键词生成器 - 用于匿名用户账号恢复
 *
 * 生成4个中文词组合的恢复关键词，格式如 "星辰-湖畔-微风-暖阳"
 * 使用 SHA256 哈希存储，明文仅在首次生成时返回给用户
 */

#pragma once
#include <string>

namespace heartlake {
namespace utils {

class RecoveryKeyGenerator {
public:
    /// 生成4个中文词组合的恢复关键词，格式如 "星辰-湖畔-微风-暖阳"
    static std::string generate();

    /// 对关键词进行 SHA256 哈希
    static std::string hash(const std::string& key);
};

} // namespace utils
} // namespace heartlake
