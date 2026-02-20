/**
 * @file MediaService.cpp
 * @brief MediaService 模块实现
 * Created by 白洋
 */
#include "infrastructure/services/MediaService.h"
#include <drogon/drogon.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <random>

namespace fs = std::filesystem;
using namespace heartlake::services;

MediaService& MediaService::getInstance() {
    static MediaService instance;
    return instance;
}

MediaType MediaService::detectMediaType(const std::string& mime_type) {
    if (std::find(ALLOWED_IMAGE_TYPES.begin(), ALLOWED_IMAGE_TYPES.end(), mime_type) 
        != ALLOWED_IMAGE_TYPES.end()) {
        return MediaType::IMAGE;
    }
    if (std::find(ALLOWED_VIDEO_TYPES.begin(), ALLOWED_VIDEO_TYPES.end(), mime_type) 
        != ALLOWED_VIDEO_TYPES.end()) {
        return MediaType::VIDEO;
    }
    if (std::find(ALLOWED_AUDIO_TYPES.begin(), ALLOWED_AUDIO_TYPES.end(), mime_type) 
        != ALLOWED_AUDIO_TYPES.end()) {
        return MediaType::AUDIO;
    }
    return MediaType::UNKNOWN;
}

bool MediaService::validateMedia(const std::string& mime_type, size_t size) {
    MediaType type = detectMediaType(mime_type);
    
    switch (type) {
        case MediaType::IMAGE:
            return size <= MAX_IMAGE_SIZE;
        case MediaType::VIDEO:
            return size <= MAX_VIDEO_SIZE;
        case MediaType::AUDIO:
            return size <= MAX_AUDIO_SIZE;
        default:
            return false;
    }
}

std::string MediaService::getStoragePath(MediaType type, const std::string& user_id) {
    std::string base_path = "./uploads/";
    std::string type_path;
    
    switch (type) {
        case MediaType::IMAGE:
            type_path = "images/";
            break;
        case MediaType::VIDEO:
            type_path = "videos/";
            break;
        case MediaType::AUDIO:
            type_path = "audio/";
            break;
        default:
            type_path = "others/";
    }
    
    // 按日期和用户组织目录
    time_t now = time(nullptr);
    struct tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &now);
#else
    localtime_r(&now, &timeinfo);
#endif
    char date_dir[32];
    strftime(date_dir, sizeof(date_dir), "%Y%m%d", &timeinfo);

    // 验证user_id防止路径遍历攻击 - 使用白名单验证
    if (user_id.empty() || user_id.length() > 64) {
        throw std::runtime_error("Invalid user_id");
    }
    for (char c : user_id) {
        if (!std::isalnum(c) && c != '-' && c != '_') {
            throw std::runtime_error("Invalid user_id");
        }
    }

    std::string full_path = base_path + type_path + date_dir + "/" + user_id + "/";
    
    // 创建目录
    fs::create_directories(full_path);
    
    return full_path;
}

std::string MediaService::generateMediaId() {
    // 使用时间戳 + 随机数生成
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    return "MED" + std::to_string(timestamp) + std::to_string(dis(gen));
}

std::string MediaService::saveFile(const std::string& data, 
                                  const std::string& file_name,
                                  const std::string& user_id) {
    // 提取文件扩展名
    std::string ext;
    size_t dot_pos = file_name.find_last_of('.');
    if (dot_pos != std::string::npos) {
        ext = file_name.substr(dot_pos);
    }
    
    // 生成唯一文件名
    std::string unique_name = generateMediaId() + ext;
    
    // 获取mime type (简化版)
    std::string mime_type = "application/octet-stream";
    if (ext == ".jpg" || ext == ".jpeg") mime_type = "image/jpeg";
    else if (ext == ".png") mime_type = "image/png";
    else if (ext == ".gif") mime_type = "image/gif";
    else if (ext == ".mp4") mime_type = "video/mp4";
    else if (ext == ".mp3") mime_type = "audio/mpeg";
    
    MediaType type = detectMediaType(mime_type);
    std::string dir = getStoragePath(type, user_id);
    std::string file_path = dir + unique_name;
    
    // 写入文件
    std::ofstream ofs(file_path, std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Failed to open file for writing: " + file_path);
    }
    ofs.write(data.c_str(), data.size());
    if (!ofs.good()) {
        throw std::runtime_error("Failed to write file: " + file_path);
    }
    ofs.close();

    return file_path;
}

