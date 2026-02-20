/**
 * @file ABTestService.cpp
 * @brief A/B测试框架实现
 * Created by 白洋
 */
#include "infrastructure/abtest/ABTestService.h"
#include <functional>

namespace heartlake::abtest {

ABTestService& ABTestService::getInstance() {
    static ABTestService instance;
    return instance;
}

uint32_t ABTestService::hashUserId(const std::string& userId, const std::string& salt) const {
    std::hash<std::string> hasher;
    return static_cast<uint32_t>(hasher(userId + salt));
}

void ABTestService::registerExperiment(const Experiment& exp) {
    std::lock_guard<std::mutex> lock(mutex_);
    experiments_[exp.id] = exp;
    LOG_INFO << "Registered experiment: " << exp.id << " - " << exp.name;
}

void ABTestService::enableExperiment(const std::string& expId, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (experiments_.count(expId)) {
        experiments_[expId].enabled = enabled;
    }
}

std::vector<Experiment> ABTestService::listExperiments() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Experiment> result;
    for (const auto& [id, exp] : experiments_) {
        result.push_back(exp);
    }
    return result;
}

bool ABTestService::isInExperiment(const std::string& userId, const std::string& expId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = experiments_.find(expId);
    if (it == experiments_.end() || !it->second.enabled) return false;

    uint32_t hash = hashUserId(userId, expId + "_traffic");
    double bucket = (hash % 10000) / 100.0;
    return bucket < it->second.trafficPercent;
}

std::string ABTestService::getVariant(const std::string& userId, const std::string& expId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = experiments_.find(expId);
    if (it == experiments_.end() || !it->second.enabled || it->second.variants.empty()) {
        return "";
    }

    // 检查是否在实验流量中
    uint32_t trafficHash = hashUserId(userId, expId + "_traffic");
    if ((trafficHash % 10000) / 100.0 >= it->second.trafficPercent) {
        return "";
    }

    // 基于权重分配变体
    uint32_t hash = hashUserId(userId, expId);
    int bucket = hash % 100;
    int cumulative = 0;

    for (size_t i = 0; i < it->second.variants.size(); ++i) {
        cumulative += it->second.weights[i];
        if (bucket < cumulative) {
            return it->second.variants[i];
        }
    }
    return it->second.variants.back();
}

bool ABTestService::isFeatureEnabled(const std::string& userId, const std::string& featureId) {
    return !getVariant(userId, featureId).empty();
}

void ABTestService::trackEvent(const std::string& userId, const std::string& expId,
                               const std::string& eventType, const Json::Value& metadata) {
    std::lock_guard<std::mutex> lock(mutex_);
    ExperimentResult result;
    result.visitorId = userId;
    result.experimentId = expId;
    result.variant = getVariant(userId, expId);
    result.eventType = eventType;
    result.metadata = metadata;
    result.timestamp = std::chrono::system_clock::now();
    results_.push_back(result);
}

Json::Value ABTestService::getExperimentStats(const std::string& expId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    Json::Value stats;
    std::map<std::string, std::map<std::string, int>> variantEvents;

    for (const auto& r : results_) {
        if (r.experimentId == expId) {
            variantEvents[r.variant][r.eventType]++;
        }
    }

    for (const auto& [variant, events] : variantEvents) {
        for (const auto& [event, count] : events) {
            stats[variant][event] = count;
        }
    }
    return stats;
}

Json::Value ABTestService::compareVariants(const std::string& expId) const {
    Json::Value stats = getExperimentStats(expId);
    Json::Value comparison;
    comparison["experimentId"] = expId;
    comparison["variants"] = stats;
    return comparison;
}

} // namespace heartlake::abtest
