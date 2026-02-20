/**
 * @file DataExportService.cpp
 * @brief 数据导出服务实现
 */

#include "infrastructure/services/DataExportService.h"
#include <drogon/drogon.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <fstream>

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
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
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

        // 保存到文件
        std::string filePath = "./exports/" + taskId + ".json";
        std::ofstream outFile(filePath);
        outFile << jsonData;
        outFile.close();

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
