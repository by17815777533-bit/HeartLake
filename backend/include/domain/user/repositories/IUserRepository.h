/**
 * 用户仓储抽象接口
 *
 * 定义用户 Aggregate Root 的持久化契约，是领域层与基础设施层之间的
 * 依赖倒置边界（Dependency Inversion）。领域服务和应用服务只依赖此接口，
 * 具体的 PostgreSQL 实现（UserRepository）在启动阶段通过 DI 容器注入。
 */

#pragma once

#include <string>
#include <optional>

namespace heartlake::domain::user {

/**
 * @brief 用户领域实体
 * @details 映射 users 表核心字段，不包含密码哈希等安全敏感信息。
 *          密码验证由 AccountController 在接口层直接处理。
 */
struct UserEntity {
    std::string userId;
    std::string username;
    std::string nickname;
    std::string email;
    bool isAnonymous;
    std::string status;      ///< 账号状态：active / banned / deleted
    std::string createdAt;
};

/**
 * @brief 用户仓储接口（纯虚基类）
 *
 * 遵循 DDD Repository 模式，只暴露领域层真正需要的查询与命令方法。
 * 当前仅提供同步接口，异步版本在 UserRepository 实现类中扩展。
 *
 * @note 查询方法只返回 status='active' 的用户，已封禁和已删除的用户不可见。
 */
class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    /**
     * @brief 按用户 ID 查找活跃用户
     * @param userId 用户唯一标识
     * @return 用户实体，未找到或非 active 状态时返回 nullopt
     */
    virtual std::optional<UserEntity> findById(const std::string& userId) = 0;

    /**
     * @brief 按用户名查找活跃用户
     * @param username 用户名（唯一索引）
     * @return 用户实体，未找到时返回 nullopt
     */
    virtual std::optional<UserEntity> findByUsername(const std::string& username) = 0;

    /**
     * @brief 刷新用户最后活跃时间
     * @details 内部采用异步写入（fire-and-forget），不阻塞调用方。
     *          用于登录、发石头等关键操作后更新活跃状态。
     * @param userId 用户 ID
     */
    virtual void updateLastActive(const std::string& userId) = 0;
};

} // namespace heartlake::domain::user
