/**
 * AdvancedEmbeddingEngine 模块实现
 */

#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "utils/EnvUtils.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <cmath>
#include <thread>

using namespace drogon;

namespace heartlake {
namespace ai {

AdvancedEmbeddingEngine& AdvancedEmbeddingEngine::getInstance() {
    static AdvancedEmbeddingEngine instance;
    return instance;
}

void AdvancedEmbeddingEngine::initialize(size_t embeddingDim, size_t cacheSize) {
    if (initialized_) {
        LOG_WARN << "AdvancedEmbeddingEngine already initialized";
        return;
    }

    embeddingDim_ = embeddingDim;
    cacheSize_ = cacheSize;
    embeddingCache_ = std::make_unique<LRUCache>(cacheSize);

    LOG_INFO << "Initializing AdvancedEmbeddingEngine with embedding_dim=" << embeddingDim
             << ", cache_size=" << cacheSize;

    // 加载情感词典
    loadSentimentLexicon();

    initialized_ = true;

    // 应用启动前无需预热训练语料
    if (!app().isRunning()) {
        LOG_INFO << "Drogon app not running, skipping embedding warmup";
        LOG_INFO << "AdvancedEmbeddingEngine initialized successfully";
        return;
    }

    const bool warmupOnBoot = heartlake::utils::parseBoolEnv(std::getenv("EMBEDDING_WARMUP_ON_BOOT"), false);
    if (warmupOnBoot) {
        warmupTrainingDataAsync();
    } else {
        LOG_INFO << "Embedding warmup disabled by default (set EMBEDDING_WARMUP_ON_BOOT=true to enable)";
    }

    LOG_INFO << "AdvancedEmbeddingEngine initialized successfully";
}

std::vector<std::string> AdvancedEmbeddingEngine::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::string current;

