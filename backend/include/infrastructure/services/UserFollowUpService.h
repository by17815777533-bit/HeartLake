/**
 * @file UserFollowUpService.h
 * @brief 用户回访与持续关怀系统
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

    void start();
    void stop();
    void scanOnce();

private:
    UserFollowUpService() = default;
    ~UserFollowUpService();

    void scanLoop();
    void followUpBurdenedUser(const std::string& userId, int daysSinceIntervention);
    void checkInactiveUsers();
    void sendFollowUpMessage(const std::string& userId, const std::string& messageType);

    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> scanThread_;
    std::mutex mutex_;

    static constexpr int SCAN_INTERVAL_HOURS = 6;
    static constexpr int FOLLOWUP_DAY_1 = 1;
    static constexpr int FOLLOWUP_DAY_3 = 3;
    static constexpr int FOLLOWUP_DAY_7 = 7;
};

} // namespace heartlake::infrastructure
