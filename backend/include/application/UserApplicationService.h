/**
 * 用户应用服务
 *
 * 编排用户资料相关的应用层用例：查询个人主页、修改资料、搜索用户、批量拉取等。
 * 查询走 CacheManager 做读缓存（key 格式 "user:{userId}"），写操作后主动失效。
 * 搜索使用 ILIKE 模糊匹配 nickname + username，结果不缓存（实时性要求高）。
 *
 * @note 本服务不处理认证/注册逻辑，那部分由 AccountController 直接操作数据库完成。
 */

#pragma once

#include "domain/user/repositories/IUserRepository.h"
#include "infrastructure/cache/CacheManager.h"
#include "infrastructure/events/EventBus.h"
#include <memory>
#include <string>
#include <vector>
#include <json/json.h>

namespace heartlake {
namespace application {

class UserApplicationService {
private:
    std::shared_ptr<domain::user::IUserRepository> userRepository_;
    std::shared_ptr<core::cache::CacheManager> cacheManager_;
    std::shared_ptr<core::events::EventBus> eventBus_;

public:
    UserApplicationService(
        std::shared_ptr<domain::user::IUserRepository> userRepository,
        std::shared_ptr<core::cache::CacheManager> cacheManager,
        std::shared_ptr<core::events::EventBus> eventBus
    ) : userRepository_(userRepository),
        cacheManager_(cacheManager),
        eventBus_(eventBus) {}

    /**
     * @brief 获取用户公开资料
     * @details 优先从缓存读取，miss 时查库回填。返回 JSON 包含
     *          nickname、avatar、bio、stone_count、friend_count 等聚合信息。
     * @param userId 目标用户 ID
     * @return 用户资料 JSON，用户不存在时返回 error 字段
     */
    Json::Value getUserProfile(const std::string& userId);

    /**
     * @brief 更新用户资料
     * @details 支持部分更新——只修改 updates 中包含的字段。
     *          更新后清除该用户的缓存条目。
     * @param userId 当前登录用户 ID
     * @param updates 待更新字段的 JSON（如 nickname、avatar、bio）
     */
    void updateUserProfile(
        const std::string& userId,
        const Json::Value& updates
    );

    /**
     * @brief 按关键词搜索用户
     * @details 对 nickname 和 username 做 ILIKE 模糊匹配，结果按相关度排序。
     * @param keyword 搜索关键词
     * @param page 页码（从 1 开始）
     * @param pageSize 每页数量
     * @return 分页 JSON，含 items 数组和 total
     */
    Json::Value searchUsers(
        const std::string& keyword,
        int page,
        int pageSize
    );

    /**
     * @brief 批量获取用户基本信息
     * @details 用于好友列表、涟漪列表等场景的用户信息补全，避免 N+1 查询。
     *          单次最多 100 个 ID，超出截断。
     * @param userIds 用户 ID 列表
     * @return JSON 数组，每个元素包含 userId / nickname / avatar
     */
    Json::Value getUsersBatch(const std::vector<std::string>& userIds);
};

} // namespace application
} // namespace heartlake
