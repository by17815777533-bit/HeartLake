/**
 * @file AdminAuthFilter.h
 * @brief AdminAuthFilter 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <drogon/HttpFilter.h>

using namespace drogon;

namespace heartlake {
namespace filters {

/**
 * Admin认证过滤器 - 管理后台Token验证
 */
/**
 * @brief 管理员认证过滤器，用于验证管理员权限
 *
 * 详细说明
 *
 * @note 注意事项
 */
class AdminAuthFilter : public HttpFilter<AdminAuthFilter> {
public:
    static const char* classTypeName() { return "filters::AdminAuthFilter"; }
    AdminAuthFilter() = default;
    
    void doFilter(const HttpRequestPtr& req,
                  FilterCallback&& fcb,
                  FilterChainCallback&& fccb) override;
                  
private:
    /**
     * @brief verifyAdminToken方法
     *
     * @param token 参数说明
     * @param adminId 参数说明
     * @param role 参数说明
     * @return 返回值说明
     */
    bool verifyAdminToken(const std::string& token, std::string& adminId, std::string& role);
};

} // namespace filters
} // namespace heartlake
