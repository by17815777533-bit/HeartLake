#pragma once

#include <json/value.h>
#include <string>

namespace heartlake::utils {

Json::Value buildRealtimeEvent(
    const std::string &type,
    Json::Value payload = Json::Value(Json::objectValue));

void normalizeRealtimeEvent(Json::Value &payload);

} // namespace heartlake::utils
