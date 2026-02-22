/**
 * @file DualMemoryRAG.cpp
 * @brief 双记忆RAG情感守护系统实现
 *
 * 基于SoulSpeak论文(arXiv, Dec 2024)的双记忆架构：
 * - 短期记忆：滑动窗口保留最近5次交互
 * - 长期记忆：从emotion_tracking表聚合30天情绪画像
 * - RAG提示词融合双记忆上下文，生成个性化心理支持回复
 */

#include "infrastructure/ai/DualMemoryRAG.h"
#include "infrastructure/ai/AIService.h"
#include "utils/IdentityShadowMap.h"
#include <drogon/drogon.h>
#include <sstream>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <iomanip>

namespace heartlake::ai {

DualMemoryRAG& DualMemoryRAG::getInstance() {
    static DualMemoryRAG instance;
    return instance;
}

EmotionMemory& DualMemoryRAG::getOrCreateMemory(const std::string& userId) {
    auto it = memories_.find(userId);
    if (it == memories_.end()) {
        EmotionMemory mem;
        mem.userId = userId;
        memories_[userId] = std::move(mem);
    }
    return memories_[userId];
}

void DualMemoryRAG::updateShortTermMemory(
    const std::string& userId,
    const std::string& content,
    const std::string& emotion,
    float score
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& memory = getOrCreateMemory(userId);

    // 生成时间戳
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    EmotionMemory::ShortTermEntry entry;
    entry.content = content;
    entry.emotion = emotion;
    entry.score = score;
    entry.timestamp = oss.str();

    memory.shortTerm.push_back(std::move(entry));

    // 滑动窗口：保留最近MAX_SHORT_TERM条
    if (static_cast<int>(memory.shortTerm.size()) > MAX_SHORT_TERM) {
        memory.shortTerm.erase(memory.shortTerm.begin());
    }
}

void DualMemoryRAG::refreshLongTermMemory(const std::string& userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& memory = getOrCreateMemory(userId);

    try {
        auto db = drogon::app().getDbClient("default");
        if (!db) {
            return;
        }
        auto result = db->execSqlSync(
            "SELECT COUNT(*) as total_posts, "
            "AVG(emotion_score) as avg_score, "
            "STDDEV(emotion_score) as score_stddev, "
            "MAX(created_at)::text as last_active "
            "FROM stones WHERE user_id = $1 "
            "AND created_at > NOW() - INTERVAL '30 days' "
            "AND deleted_at IS NULL",
            userId
        );

        if (!result.empty() && !result[0]["total_posts"].isNull()) {
            auto& lt = memory.longTerm;
            lt.totalPosts = result[0]["total_posts"].as<int>();
            lt.avgEmotionScore = result[0]["avg_score"].isNull() ? 0.0f : result[0]["avg_score"].as<float>();
            lt.emotionVolatility = result[0]["score_stddev"].isNull() ? 0.0f : result[0]["score_stddev"].as<float>();
            lt.lastActiveDate = result[0]["last_active"].isNull() ? "" : result[0]["last_active"].as<std::string>();
        }

        // 获取主导情绪
        auto moodResult = db->execSqlSync(
            "SELECT mood_type, COUNT(*) as cnt FROM stones "
            "WHERE user_id = $1 AND created_at > NOW() - INTERVAL '30 days' "
            "AND deleted_at IS NULL AND mood_type IS NOT NULL "
            "GROUP BY mood_type ORDER BY cnt DESC LIMIT 1",
            userId
        );
        if (!moodResult.empty()) {
            memory.longTerm.dominantMood = moodResult[0]["mood_type"].as<std::string>();
        }

        // 计算情绪趋势：对比最近7天 vs 前7-14天
        auto trendResult = db->execSqlSync(
            "SELECT "
            "AVG(CASE WHEN created_at > NOW() - INTERVAL '7 days' THEN emotion_score END) as recent_avg, "
            "AVG(CASE WHEN created_at BETWEEN NOW() - INTERVAL '14 days' AND NOW() - INTERVAL '7 days' THEN emotion_score END) as prev_avg "
            "FROM stones WHERE user_id = $1 "
            "AND created_at > NOW() - INTERVAL '14 days' AND deleted_at IS NULL",
            userId
        );
        if (!trendResult.empty()) {
            float recentAvg = trendResult[0]["recent_avg"].isNull() ? 0.0f : trendResult[0]["recent_avg"].as<float>();
            float prevAvg = trendResult[0]["prev_avg"].isNull() ? 0.0f : trendResult[0]["prev_avg"].as<float>();
            float diff = recentAvg - prevAvg;
            if (diff > 0.15f) memory.longTerm.emotionTrend = "rising";
            else if (diff < -0.15f) memory.longTerm.emotionTrend = "falling";
            else memory.longTerm.emotionTrend = "stable";
        }

        // 连续负面天数
        auto negResult = db->execSqlSync(
            "SELECT DATE(created_at) as d, AVG(emotion_score) as day_avg "
            "FROM stones WHERE user_id = $1 "
            "AND created_at > NOW() - INTERVAL '14 days' AND deleted_at IS NULL "
            "GROUP BY DATE(created_at) ORDER BY d DESC",
            userId
        );
        int consecutiveNeg = 0;
        for (const auto& row : negResult) {
            float dayAvg = row["day_avg"].isNull() ? 0.0f : row["day_avg"].as<float>();
            if (dayAvg < -0.2f) {
                consecutiveNeg++;
            } else {
                break;
            }
        }
        memory.longTerm.consecutiveNegativeDays = consecutiveNeg;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "DualMemoryRAG::refreshLongTermMemory failed: " << e.base().what();
    }
}

std::string DualMemoryRAG::buildRAGPrompt(
    const EmotionMemory& memory,
    const std::string& currentContent,
    const std::string& currentEmotion
) {
    std::ostringstream prompt;

    prompt << "你是「心湖湖神」，一位具有专业心理咨询素养的AI情感守护者。\n\n";

    // === 长期记忆上下文 ===
    prompt << "【用户情绪画像 - 长期记忆】\n";
    const auto& lt = memory.longTerm;
    if (lt.totalPosts > 0) {
        prompt << "- 近30天发布: " << lt.totalPosts << "条内容\n";
        prompt << "- 平均情绪分数: " << std::fixed << std::setprecision(2) << lt.avgEmotionScore
               << " (范围-1.0到1.0)\n";
        prompt << "- 主导情绪: " << lt.dominantMood << "\n";
        prompt << "- 情绪波动度: " << std::fixed << std::setprecision(2) << lt.emotionVolatility << "\n";
        prompt << "- 情绪趋势: ";
        if (lt.emotionTrend == "rising") prompt << "好转中 ↑\n";
        else if (lt.emotionTrend == "falling") prompt << "下降中 ↓\n";
        else prompt << "平稳 →\n";
        if (lt.consecutiveNegativeDays > 0) {
            prompt << "- ⚠️ 连续" << lt.consecutiveNegativeDays << "天情绪偏低\n";
        }
    } else {
        prompt << "- 新用户，暂无历史数据\n";
    }

    // === 短期记忆上下文 ===
    prompt << "\n【近期交互 - 短期记忆】\n";
    if (memory.shortTerm.empty()) {
        prompt << "- 这是用户的首次互动\n";
    } else {
        for (size_t i = 0; i < memory.shortTerm.size(); ++i) {
            const auto& entry = memory.shortTerm[i];
            prompt << (i + 1) << ". [" << entry.emotion << " / "
                   << std::fixed << std::setprecision(1) << entry.score << "] "
                   << entry.content.substr(0, 80);
            if (entry.content.size() > 80) prompt << "...";
            prompt << "\n";
        }
    }

    // === 当前状态 ===
    prompt << "\n【当前状态】\n";
    prompt << "- 情绪: " << currentEmotion << "\n";
    prompt << "- 内容: " << currentContent << "\n";

    // === 回复指导原则 ===
    prompt << R"(

【回复指导原则 - 基于心理咨询最佳实践】
1. 共情优先：先准确反映用户的感受，让TA感到被理解和接纳
2. 纵向关怀：结合用户的情绪画像和近期变化，体现持续关注
3. 个性化回应：
   - 如果情绪趋势好转：肯定进步，温和鼓励
   - 如果情绪持续低落：加强陪伴感，避免说教，适时建议专业帮助
   - 如果情绪波动大：帮助觉察情绪模式，提供稳定感
   - 如果是新用户：温暖欢迎，建立信任
4. 安全边界：
   - 不做诊断，不开处方
   - 发现严重风险时，温和建议寻求专业心理援助
   - 保持积极但不虚假的态度
5. 语言风格：温暖自然，像一位值得信赖的朋友，80-150字

请直接给出回复内容，不需要任何前缀或标签。)";

