/**
 * @file ImageModerationEngine.cpp
 * @brief ImageModerationEngine 模块实现 (AWS Rekognition REST API)
 * Created by 王璐瑶
 */

#include "infrastructure/ai/ImageModerationEngine.h"
#include "utils/AwsSigner.h"
#include "utils/ContentFilter.h"
#include <drogon/drogon.h>
#include <drogon/HttpClient.h>
#include <json/json.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <atomic>
#include <mutex>

using namespace drogon;

namespace heartlake {
namespace ai {

ImageModerationEngine& ImageModerationEngine::getInstance() {
    static ImageModerationEngine instance;
    return instance;
}

void ImageModerationEngine::initialize() {
    if (initialized_) return;

    LOG_INFO << "Initializing ImageModerationEngine";

    // 从配置加载 AWS 凭证
    auto& config = app().getCustomConfig();
    awsRegion_ = config.get("aws_region", "us-east-1").asString();
    awsAccessKeyId_ = config.get("aws_access_key_id", "").asString();
    awsSecretAccessKey_ = config.get("aws_secret_access_key", "").asString();

    if (awsAccessKeyId_.empty() || awsSecretAccessKey_.empty()) {
        LOG_WARN << "AWS credentials not configured, image moderation will use fallback";
    }

    loadBlacklist();

    // 加载已知哈希
    try {
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "SELECT DISTINCT image_hash FROM media_files WHERE image_hash IS NOT NULL"
        );
        for (const auto& row : result) {
            std::string hash = row["image_hash"].as<std::string>();
            if (!hash.empty()) knownHashes_.insert(hash);
        }
        LOG_INFO << "Loaded " << knownHashes_.size() << " known image hashes";
    } catch (const std::exception& e) {
        LOG_WARN << "Failed to load known hashes: " << e.what();
    }

    initialized_ = true;
    LOG_INFO << "ImageModerationEngine initialized";
}

std::string ImageModerationEngine::callRekognitionApi(const std::string& action, const std::string& payload) {
    if (awsAccessKeyId_.empty()) return "{}";

    std::string host = "rekognition." + awsRegion_ + ".amazonaws.com";
    auto signed_req = utils::AwsSigner::sign("POST", host, "/", awsRegion_, "rekognition",
                                              payload, awsAccessKeyId_, awsSecretAccessKey_);

    // 同步 HTTP 调用
    auto client = HttpClient::newHttpClient("https://" + host);
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/");
    req->setBody(payload);
    req->addHeader("Content-Type", "application/x-amz-json-1.1");
    req->addHeader("X-Amz-Target", "RekognitionService." + action);
    req->addHeader("X-Amz-Date", signed_req.headers["X-Amz-Date"]);
    req->addHeader("Authorization", signed_req.authorizationHeader);

    std::string responseBody;
    std::promise<void> promise;
    auto future = promise.get_future();

    client->sendRequest(req, [&responseBody, &promise](ReqResult result, const HttpResponsePtr& resp) {
        if (result == ReqResult::Ok && resp) {
            responseBody = std::string(resp->getBody());
        }
        promise.set_value();
    }, 30.0);

    future.wait();
    return responseBody;
}

std::pair<float, float> ImageModerationEngine::detectModerationLabels(const std::vector<unsigned char>& imageBytes) {
    if (awsAccessKeyId_.empty()) return {0.1f, 0.1f};

    std::string base64 = drogon::utils::base64Encode(imageBytes.data(), imageBytes.size());
    Json::Value payload;
    payload["Image"]["Bytes"] = base64;

    std::string response = callRekognitionApi("DetectModerationLabels", Json::FastWriter().write(payload));

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) return {0.1f, 0.1f};

    float nsfwScore = 0.0f, violenceScore = 0.0f;
    for (const auto& label : root["ModerationLabels"]) {
        std::string name = label["Name"].asString();
        float confidence = label["Confidence"].asFloat() / 100.0f;
        if (name.find("Explicit") != std::string::npos ||
            name.find("Nudity") != std::string::npos ||
            name.find("Sexual") != std::string::npos) {
            nsfwScore = std::max(nsfwScore, confidence);
        }
        if (name.find("Violence") != std::string::npos ||
            name.find("Graphic") != std::string::npos ||
            name.find("Gore") != std::string::npos) {
            violenceScore = std::max(violenceScore, confidence);
        }
    }
    return {nsfwScore, violenceScore};
}

float ImageModerationEngine::detectNSFW(const std::vector<unsigned char>& imageBytes) {
    return detectModerationLabels(imageBytes).first;
}

float ImageModerationEngine::detectViolence(const std::vector<unsigned char>& imageBytes) {
    return detectModerationLabels(imageBytes).second;
}

