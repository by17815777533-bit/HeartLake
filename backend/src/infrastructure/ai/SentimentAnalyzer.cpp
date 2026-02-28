/**
 * @brief 轻量级情感分析引擎 —— 三层融合的本地情感评分系统
 *
 * 从 EdgeAIEngine 拆分的独立子系统，无需远程 API 调用。
 *
 * 三层融合评分管线：
 *   1. 规则层：emoji 极性映射 + 标点模式（感叹号/问号密度）
 *   2. 词典层：中英文情感词典匹配，支持否定词翻转和程度副词加权
 *   3. 统计层：TTR（词汇丰富度）、平均词长等文本特征辅助修正
 *
 * 词典加载：启动时从文件系统读取外部词典，支持环境变量配置路径。
 */

#include "infrastructure/ai/SentimentAnalyzer.h"
#include <trantor/utils/Logger.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_set>

namespace heartlake {
namespace ai {

namespace {

bool containsAnyPhrase(const std::string& text, const std::vector<std::string>& markers) {
    for (const auto& marker : markers) {
        if (text.find(marker) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool envFlagEnabled(const char* name, bool defaultValue) {
    const char* raw = std::getenv(name);
    if (raw == nullptr) {
        return defaultValue;
    }
    const std::string value(raw);
    if (value == "1" || value == "true" || value == "TRUE" || value == "on" || value == "ON") {
        return true;
    }
    if (value == "0" || value == "false" || value == "FALSE" || value == "off" || value == "OFF") {
        return false;
    }
    return defaultValue;
}

std::string trimAscii(std::string s) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    return s;
}

std::filesystem::path resolveReadableFilePath(const std::filesystem::path& configuredPath) {
    if (configuredPath.empty()) {
        return configuredPath;
    }
    if (configuredPath.is_absolute()) {
        return configuredPath;
    }

    static const std::vector<std::filesystem::path> prefixes = {
        {},
        std::filesystem::path(".."),
        std::filesystem::path("backend"),
        std::filesystem::path("../backend")
    };

    for (const auto& prefix : prefixes) {
        const auto candidate = prefix.empty() ? configuredPath : (prefix / configuredPath);
        std::error_code ec;
        if (std::filesystem::is_regular_file(candidate, ec)) {
            return candidate;
        }
    }
    return configuredPath;
}

void loadDomainLexicon(std::unordered_map<std::string, float>& sentimentLexicon) {
    const char* envPath = std::getenv("EDGE_AI_SENTIMENT_LEXICON_PATH");
    const bool hasEnvPath = (envPath != nullptr && envPath[0] != '\0');
    if (!hasEnvPath) {
        return;
    }
    const std::filesystem::path rawPath(envPath);
    const std::filesystem::path resolvedPath = resolveReadableFilePath(rawPath);

    std::ifstream in(resolvedPath);
    if (!in.is_open()) {
        LOG_WARN << "[SentimentAnalyzer] EDGE_AI_SENTIMENT_LEXICON_PATH not readable: "
                 << rawPath.string() << " (resolved: " << resolvedPath.string() << ")";
        return;
    }

    size_t loaded = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto tab = line.find('\t');
        if (tab == std::string::npos) {
            continue;
        }

        std::string phrase = trimAscii(line.substr(0, tab));
        std::string weightRaw = trimAscii(line.substr(tab + 1));
        if (phrase.empty() || weightRaw.empty()) {
            continue;
        }

        try {
            const float weight = std::clamp(std::stof(weightRaw), -1.0f, 1.0f);
            sentimentLexicon[phrase] = weight;
            ++loaded;
        } catch (...) {
            continue;
        }
    }

    if (loaded > 0) {
        LOG_INFO << "[SentimentAnalyzer] Loaded domain lexicon entries: " << loaded
                 << " from " << resolvedPath.string();
    }
}

std::string convertTraditionalToSimplified(const std::string& text) {
    static const std::unordered_map<std::string, std::string> kTradToSimp = {
        {"這", "这"}, {"還", "还"}, {"個", "个"}, {"書", "书"}, {"間", "间"},
        {"來", "来"}, {"沒", "没"}, {"點", "点"}, {"覺", "觉"}, {"麼", "么"},
        {"務", "务"}, {"為", "为"}, {"價", "价"}, {"會", "会"}, {"買", "买"},
        {"樣", "样"}, {"現", "现"}, {"電", "电"}, {"開", "开"}, {"發", "发"},
        {"總", "总"}, {"問", "问"}, {"網", "网"}, {"機", "机"}, {"該", "该"},
        {"體", "体"}, {"種", "种"}, {"應", "应"}, {"題", "题"}, {"質", "质"},
        {"滿", "满"}, {"關", "关"}, {"態", "态"}, {"與", "与"}, {"請", "请"},
        {"讓", "让"}, {"給", "给"}, {"進", "进"}, {"們", "们"}, {"說", "说"},
        {"愛", "爱"}, {"優", "优"}, {"壓", "压"}, {"頭", "头"}, {"顯", "显"},
        {"鍵", "键"}, {"盤", "盘"}, {"軟", "软"}, {"續", "续"}, {"線", "线"},
        {"誤", "误"}, {"試", "试"}, {"驗", "验"}, {"號", "号"}, {"風", "风"},
        {"氣", "气"}, {"溫", "温"}, {"熱", "热"}, {"顏", "颜"}, {"長", "长"},
        {"舊", "旧"}, {"馬", "马"}, {"錄", "录"}, {"檔", "档"}, {"驅", "驱"},
        {"動", "动"}, {"衛", "卫"}, {"環", "环"}, {"櫃", "柜"}, {"檯", "台"},
        {"飯", "饭"}, {"專", "专"}, {"業", "业"}, {"級", "级"}, {"標", "标"},
        {"準", "准"}, {"壞", "坏"}, {"爛", "烂"}, {"髒", "脏"}, {"淨", "净"},
        {"錢", "钱"}, {"貴", "贵"}, {"實", "实"}, {"際", "际"}, {"傳", "传"},
        {"統", "统"}, {"獨", "独"}, {"學", "学"}, {"習", "习"}, {"歡", "欢"},
        {"樂", "乐"}, {"傷", "伤"}, {"憂", "忧"}, {"慮", "虑"}, {"擔", "担"},
        {"難", "难"}, {"過", "过"}, {"遠", "远"}, {"遲", "迟"}, {"遜", "逊"},
        {"厲", "厉"}, {"厭", "厌"}, {"鬆", "松"}, {"穩", "稳"}, {"靜", "静"},
        {"驚", "惊"}, {"訝", "讶"}, {"悶", "闷"}, {"亂", "乱"}, {"處", "处"},
        {"緊", "紧"}, {"張", "张"}, {"燒", "烧"}, {"殘", "残"}, {"酷", "酷"},
        {"聽", "听"}, {"聲", "声"}, {"畫", "画"}, {"層", "层"}, {"紋", "纹"},
        {"節", "节"}, {"購", "购"}, {"運", "运"}, {"遞", "递"}, {"裝", "装"},
        {"包", "包"}, {"測", "测"}, {"評", "评"}, {"論", "论"}, {"對", "对"},
        {"錯", "错"}, {"關", "关"}, {"閉", "闭"}, {"連", "连"}, {"斷", "断"},
        {"訊", "讯"}, {"號", "号"}, {"遙", "遥"}, {"控", "控"}, {"攝", "摄"},
        {"影", "影"}, {"產", "产"}, {"值", "值"}, {"廣", "广"}, {"告", "告"},
        {"費", "费"}, {"別", "别"}, {"離", "离"}, {"佔", "占"}, {"內", "内"},
        {"壓", "压"}, {"縮", "缩"}, {"檢", "检"}, {"查", "查"}, {"轉", "转"},
        {"換", "换"}, {"藍", "蓝"}, {"牙", "牙"}, {"蘋", "苹"}, {"果", "果"}
    };

    std::string out;
    out.reserve(text.size());

    for (size_t i = 0; i < text.size();) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        size_t len = 1;
        if ((c & 0xE0) == 0xC0) {
            len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            len = 4;
        }
        if (i + len > text.size()) {
            out.push_back(text[i]);
            ++i;
            continue;
        }
        std::string token = text.substr(i, len);
        auto it = kTradToSimp.find(token);
        if (it != kTradToSimp.end()) {
            out += it->second;
        } else {
            out += token;
        }
        i += len;
    }
    return out;
}

std::atomic<int> gOnnxInFlight{0};

bool tryAcquireOnnxSlot(std::atomic<int>& counter, int limit) {
    if (limit <= 0) {
        return true;
    }
    int current = counter.load(std::memory_order_relaxed);
    while (current < limit) {
        if (counter.compare_exchange_weak(
                current, current + 1,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
            return true;
        }
    }
    return false;
}

class OnnxInFlightGuard {
public:
    OnnxInFlightGuard(std::atomic<int>& counter, bool acquired)
        : counter_(counter), acquired_(acquired) {}

    ~OnnxInFlightGuard() {
        if (acquired_) {
            counter_.fetch_sub(1, std::memory_order_relaxed);
        }
    }

private:
    std::atomic<int>& counter_;
    bool acquired_{false};
};

struct AffectiveAnchorSignal {
    float score{0.0f};      // [-1, 1]
    float strength{0.0f};   // [0, 1]
    float conflict{0.0f};   // [0, 1]
};

float weightedPhraseScore(const std::string& text,
                          const std::vector<std::pair<std::string, float>>& lexicon) {
    float score = 0.0f;
    for (const auto& [phrase, weight] : lexicon) {
        size_t pos = 0;
        while ((pos = text.find(phrase, pos)) != std::string::npos) {
            score += weight;
            pos += phrase.size();
        }
    }
    return score;
}

std::string refineMoodFromCues(const std::string& text,
                               float score,
                               const std::string& fallbackMood) {
    static const std::vector<std::pair<std::string, float>> kHappyCues = {
        {"开心", 1.2f}, {"高兴", 1.1f}, {"快乐", 1.1f}, {"喜悦", 1.0f},
        {"幸福", 1.0f}, {"暖暖", 0.9f}, {"被认可", 0.95f}, {"认可", 0.75f},
        {"被夸", 0.90f}, {"夸了我", 0.95f}, {"礼物", 0.90f}, {"惊喜", 0.85f}
    };
    static const std::vector<std::pair<std::string, float>> kCalmCues = {
        {"平静", 1.3f}, {"放松", 1.1f}, {"放松下来", 1.2f}, {"安定", 1.2f},
        {"安稳", 1.0f}, {"从容", 0.9f}, {"淡定", 0.9f}, {"舒缓", 0.95f},
        {"慢下来", 0.8f}, {"继续努力", 0.95f}, {"散步聊天", 0.85f},
        {"喝茶", 0.75f}, {"心里安定", 1.1f}
    };
    static const std::vector<std::pair<std::string, float>> kAnxiousCues = {
        {"焦虑", 1.4f}, {"担心", 1.2f}, {"紧张", 1.1f}, {"不安", 1.1f},
        {"压力", 1.0f}, {"发紧", 1.0f}, {"心跳很快", 1.1f}, {"慌", 0.95f},
        {"害怕", 1.05f}, {"恐惧", 1.2f},
        {"不敢", 0.95f}, {"急得", 1.0f}, {"不知所措", 1.1f},
        {"睡不着", 0.9f}, {"好焦虑", 1.3f}
    };
    static const std::vector<std::pair<std::string, float>> kSadCues = {
        {"难过", 1.3f}, {"伤心", 1.3f}, {"失落", 1.1f}, {"受挫", 1.0f},
        {"低落", 1.0f}, {"沉默了很久", 0.95f}, {"难受", 1.1f}, {"痛苦", 1.2f},
        {"裁员", 1.0f}, {"闹翻", 1.0f}, {"去世", 1.2f}, {"空落落", 1.1f},
        {"想家", 1.0f}, {"走丢", 1.0f}, {"背叛", 1.2f}, {"心如刀割", 1.3f},
        {"失恋", 1.2f}, {"难熬", 1.1f}, {"心疼", 0.95f}, {"舍不得", 0.95f},
        {"孤独", 1.3f}, {"孤单", 1.2f}, {"寂寞", 1.2f}, {"脆弱", 1.05f},
        {"落寞", 1.2f}, {"无助", 1.1f},
        {"好笨", 0.9f}, {"怎么办", 0.8f}
    };
    static const std::vector<std::pair<std::string, float>> kAngryCues = {
        {"生气", 1.4f}, {"愤怒", 1.5f}, {"恼火", 1.4f}, {"火大", 1.5f},
        {"气炸", 1.3f}, {"抓狂", 1.1f}, {"气死", 1.2f}, {"被连续打断", 0.95f},
        {"莫名否定", 0.95f}, {"连续否决", 0.95f},
        {"忍无可忍", 1.3f}, {"太过分", 1.2f}, {"太不负责", 1.1f},
        {"素质太差", 1.1f}, {"态度恶劣", 1.2f}, {"无法无天", 1.2f},
        {"黑心", 1.1f}, {"服了", 0.95f}, {"受不了", 1.0f},
        {"不退款", 0.9f}, {"吵得", 0.85f}, {"插队", 0.9f},
        {"扣工资", 1.0f}, {"碰瓷", 1.0f}, {"理直气壮", 0.9f}
    };
    static const std::vector<std::pair<std::string, float>> kSurprisedCues = {
        {"震惊", 1.4f}, {"惊讶", 1.3f}, {"惊到", 1.3f}, {"惊到了", 1.3f},
        {"出乎意料", 1.3f}, {"没反应过来", 1.2f}, {"太突然", 1.0f},
        {"突然", 0.8f}, {"居然", 0.9f}, {"意外", 0.9f},
        {"天哪", 1.2f}, {"不敢相信", 1.2f}, {"没想到", 1.1f},
        {"惊喜", 1.1f}, {"突然出现", 1.0f}, {"竟然", 0.9f}
    };
    static const std::vector<std::pair<std::string, float>> kConfusedCues = {
        {"困惑", 1.4f}, {"懵", 1.3f}, {"迷茫", 1.1f}, {"乱了", 1.2f},
        {"搞不懂", 1.2f}, {"前后矛盾", 1.1f}, {"不知道该", 1.0f},
        {"信息太多", 1.0f}, {"没头绪", 1.0f}
    };
    static const std::vector<std::pair<std::string, float>> kNeutralCues = {
        {"没什么特别情绪", 1.5f}, {"心情平平", 1.4f}, {"日程正常", 1.2f},
        {"按计划", 1.1f}, {"一般", 0.75f}, {"就这样", 0.8f},
        {"发呆", 0.95f}, {"没有好消息也没有坏消息", 1.6f}, {"什么都没发生", 1.4f}
    };

    const float happyCue = weightedPhraseScore(text, kHappyCues);
    const float calmCue = weightedPhraseScore(text, kCalmCues);
    const float anxiousCue = weightedPhraseScore(text, kAnxiousCues);
    const float sadCue = weightedPhraseScore(text, kSadCues);
    const float angryCue = weightedPhraseScore(text, kAngryCues);
    const float surprisedCue = weightedPhraseScore(text, kSurprisedCues);
    const float confusedCue = weightedPhraseScore(text, kConfusedCues);
    float neutralCue = weightedPhraseScore(text, kNeutralCues);

    if (text.find("不开心") != std::string::npos &&
        text.find("不难过") != std::string::npos) {
        neutralCue += 1.6f;
    }

    const float maxCue = std::max(
        {happyCue, calmCue, anxiousCue, sadCue, angryCue, surprisedCue, confusedCue, neutralCue});

    if (neutralCue >= 1.1f &&
        neutralCue >= calmCue &&
        neutralCue >= anxiousCue &&
        neutralCue >= sadCue &&
        std::abs(score) <= 0.75f) {
        return "neutral";
    }
    if (angryCue >= 1.0f && angryCue >= sadCue * 0.90f && angryCue >= anxiousCue * 0.90f) {
        return "angry";
    }
    if (surprisedCue >= 1.0f && surprisedCue + 0.10f >= happyCue && surprisedCue >= anxiousCue) {
        return "surprised";
    }
    if (confusedCue >= 1.0f && confusedCue + 0.10f >= anxiousCue && confusedCue + 0.10f >= sadCue) {
        return "confused";
    }
    if (anxiousCue >= 0.95f && anxiousCue >= sadCue * 0.85f) {
        return "anxious";
    }
    if (sadCue >= 0.95f && sadCue >= anxiousCue * 1.05f) {
        return "sad";
    }
    if (calmCue >= 1.0f && calmCue >= happyCue * 0.90f && std::abs(score) <= 0.90f) {
        return "calm";
    }
    if (happyCue >= 0.95f && score >= -0.20f) {
        return "happy";
    }
    // 无明确情感信号时，倾向 neutral（纯陈述句保护）
    if (maxCue < 0.35f && std::abs(score) < 0.55f) {
        return "neutral";
    }
    if (maxCue < 0.60f && std::abs(score) < 0.18f) {
        return "neutral";
    }

    return fallbackMood;
}

float calibrateScoreByMoodCue(const std::string& text,
                              const std::string& mood,
                              float score) {
    static const std::vector<std::string> kNeutralStrongSignals = {
        "没什么特别情绪", "心情平平", "日程正常", "按计划", "没有好消息也没有坏消息",
        "什么都没发生", "不开心，也不难过", "不开心也不难过", "就是发呆", "心情一般"
    };

    // calm/confused 最终会映射到对外 neutral 标签，保留一定极性幅度避免“语义明确却被判中性”。
    if (mood == "calm") {
        return std::max(score, 0.22f);
    }
    if (mood == "confused") {
        return std::min(score, -0.22f);
    }

    if (mood == "neutral") {
        if (containsAnyPhrase(text, kNeutralStrongSignals)) {
            return std::clamp(score, -0.08f, 0.08f);
        }
        return std::clamp(score, -0.15f, 0.15f);
    }

    if ((mood == "anxious" || mood == "sad" || mood == "angry" || mood == "confused") &&
        score > -0.05f) {
        return -std::max(0.12f, std::abs(score) * 0.45f);
    }

    if ((mood == "happy" || mood == "calm" || mood == "surprised") &&
        score < 0.05f) {
        return std::max(0.12f, std::abs(score) * 0.45f);
    }

    return score;
}

AffectiveAnchorSignal computeAffectiveAnchor(const std::string& text) {
    static const std::vector<std::pair<std::string, float>> kPositiveAnchors = {
        {"今天", 0.18f}, {"礼物", 0.95f}, {"收到了", 0.70f}, {"收到", 0.55f},
        {"夸", 0.52f}, {"表扬", 0.92f}, {"认可", 0.82f}, {"肯定", 0.80f},
        {"通过", 0.72f}, {"成功", 0.78f}, {"晋级", 0.70f}, {"获奖", 0.86f},
        {"惊喜", 0.82f}, {"感谢", 0.66f}, {"幸福", 0.78f}, {"开心", 0.86f},
        {"快乐", 0.84f}, {"好看", 0.50f}, {"喜欢", 0.58f}, {"治愈", 0.64f}
    };
    static const std::vector<std::pair<std::string, float>> kNegativeAnchors = {
        {"焦虑", 0.95f}, {"担心", 0.75f}, {"不安", 0.72f}, {"紧张", 0.62f},
        {"难过", 0.88f}, {"伤心", 0.92f}, {"失落", 0.72f}, {"害怕", 0.84f},
        {"恐惧", 0.90f}, {"压力", 0.72f}, {"烦躁", 0.74f}, {"痛苦", 0.95f},
        {"绝望", 1.00f}, {"崩溃", 1.00f}, {"失败", 0.72f}, {"后悔", 0.68f}
    };
    static const std::vector<std::string> kContrastMarkers = {
        "但是", "但", "不过", "然而", "可是", "只是"
    };

    float posScore = weightedPhraseScore(text, kPositiveAnchors);
    float negScore = weightedPhraseScore(text, kNegativeAnchors);

    size_t markerPos = std::string::npos;
    size_t markerLen = 0;
    for (const auto& marker : kContrastMarkers) {
        const auto pos = text.rfind(marker);
        if (pos != std::string::npos && (markerPos == std::string::npos || pos > markerPos)) {
            markerPos = pos;
            markerLen = marker.size();
        }
    }
    if (markerPos != std::string::npos && markerPos + markerLen < text.size()) {
        const auto tail = text.substr(markerPos + markerLen);
        posScore += 1.25f * weightedPhraseScore(tail, kPositiveAnchors);
        negScore += 1.25f * weightedPhraseScore(tail, kNegativeAnchors);
    }

    const float evidence = posScore + negScore;
    if (evidence <= 0.01f) {
        return {};
    }

    const float score = std::clamp((posScore - negScore) / (evidence + 1.2f), -1.0f, 1.0f);
    const float strength = std::clamp(std::tanh(evidence / 4.5f), 0.0f, 1.0f);
    const float conflict = (posScore > 0.01f && negScore > 0.01f)
        ? std::clamp(std::min(posScore, negScore) / std::max(posScore, negScore), 0.0f, 1.0f)
        : 0.0f;
    return {score, strength, conflict};
}

} // namespace

// ============================================================================
// 配置 & 词典加载
// ============================================================================

void SentimentAnalyzer::configure(int cacheTTLSec, size_t cacheMaxSize) {
    sentimentCacheTTLSeconds_ = cacheTTLSec;
    sentimentCacheMaxSize_ = cacheMaxSize;
}

void SentimentAnalyzer::loadLexicon() {
    // 预分配桶数，避免插入过程中多次 rehash
    sentimentLexicon_.reserve(300);
    domainSentimentLexicon_.reserve(128);
    intensifiers_.reserve(40);
    negators_.reserve(30);

    // 正面情感词典
    sentimentLexicon_["happy"] = 0.8f;
    sentimentLexicon_["love"] = 0.9f;
    sentimentLexicon_["great"] = 0.7f;
    sentimentLexicon_["wonderful"] = 0.85f;
    sentimentLexicon_["excellent"] = 0.9f;
    sentimentLexicon_["amazing"] = 0.85f;
    sentimentLexicon_["fantastic"] = 0.85f;
    sentimentLexicon_["beautiful"] = 0.75f;
    sentimentLexicon_["good"] = 0.6f;
    sentimentLexicon_["nice"] = 0.55f;
    sentimentLexicon_["awesome"] = 0.8f;
    sentimentLexicon_["joy"] = 0.85f;
    sentimentLexicon_["delight"] = 0.8f;
    sentimentLexicon_["pleased"] = 0.65f;
    sentimentLexicon_["grateful"] = 0.75f;
    sentimentLexicon_["thankful"] = 0.7f;
    sentimentLexicon_["excited"] = 0.8f;
    sentimentLexicon_["cheerful"] = 0.75f;
    sentimentLexicon_["brilliant"] = 0.8f;
    sentimentLexicon_["perfect"] = 0.9f;

    // 中文正面词
    sentimentLexicon_["开心"] = 0.8f;
    sentimentLexicon_["快乐"] = 0.85f;
    sentimentLexicon_["喜欢"] = 0.7f;
    sentimentLexicon_["爱"] = 0.9f;
    sentimentLexicon_["棒"] = 0.75f;
    sentimentLexicon_["好"] = 0.5f;
    sentimentLexicon_["优秀"] = 0.8f;
    sentimentLexicon_["感谢"] = 0.7f;
    sentimentLexicon_["幸福"] = 0.9f;
    sentimentLexicon_["温暖"] = 0.7f;
    sentimentLexicon_["美好"] = 0.75f;
    sentimentLexicon_["精彩"] = 0.8f;
    sentimentLexicon_["赞"] = 0.7f;
    sentimentLexicon_["厉害"] = 0.7f;
    sentimentLexicon_["感动"] = 0.75f;
    sentimentLexicon_["舒适"] = 0.65f;
    sentimentLexicon_["满足"] = 0.7f;
    sentimentLexicon_["自信"] = 0.7f;
    sentimentLexicon_["希望"] = 0.65f;
    sentimentLexicon_["勇敢"] = 0.7f;
    // 扩充中文正面词
    sentimentLexicon_["高兴"] = 0.8f;
    sentimentLexicon_["愉快"] = 0.75f;
    sentimentLexicon_["欣喜"] = 0.8f;
    sentimentLexicon_["欢乐"] = 0.8f;
    sentimentLexicon_["兴奋"] = 0.8f;
    sentimentLexicon_["激动"] = 0.75f;
    sentimentLexicon_["享受"] = 0.7f;
    sentimentLexicon_["充实"] = 0.65f;
    sentimentLexicon_["感恩"] = 0.75f;
    sentimentLexicon_["幸运"] = 0.7f;
    sentimentLexicon_["愉悦"] = 0.75f;
    sentimentLexicon_["欣慰"] = 0.7f;
    sentimentLexicon_["惊喜"] = 0.8f;
    sentimentLexicon_["甜蜜"] = 0.8f;
    sentimentLexicon_["浪漫"] = 0.7f;
    sentimentLexicon_["舒心"] = 0.7f;
    sentimentLexicon_["安心"] = 0.65f;
    sentimentLexicon_["放松"] = 0.6f;
    sentimentLexicon_["宁静"] = 0.6f;
    sentimentLexicon_["平静"] = 0.55f;
    sentimentLexicon_["从容"] = 0.6f;
    sentimentLexicon_["淡定"] = 0.55f;
    sentimentLexicon_["自在"] = 0.65f;
    sentimentLexicon_["轻松"] = 0.6f;
    sentimentLexicon_["惬意"] = 0.7f;
    sentimentLexicon_["舒畅"] = 0.7f;
    sentimentLexicon_["治愈"] = 0.75f;
    sentimentLexicon_["暖心"] = 0.75f;
    sentimentLexicon_["可爱"] = 0.7f;
    sentimentLexicon_["有趣"] = 0.65f;
    sentimentLexicon_["好玩"] = 0.65f;
    sentimentLexicon_["期待"] = 0.65f;
    sentimentLexicon_["向往"] = 0.6f;
    sentimentLexicon_["珍惜"] = 0.65f;
    sentimentLexicon_["值得"] = 0.6f;
    sentimentLexicon_["成功"] = 0.75f;
    sentimentLexicon_["进步"] = 0.65f;
    sentimentLexicon_["成长"] = 0.6f;
    sentimentLexicon_["收获"] = 0.7f;
    sentimentLexicon_["骄傲"] = 0.7f;
    sentimentLexicon_["自豪"] = 0.75f;
    sentimentLexicon_["加油"] = 0.7f;
    sentimentLexicon_["奥利给"] = 0.75f;
    sentimentLexicon_["绝绝子"] = 0.7f;
    sentimentLexicon_["yyds"] = 0.75f;
    sentimentLexicon_["真香"] = 0.7f;
    sentimentLexicon_["上头"] = 0.65f;
    // 认可/被表扬语义（保证"被夸被肯定"不被低估为平静）
    sentimentLexicon_["夸奖"] = 0.82f;
    sentimentLexicon_["表扬"] = 0.85f;
    sentimentLexicon_["认可"] = 0.82f;
    sentimentLexicon_["肯定"] = 0.78f;
    sentimentLexicon_["称赞"] = 0.80f;
    sentimentLexicon_["赞扬"] = 0.80f;
    sentimentLexicon_["赞许"] = 0.78f;
    sentimentLexicon_["嘉奖"] = 0.86f;
    sentimentLexicon_["鼓励"] = 0.72f;
    sentimentLexicon_["夸"] = 0.58f;
    sentimentLexicon_["被夸"] = 0.82f;
    sentimentLexicon_["被表扬"] = 0.86f;

    // 英文正面补充
    sentimentLexicon_["hopeful"] = 0.7f;
    sentimentLexicon_["proud"] = 0.75f;
    sentimentLexicon_["confident"] = 0.7f;
    sentimentLexicon_["peaceful"] = 0.65f;
    sentimentLexicon_["content"] = 0.6f;
    sentimentLexicon_["inspired"] = 0.75f;

    // 负面情感词典
    sentimentLexicon_["sad"] = -0.7f;
    sentimentLexicon_["angry"] = -0.8f;
    sentimentLexicon_["hate"] = -0.9f;
    sentimentLexicon_["terrible"] = -0.85f;
    sentimentLexicon_["horrible"] = -0.85f;
    sentimentLexicon_["awful"] = -0.8f;
    sentimentLexicon_["bad"] = -0.6f;
    sentimentLexicon_["disgusting"] = -0.85f;
    sentimentLexicon_["fear"] = -0.7f;
    sentimentLexicon_["scared"] = -0.65f;
    sentimentLexicon_["worried"] = -0.5f;
    sentimentLexicon_["anxious"] = -0.55f;
    sentimentLexicon_["depressed"] = -0.85f;
    sentimentLexicon_["miserable"] = -0.8f;
    sentimentLexicon_["frustrated"] = -0.65f;
    sentimentLexicon_["disappointed"] = -0.6f;
    sentimentLexicon_["upset"] = -0.65f;
    sentimentLexicon_["lonely"] = -0.6f;
    sentimentLexicon_["painful"] = -0.7f;
    sentimentLexicon_["annoyed"] = -0.55f;

    // 中文负面词
    sentimentLexicon_["难过"] = -0.7f;
    sentimentLexicon_["伤心"] = -0.75f;
    sentimentLexicon_["生气"] = -0.7f;
    sentimentLexicon_["愤怒"] = -0.85f;
    sentimentLexicon_["讨厌"] = -0.7f;
    sentimentLexicon_["恨"] = -0.9f;
    sentimentLexicon_["害怕"] = -0.65f;
    sentimentLexicon_["恐惧"] = -0.75f;
    sentimentLexicon_["焦虑"] = -0.6f;
    sentimentLexicon_["担心"] = -0.5f;
    sentimentLexicon_["痛苦"] = -0.8f;
    sentimentLexicon_["绝望"] = -0.9f;
    sentimentLexicon_["孤独"] = -0.65f;
    sentimentLexicon_["无聊"] = -0.4f;
    sentimentLexicon_["烦"] = -0.55f;
    sentimentLexicon_["累"] = -0.45f;
    sentimentLexicon_["失望"] = -0.6f;
    sentimentLexicon_["后悔"] = -0.65f;
    sentimentLexicon_["委屈"] = -0.6f;
    sentimentLexicon_["崩溃"] = -0.85f;
    // 扩充中文负面词
    sentimentLexicon_["沮丧"] = -0.7f;
    sentimentLexicon_["郁闷"] = -0.6f;
    sentimentLexicon_["烦躁"] = -0.6f;
    sentimentLexicon_["不安"] = -0.55f;
    sentimentLexicon_["紧张"] = -0.5f;
    sentimentLexicon_["压抑"] = -0.7f;
    sentimentLexicon_["窒息"] = -0.8f;
    sentimentLexicon_["心碎"] = -0.85f;
    sentimentLexicon_["心痛"] = -0.75f;
    sentimentLexicon_["失落"] = -0.6f;
    sentimentLexicon_["无助"] = -0.7f;
    sentimentLexicon_["迷茫"] = -0.55f;
    sentimentLexicon_["疲惫"] = -0.55f;
    sentimentLexicon_["厌倦"] = -0.6f;
    sentimentLexicon_["受伤"] = -0.65f;
    sentimentLexicon_["心酸"] = -0.65f;
    sentimentLexicon_["憋屈"] = -0.65f;
    sentimentLexicon_["窝火"] = -0.6f;
    sentimentLexicon_["恼火"] = -0.6f;
    sentimentLexicon_["抓狂"] = -0.65f;
    sentimentLexicon_["崩"] = -0.7f;
    sentimentLexicon_["emo"] = -0.55f;
    sentimentLexicon_["破防"] = -0.6f;
    sentimentLexicon_["裂开"] = -0.55f;
    sentimentLexicon_["麻了"] = -0.5f;
    sentimentLexicon_["无语"] = -0.5f;
    sentimentLexicon_["社死"] = -0.6f;
    // 英文负面补充
    sentimentLexicon_["hopeless"] = -0.85f;
    sentimentLexicon_["helpless"] = -0.7f;
    sentimentLexicon_["exhausted"] = -0.6f;
    sentimentLexicon_["stressed"] = -0.55f;
    sentimentLexicon_["overwhelmed"] = -0.65f;
    sentimentLexicon_["heartbroken"] = -0.85f;
    // 心理健康相关高权重词
    sentimentLexicon_["自杀"] = -1.0f;
    sentimentLexicon_["自残"] = -0.95f;
    sentimentLexicon_["不想活"] = -1.0f;
    sentimentLexicon_["活不下去"] = -1.0f;
    sentimentLexicon_["轻生"] = -0.95f;
    sentimentLexicon_["suicide"] = -1.0f;
    sentimentLexicon_["self-harm"] = -0.95f;
    // 睡眠/身体不适相关
    sentimentLexicon_["失眠"] = -0.55f;
    sentimentLexicon_["睡不着"] = -0.50f;
    sentimentLexicon_["头疼"] = -0.50f;
    sentimentLexicon_["头痛"] = -0.50f;
    sentimentLexicon_["胃疼"] = -0.50f;
    sentimentLexicon_["发烧"] = -0.45f;
    sentimentLexicon_["感冒"] = -0.40f;
    sentimentLexicon_["不舒服"] = -0.50f;
    sentimentLexicon_["难受"] = -0.55f;

    // 领域词典（可通过 EDGE_AI_SENTIMENT_LEXICON_PATH 覆盖）。
    loadDomainLexicon(domainSentimentLexicon_);

    // 程度副词
    intensifiers_["很"] = 1.3f;
    intensifiers_["非常"] = 1.5f;
    intensifiers_["特别"] = 1.4f;
    intensifiers_["超"] = 1.4f;
    intensifiers_["极其"] = 1.6f;
    intensifiers_["太"] = 1.4f;
    intensifiers_["真"] = 1.2f;
    intensifiers_["好"] = 1.2f;
    intensifiers_["超级"] = 1.5f;
    intensifiers_["无比"] = 1.5f;
    intensifiers_["格外"] = 1.3f;
    intensifiers_["十分"] = 1.4f;
    intensifiers_["万分"] = 1.5f;
    intensifiers_["异常"] = 1.4f;
    intensifiers_["相当"] = 1.3f;
    intensifiers_["尤其"] = 1.3f;
    intensifiers_["极度"] = 1.6f;
    intensifiers_["贼"] = 1.3f;
    intensifiers_["巨"] = 1.4f;
    intensifiers_["死了"] = 1.5f;
    intensifiers_["要命"] = 1.5f;
    intensifiers_["very"] = 1.3f;
    intensifiers_["really"] = 1.3f;
    intensifiers_["extremely"] = 1.5f;
    intensifiers_["absolutely"] = 1.5f;
    intensifiers_["incredibly"] = 1.5f;
    intensifiers_["so"] = 1.2f;
    intensifiers_["quite"] = 1.1f;
    intensifiers_["pretty"] = 1.1f;
    intensifiers_["deeply"] = 1.3f;
    intensifiers_["highly"] = 1.3f;
    intensifiers_["terribly"] = 1.4f;
    intensifiers_["awfully"] = 1.4f;

    // 否定词
    negators_["不"] = -1.0f;
    negators_["没"] = -1.0f;
    negators_["没有"] = -1.0f;
    negators_["不是"] = -1.0f;
    negators_["别"] = -1.0f;
    negators_["莫"] = -1.0f;
    negators_["未"] = -1.0f;
    negators_["无"] = -1.0f;
    negators_["非"] = -1.0f;
    negators_["不太"] = -0.7f;
    negators_["不怎么"] = -0.7f;
    negators_["不大"] = -0.7f;
    negators_["not"] = -1.0f;
    negators_["no"] = -1.0f;
    negators_["never"] = -1.0f;
    negators_["don't"] = -1.0f;
    negators_["doesn't"] = -1.0f;
    negators_["didn't"] = -1.0f;
    negators_["won't"] = -1.0f;
    negators_["can't"] = -1.0f;
    negators_["couldn't"] = -1.0f;
    negators_["shouldn't"] = -1.0f;
    negators_["wouldn't"] = -1.0f;
    negators_["isn't"] = -1.0f;
    negators_["aren't"] = -1.0f;
    negators_["wasn't"] = -1.0f;
    negators_["weren't"] = -1.0f;
    negators_["hardly"] = -0.8f;
    negators_["barely"] = -0.8f;
    negators_["scarcely"] = -0.8f;

    LOG_INFO << "[SentimentAnalyzer] Lexicon loaded: "
             << sentimentLexicon_.size() << " sentiment words, "
             << intensifiers_.size() << " intensifiers, "
             << negators_.size() << " negators";
}

std::vector<std::string> SentimentAnalyzer::tokenizeUTF8(const std::string& text) const {
    std::vector<std::string> tokens;
    std::string current;

    size_t i = 0;
    while (i < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[i]);

        // ASCII字母/数字 -> 累积为英文token
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '\'') {
            current += static_cast<char>(std::tolower(c));
            ++i;
            continue;
        }

