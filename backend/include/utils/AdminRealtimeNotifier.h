#pragma once

#include <json/json.h>
#include <string>

namespace heartlake::utils {

void broadcastAdminRealtimeStatsUpdate(
    std::string reason,
    Json::Value extra = Json::Value(Json::objectValue));

} // namespace heartlake::utils
