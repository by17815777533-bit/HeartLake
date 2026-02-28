/**
 * @brief HTTP 请求辅助工具函数集
 *
 * 提供零异常的参数解析、安全的分页参数提取、以及 ORM 查询结果的
 * 安全访问封装，避免 Controller 层出现裸的 stoi/result[0] 等危险调用。
 */
#pragma once
#include <string>
#include <optional>
#include <charconv>
#include <drogon/HttpRequest.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Row.h>
#include <drogon/orm/Field.h>

namespace heartlake::utils {

/**
 * @brief 安全的字符串转 int，基于 std::from_chars（不抛异常）
 * @param s 输入字符串
 * @param def 解析失败时的默认值
 * @return 解析结果或默认值
 */
inline int safeInt(const std::string& s, int def = 0) {
    if (s.empty()) return def;
    int val = def;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
    return ec == std::errc{} ? val : def;
}

/// 安全的字符串转 double，解析失败返回默认值
inline double safeDouble(const std::string& s, double def = 0.0) {
    if (s.empty()) return def;
    double val = def;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
    return ec == std::errc{} ? val : def;
}

/**
 * @brief 从请求中安全提取分页参数，带边界校验
 * @param req HTTP 请求
 * @param defPage 默认页码（page < 1 或 > 10000 时回退）
 * @param defSize 默认每页大小（page_size < 1 或 > 100 时回退）
 * @return {page, page_size}
 */
inline std::pair<int,int> safePagination(const drogon::HttpRequestPtr& req,
                                          int defPage = 1, int defSize = 20) {
    int page = safeInt(req->getParameter("page"), defPage);
    int size = safeInt(req->getParameter("page_size"), defSize);
    if (page < 1 || page > 10000) page = defPage;
    if (size < 1 || size > 100) size = defSize;
    return {page, size};
}

/// 安全获取查询结果的第一行，结果集为空时返回 nullopt
inline std::optional<drogon::orm::Row> safeRow(const drogon::orm::Result& result) {
    if (result.empty()) return std::nullopt;
    return result[0];
}

/// 安全获取 COUNT 查询结果，结果集为空时返回 0
inline int safeCount(const drogon::orm::Result& result, const std::string& col = "total") {
    if (result.empty()) return 0;
    return result[0][col].as<int>();
}

} // namespace heartlake::utils
