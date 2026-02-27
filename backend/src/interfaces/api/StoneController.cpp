/**
 * StoneController 模块实现
 */

#include "interfaces/api/StoneController.h"
#include "application/StoneApplicationService.h"
#include "infrastructure/di/ServiceLocator.h"
#include "utils/ResponseUtil.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"
#include "utils/ContentFilter.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "infrastructure/ai/EmotionResonanceEngine.h"
#include <memory>
#include <regex>
#include <set>

using namespace heartlake::controllers;
using namespace heartlake::utils;
using namespace heartlake::application;

static std::shared_ptr<StoneApplicationService> getStoneService() {
    return heartlake::core::di::ServiceLocator::instance().resolve<StoneApplicationService>();
}

// ==================== 安全辅助函数 ====================

// SEC-HTML: 对输出字符串进行 HTML 实体转义，防止 XSS
static std::string escapeHtml(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '&':  output += "&amp;";  break;
            case '<':  output += "&lt;";   break;
            case '>':  output += "&gt;";   break;
            case '"':  output += "&quot;"; break;
            case '\'': output += "&#x27;"; break;
            default:   output += c;        break;
        }
    }
    return output;
}

// SEC-VALIDATE: stoneId 格式校验（仅允许 UUID 或安全 ID 格式）
static bool isValidStoneId(const std::string& id) {
    if (id.empty() || id.size() > 64) return false;
    // 允许 UUID 格式、纯字母数字、以及带下划线/短横线的 ID
    static const std::regex idPattern("^[a-zA-Z0-9_-]{1,64}$");
    return std::regex_match(id, idPattern);
}

// SEC-VALIDATE: 颜色值校验（仅允许 #RRGGBB 格式）
static bool isValidColor(const std::string& color) {
    static const std::regex colorPattern("^#[0-9a-fA-F]{6}$");
    return std::regex_match(color, colorPattern);
}

// 白名单常量
static const std::set<std::string> VALID_STONE_TYPES = {"small", "medium", "large", "light", "heavy"};
static const std::set<std::string> VALID_MOOD_TYPES = {
    "calm", "happy", "sad", "angry", "anxious", "hopeful", "lonely", "grateful", "confused", "peaceful",
    "surprised", "neutral"
};
static const std::set<std::string> VALID_SORT_VALUES = {"latest", "hot", "created_at"};

// ==================== 石头发布 ====================

void StoneController::createStone(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] createStone called";

        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
            return;
        }

        // SEC-AUTH: 必须登录
        auto userIdOpt = Validator::getUserId(req);
        if (!userIdOpt) {
            callback(ResponseUtil::unauthorized("未登录，请先登录"));
            return;
        }
        auto userId = *userIdOpt;

        // 验证必填字段
        if (!json->isMember("content") || !(*json)["content"].isString() || (*json)["content"].asString().empty()) {
            callback(ResponseUtil::badRequest("content 不能为空"));
            return;
        }

        std::string content = (*json)["content"].asString();

        // SEC-LEN: 内容长度限制（最少1字符，最多2000字符）
        if (content.size() > 8000) {
            // UTF-8 中一个中文字符最多 4 字节，2000 字 ≈ 8000 字节
            callback(ResponseUtil::badRequest("内容长度不能超过 2000 字"));
            return;
        }

        // SEC-TYPE: stone_type 白名单校验
        std::string stoneType = (*json).get("stone_type", "medium").asString();
        if (VALID_STONE_TYPES.find(stoneType) == VALID_STONE_TYPES.end()) {
            callback(ResponseUtil::badRequest("无效的 stone_type，可选值: small, medium, large"));
            return;
        }

        // SEC-COLOR: stone_color 格式校验
        std::string stoneColor = (*json).get("stone_color", "#7A92A3").asString();
        if (!isValidColor(stoneColor)) {
            callback(ResponseUtil::badRequest("无效的 stone_color，格式应为 #RRGGBB"));
            return;
        }

        // SEC-MOOD: mood_type 白名单校验
        std::string moodType = (*json).get("mood_type", "calm").asString();
        if (VALID_MOOD_TYPES.find(moodType) == VALID_MOOD_TYPES.end()) {
            callback(ResponseUtil::badRequest("无效的 mood_type"));
            return;
        }

        bool isAnonymous = (*json).get("is_anonymous", true).asBool();

        // 内容安全检查
        std::string safetyLevel = ContentFilter::checkContentSafety(content);
        if (safetyLevel == "high_risk") {
            Json::Value warning;
            warning["message"] = ContentFilter::getMentalHealthTip();
            callback(ResponseUtil::error(403, "检测到高危内容，请寻求专业帮助", warning));
            return;
        }

        // 使用ApplicationService发布石头
        auto service = getStoneService();
        auto result = service->publishStone(userId, content, stoneType, stoneColor, moodType, isAnonymous);

        std::string stoneId = result["stone_id"].asString();

        // 异步处理AI任务（情感分析、向量嵌入、风险评估、通知推送、AI评论）
        service->processStoneAsync(stoneId, userId, content, moodType);

        // 使缓存失效
        auto& cacheManager = heartlake::core::cache::CacheManager::getInstance();
        cacheManager.invalidatePattern("stone_list:*");

        // 广播新石头事件到 lake 房间，只有观湖页面的用户收到
        Json::Value broadcastMsg;
        broadcastMsg["type"] = "new_stone";
        broadcastMsg["stone"] = result;
        broadcastMsg["triggered_by"] = userId;
        broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
        BroadcastWebSocketController::sendToRoom("lake", broadcastMsg);

        callback(ResponseUtil::success(result, "投石成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in createStone: " << e.what();
        // SEC-XSS: 转义错误消息，防止异常信息中包含用户输入导致 XSS
        callback(ResponseUtil::error(400, escapeHtml(e.what())));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in createStone: " << e.what();
        // SEC-INFO: 不向客户端暴露内部错误详情
        callback(ResponseUtil::internalError("创建石头失败"));
    }
}

