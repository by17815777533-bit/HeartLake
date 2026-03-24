#include "utils/RealtimeEvent.h"
#include <ctime>
#include <initializer_list>

namespace heartlake::utils {

namespace {

bool hasMeaningfulValue(const Json::Value &value) {
    if (value.isNull()) {
        return false;
    }
    if (value.isString()) {
        return !value.asString().empty();
    }
    return true;
}

void mirrorAlias(
    Json::Value &payload,
    const std::string &canonicalKey,
    std::initializer_list<const char *> aliasKeys) {
    Json::Value resolved;
    if (hasMeaningfulValue(payload[canonicalKey])) {
        resolved = payload[canonicalKey];
    } else {
        for (const auto *aliasKey : aliasKeys) {
            if (hasMeaningfulValue(payload[aliasKey])) {
                resolved = payload[aliasKey];
                break;
            }
        }
    }

    if (!hasMeaningfulValue(resolved)) {
        return;
    }

    payload[canonicalKey] = resolved;
    for (const auto *aliasKey : aliasKeys) {
        payload[aliasKey] = resolved;
    }
}

void flattenStone(Json::Value &payload) {
    if (!payload["stone"].isObject()) {
        return;
    }

    const auto &stone = payload["stone"];
    if (hasMeaningfulValue(stone["stone_id"])) {
        payload["stone_id"] = stone["stone_id"];
    }
    if (hasMeaningfulValue(stone["user_id"])) {
        payload["stone_user_id"] = stone["user_id"];
    }
    if (hasMeaningfulValue(stone["content"])) {
        payload["stone_content"] = stone["content"];
    }
    if (hasMeaningfulValue(stone["created_at"])) {
        payload["stone_created_at"] = stone["created_at"];
    }
    if (hasMeaningfulValue(stone["mood_type"])) {
        payload["stone_mood_type"] = stone["mood_type"];
    }
}

} // namespace

void normalizeRealtimeEvent(Json::Value &payload) {
    if (!payload.isObject()) {
        payload = Json::Value(Json::objectValue);
    }

    flattenStone(payload);

    mirrorAlias(payload, "user_id", {"userId"});
    mirrorAlias(payload, "sender_id", {"senderId"});
    mirrorAlias(payload, "receiver_id", {"receiverId"});
    mirrorAlias(payload, "from_user_id", {"fromUserId"});
    mirrorAlias(payload, "to_user_id", {"toUserId"});
    mirrorAlias(payload, "friend_id", {"friendId", "friend_user_id", "friendUserId", "peer_id", "peerId"});
    mirrorAlias(payload, "peer_id", {"peerId", "friend_id", "friendId", "friend_user_id", "friendUserId"});
    mirrorAlias(payload, "friendship_id", {"friendshipId"});
    mirrorAlias(payload, "stone_id", {"stoneId"});
    mirrorAlias(payload, "stone_user_id", {"stoneUserId"});
    mirrorAlias(payload, "stone_content", {"stoneContent"});
    mirrorAlias(payload, "stone_created_at", {"stoneCreatedAt"});
    mirrorAlias(payload, "stone_mood_type", {"stoneMoodType"});
    mirrorAlias(payload, "boat_id", {"boatId"});
    mirrorAlias(payload, "ripple_id", {"rippleId"});
    mirrorAlias(payload, "notification_id", {"id", "notificationId"});
    mirrorAlias(payload, "created_at", {"createdAt"});
    mirrorAlias(payload, "boat_count", {"boatCount"});
    mirrorAlias(payload, "ripple_count", {"rippleCount"});
    mirrorAlias(payload, "total_users", {"totalUsers"});
    mirrorAlias(payload, "total_stones", {"totalStones"});
    mirrorAlias(payload, "today_stones", {"todayStones"});
    mirrorAlias(payload, "online_users", {"online_count", "onlineCount"});
    mirrorAlias(payload, "pending_reports", {"pendingReports"});
}

Json::Value buildRealtimeEvent(const std::string &type, Json::Value payload) {
    if (!payload.isObject()) {
        payload = Json::Value(Json::objectValue);
    }

    payload["type"] = type;
    if (!hasMeaningfulValue(payload["event"])) {
        payload["event"] = type;
    }
    if (!hasMeaningfulValue(payload["timestamp"])) {
        payload["timestamp"] = static_cast<Json::Int64>(time(nullptr));
    }

    normalizeRealtimeEvent(payload);
    return payload;
}

} // namespace heartlake::utils
