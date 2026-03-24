#pragma once

/**
 * 媒体控制器 - 统一文件上传与公开访问
 *
 * 负责头像、图片、音频、视频的 multipart 上传，以及上传后媒体的稳定访问地址。
 * 上传接口统一落盘到 `app().getUploadPath()` 下的分类目录，并返回可直接给前端使用的 URL。
 */

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

  /// POST /api/media/upload - 上传头像或通用媒体文件（multipart/form-data）
  void upload(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);

  /// GET /api/media/{category}/{filename} - 公开访问已上传媒体文件
  void serve(const HttpRequestPtr &req,
             std::function<void(const HttpResponsePtr &)> &&callback,
             const std::string &category, const std::string &filename);
};

} // namespace controllers
} // namespace heartlake
