/**
 * @file DualMemoryRAG.cpp
 * @brief 双记忆 RAG 情感守护系统 —— 短期滑动窗口 + 长期情绪画像融合生成
 *
 * 基于 SoulSpeak 论文 (arXiv, Dec 2024) 的双记忆架构：
 *   - 短期记忆：滑动窗口保留最近 MAX_SHORT_TERM 次交互，
 *     淘汰策略融合语义相关度(40%) + 时间衰减(25%) + 访问频率(20%) +
 * 情绪一致(15%)
 *   - 长期记忆：从 stones 表聚合 30 天情绪画像，
 *     含 Ebbinghaus 指数衰减加权分、趋势(7d vs 7-14d)、连续负面天数
 *   - RAG 提示词融合双记忆上下文 + 心理咨询最佳实践指导原则
 *   - 超时快速回退：AI 响应超时(默认3s)时降级到本地陪伴文案生成
 *   - 模板检测：过滤掉模型产出的套话回复，确保个性化
 */

#include "infrastructure/ai/DualMemoryRAG.h"
#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "utils/IdentityShadowMap.h"
#include "utils/RequestHelper.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <drogon/drogon.h>
#include <iomanip>
#include <memory>
#include <numeric>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <vector>

using heartlake::utils::safeRow;

namespace heartlake::ai {

namespace {

float cosineSimilarity(const std::vector<float> &a,
                       const std::vector<float> &b) {
  if (a.empty() || b.empty() || a.size() != b.size()) {
    return 0.0f;
  }

  double dot = 0.0;
  double na = 0.0;
  double nb = 0.0;
  for (size_t i = 0; i < a.size(); ++i) {
    dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
    na += static_cast<double>(a[i]) * static_cast<double>(a[i]);
    nb += static_cast<double>(b[i]) * static_cast<double>(b[i]);
  }
  if (na <= 1e-9 || nb <= 1e-9) {
    return 0.0f;
  }
  return static_cast<float>(dot / (std::sqrt(na) * std::sqrt(nb)));
}

double lexicalHint(const std::string &current, const std::string &history) {
  if (current.empty() || history.empty()) {
    return 0.0;
  }
  const size_t probeLen =
      std::min<size_t>(12, std::min(current.size(), history.size()));
  if (probeLen < 3) {
    return 0.0;
  }
  const std::string probe = history.substr(0, probeLen);
  return current.find(probe) != std::string::npos ? 0.2 : 0.0;
}

std::vector<size_t> selectRelevantShortTermEntries(
    const std::vector<EmotionMemory::ShortTermEntry> &entries,
    const std::string &currentContent, const std::string &currentEmotion,
    const std::vector<float> *currentEmbeddingHint = nullptr) {
  if (entries.empty()) {
    return {};
  }

  std::vector<float> currentEmbedding;
  const std::vector<float> *currentEmbeddingPtr = currentEmbeddingHint;
  bool embeddingReady = currentEmbeddingPtr && !currentEmbeddingPtr->empty();
  if (!embeddingReady) {
    try {
      currentEmbedding =
          AdvancedEmbeddingEngine::getInstance().generateEmbedding(
              currentContent);
      embeddingReady = !currentEmbedding.empty();
      currentEmbeddingPtr = embeddingReady ? &currentEmbedding : nullptr;
    } catch (const std::exception &e) {
      LOG_WARN << "Embedding generation failed: " << e.what();
      embeddingReady = false;
      currentEmbeddingPtr = nullptr;
    }
  }

  struct ScoredEntry {
    size_t index;
    double score;
  };
  std::vector<ScoredEntry> scored;
  scored.reserve(entries.size());

  const auto n = entries.size();
  for (size_t i = 0; i < n; ++i) {
    const auto &entry = entries[i];
    if (entry.content.empty() || entry.content == currentContent) {
      continue;
    }

    // recency: 越新越高（0~0.6）
    const double recencyScore =
        static_cast<double>(i + 1) / static_cast<double>(n);
    double total = recencyScore * 0.6;

    // 情绪一致性加权
    if (!currentEmotion.empty() && entry.emotion == currentEmotion) {
      total += 0.45;
    }

    // 轻量词面提示
    total += lexicalHint(currentContent, entry.content);

    // 语义相关度加权（0~1.2）
    if (embeddingReady) {
      try {
        const std::vector<float> *histEmbedding = nullptr;
        std::vector<float> generatedEmbedding;
        if (!entry.embedding.empty() &&
            entry.embedding.size() == currentEmbeddingPtr->size()) {
          histEmbedding = &entry.embedding;
        } else {
          generatedEmbedding =
              AdvancedEmbeddingEngine::getInstance().generateEmbedding(
                  entry.content);
          histEmbedding = &generatedEmbedding;
        }
        const float semantic =
            cosineSimilarity(*currentEmbeddingPtr, *histEmbedding);
        if (semantic > 0.0f) {
          total += static_cast<double>(semantic) * 1.2;
        }
      } catch (const std::exception &e) {
        LOG_WARN << "Semantic scoring failed: " << e.what();
        // 忽略语义失败，保持轻量回退
      }
    }

    scored.push_back({i, total});
  }

  if (scored.empty()) {
    return {};
  }

  std::sort(scored.begin(), scored.end(),
            [](const ScoredEntry &a, const ScoredEntry &b) {
              return a.score > b.score;
            });

  const size_t keep = std::min<size_t>(3, scored.size());
  std::vector<size_t> picked;
  picked.reserve(keep);
  for (size_t i = 0; i < keep; ++i) {
    picked.push_back(scored[i].index);
  }

  std::sort(picked.begin(), picked.end());
  return picked;
}

size_t stableBucket(const std::string &userId, const std::string &content,
                    const std::string &salt, size_t modulo) {
  if (modulo == 0)
    return 0;
  std::hash<std::string> hasher;
  return hasher(userId + "|" + salt + "|" + content) % modulo;
}

std::string contentHint(const std::string &content) {
  if (content.empty())
    return "";
  std::string hint = content.substr(0, std::min<size_t>(18, content.size()));
  for (char &c : hint) {
    if (c == '\n' || c == '\r' || c == '\t') {
      c = ' ';
    }
  }
  return hint;
}

bool isGenericTemplateReply(const std::string &text) {
  auto normalize = [](const std::string &raw) {
    std::string out;
    out.reserve(raw.size());
    for (unsigned char ch : raw) {
      if (std::isspace(ch)) {
        continue;
      }
      if (ch == ',' || ch == '.' || ch == '!' || ch == '?' || ch == ';' ||
          ch == ':' || ch == '"' || ch == '\'') {
        continue;
      }
      out.push_back(static_cast<char>(std::tolower(ch)));
    }
    return out;
  };

  static const std::vector<std::string> templates = {
      "我感受到了你此刻的心情",         "我感受到你此刻的心情",
      "无论如何，你并不孤单",           "无论如何你并不孤单",
      "我理解你的感受，有什么想聊的吗", "我理解你的感受有什么想聊的吗",
      "我现在有点忙，稍后再聊好吗"};
  const std::string normalized = normalize(text);
  for (const auto &t : templates) {
    const auto normalizedTemplate = normalize(t);
    if (!normalizedTemplate.empty() &&
        normalized.find(normalizedTemplate) != std::string::npos) {
      return true;
    }
  }
  if (text.size() < 36 && (text.find("理解你的感受") != std::string::npos ||
                           text.find("并不孤单") != std::string::npos ||
                           text.find("想聊的吗") != std::string::npos)) {
    return true;
  }
  return false;
}

int ragResponseTimeoutSeconds() {
  static int timeoutSec = []() {
    int value = 3; // 默认快速回退，避免前端长时间卡住
    if (const char *raw = std::getenv("RAG_RESPONSE_TIMEOUT_SEC")) {
      try {
        value = std::stoi(raw);
      } catch (...) {
        value = 3;
      }
    }
    return std::clamp(value, 1, 10);
  }();
  return timeoutSec;
}

double currentEpochSeconds() {
  return static_cast<double>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

std::string formatLocalTimestamp(std::time_t timeValue) {
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &timeValue);
#else
  localtime_r(&timeValue, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

const std::string &stoneScoreSql() {
  static const std::string kSql = "COALESCE(emotion_score, sentiment_score, "
                                  "  CASE COALESCE(mood_type, 'neutral') "
                                  "    WHEN 'happy' THEN 0.70 "
                                  "    WHEN 'calm' THEN 0.35 "
                                  "    WHEN 'grateful' THEN 0.60 "
                                  "    WHEN 'hopeful' THEN 0.45 "
                                  "    WHEN 'neutral' THEN 0.0 "
                                  "    WHEN 'confused' THEN -0.10 "
                                  "    WHEN 'anxious' THEN -0.45 "
                                  "    WHEN 'sad' THEN -0.65 "
                                  "    WHEN 'angry' THEN -0.55 "
                                  "    WHEN 'lonely' THEN -0.50 "
                                  "    ELSE 0.0 END"
                                  ")";
  return kSql;
}

std::string
buildLocalCompanionReply(const EmotionMemory::LongTermProfile &profile,
                         const std::string &userId, const std::string &content,
                         const std::string &emotion, float score) {
  const bool isLowMood = score < -0.35f || emotion == "sad" ||
                         emotion == "anxious" || emotion == "lonely";
  const bool isHighMood = score > 0.35f || emotion == "happy" ||
                          emotion == "grateful" || emotion == "hopeful";

  std::vector<std::string> empathy;
  if (isLowMood) {
    empathy = {"这段心情我认真看到了，先抱抱你。",
               "你愿意把这些说出来，本身就很不容易。",
               "看到你的描述，我能感到你正在扛着不小的压力。"};
  } else if (isHighMood) {
    empathy = {"从你的表达里能感觉到一些亮光，这很珍贵。",
               "你现在的状态比前段时间更有力量了。",
               "这种积极感受值得被好好记住。"};
  } else {
    empathy = {"谢谢你把这段感受交给我。", "你说得很真诚，我在认真听。",
               "你的心情变化我记下了。"};
  }

  std::vector<std::string> trendCare;
  if (profile.emotionTrend == "falling") {
    trendCare = {"最近状态有些下滑，我们先把节奏放慢一点。",
                 "这几天波动偏低，先把目标定得更小、更稳。",
                 "如果连续几天都很累，先照顾睡眠和饮食会更重要。"};
  } else if (profile.emotionTrend == "rising") {
    trendCare = {"你已经在慢慢回升，继续保持这个步幅就很好。",
                 "这段时间有在变好，说明你的方法是有效的。",
                 "你正在往更稳的方向走，可以给自己一点肯定。"};
  } else {
    trendCare = {"先从今天最在意的一件小事开始梳理就够了。",
                 "我们可以先把最困扰你的那一段拆开来说。",
                 "你不需要一下子解决全部问题，先照顾当下就好。"};
  }

  std::vector<std::string> action;
  if (isLowMood) {
    action = {"如果愿意，我可以陪你一起把\"现在最难的点\"说清楚。",
              "你可以先做一个30秒深呼吸，再告诉我此刻最重的一件事。",
              "先把这件事分成两步，我和你一起看第一步。"};
  } else {
    action = {"你愿意继续说说这一段里最关键的细节吗？",
              "我们可以把这个变化记录下来，帮你找到可重复的方法。",
              "如果你想，我可以继续陪你把下一步想清楚。"};
  }

  const size_t eIdx = stableBucket(userId, content, "empathy", empathy.size());
  const size_t tIdx = stableBucket(userId, content, "trend", trendCare.size());
  const size_t aIdx = stableBucket(userId, content, "action", action.size());

  std::ostringstream oss;
  oss << empathy[eIdx] << trendCare[tIdx];

  const auto hint = contentHint(content);
  if (!hint.empty() && stableBucket(userId, content, "hint", 2) == 0) {
    oss << "你刚刚提到“" << hint << "”，这件事对你很关键。";
  }

  oss << action[aIdx];
  return oss.str();
}

} // namespace

DualMemoryRAG &DualMemoryRAG::getInstance() {
  static DualMemoryRAG instance;
  return instance;
}

std::string DualMemoryRAG::resolveShadowUserId(const std::string &userId) {
  return heartlake::utils::IdentityShadowMap::getInstance()
      .getOrCreateShadowId(userId);
}

EmotionMemory &DualMemoryRAG::getOrCreateMemory(
    const std::string &shadowUserId) {
  auto [it, inserted] = memories_.try_emplace(shadowUserId);
  if (inserted) {
    it->second.userId = shadowUserId;
  }
  return it->second;
}

EmotionMemory DualMemoryRAG::getMemorySnapshot(const std::string &shadowUserId) {
  std::shared_lock<std::shared_mutex> readLock(mutex_);
  auto it = memories_.find(shadowUserId);
  if (it != memories_.end()) {
    return it->second;
  }

  readLock.unlock();
  std::unique_lock<std::shared_mutex> writeLock(mutex_);
  return getOrCreateMemory(shadowUserId);
}

void DualMemoryRAG::updateShortTermMemory(const std::string &userId,
                                          const std::string &content,
                                          const std::string &emotion,
                                          float score) {
  std::vector<float> contentEmbedding;
  if (!content.empty()) {
    try {
      contentEmbedding =
          AdvancedEmbeddingEngine::getInstance().generateEmbedding(content);
    } catch (const std::exception &e) {
      LOG_WARN << "Short-term embedding generation failed: " << e.what();
    }
  }

  updateShortTermMemoryInternal(userId, content, emotion, score,
                                contentEmbedding);
}

void DualMemoryRAG::updateShortTermMemoryInternal(
    const std::string &userId, const std::string &content,
    const std::string &emotion, float score,
    const std::vector<float> &contentEmbedding) {
  const std::string shadowUserId = resolveShadowUserId(userId);
  std::unique_lock<std::shared_mutex> lock(mutex_);
  auto &memory = getOrCreateMemory(shadowUserId);

  // 生成时间戳
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  EmotionMemory::ShortTermEntry entry;
  entry.content = content;
  entry.emotion = emotion;
  entry.score = score;
  entry.timestamp = formatLocalTimestamp(time_t_now);
  entry.embedding = contentEmbedding;
  entry.accessCount = 0;
  entry.lastAccessTime = currentEpochSeconds();

  memory.shortTerm.push_back(std::move(entry));

  // 基于相关性淘汰：保留与当前对话最相关的记忆
  if (static_cast<int>(memory.shortTerm.size()) > MAX_SHORT_TERM) {
    evictLeastRelevant(memory.shortTerm, content, emotion, &contentEmbedding);
  }
}

void DualMemoryRAG::evictLeastRelevant(
    std::vector<EmotionMemory::ShortTermEntry> &entries,
    const std::string &currentContent, const std::string &currentEmotion,
    const std::vector<float> *currentEmbeddingHint) {
  if (entries.size() <= static_cast<size_t>(MAX_SHORT_TERM)) {
    return;
  }

  // 为每条记忆计算保留分数（不含最新条目，最新条目始终保留）
  const size_t lastIdx = entries.size() - 1;

  // 尝试获取当前内容的嵌入
  std::vector<float> currentEmbedding;
  const std::vector<float> *currentEmbeddingPtr = currentEmbeddingHint;
  bool embeddingReady = currentEmbeddingPtr && !currentEmbeddingPtr->empty();
  if (!embeddingReady) {
    try {
      currentEmbedding =
          AdvancedEmbeddingEngine::getInstance().generateEmbedding(
              currentContent);
      embeddingReady = !currentEmbedding.empty();
      currentEmbeddingPtr = embeddingReady ? &currentEmbedding : nullptr;
    } catch (const std::exception &e) {
      LOG_WARN << "Embedding generation failed: " << e.what();
      embeddingReady = false;
      currentEmbeddingPtr = nullptr;
    }
  }

  auto now = std::chrono::system_clock::now();
  double nowEpoch = static_cast<double>(
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count());

  // 评分权重: alpha=0.4(语义), beta=0.25(时间衰减), gamma=0.2(访问频率),
  // delta=0.15(情绪一致)
  constexpr double alpha = 0.4;
  constexpr double beta = 0.25;
  constexpr double gamma = 0.2;
  constexpr double delta = 0.15;

  double worstScore = std::numeric_limits<double>::max();
  size_t worstIdx = 0;

  for (size_t i = 0; i < lastIdx; ++i) {
    const auto &e = entries[i];
    double retainScore = 0.0;

    // 语义相关度
    if (embeddingReady) {
      try {
        const std::vector<float> *emb = nullptr;
        std::vector<float> generatedEmbedding;
        if (!e.embedding.empty() &&
            e.embedding.size() == currentEmbeddingPtr->size()) {
          emb = &e.embedding;
        } else {
          generatedEmbedding =
              AdvancedEmbeddingEngine::getInstance().generateEmbedding(
                  e.content);
          emb = &generatedEmbedding;
        }
        float sim = cosineSimilarity(*currentEmbeddingPtr, *emb);
        retainScore += alpha * std::max(0.0f, sim);
      } catch (const std::exception &ex) {
        LOG_WARN << "Embedding similarity failed: " << ex.what();
      }
    }
    // 词面回退
    retainScore += alpha * lexicalHint(currentContent, e.content);

    // 时间衰减: 越近越高 (指数衰减，以小时为单位)
    double hoursElapsed = std::max(0.0, (nowEpoch - e.lastAccessTime) / 3600.0);
    retainScore += beta * std::exp(-0.1 * hoursElapsed);

    // 访问频率: 被检索越多越重要 (归一化到0~1)
    retainScore +=
        gamma * std::min(1.0, static_cast<double>(e.accessCount) / 5.0);

    // 情绪一致性
    if (!currentEmotion.empty() && e.emotion == currentEmotion) {
      retainScore += delta;
    }

    if (retainScore < worstScore) {
      worstScore = retainScore;
      worstIdx = i;
    }
  }

  entries.erase(entries.begin() + static_cast<ptrdiff_t>(worstIdx));
}

void DualMemoryRAG::markShortTermEntriesAccessed(
    const std::string &userId, const std::vector<size_t> &indices) {
  if (indices.empty()) {
    return;
  }

  const std::string shadowUserId = resolveShadowUserId(userId);
  std::unique_lock<std::shared_mutex> lock(mutex_);
  auto it = memories_.find(shadowUserId);
  if (it == memories_.end()) {
    return;
  }

  auto &entries = it->second.shortTerm;
  const double nowEpoch = currentEpochSeconds();
  for (size_t index : indices) {
    if (index >= entries.size()) {
      continue;
    }
    auto &entry = entries[index];
    entry.accessCount += 1;
    entry.lastAccessTime = nowEpoch;
  }
}

void DualMemoryRAG::refreshLongTermMemory(const std::string &userId) {
  refreshLongTermMemoryImpl(userId, true, currentEpochSeconds());
}

void DualMemoryRAG::refreshLongTermMemoryImpl(const std::string &userId,
                                              bool forceRefresh,
                                              double nowEpoch) {
  const std::string shadowUserId = resolveShadowUserId(userId);
  {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto &memory = getOrCreateMemory(shadowUserId);
    auto &longTerm = memory.longTerm;
    if (longTerm.refreshInFlight) {
      return;
    }
    if (!forceRefresh && longTerm.lastRefreshTime > 0.0 &&
        (nowEpoch - longTerm.lastRefreshTime) <
            LONG_TERM_REFRESH_INTERVAL_SECONDS) {
      return;
    }
    longTerm.refreshInFlight = true;
  }

  EmotionMemory::LongTermProfile refreshedProfile;
  bool refreshSucceeded = false;

  try {
    auto db = drogon::app().getDbClient("default");
    if (!db) {
      throw std::runtime_error("default db client unavailable");
    }

    std::ostringstream sql;
    sql << "WITH scored AS ("
        << "  SELECT " << stoneScoreSql() << " AS score, "
        << "         COALESCE(mood_type, 'neutral') AS mood_type, "
        << "         created_at, "
        << "         DATE(created_at) AS day_bucket "
        << "  FROM stones "
        << "  WHERE user_id = $1 "
        << "    AND created_at > NOW() - INTERVAL '" << LONG_TERM_RETENTION_DAYS
        << " days' "
        << "    AND deleted_at IS NULL"
        << "), aggregate_stats AS ("
        << "  SELECT COUNT(*) AS total_posts, "
        << "         AVG(score) AS avg_score, "
        << "         STDDEV(score) AS score_stddev, "
        << "         MAX(created_at)::text AS last_active, "
        << "         SUM(score * EXP(-" << DECAY_LAMBDA
        << " * EXTRACT(EPOCH FROM (NOW() - created_at)) / 86400.0)) / "
        << "           NULLIF(SUM(EXP(-" << DECAY_LAMBDA
        << " * EXTRACT(EPOCH FROM (NOW() - created_at)) / 86400.0)), 0) AS "
           "decay_score "
        << "  FROM scored"
        << "), dominant_mood AS ("
        << "  SELECT mood_type "
        << "  FROM scored "
        << "  GROUP BY mood_type "
        << "  ORDER BY COUNT(*) DESC, mood_type ASC "
        << "  LIMIT 1"
        << "), trend_stats AS ("
        << "  SELECT AVG(score) FILTER (WHERE created_at > NOW() - INTERVAL '7 "
           "days') AS recent_avg, "
        << "         AVG(score) FILTER ("
        << "             WHERE created_at <= NOW() - INTERVAL '7 days' "
        << "               AND created_at > NOW() - INTERVAL '14 days'"
        << "         ) AS prev_avg "
        << "  FROM scored "
        << "  WHERE created_at > NOW() - INTERVAL '14 days'"
        << "), daily_scores AS ("
        << "  SELECT day_bucket, AVG(score) AS day_avg "
        << "  FROM scored "
        << "  WHERE created_at > NOW() - INTERVAL '14 days' "
        << "  GROUP BY day_bucket"
        << "), ordered_days AS ("
        << "  SELECT day_bucket, day_avg, "
        << "         SUM(CASE WHEN day_avg < -0.2 THEN 0 ELSE 1 END) OVER ("
        << "             ORDER BY day_bucket DESC "
        << "             ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW"
        << "         ) AS non_negative_seen "
        << "  FROM daily_scores"
        << "), negative_streak AS ("
        << "  SELECT COUNT(*) AS consecutive_negative_days "
        << "  FROM ordered_days "
        << "  WHERE day_avg < -0.2 AND non_negative_seen = 0"
        << ") "
        << "SELECT COALESCE(a.total_posts, 0) AS total_posts, "
        << "       a.avg_score, "
        << "       a.score_stddev, "
        << "       a.last_active, "
        << "       a.decay_score, "
        << "       COALESCE(d.mood_type, 'neutral') AS dominant_mood, "
        << "       t.recent_avg, "
        << "       t.prev_avg, "
        << "       COALESCE(n.consecutive_negative_days, 0) AS "
           "consecutive_negative_days "
        << "FROM aggregate_stats a "
        << "LEFT JOIN dominant_mood d ON TRUE "
        << "LEFT JOIN trend_stats t ON TRUE "
        << "LEFT JOIN negative_streak n ON TRUE";

    auto result = db->execSqlSync(sql.str(), userId);

    if (auto rowOpt = safeRow(result)) {
      auto &row = *rowOpt;
      refreshedProfile.totalPosts = row["total_posts"].as<int>();
      refreshedProfile.avgEmotionScore =
          row["avg_score"].isNull() ? 0.0f : row["avg_score"].as<float>();
      refreshedProfile.decayWeightedScore =
          row["decay_score"].isNull() ? 0.0f : row["decay_score"].as<float>();
      refreshedProfile.emotionVolatility =
          row["score_stddev"].isNull() ? 0.0f : row["score_stddev"].as<float>();
      refreshedProfile.lastActiveDate =
          row["last_active"].isNull() ? ""
                                      : row["last_active"].as<std::string>();
      refreshedProfile.dominantMood =
          row["dominant_mood"].isNull()
              ? "neutral"
              : row["dominant_mood"].as<std::string>();

      const float recentAvg =
          row["recent_avg"].isNull() ? 0.0f : row["recent_avg"].as<float>();
      const float prevAvg =
          row["prev_avg"].isNull() ? 0.0f : row["prev_avg"].as<float>();
      float diff = recentAvg - prevAvg;
      if (diff > 0.15f)
        refreshedProfile.emotionTrend = "rising";
      else if (diff < -0.15f)
        refreshedProfile.emotionTrend = "falling";
      else
        refreshedProfile.emotionTrend = "stable";
      refreshedProfile.consecutiveNegativeDays =
          row["consecutive_negative_days"].isNull()
              ? 0
              : row["consecutive_negative_days"].as<int>();
    }
    refreshedProfile.lastRefreshTime = currentEpochSeconds();
    refreshSucceeded = true;
  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "DualMemoryRAG::refreshLongTermMemory failed: "
              << e.base().what();
  } catch (const std::exception &e) {
    LOG_ERROR << "DualMemoryRAG::refreshLongTermMemory failed: " << e.what();
  }

  std::unique_lock<std::shared_mutex> lock(mutex_);
  auto &memory = getOrCreateMemory(shadowUserId);
  if (refreshSucceeded) {
    refreshedProfile.refreshInFlight = false;
    memory.longTerm = std::move(refreshedProfile);
  } else {
    memory.longTerm.refreshInFlight = false;
  }
}

std::string
DualMemoryRAG::buildRAGPrompt(const EmotionMemory &memory,
                              const std::string &currentContent,
                              const std::string &currentEmotion,
                              const std::vector<size_t> *selectedShortTerm) {
  std::ostringstream prompt;

  prompt << "你是「心湖湖神」，一位具有专业心理咨询素养的情感守护者。\n\n";

  // === 长期记忆上下文 ===
  prompt << "【用户情绪画像 - 长期记忆(结构化)】\n";
  const auto &lt = memory.longTerm;
  if (lt.totalPosts > 0) {
    prompt << "- posts_30d: " << lt.totalPosts << "\n";
    prompt << "- avg_score: " << std::fixed << std::setprecision(2)
           << lt.avgEmotionScore << "\n";
    prompt << "- dominant_mood: " << lt.dominantMood << "\n";
    prompt << "- volatility: " << std::fixed << std::setprecision(2)
           << lt.emotionVolatility << "\n";
    prompt << "- trend: ";
    if (lt.emotionTrend == "rising")
      prompt << "好转中 ↑\n";
    else if (lt.emotionTrend == "falling")
      prompt << "下降中 ↓\n";
    else
      prompt << "平稳 →\n";
    prompt << "- consecutive_negative_days: " << lt.consecutiveNegativeDays
           << "\n";
  } else {
    prompt << "- new_user: true\n";
  }

  // === 短期记忆上下文 ===
  prompt << "\n【近期交互 - 短期记忆(相关检索)】\n";
  std::vector<size_t> computedShortTerm;
  if (!selectedShortTerm) {
    computedShortTerm = selectRelevantShortTermEntries(
        memory.shortTerm, currentContent, currentEmotion);
    selectedShortTerm = &computedShortTerm;
  }
  const auto &pickedShortTerm = *selectedShortTerm;
  if (pickedShortTerm.empty()) {
    prompt << "- 暂无可复用的高相关历史\n";
  } else {
    prompt << "- 已检索到 " << pickedShortTerm.size() << " 条高相关历史\n";
    for (size_t i = 0; i < pickedShortTerm.size(); ++i) {
      const auto &entry = memory.shortTerm[pickedShortTerm[i]];
      prompt << (i + 1) << ". [" << entry.emotion << " / " << std::fixed
             << std::setprecision(1) << entry.score << "] "
             << entry.content.substr(0, 80);
      if (entry.content.size() > 80)
        prompt << "...";
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

RagReplyResult DualMemoryRAG::generateResponseResult(
    const std::string &userId, const std::string &currentContent,
    const std::string &currentEmotion, float emotionScore) {
  const std::string shadowUserId = resolveShadowUserId(userId);
  // 1. 节流刷新长期记忆，避免每次请求都刷库
  refreshLongTermMemoryImpl(userId, false, currentEpochSeconds());

  std::vector<float> currentEmbedding;
  if (!currentContent.empty()) {
    try {
      currentEmbedding =
          AdvancedEmbeddingEngine::getInstance().generateEmbedding(
              currentContent);
    } catch (const std::exception &e) {
      LOG_WARN << "DualMemoryRAG current embedding generation failed: "
               << e.what();
    }
  }

  // 2. 更新短期记忆
  updateShortTermMemoryInternal(userId, currentContent, currentEmotion,
                                emotionScore, currentEmbedding);

  // 3. 构建RAG提示词
  EmotionMemory memoryCopy = getMemorySnapshot(shadowUserId);
  const auto pickedShortTerm = selectRelevantShortTermEntries(
      memoryCopy.shortTerm, currentContent, currentEmotion, &currentEmbedding);
  markShortTermEntriesAccessed(userId, pickedShortTerm);
  std::string ragPrompt = buildRAGPrompt(memoryCopy, currentContent,
                                         currentEmotion, &pickedShortTerm);

  // 4. 调用AIService生成回复（同步等待）
  struct ReplyWaitState {
    std::mutex mutex;
    std::condition_variable cv;
    std::string result;
    std::string error;
    bool done = false;
    bool abandoned = false;
  };
  auto waitState = std::make_shared<ReplyWaitState>();
  std::string result;
  std::string aiError;
  RagReplyResult replyResult;

  auto &aiService = AIService::getInstance();
  aiService.generateReply(
      currentContent, ragPrompt,
      [waitState](const std::string &reply, const std::string &error) {
        std::lock_guard<std::mutex> lock(waitState->mutex);
        if (waitState->done || waitState->abandoned) {
          return;
        }
        waitState->result = reply;
        waitState->error = error;
        waitState->done = true;
        waitState->cv.notify_one();
      });

  // 等待回复（默认3秒），超时快速回退到本地陪伴文案
  bool completed = false;
  const int timeoutSec = ragResponseTimeoutSeconds();
  {
    std::unique_lock<std::mutex> lock(waitState->mutex);
    completed =
        waitState->cv.wait_for(lock, std::chrono::seconds(timeoutSec),
                               [waitState]() { return waitState->done; });
    if (completed) {
      result = waitState->result;
      aiError = waitState->error;
    } else {
      // 超时后忽略迟到回调，避免异步写入已完成流程。
      waitState->abandoned = true;
    }
  }

  if (!completed) {
    aiError = "AI response timed out";
    LOG_WARN << "DualMemoryRAG AI response timeout(" << timeoutSec
             << "s), fallback to local companion reply";
  }

  if (!aiError.empty()) {
    LOG_WARN << "DualMemoryRAG AI response error: " << aiError;
  }

  if (!completed || !aiError.empty() || result.empty() ||
      isGenericTemplateReply(result)) {
    replyResult.reply =
        buildLocalCompanionReply(memoryCopy.longTerm, shadowUserId, currentContent,
                                 currentEmotion, emotionScore);
    replyResult.source = "local_fallback";
    replyResult.degraded = true;
    if (!aiError.empty()) {
      replyResult.error = aiError;
    } else if (result.empty()) {
      replyResult.error = "AI response is empty";
    } else {
      replyResult.error = "AI response is generic";
    }
    return replyResult;
  }

  replyResult.reply = result;
  return replyResult;
}

std::string DualMemoryRAG::generateResponse(const std::string &userId,
                                            const std::string &currentContent,
                                            const std::string &currentEmotion,
                                            float emotionScore) {
  return generateResponseResult(userId, currentContent, currentEmotion,
                                emotionScore)
      .reply;
}

Json::Value DualMemoryRAG::getEmotionInsights(const std::string &userId) {
  const std::string shadowUserId = resolveShadowUserId(userId);
  // 节流刷新长期记忆，避免 insights 频繁打 DB
  refreshLongTermMemoryImpl(userId, false, currentEpochSeconds());

  EmotionMemory memoryCopy = getMemorySnapshot(shadowUserId);

  Json::Value insights;
  insights["user_id"] = userId;
  insights["shadow_id"] = shadowUserId;
  insights["memory_user_id"] = shadowUserId;
  const auto &lt = memoryCopy.longTerm;

  // 情绪画像
  Json::Value profile;
  profile["avg_emotion_score"] = lt.avgEmotionScore;
  profile["decay_weighted_score"] = lt.decayWeightedScore;
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
    suggestions.append(
        "连续多天情绪偏低，建议尝试与信任的朋友聊聊，或考虑寻求专业心理咨询");
  }
  if (lt.emotionVolatility > 0.5f) {
    suggestions.append("情绪波动较大，可以尝试正念冥想或写日记来帮助情绪觉察");
  }
  if (lt.totalPosts > 0 && lt.avgEmotionScore > 0.3f) {
    suggestions.append("你的整体情绪状态不错，继续保持分享和表达的习惯");
  }
  if (lt.totalPosts == 0) {
    suggestions.append(
        "欢迎来到心湖，试着投下你的第一颗石头，让心事有个安放的地方");
  }
  insights["suggestions"] = suggestions;

  // 近期交互摘要
  Json::Value recentInteractions(Json::arrayValue);
  for (const auto &entry : memoryCopy.shortTerm) {
    Json::Value item;
    item["emotion"] = entry.emotion;
    item["score"] = entry.score;
    item["timestamp"] = entry.timestamp;
    item["content_preview"] =
        entry.content.substr(0, 50) + (entry.content.size() > 50 ? "..." : "");
    recentInteractions.append(item);
  }
  insights["recent_interactions"] = recentInteractions;

  return insights;
}

std::string DualMemoryRAG::calculateTrend(const std::vector<float> &scores) {
  if (scores.size() < 2)
    return "stable";
  size_t mid = scores.size() / 2;
  float firstHalf =
      std::accumulate(scores.begin(), scores.begin() + mid, 0.0f) / mid;
  float secondHalf = std::accumulate(scores.begin() + mid, scores.end(), 0.0f) /
                     (scores.size() - mid);
  float diff = secondHalf - firstHalf;
  if (diff > 0.15f)
    return "rising";
  if (diff < -0.15f)
    return "falling";
  return "stable";
}

float DualMemoryRAG::calculateVolatility(const std::vector<float> &scores) {
  if (scores.size() < 2)
    return 0.0f;
  float mean =
      std::accumulate(scores.begin(), scores.end(), 0.0f) / scores.size();
  float variance = 0.0f;
  for (float s : scores) {
    variance += (s - mean) * (s - mean);
  }
  variance /= (scores.size() - 1); // 样本方差（Bessel 修正）
  return std::sqrt(variance);
}

Json::Value DualMemoryRAG::getStats() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  Json::Value stats;
  stats["active_users"] = static_cast<int>(memories_.size());
  stats["max_short_term_entries"] = MAX_SHORT_TERM;
  stats["long_term_retention_days"] = LONG_TERM_RETENTION_DAYS;
  stats["long_term_refresh_interval_seconds"] =
      LONG_TERM_REFRESH_INTERVAL_SECONDS;

  int totalShortTermEntries = 0;
  int usersWithLongTerm = 0;
  float avgEmotionScore = 0.0f;

  for (const auto &[uid, mem] : memories_) {
    totalShortTermEntries += static_cast<int>(mem.shortTerm.size());
    if (mem.longTerm.totalPosts > 0) {
      usersWithLongTerm++;
      avgEmotionScore += mem.longTerm.avgEmotionScore;
    }
  }

  stats["total_short_term_entries"] = totalShortTermEntries;
  stats["users_with_long_term_profile"] = usersWithLongTerm;
  stats["avg_emotion_score"] =
      usersWithLongTerm > 0 ? avgEmotionScore / usersWithLongTerm : 0.0f;

  return stats;
}

} // namespace heartlake::ai
