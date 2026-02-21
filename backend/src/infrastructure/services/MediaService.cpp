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
#include <set>

namespace fs = std::filesystem;

// 转义 shell 单引号：将 ' 替换为 '\''
static std::string shellEscape(const std::string& s) {
    std::string result = "'";
    for (char c : s) {
        if (c == '\'') {
            result += "'\\''";
        } else {
            result += c;
        }
    }
    result += "'";
    return result;
}

// 验证路径不包含路径遍历
static bool isPathSafe(const std::string& path) {
    if (path.find("..") != std::string::npos) return false;
    if (path.find('\0') != std::string::npos) return false;
    return true;
}
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
    // 安全检查
    if (!isPathSafe(input_path)) {
        LOG_WARN << "compressImage: unsafe path rejected: " << input_path;
        return input_path;
    }
    // 检查输入文件是否存在
    if (!fs::exists(input_path)) {
        LOG_WARN << "compressImage: input file not found: " << input_path;
        return input_path;
    }

    // 参数范围限制
    max_width = std::clamp(max_width, 100, 7680);
    quality = std::clamp(quality, 1, 100);

    // 构造输出路径：在文件名后加 _compressed
    fs::path p(input_path);
    std::string stem = p.stem().string();
    std::string ext = p.extension().string();
    fs::path output_path = p.parent_path() / (stem + "_compressed" + ext);

    // 检测 ImageMagick 是否可用（优先 magick，回退 convert）
    std::string magick_cmd;
    if (std::system("which magick > /dev/null 2>&1") == 0) {
        magick_cmd = "magick";
    } else if (std::system("which convert > /dev/null 2>&1") == 0) {
        magick_cmd = "convert";
    } else {
        LOG_WARN << "compressImage: ImageMagick not available, returning original file";
        return input_path;
    }

    // 构造命令：使用 shellEscape 防止命令注入
    std::string cmd = magick_cmd + " "
        + shellEscape(input_path)
        + " -resize " + std::to_string(max_width) + "x\\>"
        + " -quality " + std::to_string(quality)
        + " " + shellEscape(output_path.string())
        + " 2>&1";

    LOG_INFO << "compressImage: executing: " << cmd;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOG_WARN << "compressImage: failed to execute command";
        return input_path;
    }

    char buffer[256];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    int ret = pclose(pipe);

    if (ret != 0) {
        LOG_WARN << "compressImage: command failed (exit " << ret << "): " << result;
        return input_path;
    }

    if (!fs::exists(output_path)) {
        LOG_WARN << "compressImage: output file not created";
        return input_path;
    }

    LOG_INFO << "compressImage: compressed " << input_path
             << " -> " << output_path.string()
             << " (original: " << fs::file_size(input_path)
             << " bytes, compressed: " << fs::file_size(output_path) << " bytes)";
    return output_path.string();
}

std::string MediaService::compressVideo(const std::string& input_path, int max_bitrate) {
    // 安全检查
    if (!isPathSafe(input_path)) {
        LOG_WARN << "compressVideo: unsafe path rejected: " << input_path;
        return input_path;
    }
    // 检查输入文件是否存在
    if (!fs::exists(input_path)) {
        LOG_WARN << "compressVideo: input file not found: " << input_path;
        return input_path;
    }

    // 参数范围限制
    max_bitrate = std::clamp(max_bitrate, 100, 50000);

    // 检测 FFmpeg 是否可用
    if (std::system("which ffmpeg > /dev/null 2>&1") != 0) {
        LOG_WARN << "compressVideo: FFmpeg not available, returning original file";
        return input_path;
    }

    // 构造输出路径
    fs::path p(input_path);
    std::string stem = p.stem().string();
    std::string ext = p.extension().string();
    fs::path output_path = p.parent_path() / (stem + "_compressed" + ext);

    std::string cmd = "ffmpeg -y -i"
        " " + shellEscape(input_path) +
        " -c:v libx264 -preset medium -crf 23"
        " -vf 'scale=-2:min(720\\,ih)'"
        " -maxrate " + std::to_string(max_bitrate) + "k"
        " -bufsize " + std::to_string(max_bitrate * 2) + "k"
        " -c:a aac -b:a 128k"
        " -movflags +faststart"
        " " + shellEscape(output_path.string()) +
        " 2>&1";

    LOG_INFO << "compressVideo: executing: " << cmd;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOG_WARN << "compressVideo: failed to execute command";
        return input_path;
    }

    char buffer[256];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    int ret = pclose(pipe);

    if (ret != 0) {
        LOG_WARN << "compressVideo: command failed (exit " << ret << "): " << result;
        return input_path;
    }

    if (!fs::exists(output_path)) {
        LOG_WARN << "compressVideo: output file not created";
        return input_path;
    }

    LOG_INFO << "compressVideo: compressed " << input_path
             << " -> " << output_path.string()
             << " (original: " << fs::file_size(input_path)
             << " bytes, compressed: " << fs::file_size(output_path) << " bytes)";
    return output_path.string();
}

std::string MediaService::convertAudio(const std::string& input_path,
                                      const std::string& output_format) {
    // 安全检查
    if (!isPathSafe(input_path)) {
        LOG_WARN << "convertAudio: unsafe path rejected: " << input_path;
        return input_path;
    }
    // 验证 output_format 只允许白名单格式
    static const std::set<std::string> allowed_formats = {"mp3", "aac", "m4a", "ogg", "wav", "flac"};
    if (allowed_formats.find(output_format) == allowed_formats.end()) {
        LOG_WARN << "convertAudio: unsupported format rejected: " << output_format;
        return input_path;
    }
    // 检查输入文件是否存在
    if (!fs::exists(input_path)) {
        LOG_WARN << "convertAudio: input file not found: " << input_path;
        return input_path;
    }

    // 检测 FFmpeg 是否可用
    if (std::system("which ffmpeg > /dev/null 2>&1") != 0) {
        LOG_WARN << "convertAudio: FFmpeg not available, returning original file";
        return input_path;
    }

    // 构造输出路径：替换扩展名为目标格式，加 _compressed 后缀
    fs::path p(input_path);
    std::string stem = p.stem().string();
    std::string target_ext = "." + output_format;
    fs::path output_path = p.parent_path() / (stem + "_compressed" + target_ext);

    // 根据输出格式选择编码器
    std::string audio_codec;
    if (output_format == "mp3") {
        audio_codec = "libmp3lame";
    } else if (output_format == "aac" || output_format == "m4a") {
        audio_codec = "aac";
    } else if (output_format == "ogg") {
        audio_codec = "libvorbis";
    } else {
        audio_codec = "aac";
    }

    std::string cmd = "ffmpeg -y -i"
        " " + shellEscape(input_path) +
        " -c:a " + audio_codec +
        " -b:a 128k"
        " -ar 44100"
        " " + shellEscape(output_path.string()) +
        " 2>&1";

    LOG_INFO << "convertAudio: executing: " << cmd;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOG_WARN << "convertAudio: failed to execute command";
        return input_path;
    }

    char buffer[256];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    int ret = pclose(pipe);

    if (ret != 0) {
        LOG_WARN << "convertAudio: command failed (exit " << ret << "): " << result;
        return input_path;
    }

    if (!fs::exists(output_path)) {
        LOG_WARN << "convertAudio: output file not created";
        return input_path;
    }

    LOG_INFO << "convertAudio: converted " << input_path
             << " -> " << output_path.string()
             << " (original: " << fs::file_size(input_path)
             << " bytes, converted: " << fs::file_size(output_path) << " bytes)";
    return output_path.string();
}

