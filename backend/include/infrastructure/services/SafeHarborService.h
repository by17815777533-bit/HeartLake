/**
 * @file SafeHarborService.h
 * @brief 安全港湾危机干预服务
 */

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <json/json.h>

namespace heartlake::infrastructure {

struct CrisisResource {
    std::string name;
    std::string phone;
    std::string description;
    bool is24Hours;
};

class SafeHarborService {
public:
    static SafeHarborService& getInstance() {
        static SafeHarborService instance;
        return instance;
    }

    Json::Value getHotlines() const;
    Json::Value getSelfHelpTools() const;
    Json::Value getWarmPrompt(const std::string& riskLevel) const;
    Json::Value addResource(const Json::Value& data);
    bool updateResource(const std::string& id, const Json::Value& data);
    bool deleteResource(const std::string& id);
    Json::Value getResources(const std::string& type) const;
    void recordUserAccess(const std::string& userId, const std::string& resourceId);
    Json::Value getUserAccessHistory(const std::string& userId) const;
    Json::Value recommendByEmotion(const std::string& userId, const std::string& emotion) const;

private:
    SafeHarborService();

    std::vector<CrisisResource> hotlines_;
    std::vector<Json::Value> resources_;
    std::vector<Json::Value> accessHistory_;
    int resourceIdCounter_ = 0;
    mutable std::mutex mutex_;  ///< 保护 resources_/accessHistory_ 的并发访问
};

} // namespace heartlake::infrastructure