// ==================== 石头列表 ====================

void StoneController::getStones(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] getStones called";

        auto [page, pageSize] = safePagination(req);
        std::string sort = req->getParameter("sort").empty() ? "created_at" : req->getParameter("sort");
        std::string filterMood = req->getParameter("mood");

        // SEC-WHITELIST: sort 参数白名单校验，防止 SQL 注入
        if (VALID_SORT_VALUES.find(sort) == VALID_SORT_VALUES.end()) {
            sort = "created_at";
        }

        // SEC-WHITELIST: mood 参数白名单校验，防止 SQL 注入
        if (!filterMood.empty() && VALID_MOOD_TYPES.find(filterMood) == VALID_MOOD_TYPES.end()) {
            callback(ResponseUtil::badRequest("无效的 mood 筛选值"));
            return;
        }

        // 映射排序参数
        std::string sortBy = "created_at";
        if (sort == "hot") {
            sortBy = "ripple_count";
        } else if (sort == "latest") {
            sortBy = "created_at";
        }

        std::string currentUserId = Validator::getUserId(req).value_or("");

        auto service = getStoneService();
        auto result = service->getStoneList(page, pageSize, sortBy, filterMood, "", currentUserId);

        callback(ResponseUtil::success(result));

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getStones: " << e.what();
        callback(ResponseUtil::internalError("获取石头列表失败"));
    }
}

// ==================== 石头详情 ====================

void StoneController::getStoneById(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& stoneId
) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] getStoneById id=" << stoneId;

        // SEC-VALIDATE: stoneId 格式校验
        if (!isValidStoneId(stoneId)) {
            callback(ResponseUtil::badRequest("无效的石头 ID 格式"));
            return;
        }

        std::string currentUserId = Validator::getUserId(req).value_or("");

        auto service = getStoneService();
        auto result = service->getStoneDetail(stoneId, currentUserId);

        // 异步增加浏览量
        service->incrementViewCount(stoneId);

        callback(ResponseUtil::success(result));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in getStoneById: " << e.what();
        // SEC-XSS: 转义错误消息
        callback(ResponseUtil::notFound(escapeHtml(e.what())));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in getStoneById: " << e.what();
        callback(ResponseUtil::internalError("获取石头详情失败"));
    }
}

// ==================== 我的石头 ====================

void StoneController::getMyStones(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] getMyStones called";

        auto userIdOpt = Validator::getUserId(req);
        // SEC-AUTH: 必须登录
        if (!userIdOpt) {
            callback(ResponseUtil::unauthorized("未登录，请先登录"));
            return;
        }
        auto userId = *userIdOpt;

        auto [page, pageSize] = safePagination(req);

        auto service = getStoneService();
        auto result = service->getStoneList(page, pageSize, "created_at", "", userId, userId);

        callback(ResponseUtil::success(result));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in getMyStones: " << e.what();
        // SEC-XSS: 转义错误消息
        callback(ResponseUtil::error(400, escapeHtml(e.what())));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in getMyStones: " << e.what();
        callback(ResponseUtil::internalError("获取我的石头失败"));
    }
}

// ==================== 删除石头 ====================

void StoneController::deleteStone(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& stoneId
) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] deleteStone id=" << stoneId;

        auto userIdOpt = Validator::getUserId(req);
        // SEC-AUTH: 必须登录
        if (!userIdOpt) {
            callback(ResponseUtil::unauthorized("未登录，请先登录"));
            return;
        }
        auto userId = *userIdOpt;

        // SEC-VALIDATE: stoneId 格式校验
        if (!isValidStoneId(stoneId)) {
            callback(ResponseUtil::badRequest("无效的石头 ID 格式"));
            return;
        }

        auto service = getStoneService();
        service->deleteStone(stoneId, userId);

        // 广播石头删除事件到 lake 房间
        Json::Value broadcastMsg;
        broadcastMsg["type"] = "stone_deleted";
        broadcastMsg["stone_id"] = stoneId;
        broadcastMsg["triggered_by"] = userId;
        broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
        BroadcastWebSocketController::sendToRoom("lake", broadcastMsg);

        callback(ResponseUtil::success(Json::Value(), "删除成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in deleteStone: " << e.what();
        // SEC-XSS: 转义错误消息
        callback(ResponseUtil::error(400, escapeHtml(e.what())));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in deleteStone: " << e.what();
        callback(ResponseUtil::internalError("删除石头失败"));
    }
}