    return prompt.str();
}

std::string DualMemoryRAG::generateResponse(
    const std::string& userId,
    const std::string& currentContent,
    const std::string& currentEmotion,
    float emotionScore
) {
    // 1. 刷新长期记忆
    refreshLongTermMemory(userId);

    // 2. 更新短期记忆
    updateShortTermMemory(userId, currentContent, currentEmotion, emotionScore);

    // 3. 构建RAG提示词
    EmotionMemory memoryCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        memoryCopy = getOrCreateMemory(userId);
    }
    std::string ragPrompt = buildRAGPrompt(memoryCopy, currentContent, currentEmotion);

    // 4. 调用AIService生成回复（同步等待）
    std::string result;
    std::string aiError;
    std::mutex waitMutex;
    std::condition_variable cv;
    bool done = false;

    auto& aiService = AIService::getInstance();
    aiService.generateReply(currentContent, ragPrompt,
        [&result, &aiError, &waitMutex, &cv, &done](const std::string& reply, const std::string& error) {
            std::lock_guard<std::mutex> lock(waitMutex);
            result = reply;
            aiError = error;
            done = true;
            cv.notify_one();
        }
    );

    // 等待AI回复，最多10秒
    {
        std::unique_lock<std::mutex> lock(waitMutex);
        cv.wait_for(lock, std::chrono::seconds(10), [&done]() { return done; });
    }

    if (!aiError.empty()) {
        LOG_WARN << "DualMemoryRAG AI response error: " << aiError;
    }

    if (result.empty()) {
        result = "我感受到了你此刻的心情，无论如何，你并不孤单。";
    }

    return result;
}

