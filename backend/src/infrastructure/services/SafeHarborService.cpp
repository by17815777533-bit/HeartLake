/**
 * @file SafeHarborService.cpp
 * @brief 安全港湾危机干预服务实现
 */

#include "infrastructure/services/SafeHarborService.h"
#include <ctime>

namespace heartlake::infrastructure {

SafeHarborService::SafeHarborService() {
    hotlines_ = {
        {"全国心理援助热线", "400-161-9995", "24小时免费心理咨询", true},
        {"北京心理危机研究与干预中心", "010-82951332", "专业心理危机干预", true},
        {"生命热线", "400-821-1215", "倾听你的声音", true}
    };
}

Json::Value SafeHarborService::getHotlines() const {
    Json::Value result(Json::arrayValue);
    for (const auto& r : hotlines_) {
        Json::Value item;
        item["name"] = r.name;
        item["phone"] = r.phone;
        item["description"] = r.description;
        item["is_24_hours"] = r.is24Hours;
        result.append(item);
    }
    return result;
}

Json::Value SafeHarborService::getSelfHelpTools() const {
    Json::Value tools(Json::arrayValue);
    Json::Value t1; t1["id"] = "breathing"; t1["name"] = "深呼吸练习"; t1["description"] = "跟随引导，平静内心";
    Json::Value t2; t2["id"] = "grounding"; t2["name"] = "五感着陆"; t2["description"] = "感受当下，回归平静";
    Json::Value t3; t3["id"] = "journal"; t3["name"] = "情绪日记"; t3["description"] = "写下感受，释放压力";
    tools.append(t1); tools.append(t2); tools.append(t3);
    return tools;
}

Json::Value SafeHarborService::getWarmPrompt(const std::string& riskLevel) const {
    Json::Value prompt;
    if (riskLevel == "HIGH" || riskLevel == "CRITICAL") {
        prompt["title"] = "需要有人陪伴吗？";
        prompt["message"] = "我们注意到你可能正在经历一些困难。这里有一些资源可以帮助你。";
        prompt["show_hotline"] = true;
    } else {
        prompt["title"] = "心湖在这里陪伴你";
        prompt["message"] = "如果你需要倾诉，可以随时寻求专业帮助。";
        prompt["show_hotline"] = false;
    }
    prompt["cta_text"] = "寻求专业帮助";
    return prompt;
}

Json::Value SafeHarborService::addResource(const Json::Value& data) {
    Json::Value res;
    res["id"] = "res_" + std::to_string(++resourceIdCounter_);
    res["name"] = data.get("name", "").asString();
    res["type"] = data.get("type", "").asString();
    resources_.push_back(res);
    return res;
}

bool SafeHarborService::updateResource(const std::string& id, const Json::Value& data) {
    for (auto& r : resources_) {
        if (r["id"].asString() == id) {
            if (data.isMember("name")) r["name"] = data["name"];
            if (data.isMember("type")) r["type"] = data["type"];
            return true;
        }
    }
    return false;
}

bool SafeHarborService::deleteResource(const std::string& id) {
    for (auto it = resources_.begin(); it != resources_.end(); ++it) {
        if ((*it)["id"].asString() == id) { resources_.erase(it); return true; }
    }
    return false;
}

Json::Value SafeHarborService::getResources(const std::string& type) const {
    Json::Value result(Json::arrayValue);
    for (const auto& r : resources_) {
        if (type.empty() || r["type"].asString() == type) result.append(r);
    }
    return result;
}

void SafeHarborService::recordUserAccess(const std::string& userId, const std::string& resourceId) {
    Json::Value record;
    record["user_id"] = userId;
    record["resource_id"] = resourceId;
    record["timestamp"] = static_cast<Json::Int64>(time(nullptr));
    accessHistory_.push_back(record);
}

Json::Value SafeHarborService::getUserAccessHistory(const std::string& userId) const {
    Json::Value result(Json::arrayValue);
    for (const auto& r : accessHistory_) {
        if (r["user_id"].asString() == userId) result.append(r);
    }
    return result;
}

Json::Value SafeHarborService::recommendByEmotion(const std::string& /*userId*/, const std::string& /*emotion*/) const {
    Json::Value result(Json::arrayValue);
    result.append(getSelfHelpTools()[0]);
    return result;
}

} // namespace heartlake::infrastructure