    for (size_t i = 0; i < text.length(); ++i) {
        unsigned char c = text[i];

        // 处理UTF-8四字节字符（emoji等）: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        if ((c & 0xF8) == 0xF0 && i + 3 < text.length()) {
            unsigned char c2 = text[i + 1];
            unsigned char c3 = text[i + 2];
            unsigned char c4 = text[i + 3];
            if ((c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80 && (c4 & 0xC0) == 0x80) {
                current = text.substr(i, 4);
                tokens.push_back(current);
                current.clear();
                i += 3;
                continue;
            }
        }
        // 处理UTF-8三字节字符（中文等）: 1110xxxx 10xxxxxx 10xxxxxx
        if ((c & 0xF0) == 0xE0 && i + 2 < text.length()) {
            unsigned char c2 = text[i + 1];
            unsigned char c3 = text[i + 2];
            // 验证后续字节是否为10xxxxxx格式
            if ((c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80) {
                current = text.substr(i, 3);
                tokens.push_back(current);
                current.clear();
                i += 2;
            }
        }
        // 处理ASCII字母数字
        else if (std::isalnum(c)) {
            current += c;
        }
        // 分词边界
        else if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

std::vector<std::string> AdvancedEmbeddingEngine::extractNGrams(
    const std::vector<std::string>& tokens, int n
) const {
    std::vector<std::string> ngrams;
    if (tokens.size() < static_cast<size_t>(n)) {
        return ngrams;
    }

    ngrams.reserve(tokens.size() - n + 1);
    for (size_t i = 0; i <= tokens.size() - n; ++i) {
        size_t totalLen = static_cast<size_t>(n - 1); // spaces
        for (int j = 0; j < n; ++j) totalLen += tokens[i + j].size();
        std::string ngram;
        ngram.reserve(totalLen);
        for (int j = 0; j < n; ++j) {
            if (j > 0) ngram += ' ';
            ngram.append(tokens[i + j]);
        }
        ngrams.push_back(std::move(ngram));
    }

    return ngrams;
}

std::pair<size_t, int> AdvancedEmbeddingEngine::featureHash(const std::string& feature, size_t numBuckets) const {
    // MurmurHash3风格的哈希函数 - 主哈希决定桶位置
    size_t hash = 0;
    const size_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;

    for (char c : feature) {
        hash ^= static_cast<size_t>(c);
        hash *= m;
        hash ^= (hash >> r);
    }

    // 第二哈希决定符号 (Weinberger et al., ICML 2009)
    // 使冲突的期望偏差为零
    size_t hash2 = hash ^ 0x9e3779b97f4a7c15ULL;
    hash2 *= 0xbf58476d1ce4e5b9ULL;
    hash2 ^= (hash2 >> 31);
    int sign = (hash2 & 1) ? 1 : -1;

    return {hash % numBuckets, sign};
}

std::vector<float> AdvancedEmbeddingEngine::extractTFIDFFeatures(
    const std::vector<std::string>& tokens
) const {
    size_t tfidfDim = embeddingDim_ * 60 / 100; // 60%维度用于TF-IDF
    std::vector<float> features(tfidfDim, 0.0f);

    if (tokens.empty()) {
        return features;
    }

    // 计算词频
    std::unordered_map<std::string, int> termFreq;
    for (const auto& token : tokens) {
        termFreq[token]++;
    }

    // 计算TF-IDF并使用特征哈希
    std::shared_lock<std::shared_mutex> lock(idfMutex_);
    for (const auto& [word, freq] : termFreq) {
        float tf = static_cast<float>(freq) / tokens.size();

        // 获取IDF权重
        float idf = 1.0f;
        auto it = idfWeights_.find(word);
        if (it != idfWeights_.end()) {
            idf = it->second;
        }

        float tfidf = tf * idf;

        // 签名特征哈希：将词映射到固定维度，符号减少冲突偏差
        auto [idx, sign] = featureHash(word, tfidfDim);
        features[idx] += sign * tfidf;
    }

    return features;
}

std::vector<float> AdvancedEmbeddingEngine::extractSentimentFeatures(
    const std::vector<std::string>& tokens
) const {
    std::vector<float> features(15, 0.0f); // 15维情感特征

    float positiveScore = 0.0f;
    float negativeScore = 0.0f;
    int matchCount = 0;

    for (const auto& token : tokens) {
        auto it = sentimentLexicon_.find(token);
        if (it != sentimentLexicon_.end()) {
            float score = it->second;
            if (score > 0) {
                positiveScore += score;
            } else {
                negativeScore += std::abs(score);
            }
            matchCount++;
        }
    }

    if (matchCount > 0 && !tokens.empty()) {
        features[0] = positiveScore / matchCount;                    // 平均正面分数
        features[1] = negativeScore / matchCount;                    // 平均负面分数
        features[2] = (positiveScore - negativeScore) / matchCount;  // 情感极性
        features[3] = static_cast<float>(matchCount) / tokens.size(); // 情感词密度
        features[4] = std::tanh(positiveScore);                      // 正面强度
        features[5] = std::tanh(negativeScore);                      // 负面强度
        features[6] = positiveScore > negativeScore ? 1.0f : 0.0f;   // 主导情感
        features[7] = std::abs(positiveScore - negativeScore);       // 情感对比度
    }

    // 情感波动性
    if (matchCount > 1) {
        float meanScore = (positiveScore - negativeScore) / matchCount;
        float variance = 0.0f;
        for (const auto& token : tokens) {
            auto it = sentimentLexicon_.find(token);
            if (it != sentimentLexicon_.end()) {
                float diff = it->second - meanScore;
                variance += diff * diff;
            }
        }
        features[8] = std::sqrt(variance / matchCount);
    }

    // 情感复杂度特征
    features[9] = (positiveScore > 0 && negativeScore > 0) ?
                  std::min(positiveScore, negativeScore) / std::max(positiveScore, negativeScore) : 0.0f;
    features[10] = std::tanh(positiveScore + negativeScore);  // 总情感强度

    return features;
}

std::vector<float> AdvancedEmbeddingEngine::extractStatisticalFeatures(
    const std::vector<std::string>& tokens,
    const std::string& text
) const {
    std::vector<float> features(15, 0.0f); // 15维统计特征

    if (tokens.empty()) {
        return features;
    }

    // 文本长度特征
    features[0] = static_cast<float>(std::log1p(tokens.size())) / 10.0f;
    features[1] = static_cast<float>(std::log1p(text.length())) / 100.0f;

    // 一次遍历同时计算词频、平均词长（freq map 的 size 即 unique count）
    std::unordered_map<std::string, int> freq;
    float totalWordLen = 0.0f;
    for (const auto& token : tokens) {
        freq[token]++;
        totalWordLen += token.length();
    }

    // 词汇多样性（用 freq.size() 代替 uniqueWords.size()）
    features[2] = static_cast<float>(freq.size()) / tokens.size();

    // 平均词长
    features[3] = totalWordLen / tokens.size() / 10.0f;

    // 词频分布熵 + 最高词频 + 单次词比例（一次遍历 freq）
    float entropy = 0.0f;
    int maxFreq = 0;
    int singletonCount = 0;
    for (const auto& [word, count] : freq) {
        float p = static_cast<float>(count) / tokens.size();
        entropy -= p * std::log2(p + 1e-10f);
        maxFreq = std::max(maxFreq, count);
        if (count == 1) singletonCount++;
    }
    features[4] = entropy / 10.0f;
    features[5] = static_cast<float>(maxFreq) / tokens.size();
    features[6] = static_cast<float>(singletonCount) / freq.size();

    return features;
}

std::vector<float> AdvancedEmbeddingEngine::extractNGramFeatures(
    const std::vector<std::string>& tokens
) const {
    size_t ngramDim = embeddingDim_ * 10 / 100; // 10%维度用于N-gram
    std::vector<float> features(ngramDim, 0.0f);

    // 内联滑窗哈希常量（与 featureHash 完全一致）
    constexpr size_t m = 0xc6a4a7935bd1e995ULL;
    constexpr int r = 47;

    // 对连续 n 个 token 拼接（含空格分隔）直接算哈希，不构造中间 string
    auto inlineHash = [&](size_t start, int n) -> std::pair<size_t, int> {
        size_t hash = 0;
        for (int j = 0; j < n; ++j) {
            if (j > 0) {
                hash ^= static_cast<size_t>(' ');
                hash *= m;
                hash ^= (hash >> r);
            }
            for (char c : tokens[start + j]) {
                hash ^= static_cast<size_t>(c);
                hash *= m;
                hash ^= (hash >> r);
            }
        }
        size_t hash2 = hash ^ 0x9e3779b97f4a7c15ULL;
        hash2 *= 0xbf58476d1ce4e5b9ULL;
        hash2 ^= (hash2 >> 31);
        int sign = (hash2 & 1) ? 1 : -1;
        return {hash % ngramDim, sign};
    };

    // 2-gram 滑窗
    if (tokens.size() >= 2) {
        for (size_t i = 0; i <= tokens.size() - 2; ++i) {
            auto [idx, sign] = inlineHash(i, 2);
            features[idx] += sign * 1.0f;
        }
    }

    // 3-gram 滑窗
    if (tokens.size() >= 3) {
        for (size_t i = 0; i <= tokens.size() - 3; ++i) {
            auto [idx, sign] = inlineHash(i, 3);
            features[idx] += sign * 0.5f;
        }
    }

    // 归一化（使用绝对值之和，因为签名哈希产生正负值）
    float sum = 0.0f;
    for (float f : features) sum += std::abs(f);
    if (sum > 0) {
        for (auto& f : features) {
            f /= sum;
        }
    }

    return features;
}

std::vector<float> AdvancedEmbeddingEngine::combineFeatures(
    const std::vector<float>& tfidf,
    const std::vector<float>& sentiment,
    const std::vector<float>& statistical,
    const std::vector<float>& ngram
) const {
    std::vector<float> combined(embeddingDim_, 0.0f);

    // 直接写入对应偏移，避免多次 push_back/insert
    size_t offset = 0;
    auto copyBlock = [&](const std::vector<float>& src) {
        size_t n = std::min(src.size(), embeddingDim_ - offset);
        std::copy_n(src.begin(), n, combined.begin() + offset);
        offset += n;
    };
    copyBlock(tfidf);
    copyBlock(sentiment);
    copyBlock(statistical);
    copyBlock(ngram);

    return combined;
}

void AdvancedEmbeddingEngine::normalizeVector(std::vector<float>& vec) {
    // double 累加 + 4路展开，减少浮点累加误差
    const size_t n = vec.size();
    const size_t n4 = n - (n % 4);

    double s0 = 0.0, s1 = 0.0, s2 = 0.0, s3 = 0.0;
    for (size_t i = 0; i < n4; i += 4) {
        double v0 = vec[i], v1 = vec[i+1], v2 = vec[i+2], v3 = vec[i+3];
        s0 += v0 * v0; s1 += v1 * v1; s2 += v2 * v2; s3 += v3 * v3;
    }
    double norm = (s0 + s1) + (s2 + s3);
    for (size_t i = n4; i < n; ++i) {
        double v = vec[i];
        norm += v * v;
    }
    norm = std::sqrt(norm);

    if (norm > 1e-12) {
        float invNorm = static_cast<float>(1.0 / norm);
        for (float& val : vec) {
            val *= invNorm;
        }
    }
}

std::vector<float> AdvancedEmbeddingEngine::generateEmbedding(const std::string& text) {
    // 检查缓存
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        std::vector<float> cached;
        if (embeddingCache_->get(text, cached)) {
            return cached;
        }
    }

    // 分词
    auto tokens = tokenize(text);
    if (tokens.empty()) {
        return std::vector<float>(embeddingDim_, 0.0f);
    }

    // 提取多层次特征
    auto tfidfFeatures = extractTFIDFFeatures(tokens);
    auto sentimentFeatures = extractSentimentFeatures(tokens);
    auto statisticalFeatures = extractStatisticalFeatures(tokens, text);
    auto ngramFeatures = extractNGramFeatures(tokens);

    // 组合特征
    auto embedding = combineFeatures(tfidfFeatures, sentimentFeatures,
                                     statisticalFeatures, ngramFeatures);

    // L2归一化
    normalizeVector(embedding);

    // 缓存结果并返回（保留原件返回，拷贝给缓存 move 进去）
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto copy = embedding;
        embeddingCache_->put(text, std::move(copy));
    }

    return embedding;
}

std::vector<std::vector<float>> AdvancedEmbeddingEngine::generateEmbeddingBatch(
    const std::vector<std::string>& texts
) {
    std::vector<std::vector<float>> embeddings;
    embeddings.reserve(texts.size());

    for (const auto& text : texts) {
        embeddings.push_back(generateEmbedding(text));
    }

    return embeddings;
}

float AdvancedEmbeddingEngine::cosineSimilarity(
    const std::vector<float>& vec1,
    const std::vector<float>& vec2
) {
    if (vec1.size() != vec2.size()) {
        LOG_ERROR << "Vector size mismatch: " << vec1.size() << " vs " << vec2.size();
        return 0.0f;
    }

    // double 累加 + 4路展开，减少128维浮点累加误差
    double dotProduct = 0.0, norm1 = 0.0, norm2 = 0.0;
    const size_t n = vec1.size();
    const size_t n4 = n - (n % 4);

    double dot0 = 0.0, dot1 = 0.0, dot2 = 0.0, dot3 = 0.0;
    double n1_0 = 0.0, n1_1 = 0.0, n1_2 = 0.0, n1_3 = 0.0;
    double n2_0 = 0.0, n2_1 = 0.0, n2_2 = 0.0, n2_3 = 0.0;

    for (size_t i = 0; i < n4; i += 4) {
        double a0 = vec1[i], a1 = vec1[i+1], a2 = vec1[i+2], a3 = vec1[i+3];
        double b0 = vec2[i], b1 = vec2[i+1], b2 = vec2[i+2], b3 = vec2[i+3];
        dot0 += a0 * b0; dot1 += a1 * b1; dot2 += a2 * b2; dot3 += a3 * b3;
        n1_0 += a0 * a0; n1_1 += a1 * a1; n1_2 += a2 * a2; n1_3 += a3 * a3;
        n2_0 += b0 * b0; n2_1 += b1 * b1; n2_2 += b2 * b2; n2_3 += b3 * b3;
    }
    dotProduct = (dot0 + dot1) + (dot2 + dot3);
    norm1 = (n1_0 + n1_1) + (n1_2 + n1_3);
    norm2 = (n2_0 + n2_1) + (n2_2 + n2_3);

    for (size_t i = n4; i < n; ++i) {
        double a = vec1[i], b = vec2[i];
        dotProduct += a * b;
        norm1 += a * a;
        norm2 += b * b;
    }

    double denominator = std::sqrt(norm1) * std::sqrt(norm2);
    return denominator > 1e-12 ? static_cast<float>(dotProduct / denominator) : 0.0f;
}

void AdvancedEmbeddingEngine::trainFromCorpus(const std::vector<std::string>& texts) {
    if (texts.empty()) {
        LOG_WARN << "Empty corpus, skipping embedding training";
        return;
    }

    LOG_INFO << "Training embedding model from " << texts.size() << " documents";

    computeIDF(texts);
    clearCache();

    size_t idfSize = 0;
    {
        std::shared_lock<std::shared_mutex> lock(idfMutex_);
        idfSize = idfWeights_.size();
    }
    LOG_INFO << "Embedding model training completed, IDF weights: " << idfSize;
}

void AdvancedEmbeddingEngine::computeIDF(const std::vector<std::string>& texts) {
    std::unordered_map<std::string, int> docFreq;

    // 统计文档频率
    for (const auto& text : texts) {
        auto tokens = tokenize(text);
        std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());

        for (const auto& token : uniqueTokens) {
            docFreq[token]++;
        }
    }

