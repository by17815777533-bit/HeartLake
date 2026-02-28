/**
 * @brief 安全港湾危机干预服务
 *
 * @details
 * 为处于心理危机状态的用户提供即时支持资源。核心功能：
 *
 * 1. 热线管理：维护全国心理援助热线列表（含 24 小时标记）
 * 2. 自助工具：提供呼吸练习、正念冥想等自助干预方案
 * 3. 情绪感知推荐：根据用户当前情绪状态智能推荐最合适的资源
 * 4. 分级暖心提示：按风险等级（low/medium/high/critical）返回不同强度的关怀文案
 * 5. 访问记录：记录用户对资源的访问历史，用于后续回访和效果评估
 *
 * 资源支持动态 CRUD，管理后台可实时增删改查。
 * 所有操作在 mutex 保护下线程安全。
 */

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <json/json.h>

namespace heartlake::infrastructure {

/// 危机干预热线资源
struct CrisisResource {
    std::string name;         ///< 热线名称
    std::string phone;        ///< 联系电话
    std::string description;  ///< 服务描述
    bool is24Hours;           ///< 是否 24 小时服务
};

class SafeHarborService {
public:
    static SafeHarborService& getInstance() {
        static SafeHarborService instance;
        return instance;
    }

    /// 获取全部心理援助热线列表
    Json::Value getHotlines() const;
    /// 获取自助干预工具列表
    Json::Value getSelfHelpTools() const;

    /**
     * @brief 根据风险等级获取暖心提示
     * @param riskLevel 风险等级："low" / "medium" / "high" / "critical"
     * @return 包含标题、内容和建议操作的 JSON
     */
    Json::Value getWarmPrompt(const std::string& riskLevel) const;

    // ---- 资源 CRUD ----

    /// 添加新资源，返回包含分配 ID 的 JSON
    Json::Value addResource(const Json::Value& data);
    /// 更新指定 ID 的资源
    bool updateResource(const std::string& id, const Json::Value& data);
    /// 删除指定 ID 的资源
    bool deleteResource(const std::string& id);
    /// 按类型筛选资源列表
    Json::Value getResources(const std::string& type) const;

    // ---- 访问记录与推荐 ----

    /// 记录用户访问了某个资源（用于后续回访评估）
    void recordUserAccess(const std::string& userId, const std::string& resourceId);
    /// 获取用户的资源访问历史
    Json::Value getUserAccessHistory(const std::string& userId) const;

    /**
     * @brief 根据用户情绪智能推荐资源
     * @param userId 用户 ID
     * @param emotion 当前情绪类型
     * @return 推荐的资源列表
     */
    Json::Value recommendByEmotion(const std::string& userId, const std::string& emotion) const;

private:
    SafeHarborService();

    std::vector<CrisisResource> hotlines_;      ///< 热线列表（构造时初始化）
    std::vector<Json::Value> resources_;         ///< 动态资源池
    std::vector<Json::Value> accessHistory_;     ///< 用户访问记录
    int resourceIdCounter_ = 0;                  ///< 资源 ID 自增计数器
    mutable std::mutex mutex_;                   ///< 保护 resources_ / accessHistory_ 的并发访问
};

} // namespace heartlake::infrastructure