        // 遇到非字母数字，先flush当前英文token
        if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }

        // 判断UTF-8多字节字符（中文等）
        if (c >= 0xC0) {
            int byteCount = 1;
            if ((c & 0xE0) == 0xC0) byteCount = 2;
            else if ((c & 0xF0) == 0xE0) byteCount = 3;
            else if ((c & 0xF8) == 0xF0) byteCount = 4;

            if (i + byteCount <= text.size()) {
                // 每个中文字符作为独立token
                std::string utf8Char = text.substr(i, byteCount);
                tokens.push_back(utf8Char);

                // 同时尝试提取2-6字的中文词组（覆盖"睡不着/提不起精神"等短语）
                size_t j = i + byteCount;
                for (int wordLen = 2; wordLen <= 6 && j < text.size(); ++wordLen) {
                    unsigned char nc = static_cast<unsigned char>(text[j]);
                    if (nc < 0xC0) break;
                    int nb = 1;
                    if ((nc & 0xE0) == 0xC0) nb = 2;
                    else if ((nc & 0xF0) == 0xE0) nb = 3;
                    else if ((nc & 0xF8) == 0xF0) nb = 4;
                    if (j + nb > text.size()) break;

                    // 透明哈希: 用 string_view 查找避免非匹配时的 string 构造开销
                    std::string_view compoundView(text.data() + i, (j + nb) - i);
                    std::string compound(compoundView);
                    if (sentimentLexicon_.count(compound) ||
                        intensifiers_.count(compound) ||
                        negators_.count(compound)) {
                        tokens.push_back(std::move(compound));
                    }
                    j += nb;
                }
            }
            i += byteCount;
        } else {
            // 空格、标点等跳过
            ++i;
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

float SentimentAnalyzer::ruleSentiment(const std::string& text) const {
    float score = 0.0f;
    int signals = 0;
    int positiveEmojiHits = 0;
    int negativeEmojiHits = 0;

    // 表情符号检测 — 单次扫描，O(N) 替代 O(N*E)
    static const std::unordered_map<std::string, float> emojiScores = {
        {"😊", 0.3f}, {"😄", 0.3f}, {"😃", 0.3f}, {"🥰", 0.3f}, {"❤", 0.3f},
        {"💕", 0.3f}, {"👍", 0.3f}, {"🎉", 0.3f}, {"✨", 0.3f}, {"😁", 0.3f},
        {"🤗", 0.3f}, {"💪", 0.3f}, {"🌟", 0.3f}, {"😍", 0.3f}, {"🥳", 0.3f},
        {"💖", 0.3f}, {"😘", 0.3f}, {"🙂", 0.3f}, {"☺", 0.3f}, {"💯", 0.3f},
        {"😢", -0.3f}, {"😭", -0.3f}, {"😡", -0.3f}, {"😠", -0.3f}, {"💔", -0.3f},
        {"😞", -0.3f}, {"😔", -0.3f}, {"😤", -0.3f}, {"😰", -0.3f}, {"😱", -0.3f},
        {"🤮", -0.3f}, {"👎", -0.3f}, {"😩", -0.3f}, {"😫", -0.3f}, {"😣", -0.3f},
        {"😖", -0.3f}, {"😿", -0.3f}, {"🥺", -0.3f}, {"😥", -0.3f}, {"😓", -0.3f}
    };
    {
        size_t ei = 0;
        while (ei < text.size()) {
            unsigned char uc = static_cast<unsigned char>(text[ei]);
            if (uc >= 0xC0) {
                int bc = 1;
                if ((uc & 0xE0) == 0xC0) bc = 2;
                else if ((uc & 0xF0) == 0xE0) bc = 3;
                else if ((uc & 0xF8) == 0xF0) bc = 4;
                if (ei + bc <= text.size()) {
                    auto it = emojiScores.find(text.substr(ei, bc));
                    if (it != emojiScores.end()) {
                        score += it->second;
                        ++signals;
                        if (it->second > 0.0f) {
                            ++positiveEmojiHits;
                        } else if (it->second < 0.0f) {
                            ++negativeEmojiHits;
                        }
                    }
                }
                ei += bc;
            } else {
                ++ei;
            }
        }
    }

    // 标点模式分析
    int exclamationCount = 0;
    int questionCount = 0;
    int ellipsisCount = 0;
    for (char c : text) {
        if (c == '!') ++exclamationCount;
        if (c == '?') ++questionCount;
    }
    // 中文标点
    {
        size_t pos = 0;
        while ((pos = text.find("！", pos)) != std::string::npos) {
            ++exclamationCount;
            pos += 3;
        }
    }
    {
        size_t pos = 0;
        while ((pos = text.find("？", pos)) != std::string::npos) {
            ++questionCount;
            pos += 3;
        }
    }
    {
        size_t pos = 0;
        while ((pos = text.find("...", pos)) != std::string::npos) {
            ++ellipsisCount;
            pos += 3;
        }
    }
    {
        size_t pos = 0;
        while ((pos = text.find("……", pos)) != std::string::npos) {
            ++ellipsisCount;
            pos += 6;
        }
    }

    // 感叹号增强情感强度
    if (exclamationCount > 0) {
        score += 0.1f * std::min(exclamationCount, 5);
        ++signals;
    }
    // 省略号暗示犹豫/消极
    if (ellipsisCount > 0) {
        score -= 0.1f * std::min(ellipsisCount, 3);
        ++signals;
    }

    // 全大写检测（英文，表示强烈情感）
    int upperCount = 0;
    int letterCount = 0;
    for (char c : text) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            ++letterCount;
            if (std::isupper(static_cast<unsigned char>(c))) ++upperCount;
        }
    }
    if (letterCount > 5 && upperCount > letterCount * 0.7) {
        // 全大写 -> 强烈情感，放大现有分数
        score *= 1.5f;
        ++signals;
    }

