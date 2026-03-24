#include "utils/AdminRealtimeNotifier.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/RequestHelper.h"
#include <drogon/drogon.h>
#include <ctime>

namespace heartlake::utils {

void broadcastAdminRealtimeStatsUpdate(std::string reason, Json::Value extra) {
    drogon::async_run([reason = std::move(reason), extra = std::move(extra)]() mutable -> drogon::Task<void> {
        try {
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
                co_return;
            }

            const int totalUsers = (*row)["total_users"].isNull()
                ? 0
                : (*row)["total_users"].as<int>();
            const int totalStones = (*row)["total_stones"].isNull()
                ? 0
                : (*row)["total_stones"].as<int>();
            const int todayStones = (*row)["today_stones"].isNull()
                ? 0
                : (*row)["today_stones"].as<int>();
            const int onlineUsers = (*row)["online_users"].isNull()
                ? 0
                : (*row)["online_users"].as<int>();
            const int pendingReports = (*row)["pending_reports"].isNull()
                ? 0
                : (*row)["pending_reports"].as<int>();

            Json::Value payload(Json::objectValue);
            payload["type"] = "stats_update";
            payload["reason"] = reason;
            payload["event"] = reason;
            payload["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            payload["total_users"] = totalUsers;
            payload["totalUsers"] = totalUsers;
            payload["total_stones"] = totalStones;
            payload["totalStones"] = totalStones;
            payload["today_stones"] = todayStones;
            payload["todayStones"] = todayStones;
            payload["online_users"] = onlineUsers;
            payload["online_count"] = onlineUsers;
            payload["onlineCount"] = onlineUsers;
            payload["pending_reports"] = pendingReports;
            payload["pendingReports"] = pendingReports;

            if (extra.isObject()) {
                for (const auto &memberName : extra.getMemberNames()) {
                    payload[memberName] = extra[memberName];
                }
            }

            controllers::BroadcastWebSocketController::broadcast(payload);
        } catch (const std::exception &e) {
            LOG_WARN << "broadcastAdminRealtimeStatsUpdate failed: " << e.what();
        }
        co_return;
    });
}

} // namespace heartlake::utils
