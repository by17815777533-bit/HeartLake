#include "interfaces/api/MediaController.h"

#include "utils/IdGenerator.h"
#include "utils/ResponseUtil.h"
#include "utils/Validator.h"

#include <drogon/MultiPart.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <regex>
#include <string_view>

using namespace heartlake::controllers;
using namespace heartlake::utils;

namespace {

constexpr size_t kMaxImageBytes = 5 * 1024 * 1024;
constexpr size_t kMaxAudioBytes = 20 * 1024 * 1024;
constexpr size_t kMaxVideoBytes = 50 * 1024 * 1024;

std::string trimAscii(const std::string &value) {
  size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start]))) {
    ++start;
  }

  size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return value.substr(start, end - start);
}

std::string toLowerAscii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });
  return value;
}

bool isSafePathComponent(const std::string &value) {
  static const std::regex pattern(R"(^[A-Za-z0-9._-]{1,128}$)");
  return std::regex_match(value, pattern);
}

std::string extractFileExtension(const std::string &filename) {
  const auto dotPos = filename.rfind('.');
  if (dotPos == std::string::npos || dotPos + 1 >= filename.size()) {
    return "";
  }
  return toLowerAscii(filename.substr(dotPos + 1));
}

std::string inferCategory(const std::string &extension) {
  static const std::array<std::string_view, 5> imageExts = {"jpg", "jpeg",
                                                             "png", "webp",
                                                             "gif"};
  static const std::array<std::string_view, 3> audioExts = {"mp3", "wav",
                                                             "aac"};
  static const std::array<std::string_view, 1> videoExts = {"mp4"};

  if (std::find(imageExts.begin(), imageExts.end(), extension) !=
      imageExts.end()) {
    return "images";
  }
  if (std::find(audioExts.begin(), audioExts.end(), extension) !=
      audioExts.end()) {
    return "audio";
  }
  if (std::find(videoExts.begin(), videoExts.end(), extension) !=
      videoExts.end()) {
    return "video";
  }
  return "";
}

size_t maxBytesForCategory(const std::string &category) {
  if (category == "images") {
    return kMaxImageBytes;
  }
  if (category == "audio") {
    return kMaxAudioBytes;
  }
  if (category == "video") {
    return kMaxVideoBytes;
  }
  return 0;
}

std::string buildMediaRelativePath(const std::string &category,
                                   const std::string &filename) {
  return "/api/media/" + category + "/" + filename;
}

std::string buildMediaUrl(const HttpRequestPtr &req, const std::string &category,
                          const std::string &filename) {
  const auto path = buildMediaRelativePath(category, filename);
  const auto forwardedHost = trimAscii(req->getHeader("X-Forwarded-Host"));
  const auto host =
      !forwardedHost.empty() ? forwardedHost : trimAscii(req->getHeader("Host"));
  if (host.empty()) {
    return path;
  }

  auto scheme = trimAscii(req->getHeader("X-Forwarded-Proto"));
  if (scheme.empty()) {
    scheme = trimAscii(req->getHeader("X-Scheme"));
  }
  if (scheme.empty()) {
    scheme = "http";
  }

  return scheme + "://" + host + path;
}

std::filesystem::path resolveMediaPath(const std::string &category,
                                       const std::string &filename) {
  return std::filesystem::path(drogon::app().getUploadPath()) / category /
         filename;
}

ValidationResult validateMediaLocation(const std::string &category,
                                       const std::string &filename) {
  if (!isSafePathComponent(category) || !isSafePathComponent(filename)) {
    return ValidationResult::invalid("媒体路径不安全");
  }
  auto categoryValidation = Validator::checkPathTraversal(category, "媒体目录");
  if (!categoryValidation) {
    return categoryValidation;
  }
  return Validator::checkPathTraversal(filename, "媒体文件");
}

} // namespace