    // 计算IDF
    std::unordered_map<std::string, float> newIdfWeights;
    newIdfWeights.reserve(docFreq.size());
    for (const auto& [word, df] : docFreq) {
        if (df > 0) {
            float idf = std::log(static_cast<float>(texts.size()) / df);
            newIdfWeights[word] = idf;
        }
    }

    {
        std::unique_lock<std::shared_mutex> lock(idfMutex_);
        idfWeights_ = std::move(newIdfWeights);
        totalDocs_ = texts.size();
    }

    LOG_INFO << "Computed IDF weights for " << docFreq.size() << " words";
}

void AdvancedEmbeddingEngine::loadSentimentLexicon() {
    // 加载扩展情感词典
    sentimentLexicon_ = {
        // 积极情感
        {"开心", 0.8f}, {"快乐", 0.9f}, {"幸福", 1.0f}, {"喜悦", 0.9f},
        {"高兴", 0.8f}, {"愉快", 0.7f}, {"满足", 0.6f}, {"感恩", 0.8f},
        {"希望", 0.7f}, {"乐观", 0.8f}, {"美好", 0.7f}, {"温暖", 0.6f},
        {"爱", 0.9f}, {"喜欢", 0.7f}, {"欣赏", 0.6f}, {"享受", 0.7f},
        {"兴奋", 0.8f}, {"激动", 0.8f}, {"骄傲", 0.7f}, {"自豪", 0.8f},
        {"放松", 0.6f}, {"平静", 0.5f}, {"安心", 0.6f}, {"舒服", 0.6f},

        // 消极情感
        {"难过", -0.7f}, {"伤心", -0.8f}, {"痛苦", -0.9f}, {"悲伤", -0.8f},
        {"焦虑", -0.7f}, {"担心", -0.6f}, {"害怕", -0.7f}, {"恐惧", -0.8f},
        {"愤怒", -0.8f}, {"生气", -0.7f}, {"失望", -0.6f}, {"沮丧", -0.7f},
        {"孤独", -0.7f}, {"寂寞", -0.6f}, {"无助", -0.8f}, {"绝望", -1.0f},
        {"讨厌", -0.7f}, {"厌恶", -0.8f}, {"烦躁", -0.6f}, {"郁闷", -0.7f},
        {"压抑", -0.7f}, {"疲惫", -0.6f}, {"累", -0.5f}, {"困", -0.4f},
        {"迷茫", -0.5f}, {"困惑", -0.4f}, {"纠结", -0.5f}, {"矛盾", -0.4f},

        // 中性但重要的词
        {"想", 0.0f}, {"觉得", 0.0f}, {"感觉", 0.0f}, {"可能", 0.0f},
        {"也许", 0.0f}, {"或许", 0.0f}, {"好像", 0.0f}, {"似乎", 0.0f}
    };

    LOG_INFO << "Loaded sentiment lexicon with " << sentimentLexicon_.size() << " words";
}

void AdvancedEmbeddingEngine::clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    embeddingCache_->clear();
    LOG_INFO << "Embedding cache cleared";
}

