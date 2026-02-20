/**
 * @file MediaController.cpp
 * @brief MediaController 模块实现
 * Created by 白洋
 */
#include "interfaces/api/MediaController.h"
#include "utils/ResponseUtil.h"
#include <drogon/HttpAppFramework.h>
#include <drogon/MultiPart.h>
#include <algorithm>
#include <cctype>

using namespace heartlake::controllers;
using namespace heartlake::utils;
using namespace heartlake::services;

void MediaController::uploadMedia(const HttpRequestPtr &req,
                                 std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        std::string user_id;
        try { user_id = req->getAttributes()->get<std::string>("user_id"); } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        // 解析 multipart/form-data
        MultiPartParser parser;
        if (parser.parse(req) != 0) {
            callback(ResponseUtil::badRequest("Invalid multipart data"));
            return;
        }

        auto& files = parser.getFiles();
        if (files.empty()) {
            callback(ResponseUtil::badRequest("No file uploaded"));
            return;
        }

        auto& file = files[0];
        std::string file_data(file.fileContent());
        std::string file_name = file.getFileName();

        // SEC-02: 文件大小限制 — 防止超大文件上传导致DoS
        constexpr size_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB
        if (file_data.size() > MAX_FILE_SIZE) {
            callback(ResponseUtil::badRequest("文件大小超过限制（最大10MB）"));
            return;
        }

        // 根据文件扩展名检测 MIME 类型
        std::string content_type = "application/octet-stream";
        std::string ext;
        size_t dot_pos = file_name.find_last_of('.');
        if (dot_pos != std::string::npos) {
            ext = file_name.substr(dot_pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            // 图片类型
            if (ext == "jpg" || ext == "jpeg") content_type = "image/jpeg";
            else if (ext == "png") content_type = "image/png";
            else if (ext == "gif") content_type = "image/gif";
            else if (ext == "webp") content_type = "image/webp";
            // 视频类型
            else if (ext == "mp4") content_type = "video/mp4";
            else if (ext == "mov") content_type = "video/quicktime";
            else if (ext == "avi") content_type = "video/x-msvideo";
            else if (ext == "mkv") content_type = "video/x-matroska";
            // 音频类型
            else if (ext == "mp3") content_type = "audio/mpeg";
            else if (ext == "wav") content_type = "audio/wav";
            else if (ext == "ogg") content_type = "audio/ogg";
            else if (ext == "m4a") content_type = "audio/mp4";
            else if (ext == "aac") content_type = "audio/aac";
        }

        // SEC-02: 文件类型白名单 — 拒绝未知/危险文件类型
        if (content_type == "application/octet-stream") {
            callback(ResponseUtil::badRequest("不支持的文件类型，仅允许图片、视频和音频文件"));
            return;
        }
        
        LOG_INFO << "Upload media: " << file_name << " (" << content_type << ", " << file_data.size() << " bytes)";

        // 上传文件
        auto& mediaService = MediaService::getInstance();
        auto info = mediaService.uploadMedia(file_data, file_name, content_type, user_id);

        Json::Value response;
        response["media_id"] = info.media_id;
        response["url"] = info.url;
        response["thumbnail_url"] = info.thumbnail_url;
        response["type"] = static_cast<int>(info.type);
        response["size"] = (Json::Int64)info.size;
        response["mime_type"] = info.mime_type;

        callback(ResponseUtil::success(response, "上传成功"));

    } catch (const std::exception& e) {
        LOG_ERROR << "Upload media error: " << e.what();
        callback(ResponseUtil::internalError("文件上传失败"));
    }
}

void MediaController::uploadMultiple(const HttpRequestPtr &req,
                                    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        std::string user_id;
        try { user_id = req->getAttributes()->get<std::string>("user_id"); } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        MultiPartParser parser;
        if (parser.parse(req) != 0) {
            callback(ResponseUtil::badRequest("Invalid multipart data"));
            return;
        }

        auto& files = parser.getFiles();
        if (files.empty()) {
            callback(ResponseUtil::badRequest("No files uploaded"));
            return;
        }

        if (files.size() > 9) {
            callback(ResponseUtil::badRequest("Maximum 9 files allowed"));
            return;
        }

        std::vector<std::pair<std::string, std::string>> file_data;
        for (const auto& file : files) {
            std::string content(file.fileContent());
            file_data.emplace_back(content, file.getFileName());
        }

        auto& mediaService = MediaService::getInstance();
        auto results = mediaService.uploadMultipleMedia(file_data, user_id);

        Json::Value response(Json::arrayValue);
        for (const auto& info : results) {
            Json::Value item;
            item["media_id"] = info.media_id;
            item["url"] = info.url;
            item["thumbnail_url"] = info.thumbnail_url;
            item["type"] = static_cast<int>(info.type);
            item["size"] = (Json::Int64)info.size;
            response.append(item);
        }

        callback(ResponseUtil::success(response, "批量上传成功"));

    } catch (const std::exception& e) {
        LOG_ERROR << "Upload multiple media error: " << e.what();
        callback(ResponseUtil::internalError("批量上传失败"));
    }
}

void MediaController::getMediaInfo(const HttpRequestPtr & /*req*/,
                                  std::function<void(const HttpResponsePtr &)> &&callback,
                                  const std::string &mediaId) {
    try {
        auto& mediaService = MediaService::getInstance();
        auto info = mediaService.getMediaInfo(mediaId);

        Json::Value response;
        response["media_id"] = info.media_id;
        response["url"] = info.url;
        response["thumbnail_url"] = info.thumbnail_url;
        response["type"] = static_cast<int>(info.type);
        response["size"] = (Json::Int64)info.size;
        response["mime_type"] = info.mime_type;

        callback(ResponseUtil::success(response));

    } catch (const std::exception& e) {
        LOG_ERROR << "Get media info error: " << e.what();
        callback(ResponseUtil::notFound("Media not found"));
    }
}

void MediaController::deleteMedia(const HttpRequestPtr &req,
                                 std::function<void(const HttpResponsePtr &)> &&callback,
                                 const std::string &mediaId) {
    try {
        std::string user_id;
        try { user_id = req->getAttributes()->get<std::string>("user_id"); } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        auto& mediaService = MediaService::getInstance();
        bool success = mediaService.deleteMedia(mediaId, user_id);

        if (success) {
            callback(ResponseUtil::success(Json::Value(), "删除成功"));
        } else {
            callback(ResponseUtil::notFound("Media not found or access denied"));
        }

    } catch (const std::exception& e) {
        LOG_ERROR << "Delete media error: " << e.what();
        callback(ResponseUtil::internalError());
    }
}
