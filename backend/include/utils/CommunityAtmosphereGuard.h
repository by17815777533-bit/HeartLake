/**
 * @file CommunityAtmosphereGuard.h
 * @brief 社区氛围守护 - 防止负能量传染
 * Created by 白洋
 */

#pragma once

#include <string>
#include <vector>
#include <json/json.h>

namespace heartlake {
namespace utils {

struct AtmosphereScore {
    float positivity;      // 正向度 0-1
    float toxicity;        // 毒性度 0-1
    bool needsModeration;  // 是否需要审核
    std::string suggestion; // 引导建议
};

class CommunityAtmosphereGuard {
public:
    static CommunityAtmosphereGuard& getInstance();

    // 评估内容氛围
    AtmosphereScore evaluate(const std::string& content);

    // 检查是否应限制曝光（连续负面内容）
    bool shouldLimitExposure(const std::string& userId);

    // 生成正向引导语
    std::string generatePositiveGuidance(const std::string& context);

    // 获取社区整体氛围指数
    Json::Value getCommunityMood();

private:
    CommunityAtmosphereGuard();
    float detectToxicity(const std::string& text);
    float detectPositivity(const std::string& text);

    std::vector<std::string> toxicPatterns_;
    std::vector<std::string> positivePatterns_;
};

} // namespace utils
} // namespace heartlake
