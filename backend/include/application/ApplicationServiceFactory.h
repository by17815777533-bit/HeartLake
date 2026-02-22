/**
 * @file ApplicationServiceFactory.h
 * @brief ApplicationServiceFactory 模块接口定义
 * Created by 白洋
 */

#pragma once


namespace heartlake {
namespace application {

/**
 * @brief 应用服务工厂
 *
 * 负责创建和初始化所有应用服务
 */
class ApplicationServiceFactory {
public:
    /**
     * @brief 初始化所有应用服务并注册到 DI 容器
     */
    static void initialize();
};

} // namespace application
} // namespace heartlake