bool ImageModerationEngine::detectFaces(const std::vector<unsigned char>& imageBytes, int& faceCount) {
    faceCount = 0;
    if (awsAccessKeyId_.empty()) return false;

    std::string base64 = drogon::utils::base64Encode(imageBytes.data(), imageBytes.size());
    Json::Value payload;
    payload["Image"]["Bytes"] = base64;

    std::string response = callRekognitionApi("DetectFaces", Json::FastWriter().write(payload));

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) return false;

    faceCount = static_cast<int>(root["FaceDetails"].size());
    return faceCount > 0;
}

std::vector<std::string> ImageModerationEngine::extractText(const std::vector<unsigned char>& imageBytes) {
    if (awsAccessKeyId_.empty()) return {};

    std::string base64 = drogon::utils::base64Encode(imageBytes.data(), imageBytes.size());
    Json::Value payload;
    payload["Image"]["Bytes"] = base64;

    std::string response = callRekognitionApi("DetectText", Json::FastWriter().write(payload));

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) return {};

    std::vector<std::string> texts;
    for (const auto& detection : root["TextDetections"]) {
        if (detection["Type"].asString() == "LINE") {
            texts.push_back(detection["DetectedText"].asString());
        }
    }
    return texts;
}

ImageModerationResult ImageModerationEngine::analyzeImageContent(const std::string& imagePath) {
    ImageModerationResult result;
    result.passed = true;
    result.confidence = 0.9f;

    std::string imageHash = computePerceptualHash(imagePath);
    result.details.imageHash = imageHash;

    // 黑名单检查
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = blacklist_.find(imageHash);
        if (it != blacklist_.end()) {
            result.passed = false;
            result.confidence = 1.0f;
            result.categories = {"blacklisted"};
            result.reason = "Image in blacklist: " + it->second;
            return result;
        }
    }

    result.details.isDuplicate = isDuplicateImage(imageHash);

    // 读取图片字节
    std::ifstream file(imagePath, std::ios::binary);
    if (!file) {
        result.passed = false;
        result.categories = {"file_error"};
        result.reason = "Cannot read image file";
        return result;
    }

    std::vector<unsigned char> imageBytes((std::istreambuf_iterator<char>(file)),
                                           std::istreambuf_iterator<char>());

    // NSFW + 暴力检测 (单次 API 调用)
    auto [nsfwScore, violenceScore] = detectModerationLabels(imageBytes);
    result.details.nsfwScore = nsfwScore;
    result.details.violenceScore = violenceScore;
    if (nsfwScore > 0.7f) {
        result.passed = false;
        result.categories.push_back("nsfw");
        result.reason = "NSFW content detected";
        result.confidence = nsfwScore;
    }
    if (violenceScore > 0.7f) {
        result.passed = false;
        result.categories.push_back("violence");
        result.reason += (result.reason.empty() ? "" : "; ") + std::string("Violence detected");
        result.confidence = std::max(result.confidence, violenceScore);
    }

    // 人脸检测
    int faceCount = 0;
    bool hasFaces = detectFaces(imageBytes, faceCount);
    result.details.hasFaces = hasFaces;
    result.details.faceCount = faceCount;

    // OCR + 敏感词
    auto detectedText = extractText(imageBytes);
    result.details.hasText = !detectedText.empty();
    result.details.detectedText = detectedText;

    if (containsSensitiveText(detectedText)) {
        result.passed = false;
        result.categories.push_back("sensitive_text");
        result.reason += (result.reason.empty() ? "" : "; ") + std::string("Sensitive text detected");
        result.confidence = 0.95f;
    }

    if (result.passed) {
        result.categories = {"safe"};
        result.reason = "Image passed all moderation checks";
    }

    return result;
}

void ImageModerationEngine::moderateImageUrl(
    const std::string& imageUrl,
    std::function<void(const ImageModerationResult& result, const std::string& error)> callback
) {
    std::string tempPath = "/tmp/heartlake_img_" + std::to_string(std::time(nullptr)) + ".jpg";

    downloadImage(imageUrl, tempPath, [this, tempPath, callback](bool success, const std::string& error) {
        if (!success) {
            ImageModerationResult result;
            result.passed = false;
            result.categories = {"download_failed"};
            result.reason = "Failed to download image: " + error;
            callback(result, error);
            return;
        }

        try {
            auto result = analyzeImageContent(tempPath);
            callback(result, "");
            std::remove(tempPath.c_str());
        } catch (const std::exception& e) {
            ImageModerationResult result;
            result.passed = false;
            result.categories = {"analysis_failed"};
            result.reason = std::string("Analysis failed: ") + e.what();
            callback(result, e.what());
            std::remove(tempPath.c_str());
        }
    });
}

