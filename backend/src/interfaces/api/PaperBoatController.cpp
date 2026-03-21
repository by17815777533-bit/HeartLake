/**
 * PaperBoatController 模块实现
 */
#include "interfaces/api/PaperBoatController.h"
#include "application/InteractionApplicationService.h"
#include "infrastructure/di/ServiceLocator.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/ContentFilter.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/Validator.h"
#include <json/json.h>

using namespace heartlake::controllers;
using namespace heartlake::utils;
using namespace heartlake::application;

namespace {

std::shared_ptr<InteractionApplicationService> getInteractionService() {
  return heartlake::core::di::ServiceLocator::instance()
      .resolve<InteractionApplicationService>();
}

} // namespace

void PaperBoatController::replyToStone(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string stone_id = (*json).get("stone_id", "").asString();
    std::string content = (*json).get("content", "").asString();
    [[maybe_unused]] std::string mood = (*json).get("mood", "").asString();

    if (stone_id.empty() || content.empty()) {
      callback(ResponseUtil::badRequest("stone_id 和 content 不能为空"));
      return;
    }

    // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto &user_id = *userIdOpt;

    // 内容安全检查
    std::string safety_level = ContentFilter::checkContentSafety(content);
    if (safety_level == "high_risk") {
      Json::Value warning;
      warning["message"] = ContentFilter::getMentalHealthTip();
      callback(
          ResponseUtil::error(403, "检测到高危内容，请寻求专业帮助", warning));
      return;
    }

    auto service = getInteractionService();
    auto result = service->createBoat(stone_id, user_id, content);
    const auto boat_id = result["boat_id"].asString();
    const int newBoatsCount = result["boat_count"].asInt();
    const auto stone_owner_id = result.get("stone_owner_id", "").asString();

    // 广播 boat_update 事件到 stone 房间，让查看该石头的用户实时更新纸船计数
    {
      Json::Value broadcastMsg;
      broadcastMsg["type"] = "boat_update";
      broadcastMsg["stone_id"] = stone_id;
      broadcastMsg["boat_id"] = boat_id;
      broadcastMsg["boat_count"] = newBoatsCount;
      broadcastMsg["triggered_by"] = user_id;
      broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
      BroadcastWebSocketController::sendToRoom("stone:" + stone_id,
                                               broadcastMsg);
      BroadcastWebSocketController::broadcast(broadcastMsg);
    }

    // 服务层已经负责入库通知、风险评估和临时好友刷新，这里只发实时通知。
    if (!stone_owner_id.empty() && stone_owner_id != user_id) {
      Json::Value notifMsg;
      notifMsg["type"] = "new_notification";
      notifMsg["notification_type"] = "boat";
      notifMsg["stone_id"] = stone_id;
      notifMsg["boat_id"] = boat_id;
      notifMsg["from_user_id"] = user_id;
      notifMsg["boat_count"] = newBoatsCount;
      notifMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
      BroadcastWebSocketController::sendToUser(stone_owner_id, notifMsg);
    }
    Json::Value data;
    data["boat_id"] = boat_id;
    data["stone_id"] = stone_id;
    data["boat_count"] = newBoatsCount;

    callback(ResponseUtil::success(data, "纸船已放置"));

  } catch (const std::runtime_error &e) {
    LOG_ERROR << "Runtime error in replyToStone: " << e.what();
    if (std::string(e.what()) == "石头不存在") {
      callback(ResponseUtil::notFound("石头不存在"));
      return;
    }
    callback(ResponseUtil::error(400, e.what()));
  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in replyToStone: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in replyToStone: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void PaperBoatController::getBoatDetail(
    const HttpRequestPtr & /*req*/,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &boatId) {
  try {
    auto dbClient = drogon::app().getDbClient("default");

    auto result = dbClient->execSqlSync(
        "SELECT b.boat_id, b.stone_id, b.content, b.boat_style AS boat_color, "
        "b.is_anonymous, "
        "b.is_ai_reply, b.status, "
        "EXTRACT(EPOCH FROM b.created_at) as created_at_ts, "
        "u.nickname "
        "FROM paper_boats b "
        "LEFT JOIN users u ON b.sender_id = u.user_id "
        "WHERE b.boat_id = $1",
        boatId);

    if (result.empty()) {
      callback(ResponseUtil::notFound("纸船不存在"));
      return;
    }

    auto row = *safeRow(result);

    Json::Value boat;
    boat["boat_id"] = row["boat_id"].as<std::string>();
    boat["stone_id"] =
        row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
    boat["content"] = row["content"].as<std::string>();
    boat["boat_color"] = row["boat_color"].isNull()
                             ? "#F5EFE7"
                             : row["boat_color"].as<std::string>();
    boat["is_anonymous"] =
        row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
    boat["is_ai_reply"] =
        row["is_ai_reply"].isNull() ? false : row["is_ai_reply"].as<bool>();
    boat["status"] = row["status"].as<std::string>();
    boat["created_at"] =
        static_cast<Json::Int64>(row["created_at_ts"].as<double>());

    Json::Value author;
    author["nickname"] = row["nickname"].isNull()
                             ? "匿名旅人"
                             : row["nickname"].as<std::string>();
    author["is_anonymous"] =
        row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
    boat["author"] = author;

    callback(ResponseUtil::success(boat));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getBoatDetail: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getBoatDetail: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void PaperBoatController::getMySentBoats(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto &user_id = *userIdOpt;

    auto [page, page_size] = safePagination(req);
    auto service = getInteractionService();
    auto result =
        service->getSentBoats(user_id, page, page_size, req->getParameter("status"));
    callback(ResponseUtil::success(result));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getMySentBoats: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getMySentBoats: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void PaperBoatController::getMyReceivedBoats(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto &user_id = *userIdOpt;

    auto [page, page_size] = safePagination(req);
    auto service = getInteractionService();
    auto result = service->getReceivedBoats(user_id, page, page_size);
    callback(ResponseUtil::success(result));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getMyReceivedBoats: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getMyReceivedBoats: " << e.what();
    callback(ResponseUtil::internalError());
  }
}