void MediaController::upload(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }

    drogon::MultiPartParser parser;
    if (parser.parse(req) != 0) {
      callback(ResponseUtil::badRequest("上传请求格式不正确"));
      return;
    }

    const auto &files = parser.getFiles();
    if (files.empty()) {
      callback(ResponseUtil::badRequest("请选择要上传的文件"));
      return;
    }

    const auto &file = files.front();
    const auto originalName = trimAscii(file.getFileName());
    if (originalName.empty()) {
      callback(ResponseUtil::badRequest("文件名不能为空"));
      return;
    }

    const std::vector<std::string> allowedExtensions = {
        "jpg", "jpeg", "png", "webp", "gif", "mp3", "wav", "aac", "mp4"};
    const auto extensionValidation =
        Validator::fileExtension(originalName, allowedExtensions, "上传文件");
    if (!extensionValidation) {
      callback(ResponseUtil::badRequest(extensionValidation.errorMessage));
      return;
    }

    const auto extension = extractFileExtension(originalName);
    const auto category = inferCategory(extension);
    if (category.empty()) {
      callback(ResponseUtil::badRequest("暂不支持该文件类型"));
      return;
    }

    const auto maxBytes = maxBytesForCategory(category);
    const auto fileSize = file.fileLength();
    if (fileSize == 0) {
      callback(ResponseUtil::badRequest("上传文件不能为空"));
      return;
    }
    if (maxBytes == 0 || fileSize > maxBytes) {
      callback(ResponseUtil::badRequest(category == "images"
                                            ? "图片大小不能超过 5MB"
                                            : (category == "audio"
                                                   ? "音频大小不能超过 20MB"
                                                   : "视频大小不能超过 50MB")));
      return;
    }

    const auto uploadDir = std::filesystem::path(drogon::app().getUploadPath()) /
                           category;
    std::error_code ec;
    std::filesystem::create_directories(uploadDir, ec);
    if (ec) {
      LOG_ERROR << "Failed to create upload directory: " << uploadDir.string()
                << ", error: " << ec.message();
      callback(ResponseUtil::internalError("创建上传目录失败"));
      return;
    }

    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();
    const auto generatedFilename =
        std::to_string(timestamp) + "_" + IdGenerator::generateUUID() + "." +
        extension;
    const auto targetPath = uploadDir / generatedFilename;
    if (file.saveAs(targetPath.string()) != 0) {
      LOG_ERROR << "Failed to save uploaded file to " << targetPath.string();
      callback(ResponseUtil::internalError("保存上传文件失败"));
      return;
    }

    Json::Value data;
    data["filename"] = generatedFilename;
    data["original_name"] = originalName;
    data["category"] = category;
    data["media_type"] = category == "images"
                             ? "image"
                             : (category == "audio" ? "audio" : "video");
    data["size_bytes"] = static_cast<Json::UInt64>(fileSize);
    data["path"] = buildMediaRelativePath(category, generatedFilename);
    data["relative_url"] = data["path"];
    data["url"] = buildMediaUrl(req, category, generatedFilename);

    callback(ResponseUtil::success(data, "上传成功"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Media upload error: " << e.what();
    callback(ResponseUtil::internalError("上传失败"));
  }
}

void MediaController::serve(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &category, const std::string &filename) {
  try {
    static const std::array<std::string_view, 3> allowedCategories = {
        "images", "audio", "video"};
    if (std::find(allowedCategories.begin(), allowedCategories.end(),
                  category) == allowedCategories.end()) {
      callback(ResponseUtil::notFound("文件不存在"));
      return;
    }

    const auto locationValidation = validateMediaLocation(category, filename);
    if (!locationValidation) {
      callback(ResponseUtil::badRequest(locationValidation.errorMessage));
      return;
    }

    const auto path = resolveMediaPath(category, filename);
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) ||
        !std::filesystem::is_regular_file(path, ec)) {
      callback(ResponseUtil::notFound("文件不存在"));
      return;
    }

    auto response = HttpResponse::newFileResponse(path.string(), "", CT_NONE,
                                                  "", req);
    response->addHeader("Cache-Control", "public, max-age=86400");
    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << "Serve media error: " << e.what();
    callback(ResponseUtil::internalError("读取文件失败"));
  }
}
