/**
 * @file ImageModerationEngine.h
 * @brief ImageModerationEngine 模块接口定义
 * Created by 王璐瑶
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <mutex>

namespace heartlake {
namespace ai {

/**
 * @brief 图片审核结果
 */
struct ImageModerationResult {
    bool passed;                              // 是否通过审核
    float confidence;                         // 置信度 [0, 1]
    std::vector<std::string> categories;      // 违规类别
    std::string reason;                       // 原因说明

    // 详细分析
    struct DetailedAnalysis {
        float nsfwScore;                      // NSFW分数
        float violenceScore;                  // 暴力分数
        float politicalScore;                 // 政治敏感分数
        bool hasFaces;                        // 是否包含人脸
        int faceCount;                        // 人脸数量
        bool hasText;                         // 是否包含文字
        std::vector<std::string> detectedText; // 检测到的文字
        std::string imageHash;                // 图片哈希值
        bool isDuplicate;                     // 是否重复图片
    } details;
};

/**
 * @brief 高性能图片审核引擎 (使用 AWS Rekognition REST API)
 */
class ImageModerationEngine {
public:
    static ImageModerationEngine& getInstance();

    void initialize();

    void moderateImageUrl(
        const std::string& imageUrl,
        std::function<void(const ImageModerationResult& result, const std::string& error)> callback
    );

    ImageModerationResult moderateImageFile(const std::string& imagePath);

    void moderateImageBatch(
        const std::vector<std::string>& imageUrls,
        std::function<void(const std::vector<ImageModerationResult>& results, const std::string& error)> callback
    );

    std::string computePerceptualHash(const std::string& imagePath);
    bool isDuplicateImage(const std::string& imageHash);
    void addToBlacklist(const std::string& imageHash, const std::string& reason);

private:
    ImageModerationEngine() = default;
    ~ImageModerationEngine() = default;
    ImageModerationEngine(const ImageModerationEngine&) = delete;
    ImageModerationEngine& operator=(const ImageModerationEngine&) = delete;

    bool initialized_ = false;
    std::mutex mutex_;

    std::unordered_map<std::string, std::string> blacklist_;
    std::unordered_set<std::string> knownHashes_;

    // AWS 配置
    std::string awsRegion_;
    std::string awsAccessKeyId_;
    std::string awsSecretAccessKey_;

    ImageModerationResult analyzeImageContent(const std::string& imagePath);
    float detectNSFW(const std::vector<unsigned char>& imageBytes);
    float detectViolence(const std::vector<unsigned char>& imageBytes);
    std::pair<float, float> detectModerationLabels(const std::vector<unsigned char>& imageBytes);
    bool detectFaces(const std::vector<unsigned char>& imageBytes, int& faceCount);
    std::vector<std::string> extractText(const std::vector<unsigned char>& imageBytes);
    bool containsSensitiveText(const std::vector<std::string>& texts);
    void downloadImage(const std::string& url, const std::string& savePath,
                      std::function<void(bool success, const std::string& error)> callback);
    void loadBlacklist();

    // Rekognition HTTP API 调用
    std::string callRekognitionApi(const std::string& action, const std::string& payload);
};

} // namespace ai
} // namespace heartlake
