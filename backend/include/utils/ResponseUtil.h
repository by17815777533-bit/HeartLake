/**
 * HTTP 响应构建工具 -- 统一 API 响应格式
 *
 * 所有 Controller 通过本工具类构建 HTTP 响应，确保全局一致的 JSON 格式：
 * {
 *   "code": <业务错误码>,
 *   "message": "<描述>",
 *   "data": <业务数据>
 * }
 *
 * 提供两类接口：
 *   - 语义化快捷方法：success / badRequest / unauthorized / notFound 等
 *   - 通用构建方法：error(ErrorCode) 支持标准错误码，error(int) 兼容旧代码
 *
 * @note 分页响应 paged() 额外包含 total / page / pageSize 字段。
 */

#pragma once

#include <drogon/HttpResponse.h>
#include <json/json.h>
#include "utils/ErrorCode.h"

namespace heartlake {
namespace utils {

/**
 * HTTP 响应构建工具类
 *
 * 提供统一的响应格式和便捷的响应构建方法
 * 支持标准错误码和自定义错误码
 */
class ResponseUtil {
public:
    /**
     * 成功响应
     * @param data 响应数据
     * @param message 成功消息
     * @return HTTP 响应对象
     */
    static drogon::HttpResponsePtr success(
        const Json::Value& data = Json::Value(),
        const std::string& message = "Success"
    );

    /**
     * 使用标准错误码的错误响应
     * @param errorCode 错误码枚举
     * @param detail 详细错误信息
     * @param extra 额外数据
     * @return HTTP 响应对象
     */
    static drogon::HttpResponsePtr error(
        ErrorCode errorCode,
        const std::string& detail = "",
        const Json::Value& extra = Json::Value()
    );

    /**
     * 自定义错误响应 (兼容旧代码)
     * @param httpStatus HTTP 状态码
     * @param message 错误消息
     * @param data 额外数据
     * @return HTTP 响应对象
     */
    static drogon::HttpResponsePtr error(
        int httpStatus,
        const std::string& message,
        const Json::Value& data = Json::Value()
    );

    /**
     * 分页响应
     * @param items 数据项列表
     * @param total 总记录数
     * @param page 当前页码
     * @param pageSize 每页大小
     * @return HTTP 响应对象
     */
    static drogon::HttpResponsePtr paged(
        const Json::Value& items,
        int total,
        int page,
        int pageSize
    );

    /**
     * 无内容响应 (204 No Content)
     * @return HTTP 响应对象
     */
    static drogon::HttpResponsePtr noContent();

    // ==================== 便捷方法 ====================

    /**
     * 201 Created 响应
     */
    static drogon::HttpResponsePtr created(
        const Json::Value& data = Json::Value(),
        const std::string& message = "Created"
    );

    /**
     * 400 Bad Request
     */
    static drogon::HttpResponsePtr badRequest(const std::string& message = "请求参数错误");

    /**
     * 401 Unauthorized
     */
    static drogon::HttpResponsePtr unauthorized(const std::string& message = "未授权");

    /**
     * 403 Forbidden
     */
    static drogon::HttpResponsePtr forbidden(const std::string& message = "禁止访问");

    /**
     * 404 Not Found
     */
    static drogon::HttpResponsePtr notFound(const std::string& message = "资源不存在");

    /**
     * 409 Conflict
     */
    static drogon::HttpResponsePtr conflict(const std::string& message = "资源冲突");

    /**
     * 429 Too Many Requests
     */
    static drogon::HttpResponsePtr tooManyRequests(const std::string& message = "请求过于频繁");

    /**
     * 500 Internal Server Error
     */
    static drogon::HttpResponsePtr internalError(const std::string& message = "服务器内部错误");

private:
    /**
     * 创建标准格式的响应
     * @param httpStatus HTTP 状态码
     * @param businessCode 业务错误码
     * @param message 消息
     * @param data 数据
     * @return HTTP 响应对象
     */
    static drogon::HttpResponsePtr createResponse(
        drogon::HttpStatusCode httpStatus,
        int businessCode,
        const std::string& message,
        const Json::Value& data = Json::Value()
    );
};

} // namespace utils
} // namespace heartlake