ImageModerationResult ImageModerationEngine::moderateImageFile(const std::string& imagePath) {
    return analyzeImageContent(imagePath);
}

void ImageModerationEngine::moderateImageBatch(
    const std::vector<std::string>& imageUrls,
    std::function<void(const std::vector<ImageModerationResult>& results, const std::string& error)> callback
) {
    auto resultsPtr = std::make_shared<std::vector<ImageModerationResult>>();
    resultsPtr->reserve(imageUrls.size());
    auto remaining = std::make_shared<std::atomic<size_t>>(imageUrls.size());
    auto mtx = std::make_shared<std::mutex>();

    if (imageUrls.empty()) {
        callback(*resultsPtr, "");
        return;
    }

    for (size_t i = 0; i < imageUrls.size(); ++i) {
        std::string tempPath = "/tmp/heartlake_batch_" + std::to_string(i) + "_" + std::to_string(std::time(nullptr)) + ".jpg";

        downloadImage(imageUrls[i], tempPath, [resultsPtr, remaining, mtx, callback, tempPath](bool success, const std::string& error) {
            ImageModerationResult result;
            if (success) {
                try {
                    result = ImageModerationEngine::getInstance().analyzeImageContent(tempPath);
                    std::remove(tempPath.c_str());
                } catch (...) {
                    result.passed = false;
                    result.categories = {"analysis_failed"};
                }
            } else {
                result.passed = false;
                result.categories = {"download_failed"};
                result.reason = error;
            }

            {
                std::lock_guard<std::mutex> lock(*mtx);
                resultsPtr->push_back(result);
            }

            if (--(*remaining) == 0) {
                callback(*resultsPtr, "");
            }
        });
    }
}

std::string ImageModerationEngine::computePerceptualHash(const std::string& imagePath) {
    std::ifstream file(imagePath, std::ios::binary);
    if (!file) return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    size_t hash = 0;
    for (char c : content) {
        hash = hash * 31 + static_cast<unsigned char>(c);
    }

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

bool ImageModerationEngine::isDuplicateImage(const std::string& imageHash) {
    std::lock_guard<std::mutex> lock(mutex_);
    return knownHashes_.find(imageHash) != knownHashes_.end();
}

void ImageModerationEngine::addToBlacklist(const std::string& imageHash, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    blacklist_[imageHash] = reason;

    try {
        auto dbClient = app().getDbClient("default");
        dbClient->execSqlAsync(
            "INSERT INTO image_blacklist (image_hash, reason, created_at) "
            "VALUES ($1, $2, NOW()) ON CONFLICT (image_hash) DO UPDATE SET reason = $2",
            [](const drogon::orm::Result&) {},
            [](const drogon::orm::DrogonDbException& e) {
                LOG_ERROR << "Failed to add to blacklist: " << e.base().what();
            },
            imageHash, reason
        );
    } catch (...) {}
}

bool ImageModerationEngine::containsSensitiveText(const std::vector<std::string>& texts) {
    auto& filter = heartlake::ContentFilter::getInstance();
    for (const auto& text : texts) {
        std::string result = filter.checkSafety(text);
        if (result == "high_risk" || result == "medium_risk") return true;
    }
    return false;
}

void ImageModerationEngine::downloadImage(
    const std::string& url,
    const std::string& savePath,
    std::function<void(bool success, const std::string& error)> callback
) {
    auto client = HttpClient::newHttpClient(url);
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);

    client->sendRequest(req, [savePath, callback](ReqResult result, const HttpResponsePtr& resp) {
        if (result != ReqResult::Ok || !resp || resp->getStatusCode() != k200OK) {
            callback(false, "Download failed");
            return;
        }

        std::ofstream file(savePath, std::ios::binary);
        if (!file) {
            callback(false, "Cannot create file");
            return;
        }

        file.write(resp->getBody().data(), resp->getBody().size());
        callback(true, "");
    }, 10.0);
}

void ImageModerationEngine::loadBlacklist() {
    try {
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "SELECT image_hash, reason FROM image_blacklist WHERE is_active = true"
        );

        std::lock_guard<std::mutex> lock(mutex_);
        blacklist_.clear();
        for (const auto& row : result) {
            blacklist_[row["image_hash"].as<std::string>()] = row["reason"].as<std::string>();
        }
        LOG_INFO << "Loaded " << blacklist_.size() << " blacklisted images";
    } catch (const std::exception& e) {
        LOG_WARN << "Failed to load blacklist: " << e.what();
    }
}

} // namespace ai
} // namespace heartlake