Json::Value DualMemoryRAG::getEmotionInsights(const std::string& userId) {
    // 刷新长期记忆以获取最新数据
    refreshLongTermMemory(userId);

    EmotionMemory memoryCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        memoryCopy = getOrCreateMemory(userId);
    }

    Json::Value insights;
    insights["user_id"] = userId;
    const auto& lt = memoryCopy.longTerm;

    // 情绪画像
    Json::Value profile;
    profile["avg_emotion_score"] = lt.avgEmotionScore;
    profile["total_posts_30d"] = lt.totalPosts;
    profile["dominant_mood"] = lt.dominantMood;
    profile["emotion_volatility"] = lt.emotionVolatility;
    profile["emotion_trend"] = lt.emotionTrend;
    profile["consecutive_negative_days"] = lt.consecutiveNegativeDays;
    profile["last_active_date"] = lt.lastActiveDate;
    insights["profile"] = profile;
    insights["long_term_profile"] = profile;

    // 趋势解读
    std::string trendDescription;
    if (lt.emotionTrend == "rising") {
        trendDescription = "你的情绪状态正在好转，继续保持积极的心态 ✨";
    } else if (lt.emotionTrend == "falling") {
        trendDescription = "最近情绪有些波动，记得给自己一些休息和关爱的时间 💙";
    } else {
        trendDescription = "你的情绪状态比较平稳，这是很好的状态 🌊";
    }
    insights["trend_description"] = trendDescription;

    // 个性化建议
    Json::Value suggestions(Json::arrayValue);
    if (lt.consecutiveNegativeDays >= 3) {
        suggestions.append("连续多天情绪偏低，建议尝试与信任的朋友聊聊，或考虑寻求专业心理咨询");
    }
    if (lt.emotionVolatility > 0.5f) {
        suggestions.append("情绪波动较大，可以尝试正念冥想或写日记来帮助情绪觉察");
    }
    if (lt.totalPosts > 0 && lt.avgEmotionScore > 0.3f) {
        suggestions.append("你的整体情绪状态不错，继续保持分享和表达的习惯");
    }
    if (lt.totalPosts == 0) {
        suggestions.append("欢迎来到心湖，试着投下你的第一颗石头，让心事有个安放的地方");
    }
    insights["suggestions"] = suggestions;

    // 近期交互摘要
    Json::Value recentInteractions(Json::arrayValue);
    for (const auto& entry : memoryCopy.shortTerm) {
        Json::Value item;
        item["emotion"] = entry.emotion;
        item["score"] = entry.score;
        item["timestamp"] = entry.timestamp;
        item["content_preview"] = entry.content.substr(0, 50) + (entry.content.size() > 50 ? "..." : "");
        recentInteractions.append(item);
    }
    insights["recent_interactions"] = recentInteractions;

    return insights;
}

std::string DualMemoryRAG::calculateTrend(const std::vector<float>& scores) {
    if (scores.size() < 2) return "stable";
    size_t mid = scores.size() / 2;
    float firstHalf = std::accumulate(scores.begin(), scores.begin() + mid, 0.0f) / mid;
    float secondHalf = std::accumulate(scores.begin() + mid, scores.end(), 0.0f) / (scores.size() - mid);
    float diff = secondHalf - firstHalf;
    if (diff > 0.15f) return "rising";
    if (diff < -0.15f) return "falling";
    return "stable";
}

float DualMemoryRAG::calculateVolatility(const std::vector<float>& scores) {
    if (scores.size() < 2) return 0.0f;
    float mean = std::accumulate(scores.begin(), scores.end(), 0.0f) / scores.size();
    float variance = 0.0f;
    for (float s : scores) {
        variance += (s - mean) * (s - mean);
    }
    variance /= scores.size();
    return std::sqrt(variance);
}

Json::Value DualMemoryRAG::getStats() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    Json::Value stats;
    stats["active_users"] = static_cast<int>(memories_.size());
    stats["max_short_term_entries"] = MAX_SHORT_TERM;
    stats["long_term_retention_days"] = 30;

    int totalShortTermEntries = 0;
    int usersWithLongTerm = 0;
    float avgEmotionScore = 0.0f;

    for (const auto& [uid, mem] : memories_) {
        totalShortTermEntries += static_cast<int>(mem.shortTerm.size());
        if (mem.longTerm.totalPosts > 0) {
            usersWithLongTerm++;
            avgEmotionScore += mem.longTerm.avgEmotionScore;
        }
    }

    stats["total_short_term_entries"] = totalShortTermEntries;
    stats["users_with_long_term_profile"] = usersWithLongTerm;
    stats["avg_emotion_score"] = usersWithLongTerm > 0
        ? avgEmotionScore / usersWithLongTerm : 0.0f;

    return stats;
}

} // namespace heartlake::ai
