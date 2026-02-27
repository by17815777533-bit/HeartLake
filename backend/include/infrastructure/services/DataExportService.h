/**
 * 数据导出服务 - 处理用户数据备份和导出
 */

#pragma once

#include <string>
#include <functional>

namespace heartlake {
namespace infrastructure {

class DataExportService {
public:
    static DataExportService& getInstance();

    void processExportTask(const std::string& taskId, const std::string& userId);
    bool verifyBackupIntegrity(const std::string& filePath, const std::string& expectedChecksum);
    std::string calculateChecksum(const std::string& data);

private:
    DataExportService() = default;
    std::string exportUserData(const std::string& userId);
    void updateTaskStatus(const std::string& taskId, const std::string& status,
                         const std::string& downloadUrl = "", const std::string& checksum = "");
};

} // namespace infrastructure
} // namespace heartlake
