/**
 * @file IntimacyService.h
 * @brief 亲密分服务：基于石头互动自动判断关系，无需手动通过/拒绝
 */

#pragma once

#include <string>
#include <vector>

namespace heartlake::infrastructure {

struct IntimacyProfile {
    std::string userId;
    std::string username;
    std::string nickname;
    double intimacyScore{0.0};
    std::string intimacyLevel{"stranger"};
    int interactionCount{0};
    int boatComments{0};
    int rippleCount{0};
    double interactionStrength{0.0};
    double reciprocityScore{0.0};
    double coRippleScore{0.0};
    double moodResonance{0.5};
    double semanticSimilarity{0.5};
    double emotionTrendAlignment{0.5};
    double freshnessScore{0.0};
    double temporalDiversity{0.0};
    double antiGamingPenalty{0.0};
    double behaviorHealth{1.0};
    double dialogueCohesion{0.0};
    double responseAgility{0.0};
    double graphAffinity{0.5};
    double emotionSynchrony{0.5};
    double aiCompatibility{0.5};
    std::string lastInteractedAt;
    bool canChat{false};
};

class IntimacyService {
public:
    static IntimacyService& getInstance();

    std::vector<IntimacyProfile> getTopIntimacyPeers(
        const std::string& userId,
        int limit = 80,
        double minScore = 20.0
    ) const;

    double getIntimacyScore(
        const std::string& userId,
        const std::string& peerId
    ) const;

    bool canChat(
        const std::string& userId,
        const std::string& peerId,
        double threshold = 20.0
    ) const;

    static std::string levelFromScore(double score);

private:
    IntimacyService() = default;
    ~IntimacyService() = default;
    IntimacyService(const IntimacyService&) = delete;
    IntimacyService& operator=(const IntimacyService&) = delete;
};

}  // namespace heartlake::infrastructure