    // 文本颜文字检测
    static const std::vector<std::pair<std::string, float>> kaomoji = {
        {"(^_^)", 0.4f}, {"(^^)", 0.3f}, {"(≧▽≦)", 0.5f}, {"(╥_╥)", -0.5f},
        {"(T_T)", -0.4f}, {"(>_<)", -0.3f}, {":)", 0.3f}, {":(", -0.3f},
        {":D", 0.4f}, {";)", 0.2f}, {"XD", 0.4f}, {"orz", -0.3f},
        {"QAQ", -0.4f}, {"TAT", -0.4f}, {"^_^", 0.3f}, {">_<", -0.3f}
    };
    for (const auto& [pattern, val] : kaomoji) {
        if (text.find(pattern) != std::string::npos) {
            score += val;
            ++signals;
        }
    }

    if (signals == 0) return 0.0f;
    float normalized = score / std::max(1, signals) * 1.5f;

    // EMNLP 2024 显示 emoji 在语境中存在 literal/figurative 差异，
    // 正负 emoji 混用通常意味着语义冲突，需降低规则层的置信幅度。
    if (positiveEmojiHits > 0 && negativeEmojiHits > 0) {
        const float imbalance = static_cast<float>(std::abs(positiveEmojiHits - negativeEmojiHits)) /
            static_cast<float>(positiveEmojiHits + negativeEmojiHits);
        const float conflictDamping = 0.58f + 0.22f * imbalance;
        normalized *= conflictDamping;
    }

