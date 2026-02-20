/**
 * @file MediaController.h
 * @brief MediaController 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/services/MediaService.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 媒体相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class MediaController : public drogon::HttpController<MediaController> {
public:
    METHOD_LIST_BEGIN
    
    ADD_METHOD_TO(MediaController::uploadMedia, "/api/media/upload", Post);
    
    ADD_METHOD_TO(MediaController::uploadMultiple, "/api/media/upload/multiple", Post);
    
    ADD_METHOD_TO(MediaController::getMediaInfo, "/api/media/{1}", Get);
    
    ADD_METHOD_TO(MediaController::deleteMedia, "/api/media/{1}", Delete);
    
    METHOD_LIST_END
    
    void uploadMedia(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);
    
    void uploadMultiple(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getMediaInfo(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     const std::string &mediaId);
    
    void deleteMedia(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &mediaId);
};

} // namespace controllers
} // namespace heartlake
