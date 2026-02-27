/**
 * ApplicationServiceFactory 模块接口定义
 */

#pragma once


namespace heartlake {
namespace application {

/**
 * 应用服务工厂
 *
 * 负责创建和初始化所有应用服务
 */
class ApplicationServiceFactory {
public:
    /**
     * 初始化所有应用服务并注册到 DI 容器
     */
    static void initialize();
};

} // namespace application
} // namespace heartlake
