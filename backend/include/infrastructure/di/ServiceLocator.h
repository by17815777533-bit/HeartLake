/**
 * @brief 轻量级依赖注入容器（Service Locator 模式）
 *
 * @details
 * 全局单例，负责管理所有服务的注册与解析。支持两种注册方式：
 *
 * 1. registerInstance -- 直接注册已有实例（适合静态单例，搭配空删除器使用）
 * 2. registerSingleton -- 注册工厂函数，首次 resolve 时惰性创建并缓存
 *
 * 解析时按 std::type_index 查找，优先返回已缓存的实例，
 * 其次调用工厂函数创建。resolveRequired() 在未注册时抛异常，
 * 并通过 abi::__cxa_demangle 输出可读的类型名方便排查。
 *
 * 线程安全：所有操作在 recursive_mutex 保护下完成，
 * 使用递归锁是因为工厂函数内部可能再次调用 resolve。
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <mutex>
#include <stdexcept>
#include <cxxabi.h>

namespace heartlake::core::di {

class ServiceLocator {
public:
    static ServiceLocator& instance() {
        static ServiceLocator inst;
        return inst;
    }

    /**
     * @brief 直接注册已有实例
     * @tparam T 服务接口类型
     * @param instance 服务实例的 shared_ptr
     * @note 对静态单例应使用空删除器包装，避免容器析构时 double-free
     */
    template<typename T>
    void registerInstance(std::shared_ptr<T> instance) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        instances_[std::type_index(typeid(T))] = instance;
    }

    /**
     * @brief 注册接口到实现的单例映射（默认构造）
     * @tparam Interface 服务接口类型
     * @tparam Implementation 具体实现类型，需支持默认构造
     */
    template<typename Interface, typename Implementation>
    void registerSingleton() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        factories_[std::type_index(typeid(Interface))] = []() -> std::shared_ptr<void> {
            return std::make_shared<Implementation>();
        };
    }

    /**
     * @brief 注册自定义工厂函数的单例
     * @tparam T 服务类型
     * @param factory 创建实例的工厂函数，首次 resolve 时调用
     */
    template<typename T>
    void registerSingleton(std::function<std::shared_ptr<T>()> factory) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        factories_[std::type_index(typeid(T))] = [factory]() -> std::shared_ptr<void> {
            return factory();
        };
    }

    /**
     * @brief 解析服务实例
     * @tparam T 服务类型
     * @return 服务实例，未注册时返回 nullptr
     */
    template<typename T>
    std::shared_ptr<T> resolve() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        auto typeIdx = std::type_index(typeid(T));

        auto instIt = instances_.find(typeIdx);
        if (instIt != instances_.end()) {
            return std::static_pointer_cast<T>(instIt->second);
        }

        auto factIt = factories_.find(typeIdx);
        if (factIt != factories_.end()) {
            auto instance = std::static_pointer_cast<T>(factIt->second());
            instances_[typeIdx] = instance;
            return instance;
        }

        return nullptr;
    }

    /**
     * @brief 解析服务实例（必须存在，否则抛异常）
     * @tparam T 服务类型
     * @return 服务实例
     * @throws std::runtime_error 服务未注册时抛出，包含 demangled 类型名
     */
    template<typename T>
    std::shared_ptr<T> resolveRequired() {
        auto result = resolve<T>();
        if (!result) {
            // demangle C++ 类型名，方便排查未注册的服务
            int status = 0;
            std::unique_ptr<char, decltype(&std::free)> demangled(
                abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status), std::free);
            throw std::runtime_error("Service not registered: " +
                std::string(status == 0 ? demangled.get() : typeid(T).name()));
        }
        return result;
    }

    /// 清空所有注册的实例和工厂（主要用于测试）
    void clear() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        instances_.clear();
        factories_.clear();
    }

private:
    ServiceLocator() = default;
    std::unordered_map<std::type_index, std::shared_ptr<void>> instances_;              ///< 已创建的实例缓存
    std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>> factories_;  ///< 惰性工厂函数
    std::recursive_mutex mutex_;  ///< 递归锁，允许工厂函数内部再次 resolve
};

} // namespace heartlake::core::di
