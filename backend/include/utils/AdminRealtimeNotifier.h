#pragma once

#include <json/json.h>
#include <string>

namespace heartlake::utils {

struct AdminRealtimeStatsSnapshot {
    int totalUsers{0};
    int totalStones{0};
    int todayStones{0};
    int onlineUsers{0};
    int pendingReports{0};

    Json::Value toJson() const;
};

AdminRealtimeStatsSnapshot queryAdminRealtimeStatsSnapshot();

void broadcastAdminRealtimeStatsUpdate(
    std::string reason,
    Json::Value extra = Json::Value(Json::objectValue));

} // namespace heartlake::utils
