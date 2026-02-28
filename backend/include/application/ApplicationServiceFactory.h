/**
 * 应用服务工厂（Composition Root）
 *
 * 在 DDD 架构中充当 Composition Root 角色：启动阶段一次性完成所有应用服务的
 * 依赖组装与 DI 容器注册。内部从 ServiceLocator 解析领域服务、仓储实现和
 * 基础设施组件，按依赖拓扑顺序组装出各 ApplicationService 实例。
 *
 * 对于 CacheManager / EventBus 等全局单例，使用空删除器（null deleter）
 * 包装为 shared_ptr，避免 DI 容器析构时重复释放。
 *
 * @note 整个应用生命周期只调用一次 initialize()，在 main() 中 Drogon 启动前执行。
 */

#pragma once


namespace heartlake {
namespace application {

class ApplicationServiceFactory {
public:
    /**
     * @brief 初始化全部应用服务并注册到 ServiceLocator
     *
     * 注册顺序遵循依赖关系：
     *   1. StoneApplicationService（依赖 StoneService + StoneRepository）
     *   2. UserApplicationService（依赖 UserRepository）
     *   3. InteractionApplicationService（依赖 CacheManager + EventBus）
     *   4. FriendApplicationService（依赖 FriendService + FriendRepository）
     *
     * @note 必须在 Drogon app().run() 之前调用，否则控制器无法解析服务依赖
     */
    static void initialize();
};

} // namespace application
} // namespace heartlake
