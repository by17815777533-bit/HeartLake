/**
 * @file CommunityAtmosphereGuard.cpp
 * @brief 社区氛围守护实现
 * Created by engineer-4
 */

#include "utils/CommunityAtmosphereGuard.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <regex>

namespace heartlake {
namespace utils {

CommunityAtmosphereGuard& CommunityAtmosphereGuard::getInstance() {
    static CommunityAtmosphereGuard instance;
    return instance;
}

CommunityAtmosphereGuard::CommunityAtmosphereGuard() {
    toxicPatterns_ = {"攻击", "辱骂", "诅咒", "去死", "垃圾", "废物", "滚"};
    positivePatterns_ = {"加油", "支持", "温暖", "感谢", "希望", "美好", "开心", "幸福"};
}

AtmosphereScore CommunityAtmosphereGuard::evaluate(const std::string& content) {
    AtmosphereScore score;
    score.toxicity = detectToxicity(content);
    score.positivity = detectPositivity(content);
    score.needsModeration = score.toxicity > 0.6f;

    if (score.toxicity > 0.8f) {
        score.suggestion = "内容可能影响社区氛围，建议修改后发布";
    } else if (score.toxicity > 0.5f) {
        score.suggestion = "试试用更温和的方式表达？";
    } else {
        score.suggestion = "";
    }
    return score;
}

bool CommunityAtmosphereGuard::shouldLimitExposure(const std::string& userId) {
    try {
        auto db = drogon::app().getDbClient("default");
        auto result = db->execSqlSync(
            "SELECT COUNT(*) as cnt FROM stones WHERE user_id = $1 "
            "AND sentiment_score < -0.5 AND created_at > NOW() - INTERVAL '24 hours'",
            userId);
        return !result.empty() && result[0]["cnt"].as<int>() >= 3;
    } catch (...) {
        return false;
    }
}

std::string CommunityAtmosphereGuard::generatePositiveGuidance(const std::string& context) {
    static const std::vector<std::string> guidances = {
        "每个人都有低落的时候，但阳光总会来的",
        "你的感受很重要，也请记得照顾好自己",
        "分享让心情变轻，希望你能感受到温暖"
    };
    return guidances[std::hash<std::string>{}(context) % guidances.size()];
}

Json::Value CommunityAtmosphereGuard::getCommunityMood() {
    Json::Value mood;
    try {
        auto db = drogon::app().getDbClient("default");
        auto result = db->execSqlSync(
            "SELECT AVG(sentiment_score) as avg_mood FROM stones "
            "WHERE created_at > NOW() - INTERVAL '1 hour'");
        mood["current"] = result.empty() ? 0.0 : result[0]["avg_mood"].as<double>();
        mood["status"] = mood["current"].asDouble() > 0 ? "positive" : "needs_care";
    } catch (...) {
        mood["current"] = 0.0;
        mood["status"] = "unknown";
    }
    return mood;
}

float CommunityAtmosphereGuard::detectToxicity(const std::string& text) {
    int matches = 0;
    for (const auto& p : toxicPatterns_) {
        if (text.find(p) != std::string::npos) matches++;
    }
    return std::min(1.0f, matches * 0.3f);
}

float CommunityAtmosphereGuard::detectPositivity(const std::string& text) {
    int matches = 0;
    for (const auto& p : positivePatterns_) {
        if (text.find(p) != std::string::npos) matches++;
    }
    return std::min(1.0f, matches * 0.2f);
}

} // namespace utils
} // namespace heartlake
