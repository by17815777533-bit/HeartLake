/**
 * 数据导出服务实现
 *
 * 支持用户个人数据的异步导出（GDPR 合规），流程：
 * 1. 创建导出任务（状态 pending → processing → completed/failed）
 * 2. 从 users / stones 表提取用户数据，序列化为 JSON
 * 3. 计算 SHA-256 校验和，写入本地文件
 * 4. 更新任务状态并生成下载链接（7 天有效期）
 *
 * taskId 经过白名单校验（仅字母数字和连字符），防止路径穿越攻击。
 */

#include "infrastructure/services/DataExportService.h"
#include <drogon/drogon.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <stdexcept>

namespace heartlake {
namespace infrastructure {

DataExportService& DataExportService::getInstance() {
    static DataExportService instance;
    return instance;
}

std::string DataExportService::calculateChecksum(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool DataExportService::verifyBackupIntegrity(const std::string& filePath, const std::string& expectedChecksum) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    return calculateChecksum(buffer.str()) == expectedChecksum;
}

std::string DataExportService::exportUserData(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    Json::Value exportData;

    // 导出用户基本信息
    auto userResult = db->execSqlSync(
        "SELECT user_id, username, nickname, bio, gender, birthday, created_at "
        "FROM users WHERE user_id = $1", userId);
    if (!userResult.empty()) {
        const auto& row = userResult[0];
        exportData["user"]["user_id"] = row["user_id"].as<std::string>();
        exportData["user"]["username"] = row["username"].as<std::string>();
        exportData["user"]["nickname"] = row["nickname"].isNull() ? "" : row["nickname"].as<std::string>();
        exportData["user"]["created_at"] = row["created_at"].as<std::string>();
    }

    // 导出石头数据
    auto stonesResult = db->execSqlSync(
        "SELECT stone_id, content, mood_type, visibility, created_at FROM stones WHERE user_id = $1", userId);
    Json::Value stones(Json::arrayValue);
    for (const auto& row : stonesResult) {
        Json::Value stone;
        stone["stone_id"] = row["stone_id"].as<std::string>();
        stone["content"] = row["content"].as<std::string>();
        stone["mood"] = row["mood_type"].isNull() ? "" : row["mood_type"].as<std::string>();
        stone["created_at"] = row["created_at"].as<std::string>();
        stones.append(stone);
    }
    exportData["stones"] = stones;

    Json::StreamWriterBuilder writer;
    return Json::writeString(writer, exportData);
}

void DataExportService::updateTaskStatus(const std::string& taskId, const std::string& status,
                                         const std::string& downloadUrl, const std::string& checksum) {
    auto db = drogon::app().getDbClient("default");
    if (status == "completed") {
        db->execSqlSync(
            "UPDATE data_export_tasks SET status = $1, download_url = $2, checksum = $3, "
            "completed_at = NOW(), expires_at = NOW() + INTERVAL '7 days' WHERE task_id = $4",
            status, downloadUrl, checksum, taskId);
    } else {
        db->execSqlSync("UPDATE data_export_tasks SET status = $1 WHERE task_id = $2", status, taskId);
    }
}

void DataExportService::processExportTask(const std::string& taskId, const std::string& userId) {
    try {
        updateTaskStatus(taskId, "processing");

        std::string jsonData = exportUserData(userId);
        std::string checksum = calculateChecksum(jsonData);

        // taskId 白名单校验：只允许字母数字和连字符，防止路径穿越
        auto isValidTaskId = [](const std::string& id) {
            return !id.empty() && std::all_of(id.begin(), id.end(), [](char c) {
                return std::isalnum(static_cast<unsigned char>(c)) || c == '-';
            });
        };
        if (!isValidTaskId(taskId)) {
            throw std::invalid_argument("非法的 taskId，仅允许字母数字和连字符");
        }

        // 保存到文件
        std::string filePath = "./exports/" + taskId + ".json";
        std::ofstream outFile(filePath);
        outFile << jsonData;
        outFile.close();

        if (!outFile.good()) {
            throw std::runtime_error("导出文件写入失败: " + filePath);
        }

        std::string downloadUrl = "/api/account/export/download/" + taskId;
        updateTaskStatus(taskId, "completed", downloadUrl, checksum);

        LOG_INFO << "Export task completed: " << taskId;
    } catch (const std::exception& e) {
        LOG_ERROR << "Export task failed: " << taskId << " - " << e.what();
        updateTaskStatus(taskId, "failed");
    }
}

} // namespace infrastructure
} // namespace heartlake
