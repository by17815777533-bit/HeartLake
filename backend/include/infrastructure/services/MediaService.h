/**
 * @file MediaService.h
 * @brief MediaService 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <drogon/HttpTypes.h>

namespace heartlake {
namespace services {

enum class MediaType {
    IMAGE,
    VIDEO,
    AUDIO,
    UNKNOWN
};

struct MediaInfo {
    std::string media_id;
    std::string file_path;
    std::string url;
    MediaType type;
    size_t size;
    int width;
    int height;
    int duration;  // For audio/video (in seconds)
    std::string mime_type;
    std::string thumbnail_url;
};

/**
 * @brief 媒体服务，用于处理媒体文件
 *
 * 详细说明
 *
 * @note 注意事项
 */
class MediaService {
public:
    static MediaService& getInstance();

    MediaInfo uploadMedia(const std::string& file_data, 
                         const std::string& file_name,
                         const std::string& mime_type,
                         const std::string& user_id);

    std::vector<MediaInfo> uploadMultipleMedia(
        const std::vector<std::pair<std::string, std::string>>& files,
        const std::string& user_id);

    std::string generateThumbnail(const std::string& file_path, 
                                  MediaType type);

    /**
     * @brief getMediaInfo方法
     *
     * @param media_id 参数说明
     * @return 返回值说明
     */
    MediaInfo getMediaInfo(const std::string& media_id);

    /**
     * @brief deleteMedia方法
     *
     * @param media_id 参数说明
     * @param user_id 参数说明
     * @return 返回值说明
     */
    bool deleteMedia(const std::string& media_id, const std::string& user_id);

    std::string compressImage(const std::string& input_path, 
                             int max_width = 1920,
                             int quality = 85);

    std::string compressVideo(const std::string& input_path,
                             int max_bitrate = 2000);  // kbps

    std::string convertAudio(const std::string& input_path,
                            const std::string& output_format = "mp3");

    /**
     * @brief detectMediaType方法
     *
     * @param mime_type 参数说明
     * @return 返回值说明
     */
    MediaType detectMediaType(const std::string& mime_type);

    /**
     * @brief validateMedia方法
     *
     * @param mime_type 参数说明
     * @param size 参数说明
     * @return 返回值说明
     */
    bool validateMedia(const std::string& mime_type, size_t size);

    std::string getStoragePath(MediaType type, const std::string& user_id);

private:
    MediaService() = default;
    ~MediaService() = default;
    MediaService(const MediaService&) = delete;
    MediaService& operator=(const MediaService&) = delete;

    std::string saveFile(const std::string& data, 
                        const std::string& file_name,
                        const std::string& user_id);

    std::string generateMediaId();
    
    const size_t MAX_IMAGE_SIZE = 10 * 1024 * 1024;    // 10MB
    const size_t MAX_VIDEO_SIZE = 100 * 1024 * 1024;   // 100MB
    const size_t MAX_AUDIO_SIZE = 20 * 1024 * 1024;    // 20MB
    
    const std::vector<std::string> ALLOWED_IMAGE_TYPES = {
        "image/jpeg", "image/png", "image/gif", "image/webp"
    };
    const std::vector<std::string> ALLOWED_VIDEO_TYPES = {
        "video/mp4", "video/quicktime", "video/x-msvideo", "video/x-matroska"
    };
    const std::vector<std::string> ALLOWED_AUDIO_TYPES = {
        "audio/mpeg", "audio/wav", "audio/ogg", "audio/mp4", "audio/aac"
    };
};

} // namespace services
} // namespace heartlake