MediaInfo MediaService::uploadMedia(const std::string& file_data,
                                   const std::string& file_name,
                                   const std::string& mime_type,
                                   const std::string& user_id) {
    MediaInfo info;
    
    // 验证
    if (!validateMedia(mime_type, file_data.size())) {
        throw std::runtime_error("Invalid media file");
    }
    
    // 生成唯一ID（只生成一次）
    info.media_id = generateMediaId();

    // 保存文件（使用已生成的media_id）
    std::string ext;
    size_t dot_pos = file_name.find_last_of('.');
    if (dot_pos != std::string::npos) {
        ext = file_name.substr(dot_pos);
    }
    info.type = detectMediaType(mime_type);
    std::string dir = getStoragePath(info.type, user_id);
    std::string file_path = dir + info.media_id + ext;

    std::ofstream ofs(file_path, std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Failed to open file for writing");
    }
    ofs.write(file_data.c_str(), file_data.size());
    if (!ofs.good()) {
        throw std::runtime_error("Failed to write file");
    }
    ofs.close();

    info.file_path = file_path;
    info.size = file_data.size();
    info.mime_type = mime_type;

    // 生成URL（基于base_path动态计算）
    const std::string base_path = "./uploads/";
    info.url = "/uploads/" + file_path.substr(base_path.length());
    
    // 如果是图片或视频，生成缩略图
    if (info.type == MediaType::IMAGE || info.type == MediaType::VIDEO) {
        try {
            info.thumbnail_url = generateThumbnail(file_path, info.type);
        } catch (...) {
            LOG_WARN << "Failed to generate thumbnail for " << file_path;
        }
    }
    
    // 保存到数据库
    auto dbClient = drogon::app().getDbClient("default");
    try {
        dbClient->execSqlSync(
            "INSERT INTO media_files (media_id, user_id, file_path, url, media_type, "
            "size, mime_type, thumbnail_url, created_at) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, NOW())",
            info.media_id, user_id, file_path, info.url, 
            static_cast<int>(info.type), info.size, mime_type, 
            info.thumbnail_url
        );
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to save media info to database: " << e.what();
        // 删除已上传的文件
        fs::remove(file_path);
        throw;
    }
    
    return info;
}

