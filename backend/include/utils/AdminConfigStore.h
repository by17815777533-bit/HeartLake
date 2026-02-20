/**
 * @file AdminConfigStore.h
 * @brief AdminConfigStore 模块接口定义
 * Created by 林子怡
 */

#pragma once

#include <json/json.h>
#include <mutex>
#include <string>

namespace heartlake {
namespace utils {

/**
 * @brief 管理员配置存储
 *
 * 详细说明
 *
 * @note 注意事项
 */
class AdminConfigStore {
public:
    static Json::Value load();
    /**
     * @brief save方法
     *
     * @param config 参数说明
     * @return 返回值说明
     */
    static bool save(const Json::Value& config);
    static Json::Value defaultConfig();
    static std::string configFilePath();

private:
    static std::mutex mutex_;
};

} // namespace utils
} // namespace heartlake