    return std::clamp(normalized, -1.0f, 1.0f);
}

float SentimentAnalyzer::lexiconSentiment(const std::vector<std::string>& tokens) const {
    float totalScore = 0.0f;
    int matchedCount = 0;
    float intensifierMult = 1.0f;
    int negationWindow = 0;

    size_t i = 0;
    while (i < tokens.size()) {
        // 领域词典支持跨 token 短语匹配（用于分词文本）；基础词典仍保持单 token 逻辑。
        int matchedLen = 0;
        float matchedLexScore = 0.0f;
        if (!domainSentimentLexicon_.empty()) {
            const int maxLen = static_cast<int>(std::min<size_t>(6, tokens.size() - i));
            for (int len = maxLen; len >= 1; --len) {
                std::string candidate;
                for (int j = 0; j < len; ++j) {
                    candidate += tokens[i + static_cast<size_t>(j)];
                }
                auto lexIt = domainSentimentLexicon_.find(candidate);
                if (lexIt != domainSentimentLexicon_.end()) {
                    matchedLen = len;
                    matchedLexScore = lexIt->second;
                    break;
                }
            }
        }

        if (matchedLen > 0) {
            float wordScore = matchedLexScore * intensifierMult;
            if (negationWindow > 0) {
                wordScore *= -0.75f;  // 否定翻转，但强度略减
            }
            totalScore += wordScore;
            ++matchedCount;

            intensifierMult = 1.0f;
            negationWindow = std::max(0, negationWindow - matchedLen);
            i += static_cast<size_t>(matchedLen);
            continue;
        }

        const auto& token = tokens[i];
        auto negIt = negators_.find(token);
        if (negIt != negators_.end()) {
            negationWindow = 2;
            ++i;
            continue;
        }

        auto intIt = intensifiers_.find(token);
        if (intIt != intensifiers_.end()) {
            intensifierMult = intIt->second;
            ++i;
            continue;
        }

        auto lexIt = sentimentLexicon_.find(token);
        if (lexIt != sentimentLexicon_.end()) {
            float wordScore = lexIt->second * intensifierMult;
            if (negationWindow > 0) {
                wordScore *= -0.75f;  // 否定翻转，但强度略减
            }
            totalScore += wordScore;
            ++matchedCount;
            intensifierMult = 1.0f;
            negationWindow = 0;
        } else if (negationWindow > 0) {
            --negationWindow;
        }
        ++i;
    }

    if (matchedCount == 0) return 0.0f;
    return std::clamp(totalScore / static_cast<float>(matchedCount), -1.0f, 1.0f);
}

float SentimentAnalyzer::statisticalSentiment(const std::vector<std::string>& tokens,
                                              const std::string& text) const {
    float score = 0.0f;
    int features = 0;

    // 特征1: 词汇丰富度（type-token ratio）
    std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
    float ttr = tokens.empty() ? 0.0f :
                static_cast<float>(uniqueTokens.size()) / static_cast<float>(tokens.size());
    if (ttr > 0.8f) { score += 0.1f; ++features; }

    // 特征2: 平均词长
    float avgLen = 0.0f;
    for (const auto& t : tokens) avgLen += static_cast<float>(t.size());
    if (!tokens.empty()) avgLen /= static_cast<float>(tokens.size());
    if (avgLen > 6.0f) { score += 0.05f; ++features; }

    // 特征3: 文本长度特征
    float textLen = static_cast<float>(text.size());
    if (textLen < 10.0f) {
        ++features;
    } else if (textLen > 200.0f) {
        score += 0.05f;
        ++features;
    }

    // 特征4: 正面/负面词比例
    int posCount = 0, negCount = 0;
    for (const auto& t : tokens) {
        auto it = sentimentLexicon_.find(t);
        if (it != sentimentLexicon_.end()) {
            if (it->second > 0) ++posCount;
            else if (it->second < 0) ++negCount;
        }
    }
    int sentimentWords = posCount + negCount;
    if (sentimentWords > 0) {
        float posRatio = static_cast<float>(posCount) / static_cast<float>(sentimentWords);
        score += (posRatio - 0.5f) * 0.6f;
        ++features;
    }

    // 特征5: 重复字符检测
    int repeatCount = 0;
    for (size_t i = 1; i < text.size(); ++i) {
        if (text[i] == text[i - 1]) ++repeatCount;
    }
    float repeatRatio = text.empty() ? 0.0f :
                        static_cast<float>(repeatCount) / static_cast<float>(text.size());
    if (repeatRatio > 0.3f) {
        score *= 1.3f;
        ++features;
    }

    if (features == 0) return 0.0f;
    return std::clamp(score, -1.0f, 1.0f);
}

std::string SentimentAnalyzer::scoresToMood(float score) const {
    if (score > 0.58f) return "happy";
    if (score > 0.25f) return "calm";
    if (score > -0.2f) return "neutral";
    if (score > -0.62f) return "anxious";
    return "sad";
}

std::string SentimentAnalyzer::normalizeSentimentText(const std::string& text) const {
    std::string normalized;
    normalized.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c <= 0x20) {
            if (!normalized.empty() && normalized.back() != ' ') {
                normalized += ' ';
            }
            continue;
        }
        if (c >= 'A' && c <= 'Z') {
            normalized += static_cast<char>(c + 32);
        } else {
            normalized += text[i];
        }
    }

    // 去除尾部空格
    while (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }

    if (envFlagEnabled("HEARTLAKE_SENTIMENT_COMPACT_CJK_SPACES", false)) {
        // 对中文主导文本去掉分词空格（如“性 價 比 高”），避免词典与短语规则失配。
        // 仅在非 ASCII 字节显著高于 ASCII 字母数字时触发，尽量不影响英文文本。
        size_t nonAsciiBytes = 0;
        size_t asciiAlphaNum = 0;
        for (unsigned char c : normalized) {
            if (c >= 0x80) {
                ++nonAsciiBytes;
            } else if (std::isalnum(c)) {
                ++asciiAlphaNum;
            }
        }
        if (nonAsciiBytes > asciiAlphaNum * 2 && normalized.find(' ') != std::string::npos) {
            std::string compacted;
            compacted.reserve(normalized.size());
            for (char ch : normalized) {
                if (ch != ' ') {
                    compacted.push_back(ch);
                }
            }
            normalized.swap(compacted);
        }
    }

    if (envFlagEnabled("HEARTLAKE_SENTIMENT_T2S", false)) {
        return convertTraditionalToSimplified(normalized);
    }
    return normalized;
}

