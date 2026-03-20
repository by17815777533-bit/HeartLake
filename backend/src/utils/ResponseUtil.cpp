/**
 * ResponseUtil 模块实现
 */

#include "utils/ResponseUtil.h"

namespace heartlake {
namespace utils {

drogon::HttpResponsePtr ResponseUtil::success(const Json::Value &data,
                                              const std::string &message) {
  return createResponse(drogon::k200OK, 0, message, data);
}

drogon::HttpResponsePtr ResponseUtil::created(const Json::Value &data,
                                              const std::string &message) {
  return createResponse(drogon::k201Created, 0, message, data);
}

drogon::HttpResponsePtr ResponseUtil::error(ErrorCode errorCode,
                                            const std::string &detail,
                                            const Json::Value &extra) {
  int httpStatus = ErrorCodeHelper::getHttpStatus(errorCode);
  std::string msg =
      detail.empty() ? ErrorCodeHelper::getMessageZh(errorCode) : detail;
  return createResponse(static_cast<drogon::HttpStatusCode>(httpStatus),
                        static_cast<int>(errorCode), msg, extra);
}

drogon::HttpResponsePtr ResponseUtil::error(int httpStatus,
                                            const std::string &message,
                                            const Json::Value &data) {
  return createResponse(static_cast<drogon::HttpStatusCode>(httpStatus),
                        httpStatus, message, data);
}

drogon::HttpResponsePtr ResponseUtil::paged(const Json::Value &items, int total,
                                            int page, int pageSize) {
  Json::Value data(Json::objectValue);
  const int totalPages = pageSize > 0 ? (total + pageSize - 1) / pageSize : 0;
  const bool hasMore = pageSize > 0 && page * pageSize < total;
  data["items"] = items;
  data["total"] = total;
  data["page"] = page;
  data["page_size"] = pageSize;
  data["pageSize"] = pageSize;
  data["total_pages"] = totalPages;
  data["totalPages"] = totalPages;
  data["has_more"] = hasMore;

  Json::Value pagination(Json::objectValue);
  pagination["total"] = total;
  pagination["page"] = page;
  pagination["page_size"] = pageSize;
  pagination["pageSize"] = pageSize;
  pagination["total_pages"] = totalPages;
  pagination["totalPages"] = totalPages;
  pagination["has_more"] = hasMore;
  data["pagination"] = pagination;
  return success(data);
}

drogon::HttpResponsePtr ResponseUtil::noContent() {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(drogon::k204NoContent);
  return resp;
}

drogon::HttpResponsePtr ResponseUtil::badRequest(const std::string &message) {
  return createResponse(drogon::k400BadRequest, 400, message);
}

drogon::HttpResponsePtr ResponseUtil::unauthorized(const std::string &message) {
  return createResponse(drogon::k401Unauthorized, 401, message);
}

drogon::HttpResponsePtr ResponseUtil::forbidden(const std::string &message) {
  return createResponse(drogon::k403Forbidden, 403, message);
}

drogon::HttpResponsePtr ResponseUtil::notFound(const std::string &message) {
  return createResponse(drogon::k404NotFound, 404, message);
}

drogon::HttpResponsePtr ResponseUtil::conflict(const std::string &message) {
  return createResponse(drogon::k409Conflict, 409, message);
}

drogon::HttpResponsePtr
ResponseUtil::tooManyRequests(const std::string &message) {
  return createResponse(drogon::k429TooManyRequests, 429, message);
}

drogon::HttpResponsePtr
ResponseUtil::internalError(const std::string &message) {
  return createResponse(drogon::k500InternalServerError, 500, message);
}

drogon::HttpResponsePtr
ResponseUtil::createResponse(drogon::HttpStatusCode httpStatus,
                             int businessCode, const std::string &message,
                             const Json::Value &data) {
  Json::Value json;
  json["code"] = businessCode;
  json["message"] = message;
  // 前端期望始终存在 data 字段: {code, data, message}
  json["data"] = data.isNull() ? Json::objectValue : data;

  auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
  resp->setStatusCode(httpStatus);
  return resp;
}

} // namespace utils
} // namespace heartlake