// ==================== 湖面天气 ==========
// 隐私增强版可通过 GET /api/lake/privacy-stats 获取（差分隐私保护）
// 参考：DifferentialPrivacyEngine - FedMultiEmo (arXiv:2507.15470)

void StoneController::getLakeWeather(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] getLakeWeather called";

        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "SELECT stone_color, COUNT(*) as count "
            "FROM stones "
            "WHERE status = 'published' AND created_at >= NOW() - INTERVAL '1 hour' "
            "GROUP BY stone_color "
            "ORDER BY count DESC"
        );

        std::map<std::string, int> colorCount;
        int totalStones = 0;

        for (const auto& row : result) {
            std::string color = row["stone_color"].as<std::string>();
            int count = row["count"].as<int>();
            colorCount[color] = count;
            totalStones += count;
        }

        Json::Value weatherData;

        if (totalStones == 0) {
            weatherData["weather"] = "calm";
            weatherData["description"] = "湖面平静，等待第一颗石子";
            weatherData["emoji"] = "🌊";
        } else {
            std::string dominantColor;
            int maxCount = 0;

            for (const auto& pair : colorCount) {
                if (pair.second > maxCount) {
                    maxCount = pair.second;
                    dominantColor = pair.first;
                }
            }

            if (dominantColor.find("526D82") != std::string::npos || dominantColor.find("blue") != std::string::npos) {
                weatherData["weather"] = "rainy";
                weatherData["description"] = "湖面微雨，有人心情忧郁";
                weatherData["emoji"] = "🌧️";
            } else if (dominantColor.find("F4D160") != std::string::npos || dominantColor.find("yellow") != std::string::npos) {
                weatherData["weather"] = "sunny";
                weatherData["description"] = "湖面晴朗，大家心情愉悦";
                weatherData["emoji"] = "☀️";
            } else if (dominantColor.find("C08B5C") != std::string::npos || dominantColor.find("brown") != std::string::npos) {
                weatherData["weather"] = "cloudy";
                weatherData["description"] = "湖面微风，情绪平稳";
                weatherData["emoji"] = "⛅";
            } else {
                weatherData["weather"] = "mixed";
                weatherData["description"] = "湖面波光粼粼，情绪交织";
                weatherData["emoji"] = "✨";
            }
        }

        weatherData["total_stones"] = totalStones;
        weatherData["color_distribution"] = Json::Value(Json::objectValue);

        for (const auto& pair : colorCount) {
            weatherData["color_distribution"][pair.first] = pair.second;
        }

        callback(ResponseUtil::success(weatherData));

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Database error in getLakeWeather: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getLakeWeather: " << e.what();
        callback(ResponseUtil::internalError());
    }
}

// ==================== 共鸣搜索 ====================

void StoneController::searchResonance(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& stoneId
) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] searchResonance stoneId=" << stoneId;

        auto userIdOpt = Validator::getUserId(req);
        // SEC-AUTH: 必须登录
        if (!userIdOpt) {
            callback(ResponseUtil::unauthorized("未登录，请先登录"));
            return;
        }
        auto userId = *userIdOpt;

        // SEC-VALIDATE: stoneId 格式校验
        if (!isValidStoneId(stoneId)) {
            callback(ResponseUtil::badRequest("无效的石头 ID 格式"));
            return;
        }

        int limit = safeInt(req->getParameter("limit"), 10);
        // SEC-06: limit 范围约束 — 防止客户端传入超大值导致性能问题
        if (limit < 1) limit = 1;
        if (limit > 50) limit = 50;

        // 使用情绪感知时序共鸣引擎
        auto& resonanceEngine = heartlake::ai::EmotionResonanceEngine::getInstance();
        auto matches = resonanceEngine.findResonance(userId, stoneId, limit);

        Json::Value result(Json::arrayValue);
        for (const auto& match : matches) {
            Json::Value item;
            item["stone_id"] = match.stoneId;
            item["total_score"] = match.totalScore;
            item["semantic_score"] = match.semanticScore;
            item["trajectory_score"] = match.trajectoryScore;
            item["temporal_score"] = match.temporalScore;
            item["diversity_score"] = match.diversityScore;
            item["resonance_reason"] = match.resonanceReason;
            result.append(item);
        }

        Json::Value data;
        data["matches"] = result;
        data["count"] = static_cast<int>(matches.size());
        data["algorithm"] = "emotion_temporal_resonance";

        callback(ResponseUtil::success(data, "共鸣搜索完成"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in searchResonance: " << e.what();
        // SEC-XSS: 转义错误消息
        callback(ResponseUtil::notFound(escapeHtml(e.what())));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in searchResonance: " << e.what();
        callback(ResponseUtil::internalError("共鸣搜索失败"));
    }
}
