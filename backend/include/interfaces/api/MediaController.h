#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

class MediaController : public drogon::HttpController<MediaController> {
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(MediaController::upload, "/api/media/upload", Post,
                "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(MediaController::serve, "/api/media/{1}/{2}", Get);
  METHOD_LIST_END

  void upload(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);

  void serve(const HttpRequestPtr &req,
             std::function<void(const HttpResponsePtr &)> &&callback,
             const std::string &category, const std::string &filename);
};

} // namespace controllers
} // namespace heartlake