void AdvancedEmbeddingEngine::warmupTrainingDataAsync() {
    try {
        auto dbClient = app().getDbClient("default");
        if (!dbClient) {
            LOG_WARN << "Embedding warmup skipped: DB client not available";
            return;
        }

        LOG_INFO << "Scheduling embedding warmup query in background";
        dbClient->execSqlAsync(
            "SELECT content FROM stones WHERE content IS NOT NULL AND LENGTH(content) > 10 LIMIT 1000",
            [this](const drogon::orm::Result& result) {
                std::vector<std::string> corpus;
                corpus.reserve(result.size());
                for (const auto& row : result) {
                    corpus.push_back(row["content"].as<std::string>());
                }

                if (corpus.empty()) {
                    LOG_INFO << "Embedding warmup skipped: no training data found";
                    return;
                }

                // 用 jthread 管理后台训练线程，析构时自动 join
                warmupThread_ = std::jthread([this, corpus = std::move(corpus)](std::stop_token) mutable {
                    try {
                        trainFromCorpus(corpus);
                    } catch (const std::exception& e) {
                        LOG_WARN << "Embedding warmup training failed: " << e.what();
                    }
                });
            },
            [](const drogon::orm::DrogonDbException& e) {
                LOG_WARN << "Embedding warmup query failed: " << e.base().what();
            }
        );
    } catch (const std::exception& e) {
        LOG_WARN << "Failed to schedule embedding warmup: " << e.what();
    }
}

AdvancedEmbeddingEngine::CacheStats AdvancedEmbeddingEngine::getCacheStats() const {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    CacheStats stats;
    stats.hits = embeddingCache_->hits;
    stats.misses = embeddingCache_->misses;
    stats.size = embeddingCache_->cache.size();

    size_t total = stats.hits + stats.misses;
    stats.hitRate = total > 0 ? static_cast<float>(stats.hits) / total : 0.0f;

    return stats;
}

} // namespace ai
} // namespace heartlake
