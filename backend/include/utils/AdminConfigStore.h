/**
 * 管理后台运行时配置的持久化存储
 *
 * 将系统配置、AI参数、限流阈值等以JSON格式持久化到磁盘，
 * 支持管理员通过后台界面动态调整而无需重启服务。
 * 配置文件路径可通过环境变量 ADMIN_CONFIG_FILE 覆盖。
 * 所有读写操作通过互斥锁保证线程安全。
 */

#pragma once

#include <json/json.h>
#include <mutex>
#include <string>

namespace heartlake {
namespace utils {

/**
 * 管理后台配置存储（线程安全）
 *
 * 提供配置的加载、保存和默认值生成。
 * load() 在文件不存在或解析失败时自动回退到默认配置，保证服务始终可用。
 */
class AdminConfigStore {
public:
    /// 从磁盘加载配置，文件缺失或损坏时返回默认配置
    static Json::Value load();

    /// 将配置写入磁盘，自动创建父目录
    /// @return 写入成功返回 true
    static bool save(const Json::Value& config);

    /// 生成包含系统、AI、限流三大模块的默认配置
    static Json::Value defaultConfig();

    /// 获取配置文件路径（优先读取 ADMIN_CONFIG_FILE 环境变量）
    static std::string configFilePath();

private:
    static std::mutex mutex_;
};

} // namespace utils
} // namespace heartlake