std::vector<MediaInfo> MediaService::uploadMultipleMedia(
    const std::vector<std::pair<std::string, std::string>>& files,
    const std::string& user_id) {
    
    std::vector<MediaInfo> results;
    
    if (files.size() > 9) {
        throw std::runtime_error("Maximum 9 files allowed");
    }
    
    for (const auto& [data, file_name] : files) {
        // 根据文件扩展名检测 MIME 类型
        std::string mime_type = "application/octet-stream";
        size_t dot_pos = file_name.find_last_of('.');
        if (dot_pos != std::string::npos) {
            std::string ext = file_name.substr(dot_pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            // 图片类型
            if (ext == "jpg" || ext == "jpeg") mime_type = "image/jpeg";
            else if (ext == "png") mime_type = "image/png";
            else if (ext == "gif") mime_type = "image/gif";
            else if (ext == "webp") mime_type = "image/webp";
            // 视频类型
            else if (ext == "mp4") mime_type = "video/mp4";
            else if (ext == "mov") mime_type = "video/quicktime";
            else if (ext == "avi") mime_type = "video/x-msvideo";
            // 音频类型
            else if (ext == "mp3") mime_type = "audio/mpeg";
            else if (ext == "wav") mime_type = "audio/wav";
            else if (ext == "m4a") mime_type = "audio/mp4";
        }
        
        try {
            MediaInfo info = uploadMedia(data, file_name, mime_type, user_id);
            results.push_back(info);
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to upload " << file_name << ": " << e.what();
        }
    }
    
    return results;
}

std::string MediaService::generateThumbnail(const std::string& file_path, MediaType type) {
    // 简化版：返回原图路径作为缩略图
    // 生产环境建议使用 ImageMagick (图片) 或 FFmpeg (视频) 生成真实缩略图
    // 命令示例:
    // - 图片: convert input.jpg -resize 300x300 -quality 80 thumbnail.jpg
    // - 视频: ffmpeg -i input.mp4 -ss 00:00:01 -vframes 1 -s 300x300 thumbnail.jpg
    
    LOG_INFO << "Using original file as thumbnail (type: " << static_cast<int>(type) << "): " << file_path;
    return file_path;
}

MediaInfo MediaService::getMediaInfo(const std::string& media_id) {
    auto dbClient = drogon::app().getDbClient("default");
    auto result = dbClient->execSqlSync(
        "SELECT * FROM media_files WHERE media_id = $1",
        media_id
    );
    
    if (result.empty()) {
        throw std::runtime_error("Media not found");
    }
    
    MediaInfo info;
    auto row = result[0];
    info.media_id = row["media_id"].as<std::string>();
    info.file_path = row["file_path"].as<std::string>();
    info.url = row["url"].as<std::string>();
    info.type = static_cast<MediaType>(row["media_type"].as<int>());
    info.size = row["size"].as<size_t>();
    info.mime_type = row["mime_type"].as<std::string>();
    info.thumbnail_url = row["thumbnail_url"].isNull() ? "" : row["thumbnail_url"].as<std::string>();
    
    return info;
}

bool MediaService::deleteMedia(const std::string& media_id, const std::string& user_id) {
    auto dbClient = drogon::app().getDbClient("default");
    
    // 验证权限
    auto result = dbClient->execSqlSync(
        "SELECT file_path FROM media_files WHERE media_id = $1 AND user_id = $2",
        media_id, user_id
    );
    
    if (result.empty()) {
        return false;
    }
    
    std::string file_path = result[0]["file_path"].as<std::string>();
    
    // 删除文件
    try {
        fs::remove(file_path);
    } catch (...) {
        LOG_WARN << "Failed to delete file: " << file_path;
    }
    
    // 从数据库删除
    dbClient->execSqlSync(
        "DELETE FROM media_files WHERE media_id = $1 AND user_id = $2",
        media_id, user_id
    );
    
    return true;
}

std::string MediaService::compressImage(const std::string& input_path, 
                                       int max_width, int quality) {
    // 简化版：返回原路径
    // 生产环境建议使用 ImageMagick 或 libvips 进行压缩
    // 命令示例: convert input.jpg -resize {max_width}x -quality {quality} output.jpg
    
    LOG_INFO << "Image compression disabled (max_width: " << max_width 
             << ", quality: " << quality << ")";
    return input_path;
}

std::string MediaService::compressVideo(const std::string& input_path, int max_bitrate) {
    // 简化版：返回原路径
    // 生产环境建议使用 FFmpeg 进行视频压缩
    // 命令示例: ffmpeg -i input.mp4 -b:v {max_bitrate}k -c:v libx264 -preset medium output.mp4
    
    LOG_INFO << "Video compression disabled (max_bitrate: " << max_bitrate << "k)";
    return input_path;
}

std::string MediaService::convertAudio(const std::string& input_path,
                                      const std::string& output_format) {
    // 简化版：返回原路径
    // 生产环境建议使用 FFmpeg 进行音频格式转换
    // 命令示例: ffmpeg -i input.wav -c:a libmp3lame -b:a 192k output.mp3
    
    LOG_INFO << "Audio conversion disabled (target format: " << output_format << ")";
    return input_path;
}

