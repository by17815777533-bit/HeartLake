/**
 * @brief 用户回访与持续关怀系统
 *
 * @details
 * 后台守护线程，每 6 小时扫描一次，对曾触发过危机干预的用户
 * 进行分阶段回访关怀。回访时间节点：
 *
 * - 干预后第 1 天：发送"你还好吗"类型的轻度关怀
 * - 干预后第 3 天：发送持续关注的暖心消息
 * - 干预后第 7 天：发送长期陪伴的鼓励消息
 *
 * 同时负责检测长期不活跃用户（超过 7 天未登录），
 * 发送召回消息尝试唤醒沉默用户。
 *
 * 回访消息通过 WarmQuoteService 获取场景化语录，
 * 再通过 NotificationPushService 推送到客户端。
 */

#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <mutex>

namespace heartlake::infrastructure {

class UserFollowUpService {
public:
    static UserFollowUpService& getInstance();

    /// 启动后台回访扫描线程
    void start();
    /// 停止扫描线程
    void stop();
    /// 手动触发一次扫描（用于测试）
    void scanOnce();

private:
    UserFollowUpService() = default;
    ~UserFollowUpService();

    /// 扫描主循环
    void scanLoop();

    /**
     * @brief 对曾触发干预的用户进行回访
     * @param userId 用户 ID
     * @param daysSinceIntervention 距上次干预的天数
     */
    void followUpBurdenedUser(const std::string& userId, int daysSinceIntervention);

    /// 检测并召回长期不活跃用户
    void checkInactiveUsers();

    /**
     * @brief 发送回访消息
     * @param userId 目标用户
     * @param messageType 消息类型（对应 WarmScene 枚举）
     */
    void sendFollowUpMessage(const std::string& userId, const std::string& messageType);

    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> scanThread_;
    std::mutex mutex_;

    static constexpr int SCAN_INTERVAL_HOURS = 6;  ///< 扫描间隔（小时）
    static constexpr int FOLLOWUP_DAY_1 = 1;       ///< 第一次回访时间点
    static constexpr int FOLLOWUP_DAY_3 = 3;       ///< 第二次回访时间点
    static constexpr int FOLLOWUP_DAY_7 = 7;       ///< 第三次回访时间点
};

} // namespace heartlake::infrastructure
