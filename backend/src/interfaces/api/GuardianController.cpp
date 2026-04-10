/**
 * @file GuardianController.cpp
 * @brief 守望者系统控制器 — 守护者激励、灯火转赠、情绪洞察、湖神对话
 *
 * 守望者是心湖社区的核心角色：通过高质量涟漪和温暖纸船积累共鸣点数，
 * 达标后晋升为"点灯人"。getStats 返回守护者统计（共鸣点、涟漪数、
 * 纸船数、是否可转赠灯火），transferLamp 实现灯火在守护者间流转，
 * getEmotionInsights 借助 DualMemoryRAG 生成个性化情绪洞察报告，
 * chat 接口提供湖神 AI 对话能力（带情绪感知和上下文记忆）。
 */

#include "interfaces/api/GuardianController.h"
#include "infrastructure/ai/DualMemoryRAG.h"
#include "infrastructure/services/GuardianIncentiveService.h"
#include "utils/IdentityShadowMap.h"
#include "utils/PasetoUtil.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/Validator.h"

using namespace heartlake::controllers;
using namespace heartlake::infrastructure;
using namespace heartlake::utils;

void GuardianController::getStats(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  auto userId = Validator::getUserId(req);
  if (!userId) {
    callback(ResponseUtil::unauthorized("未登录"));
    return;
  }

  try {
    auto stats =
        GuardianIncentiveService::getInstance().getGuardianStats(*userId);
    Json::Value response;
    response["resonance_points"] = stats.totalResonancePoints;
    response["quality_ripples"] = stats.qualityRipples;
    response["warm_boats"] = stats.warmBoats;
    response["is_guardian"] = stats.isGuardian;
    response["is_lamp_keeper"] = stats.isGuardian; // 角色统一别名（保持兼容）
    response["can_transfer_lamp"] = stats.canTransferLamp;
    response["role"] = stats.isGuardian ? "guardian_lamp_keeper" : "member";

    callback(ResponseUtil::success(response));
  } catch (const std::exception &e) {
    LOG_ERROR << "Failed to get guardian stats: " << e.what();
    callback(ResponseUtil::internalError("获取守护者数据失败"));
  }
}

void GuardianController::transferLamp(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  auto userId = Validator::getUserId(req);
  if (!userId) {
    callback(ResponseUtil::unauthorized("未登录"));
    return;
  }

  auto jsonBody = req->getJsonObject();
  if (!jsonBody || !jsonBody->isMember("to_user_id")) {
    callback(ResponseUtil::error(400, "缺少接收者ID"));
    return;
  }

  try {
    std::string toUserId = (*jsonBody)["to_user_id"].asString();
    bool success =
        GuardianIncentiveService::getInstance().transferLamp(*userId, toUserId);

    if (success) {
      Json::Value data;
      data["to_user_id"] = toUserId;
      data["target_user_id"] = toUserId;
      data["status"] = "transferred";
      callback(ResponseUtil::success(data, "灯火转赠成功"));
    } else {
      callback(ResponseUtil::error(400, "转赠失败，请检查守护者/点灯人资格"));
    }
  } catch (const std::exception &e) {
    LOG_ERROR << "Failed to transfer lamp: " << e.what();
    callback(ResponseUtil::internalError("灯火转赠失败"));
  }
}

void GuardianController::getEmotionInsights(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  auto userId = Validator::getUserId(req);
  if (!userId) {
    callback(ResponseUtil::unauthorized("未登录"));
    return;
  }

  try {
    auto &dualMemory = heartlake::ai::DualMemoryRAG::getInstance();
    auto insights = dualMemory.getEmotionInsights(*userId);
    insights["shadow_id"] =
        IdentityShadowMap::getInstance().getOrCreateShadowId(*userId);
    callback(ResponseUtil::success(insights));
  } catch (const std::exception &e) {
    LOG_ERROR << "Failed to get emotion insights for user " << *userId << ": "
              << e.what();
    callback(ResponseUtil::internalError("获取情绪洞察失败"));
  }
}

void GuardianController::chat(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  auto userId = Validator::getUserId(req);
  if (!userId) {
    callback(ResponseUtil::unauthorized("未登录"));
    return;
  }

  auto jsonBody = req->getJsonObject();
  if (!jsonBody || !jsonBody->isMember("content")) {
    callback(ResponseUtil::error(400, "缺少消息内容"));
    return;
  }

  try {
    std::string content = (*jsonBody)["content"].asString();

    // 限制单条消息长度，防止超长文本消耗过多资源
    static constexpr size_t MAX_CHAT_LENGTH = 2000;
    if (content.size() > MAX_CHAT_LENGTH) {
      callback(ResponseUtil::error(400, "消息内容不能超过2000字符"));
      return;
    }

    std::string emotion = jsonBody->get("emotion", "neutral").asString();
    float emotionScore = jsonBody->get("emotion_score", 0.5f).asFloat();

    auto &dualMemory = heartlake::ai::DualMemoryRAG::getInstance();
    auto replyResult = dualMemory.generateResponseResult(*userId, content,
                                                         emotion, emotionScore);

    Json::Value response;
    response["shadow_id"] =
        IdentityShadowMap::getInstance().getOrCreateShadowId(*userId);
    response["reply"] = replyResult.reply;
    response["agent"] = "lake_god";
    response["agent_name"] = "湖神";
    response["response_source"] = replyResult.source;
    response["degraded"] = replyResult.degraded;
    if (!replyResult.error.empty()) {
      response["warning"] = replyResult.error;
      response["ai_error"] = replyResult.error;
      LOG_WARN << "GuardianController chat degraded for user " << *userId
               << ": " << replyResult.error;
    }
    callback(ResponseUtil::success(
        response, replyResult.degraded ? "湖神本地陪伴回复" : "湖神回复成功"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Failed to chat with lake god for user " << *userId << ": "
              << e.what();
    callback(ResponseUtil::internalError("湖神暂时无法回应"));
  }
}