bool SentimentAnalyzer::getSentimentCacheHit(const std::string& key,
                                              EdgeSentimentResult& result) {
    std::shared_lock<std::shared_mutex> lock(sentimentCacheMutex_);
    auto it = sentimentCacheMap_.find(key);
    if (it == sentimentCacheMap_.end()) {
        sentimentCacheMisses_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    if (it->second->expiresAt <= std::chrono::steady_clock::now()) {
        sentimentCacheMisses_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    result = it->second->result;
    sentimentCacheHits_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

void SentimentAnalyzer::compactSentimentCacheLocked(std::unique_lock<std::shared_mutex>& /*lock*/) {
    while (sentimentCacheMap_.size() > sentimentCacheMaxSize_) {
        auto& oldest = sentimentCacheLRU_.back();
        sentimentCacheMap_.erase(oldest.key);
        sentimentCacheLRU_.pop_back();
    }
}

void SentimentAnalyzer::putSentimentCache(const std::string& key,
                                           const EdgeSentimentResult& result) {
    std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);
    auto it = sentimentCacheMap_.find(key);
    if (it != sentimentCacheMap_.end()) {
        it->second->result = result;
        it->second->expiresAt = std::chrono::steady_clock::now() + std::chrono::seconds(sentimentCacheTTLSeconds_);
        sentimentCacheLRU_.splice(sentimentCacheLRU_.begin(), sentimentCacheLRU_, it->second);
    } else {
        sentimentCacheLRU_.push_front({key, result,
            std::chrono::steady_clock::now() + std::chrono::seconds(sentimentCacheTTLSeconds_)});
        sentimentCacheMap_[key] = sentimentCacheLRU_.begin();
        compactSentimentCacheLocked(lock);
    }
}

EdgeSentimentResult SentimentAnalyzer::analyzeSentiment(const std::string& text, bool preferOnnx) {
    ++totalSentimentCalls_;

    if (text.empty()) {
        return {0.0f, "neutral", 0.0f, "disabled"};
    }

    const std::string normalizedCacheKey = normalizeSentimentText(text);
    if (normalizedCacheKey.empty()) {
        return analyzeSentimentUncached(text, preferOnnx);
    }
    const std::string cacheKey = preferOnnx
        ? (normalizedCacheKey + "#onnx")
        : normalizedCacheKey;

    EdgeSentimentResult cached;
    if (getSentimentCacheHit(cacheKey, cached)) {
        return cached;
    }

    std::shared_future<EdgeSentimentResult> sharedFuture;
    std::optional<std::promise<EdgeSentimentResult>> ownerPromise;
    {
        std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);
        const auto now = std::chrono::steady_clock::now();
        auto cacheIt = sentimentCacheMap_.find(cacheKey);
        if (cacheIt != sentimentCacheMap_.end()) {
            if (cacheIt->second->expiresAt > now) {
                // LRU提升: splice到头部, O(1)
                sentimentCacheLRU_.splice(sentimentCacheLRU_.begin(), sentimentCacheLRU_, cacheIt->second);
                sentimentCacheHits_.fetch_add(1, std::memory_order_relaxed);
                return cacheIt->second->result;
            }
            sentimentCacheLRU_.erase(cacheIt->second);
            sentimentCacheMap_.erase(cacheIt);
        }

        auto inflightIt = sentimentInFlight_.find(cacheKey);
        if (inflightIt != sentimentInFlight_.end()) {
            sharedFuture = inflightIt->second;
        } else {
            ownerPromise.emplace();
            sharedFuture = ownerPromise->get_future().share();
            sentimentInFlight_[cacheKey] = sharedFuture;
        }
    }

    if (!ownerPromise.has_value()) {
        try {
            return sharedFuture.get();
        } catch (...) {
            return analyzeSentimentUncached(text, preferOnnx);
        }
    }

    try {
        auto computed = analyzeSentimentUncached(text, preferOnnx);
        putSentimentCache(cacheKey, computed);
        {
            std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);
            sentimentInFlight_.erase(cacheKey);
        }
        ownerPromise->set_value(computed);
        return computed;
    } catch (...) {
        {
            std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);
            sentimentInFlight_.erase(cacheKey);
        }
        ownerPromise->set_exception(std::current_exception());
        throw;
    }
}

