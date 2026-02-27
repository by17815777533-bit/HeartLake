/**
 * 依赖注入容器
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

    template<typename T>
    void registerInstance(std::shared_ptr<T> instance) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        instances_[std::type_index(typeid(T))] = instance;
    }

    template<typename Interface, typename Implementation>
    void registerSingleton() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        factories_[std::type_index(typeid(Interface))] = []() -> std::shared_ptr<void> {
            return std::make_shared<Implementation>();
        };
    }

    template<typename T>
    void registerSingleton(std::function<std::shared_ptr<T>()> factory) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        factories_[std::type_index(typeid(T))] = [factory]() -> std::shared_ptr<void> {
            return factory();
        };
    }

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

    void clear() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        instances_.clear();
        factories_.clear();
    }

private:
    ServiceLocator() = default;
    std::unordered_map<std::type_index, std::shared_ptr<void>> instances_;
    std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>> factories_;
    std::recursive_mutex mutex_;
};

} // namespace heartlake::core::di
