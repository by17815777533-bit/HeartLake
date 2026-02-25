#pragma once
#include <string>
#include <optional>
#include <drogon/HttpRequest.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Row.h>
#include <drogon/orm/Field.h>

namespace heartlake::utils {

// 替代所有裸 std::stoi + catch(...)
inline int safeInt(const std::string& s, int def = 0) {
    if (s.empty()) return def;
    try { return std::stoi(s); } catch (...) { return def; }
}

inline double safeDouble(const std::string& s, double def = 0.0) {
    if (s.empty()) return def;
    try { return std::stod(s); } catch (...) { return def; }
}

// 替代所有重复的 parsePaginationParams
inline std::pair<int,int> safePagination(const drogon::HttpRequestPtr& req,
                                          int defPage = 1, int defSize = 20) {
    int page = safeInt(req->getParameter("page"), defPage);
    int size = safeInt(req->getParameter("page_size"), defSize);
    if (page < 1 || page > 10000) page = defPage;
    if (size < 1 || size > 100) size = defSize;
    return {page, size};
}

// 替代所有裸 result[0] 访问
inline std::optional<drogon::orm::Row> safeRow(const drogon::orm::Result& result) {
    if (result.empty()) return std::nullopt;
    return result[0];
}

// 替代所有 result[0]["total"].as<int>()
inline int safeCount(const drogon::orm::Result& result, const std::string& col = "total") {
    if (result.empty()) return 0;
    return result[0][col].as<int>();
}

} // namespace heartlake::utils
