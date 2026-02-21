/**
 * @file RecoveryKeyGenerator.h
 * @brief 恢复关键词生成器 - 用于匿名用户账号恢复
 *
 * 生成4个中文词组合的恢复关键词，格式如 "星辰-湖畔-微风-暖阳"
 * 使用 SHA256 哈希存储，明文仅在首次生成时返回给用户
 */

#pragma once
#include <string>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace heartlake {
namespace utils {

class RecoveryKeyGenerator {
public:
    /// 生成4个中文词组合的恢复关键词，格式如 "星辰-湖畔-微风-暖阳"
    static std::string generate() {
        static const std::vector<std::string> words = {
            "星辰", "湖畔", "微风", "暖阳", "月光", "花语", "云端", "晨曦",
            "雨露", "彩虹", "山谷", "溪流", "萤火", "海浪", "竹林", "梅花",
            "松柏", "白鹤", "蝴蝶", "蜻蜓", "银河", "极光", "朝霞", "晚霞",
            "春风", "夏雨", "秋叶", "冬雪", "清泉", "翠竹", "红枫", "白桦",
            "紫藤", "金菊", "玉兰", "桃花", "荷塘", "柳岸", "石桥", "古道",
            "琴声", "书香", "茶韵", "墨香", "棋局", "画卷", "诗行", "歌谣",
            "灯火", "烟雨", "霜降", "露珠", "飞鸟", "游鱼", "野鹿", "松鼠",
            "蓝天", "碧海", "青山", "绿水", "暮色", "黎明", "正午", "子夜"
        };

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, words.size() - 1);

        std::ostringstream oss;
        for (int i = 0; i < 4; ++i) {
            if (i > 0) oss << "-";
            oss << words[dist(gen)];
        }
        return oss.str();
    }

    /// 对关键词进行 SHA256 哈希
    static std::string hash(const std::string& key) {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(key.c_str()), key.size(), digest);

        std::ostringstream oss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(digest[i]);
        }
        return oss.str();
    }
};

} // namespace utils
} // namespace heartlake
