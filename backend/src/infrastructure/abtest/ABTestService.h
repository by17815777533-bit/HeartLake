/**
 * @file ABTestService.h
 * @brief A/B测试框架 - 支持功能灰度发布和用户分组实验
 * Created by Claude
 */
#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <chrono>
#include <drogon/drogon.h>

namespace heartlake::abtest {

struct Experiment {
    std::string id;
    std::string name;
    std::string description;
    std::vector<std::string> variants;      // 实验变体列表
    std::vector<int> weights;               // 各变体权重(百分比)
    double trafficPercent = 100.0;          // 流量百分比(灰度)
    bool enabled = true;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
};

struct ExperimentResult {
    std::string visitorId;
    std::string experimentId;
    std::string variant;
    std::string eventType;
    Json::Value metadata;
    std::chrono::system_clock::time_point timestamp;
};

class ABTestService {
public:
    static ABTestService& getInstance();

    // 实验管理
    void registerExperiment(const Experiment& exp);
    void enableExperiment(const std::string& expId, bool enabled);
    std::vector<Experiment> listExperiments() const;

    // 用户分组 - 基于用户ID的确定性分组
    std::string getVariant(const std::string& userId, const std::string& expId);
    bool isInExperiment(const std::string& userId, const std::string& expId);

    // 灰度发布检查
    bool isFeatureEnabled(const std::string& userId, const std::string& featureId);

    // 数据记录
    void trackEvent(const std::string& userId, const std::string& expId,
                    const std::string& eventType, const Json::Value& metadata = Json::Value());

    // 效果分析
    Json::Value getExperimentStats(const std::string& expId) const;
    Json::Value compareVariants(const std::string& expId) const;

private:
    ABTestService() = default;
    uint32_t hashUserId(const std::string& userId, const std::string& salt) const;

    mutable std::mutex mutex_;
    std::map<std::string, Experiment> experiments_;
    std::vector<ExperimentResult> results_;
};

} // namespace heartlake::abtest
