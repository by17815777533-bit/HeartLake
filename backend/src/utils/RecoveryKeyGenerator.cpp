/**
 * 恢复关键词生成器实现
 *
 * 词库256个词，8词组合，使用 OpenSSL RAND_bytes (CSPRNG) 选词
 * 熵: log2(256^8) = 64 bit
 */

#include "utils/RecoveryKeyGenerator.h"
#include <array>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <openssl/rand.h>
#include <openssl/evp.h>

namespace heartlake {
namespace utils {

static constexpr int kWordCount = 8;   // 8词组合，熵 = log2(256^8) = 64 bit
static constexpr int kDictSize = 256;

// 256个中文词，涵盖自然、天气、动物、植物、颜色、情感等类别
static const std::array<const char*, kDictSize> kWords = {{
    // 天文·天象 (32)
    "星辰", "月光", "银河", "极光", "朝霞", "晚霞", "流星", "北斗",
    "彗星", "日蚀", "星云", "天河", "织女", "牵牛", "南斗", "启明",
    "晨曦", "暮色", "黎明", "子夜", "正午", "黄昏", "拂晓", "薄暮",
    "云端", "彩虹", "霞光", "天幕", "苍穹", "穹顶", "星轨", "月晕",

    // 天气·气象 (32)
    "微风", "暖阳", "春风", "夏雨", "秋叶", "冬雪", "霜降", "露珠",
    "雨露", "烟雨", "细雨", "骤雨", "雷鸣", "闪电", "冰霜", "薄雾",
    "浓雾", "飘雪", "暴雪", "和风", "清风", "凉风", "暖风", "季风",
    "台风", "龙卷", "冰雹", "甘霖", "时雨", "梅雨", "霏霏", "潇潇",

    // 山水·地理 (32)
    "湖畔", "山谷", "溪流", "海浪", "清泉", "碧海", "青山", "绿水",
    "荷塘", "石桥", "古道", "峡谷", "悬崖", "瀑布", "深潭", "浅滩",
    "沙洲", "岛屿", "礁石", "冰川", "雪峰", "丘陵", "平原", "盆地",
    "峰峦", "山脊", "河湾", "湖心", "泉眼", "源头", "渡口", "港湾",

    // 植物·花卉 (32)
    "梅花", "桃花", "玉兰", "紫藤", "金菊", "翠竹", "红枫", "白桦",
    "松柏", "竹林", "柳岸", "荷花", "兰草", "牡丹", "芍药", "杜鹃",
    "茉莉", "桂花", "海棠", "丁香", "水仙", "百合", "芙蓉", "木棉",
    "银杏", "樟树", "榕树", "枫杨", "青藤", "苔藓", "蒲公", "芦苇",

    // 动物·生灵 (32)
    "白鹤", "蝴蝶", "蜻蜓", "飞鸟", "游鱼", "野鹿", "松鼠", "萤火",
    "燕子", "喜鹊", "黄鹂", "杜鹃", "鸳鸯", "天鹅", "仙鹤", "孔雀",
    "锦鲤", "海豚", "白鸽", "雄鹰", "夜莺", "云雀", "翠鸟", "画眉",
    "麋鹿", "白兔", "灵狐", "青蛙", "蟋蟀", "蜜蜂", "瓢虫", "螳螂",

    // 颜色·光影 (32)
    "蓝天", "碧玉", "翡翠", "琥珀", "珊瑚", "玛瑙", "水晶", "琉璃",
    "丹红", "绯红", "嫣红", "朱砂", "赤金", "鎏金", "鹅黄", "杏黄",
    "碧绿", "翠绿", "墨绿", "青碧", "靛蓝", "湛蓝", "藏青", "黛紫",
    "雪白", "月白", "象牙", "银灰", "烟灰", "玄墨", "漆黑", "曜石",

    // 情感·意境 (32)
    "花语", "书香", "茶韵", "墨香", "琴声", "画卷", "诗行", "歌谣",
    "灯火", "棋局", "心安", "静好", "温柔", "坚韧", "勇敢", "善良",
    "希望", "梦想", "信念", "守望", "思念", "眷恋", "欢喜", "安宁",
    "从容", "淡然", "悠然", "恬静", "明朗", "澄澈", "清朗", "空灵",

    // 人文·器物 (32)
    "折扇", "纸鸢", "风铃", "铜镜", "玉佩", "香囊", "灯笼", "烛台",
    "砚台", "毛笔", "宣纸", "古琴", "箫声", "笛韵", "鼓点", "编钟",
    "青瓷", "白瓷", "紫砂", "锦缎", "丝绸", "刺绣", "剪纸", "年画",
    "楼阁", "亭台", "廊桥", "牌坊", "庭院", "篱笆", "秋千", "摇橹"
}};

static_assert(kWords.size() == kDictSize, "词库大小必须为256");

std::string RecoveryKeyGenerator::generate() {
    // 使用 OpenSSL CSPRNG 生成随机索引
    std::array<unsigned char, kWordCount> randBytes;
    if (RAND_bytes(randBytes.data(), kWordCount) != 1) {
        throw std::runtime_error("RAND_bytes failed: CSPRNG unavailable");
    }

    std::ostringstream oss;
    for (int i = 0; i < kWordCount; ++i) {
        if (i > 0) oss << "-";
        // randBytes[i] 范围 [0,255]，恰好对应 256 个词，无模偏差
        oss << kWords[randBytes[i]];
    }
    return oss.str();
}

namespace {
constexpr int kPbkdf2Iterations = 100000;
constexpr int kSaltLen = 16;
constexpr int kDerivedKeyLen = 32;

std::string bytesToHex(const unsigned char* data, int len) {
    std::ostringstream oss;
    for (int i = 0; i < len; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::vector<unsigned char> hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        auto byte = static_cast<unsigned char>(std::stoi(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::string pbkdf2(const std::string& key, const unsigned char* salt, int saltLen) {
    unsigned char derived[kDerivedKeyLen];
    if (PKCS5_PBKDF2_HMAC(key.c_str(), static_cast<int>(key.size()),
                           salt, saltLen,
                           kPbkdf2Iterations, EVP_sha256(),
                           kDerivedKeyLen, derived) != 1) {
        throw std::runtime_error("PKCS5_PBKDF2_HMAC failed");
    }
    return bytesToHex(derived, kDerivedKeyLen);
}
} // namespace

std::string RecoveryKeyGenerator::hash(const std::string& key) {
    // 生成随机盐
    unsigned char salt[kSaltLen];
    if (RAND_bytes(salt, kSaltLen) != 1) {
        throw std::runtime_error("RAND_bytes failed: CSPRNG不可用");
    }
    std::string saltHex = bytesToHex(salt, kSaltLen);
    std::string hashHex = pbkdf2(key, salt, kSaltLen);
    // 存储格式: salt:hash
    return saltHex + ":" + hashHex;
}

bool RecoveryKeyGenerator::verify(const std::string& key, const std::string& storedHash) {
    auto sep = storedHash.find(':');
    if (sep == std::string::npos) {
        // 兼容旧版无盐SHA256格式：直接比较不再通过
        return false;
    }
    std::string saltHex = storedHash.substr(0, sep);
    std::string expectedHash = storedHash.substr(sep + 1);
    auto saltBytes = hexToBytes(saltHex);
    std::string actualHash = pbkdf2(key, saltBytes.data(), static_cast<int>(saltBytes.size()));
    // 恒定时间比较，防止时序攻击
    if (actualHash.size() != expectedHash.size()) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < actualHash.size(); ++i) {
        diff |= static_cast<unsigned char>(actualHash[i]) ^ static_cast<unsigned char>(expectedHash[i]);
    }
    return diff == 0;
}

} // namespace utils
} // namespace heartlake
