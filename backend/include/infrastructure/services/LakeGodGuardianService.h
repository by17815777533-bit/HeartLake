/**
 * @brief 湖神守护服务 -- 自动为零互动石头派送暖心纸船
 *
 * @details
 * 后台守护线程，每 15 分钟扫描一次数据库，找出投出超过 2 小时
 * 仍无任何涟漪或纸船回应的石头，以"湖神"身份自动生成一条
 * 暖心纸船回复，确保每颗石头都不会被遗忘。
 *
 * 同时负责清理长时间处于 pending 状态的纸船（可能是发送失败的残留），
 * 避免占用数据库空间和干扰统计。
 *
 * 纸船内容由 AI 摘要服务根据石头原文和情绪类型生成，
 * 语气温暖、贴合情境，不会暴露是系统自动发送。
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>

namespace heartlake::infrastructure {

class LakeGodGuardianService {
public:
    static LakeGodGuardianService& getInstance();

    /// 启动后台扫描线程
    void start();
    /// 停止扫描线程并等待退出
    void stop();
    /// 当前是否在运行
    bool isRunning() const { return running_; }

    /// 手动触发一次扫描（用于测试和调试）
    void scanOnce();

private:
    LakeGodGuardianService() = default;
    ~LakeGodGuardianService();
    LakeGodGuardianService(const LakeGodGuardianService&) = delete;
    LakeGodGuardianService& operator=(const LakeGodGuardianService&) = delete;

    /// 扫描主循环，每 SCAN_INTERVAL_MINUTES 分钟执行一轮
    void scanLoop();
    /// 清理长时间 pending 的纸船残留
    void cleanStalePendingBoats();
    /// 查找零互动石头并逐个派送纸船
    void processZeroInteractionStones();
    /// 为指定石头生成并发送湖神纸船
    void sendAutoBoat(const std::string& stoneId, const std::string& stoneContent, const std::string& mood, const std::string& boatId);

    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> scanThread_;
    std::mutex mutex_;

    static constexpr int SCAN_INTERVAL_MINUTES = 15;            ///< 扫描间隔（分钟）
    static constexpr int ZERO_INTERACTION_THRESHOLD_HOURS = 2;   ///< 零互动判定阈值（小时）
};

} // namespace heartlake::infrastructure