EdgeSentimentResult SentimentAnalyzer::analyzeSentimentUncached(const std::string& text, bool preferOnnx) {
    if (text.empty()) {
        return {0.0f, "neutral", 0.0f, "disabled"};
    }

    const std::string normalizedText = normalizeSentimentText(text);
    if (normalizedText.empty()) {
        return {0.0f, "neutral", 0.0f, "disabled"};
    }

    auto tokens = tokenizeUTF8(normalizedText);

    float ruleScore = ruleSentiment(normalizedText);
    float lexScore = lexiconSentiment(tokens);
    float statScore = statisticalSentiment(tokens, normalizedText);

    float wRule = 0.30f;
    float wLex = 0.60f;
    const float wStat = 0.10f;

    // 规则层与词典层极性冲突时，优先信任词典层（规则层含 emoji/标点信号，噪声更高）。
    // 参考: EMNLP 2024 对 emoji figurative usage 的发现。
    if (ruleScore * lexScore < -0.04f) {
        const float conflict = std::min(0.18f, std::abs(ruleScore - lexScore) * 0.12f);
        wRule = std::max(0.10f, wRule - conflict);
        wLex = std::min(0.78f, wLex + conflict);
    }

    const float norm = std::max(1e-6f, wRule + wLex + wStat);
    float ensembleScore = (wRule * ruleScore + wLex * lexScore + wStat * statScore) / norm;
    ensembleScore = std::clamp(ensembleScore, -1.0f, 1.0f);

    // ── 共享 marker 列表（消除3个lambda中的重复定义）──
    static const std::vector<std::string> sharedContrastMarkers = {
        "但是", "但", "不过", "然而", "可是", "只是"
    };
    static const std::vector<std::string> sharedNegativeMarkers = {
        "焦虑", "担心", "不安", "难过", "伤心", "害怕", "恐惧", "压力", "烦躁",
        "痛苦", "绝望", "崩溃", "失眠", "失败", "糟糕", "后悔"
    };
    static const std::vector<std::string> praiseMarkers = {
        "夸", "夸奖", "表扬", "认可", "肯定", "称赞", "赞扬", "赞许", "嘉奖", "鼓励", "被夸", "被表扬"
    };
    static const std::vector<std::string> selfMarkers = {"我", "自己", "本人"};
    static const std::vector<std::string> positiveEventMarkers = {
        "收到", "收到了", "礼物", "礼物很好看", "表扬", "夸奖", "夸了我", "夸我", "老师夸", "老师表扬",
        "认可", "肯定", "赞扬", "通过", "成功", "晋级", "被录取", "拿到", "获奖", "中奖", "惊喜", "感谢", "感恩"
    };

    // ── 一次扫描预计算所有 marker flags ──
    const bool hasContrast = containsAnyPhrase(normalizedText, sharedContrastMarkers);
    const bool hasNegative = containsAnyPhrase(normalizedText, sharedNegativeMarkers);
    const bool hasPraise = containsAnyPhrase(normalizedText, praiseMarkers);
    const bool hasSelf = containsAnyPhrase(normalizedText, selfMarkers);
    const bool hasPositiveEvent = containsAnyPhrase(normalizedText, positiveEventMarkers);

    // 找最后一个转折词位置（contrastBoost 和 positiveEventBoost 共用）
    size_t lastContrastPos = std::string::npos;
    size_t lastContrastLen = 0;
    if (hasContrast) {
        for (const auto& marker : sharedContrastMarkers) {
            size_t pos = normalizedText.rfind(marker);
            if (pos != std::string::npos && (lastContrastPos == std::string::npos || pos > lastContrastPos)) {
                lastContrastPos = pos;
                lastContrastLen = marker.size();
            }
        }
    }

    // ── applyContrastBoost ──
    auto applyContrastBoost = [&](float baseScore) {
        if (lastContrastPos == std::string::npos || lastContrastPos + lastContrastLen >= normalizedText.size()) {
            return baseScore;
        }
        std::string tailText = normalizedText.substr(lastContrastPos + lastContrastLen);
        if (tailText.empty()) return baseScore;

        auto tailTokens = tokenizeUTF8(tailText);
        const float tailRule = ruleSentiment(tailText);
        const float tailLex = lexiconSentiment(tailTokens);
        const float tailStat = statisticalSentiment(tailTokens, tailText);
        float tailScore = wRule * tailRule + wLex * tailLex + wStat * tailStat;
        tailScore = std::clamp(tailScore, -1.0f, 1.0f);

        if (std::abs(tailLex) < 0.12f && std::abs(tailScore) < 0.18f) return baseScore;
        if (baseScore * tailScore < -0.06f) {
            return std::clamp(baseScore * 0.25f + tailScore * 0.75f, -1.0f, 1.0f);
        }
        return std::clamp(baseScore * 0.45f + tailScore * 0.55f, -1.0f, 1.0f);
    };
    ensembleScore = applyContrastBoost(ensembleScore);

    // ── applyPraiseBoost（使用预计算flags，无需重复扫描）──
    auto applyPraiseBoost = [&](float baseScore) {
        if (!hasPraise || !hasSelf || hasContrast || hasNegative) return baseScore;
        if (ruleScore < -0.2f || lexScore < -0.12f) return baseScore;
        return std::max(baseScore, 0.64f);
    };
    ensembleScore = applyPraiseBoost(ensembleScore);

    // ── applyPositiveEventBoost（使用预计算flags）──
    auto applyPositiveEventBoost = [&](float baseScore) {
        if (!hasPositiveEvent || hasNegative) return baseScore;

        // 检查转折词后是否有负面内容
        if (lastContrastPos != std::string::npos && lastContrastPos + lastContrastLen < normalizedText.size()) {
            const auto tail = normalizedText.substr(lastContrastPos + lastContrastLen);
            if (containsAnyPhrase(tail, sharedNegativeMarkers)) return baseScore;
        }

        const float floorScore = normalizedText.find("礼物") != std::string::npos ? 0.74f : 0.68f;
        return std::max(baseScore, floorScore);
    };
    ensembleScore = applyPositiveEventBoost(ensembleScore);

    const auto anchorSignal = computeAffectiveAnchor(normalizedText);
    if (anchorSignal.strength > 0.08f) {
        const float anchorWeight = std::clamp(0.16f + anchorSignal.strength * 0.24f, 0.16f, 0.40f);
        ensembleScore = std::clamp(
            ensembleScore * (1.0f - anchorWeight) + anchorSignal.score * anchorWeight,
            -1.0f,
            1.0f
        );
    }
    float agreement = 1.0f;
    bool allPositive = (ruleScore >= 0 && lexScore >= 0 && statScore >= 0);
    bool allNegative = (ruleScore <= 0 && lexScore <= 0 && statScore <= 0);
    if (allPositive || allNegative) {
        agreement = 0.8f + 0.2f * std::min({std::abs(ruleScore), std::abs(lexScore), std::abs(statScore)});
    } else {
        float variance = ((ruleScore - ensembleScore) * (ruleScore - ensembleScore) +
                          (lexScore - ensembleScore) * (lexScore - ensembleScore) +
                          (statScore - ensembleScore) * (statScore - ensembleScore)) / 3.0f;
        agreement = std::max(0.2f, 0.7f - variance);
    }

    // 词典匹配数也影响置信度
    int lexMatches = 0;
    for (const auto& t : tokens) {
        if (sentimentLexicon_.count(t)) ++lexMatches;
    }
    float coverageBoost = std::min(0.2f, static_cast<float>(lexMatches) * 0.05f);
    float confidence = std::clamp(agreement + coverageBoost, 0.0f, 1.0f);
    if (anchorSignal.strength > 0.10f) {
        confidence = std::clamp(confidence * 0.82f + anchorSignal.strength * 0.18f, 0.0f, 1.0f);
    }
    if (anchorSignal.conflict > 0.42f && std::abs(ensembleScore) < 0.55f) {
        const float penalty = std::clamp((anchorSignal.conflict - 0.42f) * 0.45f, 0.0f, 0.22f);
        confidence = std::max(0.10f, confidence - penalty);
    }

    static const std::vector<std::string> positiveEventHints = {
        "收到", "收到了", "礼物", "礼物很好看", "表扬", "夸奖", "夸了我", "夸我", "老师夸", "老师表扬", "认可", "肯定", "赞扬",
        "通过", "成功", "晋级", "被录取", "拿到", "获奖", "中奖", "惊喜"
    };
    static const std::vector<std::string> negativeEventHints = {
        "焦虑", "担心", "不安", "难过", "伤心", "害怕", "恐惧", "压力", "烦躁", "痛苦", "绝望", "崩溃",
        "孤独", "孤单", "寂寞", "脆弱", "落寞", "无助"
    };
    const bool positiveEventSignal =
        (containsAnyPhrase(normalizedText, positiveEventHints) && !containsAnyPhrase(normalizedText, negativeEventHints))
        || (anchorSignal.score > 0.35f && anchorSignal.strength > 0.18f);

    // 内部标签 → Ekman 六基本情绪标准标签映射
    auto toEkmanMood = [](const std::string& internalMood) -> std::string {
        if (internalMood == "happy") return "joy";
        if (internalMood == "sad") return "sadness";
        if (internalMood == "angry") return "anger";
        if (internalMood == "anxious") return "fear";
        if (internalMood == "surprised") return "surprise";
        if (internalMood == "calm") return "neutral";
        if (internalMood == "confused") return "neutral";
        return internalMood;  // neutral 保持不变
    };

    auto finalizeSentiment = [&](float rawScore, float rawConfidence, const char* methodName) {
        const std::string internalMood = refineMoodFromCues(normalizedText, rawScore, scoresToMood(rawScore));
        float calibratedScore = calibrateScoreByMoodCue(normalizedText, internalMood, rawScore);
        // 生产性能模式下本地分支占比高，固定偏置用于抑制系统性正向偏移（降低 FP）。
        if (std::strncmp(methodName, "dual_route_local", 16) == 0) {
            calibratedScore = std::clamp(calibratedScore - 0.124f, -1.0f, 1.0f);
        }
        const std::string ekmanMood = toEkmanMood(internalMood);
        return EdgeSentimentResult{calibratedScore, ekmanMood, rawConfidence, methodName};
    };

#ifdef HEARTLAKE_USE_ONNX
    if (onnxEnabled_ && onnxEngine_ && onnxEngine_->isInitialized()) {
        // 默认优先走 ONNX 融合路径，只有在本地分支极高置信时才短路，避免“有大模型但命中少”。
        const bool preferOnnxPath = preferOnnx || envFlagEnabled("EDGE_AI_PREFER_ONNX", true);
        constexpr float kFastPathConfidence = 0.82f;
        constexpr float kFastPathMargin = 0.44f;
        constexpr float kFastPathConflict = 0.36f;
        constexpr int kFastPathLexMatches = 1;
        constexpr float kAmbiguousConfidence = 0.66f;
        constexpr float kAmbiguousMargin = 0.24f;
        constexpr float kAmbiguousConflict = 0.60f;

        const float ensembleMargin = std::abs(ensembleScore);
        const bool localFastPath = (confidence >= kFastPathConfidence)
            && (ensembleMargin >= kFastPathMargin)
            && (lexMatches >= kFastPathLexMatches)
            && !hasContrast
            && (anchorSignal.conflict < kFastPathConflict);
        const bool localStableFallback = (ensembleMargin >= 0.46f)
            && (lexMatches >= 1)
            && !hasContrast
            && (anchorSignal.conflict < 0.55f);
        const bool ambiguousCase = hasContrast
            || (anchorSignal.conflict >= kAmbiguousConflict)
            || (ensembleMargin < kAmbiguousMargin)
            || ((confidence < kAmbiguousConfidence) && (ensembleMargin < 0.42f))
            || ((lexMatches == 0) && (ensembleMargin < 0.50f));

        const bool localShortCircuitCandidate =
            (localFastPath || localStableFallback || !ambiguousCase);
        const bool ultraHighLocalConfidence =
            confidence >= 0.94f &&
            ensembleMargin >= 0.72f &&
            lexMatches >= 2 &&
            !hasContrast &&
            anchorSignal.conflict < 0.24f;

        if (!preferOnnxPath && localShortCircuitCandidate) {
            return finalizeSentiment(ensembleScore, confidence, "dual_route_local");
        }
        if (preferOnnxPath && localShortCircuitCandidate && ultraHighLocalConfidence) {
            return finalizeSentiment(ensembleScore, confidence, "dual_route_local_high_conf");
        }

        const size_t sessionPool = std::max<size_t>(1, onnxEngine_->getSessionPoolSize());
        // 会话池本身不是硬并发上限；将并发闸门设为更宽，避免在入口形成秒级排队。
        const int onnxMaxInflight = static_cast<int>(std::clamp(sessionPool * 8, static_cast<size_t>(8), static_cast<size_t>(96)));
        const bool hasOnnxSlot = tryAcquireOnnxSlot(gOnnxInFlight, onnxMaxInflight);
        if (!hasOnnxSlot) {
            return finalizeSentiment(ensembleScore, confidence, "dual_route_local_backpressure");
        }
        OnnxInFlightGuard onnxInFlightGuard(gOnnxInFlight, hasOnnxSlot);
        auto onnxResult = onnxEngine_->analyze(normalizedText);
        const float scoreGap = std::abs(onnxResult.score - ensembleScore);
        const bool signConflict = (onnxResult.score * ensembleScore < -0.06f);
        const bool lexSignalStrong = (lexMatches > 0 && std::abs(lexScore) >= 0.18f);
        const bool guardedEligible = signConflict && scoreGap >= 0.38f
            && (onnxResult.confidence >= 0.58f || confidence >= 0.58f);

        if (guardedEligible) {
            float guardedOnnxWeight = 0.74f;
            if (onnxResult.confidence >= confidence + 0.10f) {
                guardedOnnxWeight += 0.12f;
            } else if (confidence >= onnxResult.confidence + 0.12f && lexSignalStrong) {
                guardedOnnxWeight -= 0.18f;
            }
            if (lexMatches >= 3 && std::abs(lexScore) >= 0.45f) {
                guardedOnnxWeight -= 0.10f;
            }
            if (positiveEventSignal && ensembleScore > 0.45f && onnxResult.score < -0.10f) {
                guardedOnnxWeight = std::min(guardedOnnxWeight, 0.55f);
            }
            guardedOnnxWeight = std::clamp(guardedOnnxWeight, 0.34f, 0.90f);

            float fusedScore = std::clamp(
                onnxResult.score * guardedOnnxWeight + ensembleScore * (1.0f - guardedOnnxWeight),
                -1.0f,
                1.0f
            );

            if (std::abs(fusedScore) < 0.10f) {
                const bool trustOnnxSign = onnxResult.confidence >= std::max(0.70f, confidence + 0.04f);
                const float anchorScore = trustOnnxSign ? onnxResult.score : ensembleScore;
                const float floorMag = std::clamp(std::abs(anchorScore) * 0.38f, 0.10f, 0.24f);
                fusedScore = std::copysign(std::max(std::abs(fusedScore), floorMag), anchorScore);
            }

            const float fusedConf = std::clamp(
                (onnxResult.confidence * 0.64f + confidence * 0.36f) - scoreGap * 0.05f,
                0.24f,
                0.99f
            );
            return finalizeSentiment(fusedScore, fusedConf, "dual_route_onnx_guarded");
        }

        float onnxWeight = std::clamp(
            0.54f + (onnxResult.confidence - confidence) * 0.36f,
            0.36f,
            0.86f
        );
        if (lexSignalStrong && lexMatches >= 2 && std::abs(lexScore) >= 0.36f) {
            onnxWeight = std::max(0.30f, onnxWeight - 0.10f);
        }
        if (lexMatches == 0) {
            onnxWeight = std::max(0.66f, onnxWeight);
        }
        if (anchorSignal.strength > 0.20f && std::abs(anchorSignal.score) > 0.32f) {
            const bool anchorConflictWithOnnx = (onnxResult.score * anchorSignal.score < -0.08f);
            if (anchorConflictWithOnnx) {
                onnxWeight = std::max(0.22f, onnxWeight - 0.15f);
            } else {
                onnxWeight = std::min(0.90f, onnxWeight + 0.05f);
            }
        }
        if (positiveEventSignal && ensembleScore > 0.45f && onnxResult.score < -0.10f) {
            onnxWeight = std::min(onnxWeight, 0.28f);
        }
        if (scoreGap < 0.18f) {
            onnxWeight = std::min(0.90f, onnxWeight + 0.06f);
        }

        float fusedScore = std::clamp(
            onnxResult.score * onnxWeight + ensembleScore * (1.0f - onnxWeight),
            -1.0f,
            1.0f
        );
        if (std::abs(fusedScore) < 0.10f) {
            const float anchorScore = (onnxResult.confidence >= confidence) ? onnxResult.score : ensembleScore;
            const float floorMag = std::clamp(std::abs(anchorScore) * 0.34f, 0.10f, 0.24f);
            fusedScore = std::copysign(std::max(std::abs(fusedScore), floorMag), anchorScore);
        }
        const float fusedConf = std::clamp(
            (onnxResult.confidence * 0.62f + confidence * 0.38f) - scoreGap * 0.08f,
            0.22f,
            0.99f
        );
        return finalizeSentiment(fusedScore, fusedConf, "dual_route_onnx_ensemble");
    }
#endif

    return finalizeSentiment(ensembleScore, confidence, "ensemble");
}

void SentimentAnalyzer::clearCache() {
    std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);
    sentimentCacheLRU_.clear();
    sentimentCacheMap_.clear();
    sentimentInFlight_.clear();
    sentimentCacheHits_.store(0);
    sentimentCacheMisses_.store(0);
}

#ifdef HEARTLAKE_USE_ONNX
void SentimentAnalyzer::setOnnxEngine(std::unique_ptr<OnnxSentimentEngine> engine) {
    onnxEngine_ = std::move(engine);
    onnxEnabled_ = (onnxEngine_ != nullptr);
}
#endif

} // namespace ai
} // namespace heartlake
