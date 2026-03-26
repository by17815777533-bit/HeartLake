#include "utils/AdminRealtimeNotifier.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/RealtimeEvent.h"
#include "utils/RequestHelper.h"
#include <drogon/drogon.h>
#include <ctime>
#include <stdexcept>

namespace heartlake::utils {

namespace {

int requireSnapshotInt(const drogon::orm::Row& row, const char* column) {
    if (row[column].isNull()) {
        throw std::runtime_error(std::string("admin realtime snapshot missing column: ") + column);
    }
    return row[column].as<int>();
}

Json::Value buildSocketErrorPayload(const std::string& reason, const std::string& message) {
    Json::Value payload(Json::objectValue);
    payload["reason"] = reason;
    payload["message"] = message;
    payload["source"] = "admin_realtime_stats";
    return payload;
}

} // namespace

Json::Value AdminRealtimeStatsSnapshot::toJson() const {
    Json::Value data(Json::objectValue);
    data["total_users"] = totalUsers;
    data["total_stones"] = totalStones;
    data["today_stones"] = todayStones;
    data["online_users"] = onlineUsers;
    data["pending_reports"] = pendingReports;
    return data;
}

AdminRealtimeStatsSnapshot queryAdminRealtimeStatsSnapshot() {
    auto dbClient = drogon::app().getDbClient("default");
    auto result = dbClient->execSqlSync(
        "WITH user_stats AS ("
        "  SELECT "
        "    COUNT(*)::INTEGER AS total_users, "
        "    COUNT(*) FILTER ("
        "      WHERE last_active_at > NOW() - INTERVAL '5 minutes' "
        "        AND status = 'active'"
        "    )::INTEGER AS active_online_users "
        "  FROM users"
        "), stone_stats AS ("
        "  SELECT "
        "    COUNT(*) FILTER ("
        "      WHERE status = 'published' AND deleted_at IS NULL"
        "    )::INTEGER AS total_stones, "
        "    COUNT(*) FILTER ("
        "      WHERE created_at >= CURRENT_DATE "
        "        AND created_at < CURRENT_DATE + INTERVAL '1 day' "
        "        AND status = 'published' AND deleted_at IS NULL"
        "    )::INTEGER AS today_stones "
        "  FROM stones"
        "), session_stats AS ("
        "  SELECT "
        "    COUNT(DISTINCT user_id)::INTEGER AS session_online_users "
        "  FROM user_sessions "
        "  WHERE created_at > NOW() - INTERVAL '5 minutes'"
        "), report_stats AS ("
        "  SELECT COUNT(*) FILTER (WHERE status = 'pending')::INTEGER AS pending_reports "
        "  FROM reports"
        ") "
        "SELECT "
        "  us.total_users, "
        "  ss.total_stones, "
        "  ss.today_stones, "
        "  GREATEST("
        "    COALESCE(sess.session_online_users, 0), "
        "    COALESCE(us.active_online_users, 0)"
        "  ) AS online_users, "
        "  rs.pending_reports "
        "FROM user_stats us "
        "CROSS JOIN stone_stats ss "
        "CROSS JOIN session_stats sess "
        "CROSS JOIN report_stats rs");

    const auto row = safeRow(result);
    if (!row) {
        throw std::runtime_error("admin realtime snapshot query returned no rows");
    }

    AdminRealtimeStatsSnapshot snapshot;
    snapshot.totalUsers = requireSnapshotInt(*row, "total_users");
    snapshot.totalStones = requireSnapshotInt(*row, "total_stones");
    snapshot.todayStones = requireSnapshotInt(*row, "today_stones");
    snapshot.onlineUsers = requireSnapshotInt(*row, "online_users");
    snapshot.pendingReports = requireSnapshotInt(*row, "pending_reports");
    return snapshot;
}

void broadcastAdminRealtimeStatsUpdate(std::string reason, Json::Value extra) {
    drogon::async_run([reason = std::move(reason), extra = std::move(extra)]() mutable -> drogon::Task<void> {
        try {
            const auto snapshot = queryAdminRealtimeStatsSnapshot();
            Json::Value payload = snapshot.toJson();
            payload["reason"] = reason;

            if (extra.isObject()) {
                for (const auto &memberName : extra.getMemberNames()) {
                    payload[memberName] = extra[memberName];
                }
            }

            controllers::BroadcastWebSocketController::broadcast(
                buildRealtimeEvent("stats_update", std::move(payload)));
        } catch (const std::exception &e) {
            LOG_ERROR << "broadcastAdminRealtimeStatsUpdate failed: " << e.what();
            try {
                controllers::BroadcastWebSocketController::broadcast(
                    buildRealtimeEvent(
                        "socket_error",
                        buildSocketErrorPayload(reason, e.what())));
            } catch (const std::exception& broadcastError) {
                LOG_ERROR << "broadcastAdminRealtimeStatsUpdate socket_error broadcast failed: "
                          << broadcastError.what();
            }
        }
        co_return;
    });
}

} // namespace heartlake::utils
