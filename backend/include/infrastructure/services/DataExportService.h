/**
 * @brief 用户数据导出服务 -- 处理数据备份和 GDPR 合规导出
 *
 * @details
 * 支持用户发起数据导出请求，异步打包其所有个人数据（石头、涟漪、
 * 纸船、好友关系、情绪记录等），生成可下载的归档文件。
 *
 * 导出流程：
 * 1. 用户发起导出请求，创建异步任务
 * 2. processExportTask 在后台线程中收集并序列化用户数据
 * 3. 计算文件 SHA-256 校验和，确保传输完整性
 * 4. 更新任务状态为完成，附带下载链接和校验和
 *
 * 支持导出后的完整性验证：下载方可用 verifyBackupIntegrity
 * 比对文件校验和，确认数据未被篡改。
 */

#pragma once

#include <string>
#include <functional>

namespace heartlake {
namespace infrastructure {

class DataExportService {
public:
    static DataExportService& getInstance();

    /**
     * @brief 处理数据导出任务（异步执行）
     * @param taskId 导出任务 ID
     * @param userId 目标用户 ID
     */
    void processExportTask(const std::string& taskId, const std::string& userId);

    /**
     * @brief 验证备份文件完整性
     * @param filePath 文件路径
     * @param expectedChecksum 期望的 SHA-256 校验和
     * @return 校验和是否匹配
     */
    bool verifyBackupIntegrity(const std::string& filePath, const std::string& expectedChecksum);

    /// 计算数据的 SHA-256 校验和
    std::string calculateChecksum(const std::string& data);

private:
    DataExportService() = default;

    /// 收集并序列化指定用户的全部数据
    std::string exportUserData(const std::string& userId);

    /// 更新导出任务的状态（processing / completed / failed）
    void updateTaskStatus(const std::string& taskId, const std::string& status,
                         const std::string& downloadUrl = "", const std::string& checksum = "");
};

} // namespace infrastructure
} // namespace heartlake
