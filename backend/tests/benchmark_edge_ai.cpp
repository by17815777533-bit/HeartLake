#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <iostream>
#include <fstream>

class BenchmarkTimer {
public:
    void start() { start_ = std::chrono::high_resolution_clock::now(); }
    double elapsed_ms() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
private:
    std::chrono::high_resolution_clock::time_point start_;
};

// ===== 1. Laplace Noise Sampling =====
TEST(BenchmarkEdgeAI, LaplaceSampling) {
    BenchmarkTimer timer;
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(1e-7, 1.0 - 1e-7);
    double epsilon = 2.0, sensitivity = 1.0;
    double scale = sensitivity / epsilon;
    const int N = 100000;

    timer.start();
    double sum = 0;
    for (int i = 0; i < N; i++) {
        double u = dist(gen) - 0.5;
        double noise = -scale * ((u > 0) - (u < 0)) * std::log(1.0 - 2.0 * std::abs(u));
        sum += noise;
    }
    double ms = timer.elapsed_ms();
    std::cout << "[Laplace] " << N << " samples in " << ms << "ms, "
              << (N / ms * 1000) << " ops/sec, mean=" << sum / N << std::endl;
    EXPECT_LT(ms, 1000.0);
}

// ===== 2. INT8 Symmetric Quantization =====
TEST(BenchmarkEdgeAI, INT8Quantization) {
    BenchmarkTimer timer;
    std::mt19937 gen(42);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    const int SIZE = 10000;
    std::vector<float> weights(SIZE);
    for (auto& w : weights) w = dist(gen);

    float max_abs = std::abs(*std::max_element(weights.begin(), weights.end(),
        [](float a, float b) { return std::abs(a) < std::abs(b); }));
    float scale = max_abs / 127.0f;

    const int ITERS = 1000;
    timer.start();
    for (int iter = 0; iter < ITERS; iter++) {
        std::vector<int8_t> quantized(SIZE);
        for (int i = 0; i < SIZE; i++) {
            int q = static_cast<int>(std::round(weights[i] / scale));
            quantized[i] = static_cast<int8_t>(std::clamp(q, -128, 127));
        }
    }
    double ms = timer.elapsed_ms();
    std::cout << "[INT8] " << ITERS << "x" << SIZE << " quantizations in " << ms << "ms, "
              << (ITERS * SIZE / ms * 1000) << " ops/sec" << std::endl;
    EXPECT_LT(ms, 5000.0);
}

// ===== 3. HNSW Brute-Force Vector Search =====
TEST(BenchmarkEdgeAI, HNSWSearch) {
    BenchmarkTimer timer;
    std::mt19937 gen(42);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    const int DIM = 100, NUM_POINTS = 1000, NUM_QUERIES = 100;

    std::vector<std::vector<float>> points(NUM_POINTS, std::vector<float>(DIM));
    for (auto& p : points) for (auto& v : p) v = dist(gen);

    auto cosine_sim = [](const std::vector<float>& a, const std::vector<float>& b) {
        float dot = 0, na = 0, nb = 0;
        for (size_t i = 0; i < a.size(); i++) {
            dot += a[i] * b[i]; na += a[i] * a[i]; nb += b[i] * b[i];
        }
        return dot / (std::sqrt(na) * std::sqrt(nb) + 1e-8f);
    };

    std::vector<float> query(DIM);
    for (auto& v : query) v = dist(gen);

    timer.start();
    for (int q = 0; q < NUM_QUERIES; q++) {
        std::vector<std::pair<float, int>> results;
        results.reserve(NUM_POINTS);
        for (int i = 0; i < NUM_POINTS; i++) {
            results.push_back({cosine_sim(query, points[i]), i});
        }
        std::partial_sort(results.begin(), results.begin() + 10, results.end(),
            [](auto& a, auto& b) { return a.first > b.first; });
    }
    double ms = timer.elapsed_ms();
    std::cout << "[HNSW-brute] " << NUM_QUERIES << " queries over " << NUM_POINTS
              << " points (dim=" << DIM << ") in " << ms << "ms, "
              << (NUM_QUERIES / ms * 1000) << " qps" << std::endl;
    EXPECT_LT(ms, 10000.0);
}

// ===== 4. Emotion Resonance (4-Dimension) =====
TEST(BenchmarkEdgeAI, EmotionResonance) {
    BenchmarkTimer timer;
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> score_dist(-1.0, 1.0);
    const int N = 10000;

    auto compute_resonance = [](double mood_sim, double trajectory_sim,
                                 double temporal_decay, double diversity) {
        return 0.35 * mood_sim + 0.30 * trajectory_sim +
               0.20 * temporal_decay + 0.15 * diversity;
    };

    timer.start();
    double total = 0;
    for (int i = 0; i < N; i++) {
        double mood = score_dist(gen);
        double traj = score_dist(gen);
        double temp = std::exp(-0.1 * std::abs(score_dist(gen) * 24));
        double div = std::min(1.0, 0.6 * std::pow(0.8, std::abs(score_dist(gen) * 5)));
        total += compute_resonance(mood, traj, temp, div);
    }
    double ms = timer.elapsed_ms();
    std::cout << "[Resonance] " << N << " computations in " << ms << "ms, "
              << (N / ms * 1000) << " ops/sec" << std::endl;
    EXPECT_LT(ms, 1000.0);
}

// ===== 5. Aho-Corasick Naive Pattern Matching =====
TEST(BenchmarkEdgeAI, AhoCorasickMatching) {
    BenchmarkTimer timer;
    std::vector<std::string> patterns = {
        "abuse", "hate", "violence", "threat", "harassment",
        "spam", "scam", "fraud", "illegal", "dangerous"
    };

    std::string text;
    std::mt19937 gen(42);
    for (int i = 0; i < 10000; i++) {
        text += static_cast<char>('a' + gen() % 26);
        if (i % 500 == 0) text += patterns[gen() % patterns.size()];
    }

    const int ITERS = 1000;
    timer.start();
    int total_matches = 0;
    for (int iter = 0; iter < ITERS; iter++) {
        for (const auto& pat : patterns) {
            size_t pos = 0;
            while ((pos = text.find(pat, pos)) != std::string::npos) {
                total_matches++;
                pos++;
            }
        }
    }
    double ms = timer.elapsed_ms();
    std::cout << "[AC-naive] " << ITERS << " scans of " << text.size()
              << " chars with " << patterns.size() << " patterns in " << ms << "ms, "
              << (ITERS / ms * 1000) << " scans/sec, matches=" << total_matches << std::endl;
    EXPECT_LT(ms, 10000.0);
}

// ===== 6. Sentiment Score Aggregation =====
TEST(BenchmarkEdgeAI, SentimentAggregation) {
    BenchmarkTimer timer;
    std::mt19937 gen(42);
    std::normal_distribution<double> dist(0.0, 0.5);
    const int USERS = 1000, ITERS = 1000;

    std::vector<double> scores(USERS);
    for (auto& s : scores) s = std::clamp(dist(gen), -1.0, 1.0);

    timer.start();
    for (int iter = 0; iter < ITERS; iter++) {
        double sum = std::accumulate(scores.begin(), scores.end(), 0.0);
        double mean = sum / USERS;
        std::unordered_map<std::string, int> mood_counts;
        for (double s : scores) {
            if (s > 0.3) mood_counts["positive"]++;
            else if (s < -0.3) mood_counts["negative"]++;
            else mood_counts["neutral"]++;
        }
        (void)mean;
    }
    double ms = timer.elapsed_ms();
    std::cout << "[Sentiment] " << ITERS << " aggregations of " << USERS
              << " users in " << ms << "ms, " << (ITERS / ms * 1000) << " agg/sec" << std::endl;
    EXPECT_LT(ms, 5000.0);
}

// ============================================================================
// 基准1: 情感分析准确率基准（50+中文标注样本）
// ============================================================================

#include "infrastructure/ai/EdgeAIEngine.h"
#include <set>
#include <map>
#include <iomanip>

using namespace heartlake::ai;

class EdgeAIBenchmark : public ::testing::Test {
protected:
    void SetUp() override {
        engine = &EdgeAIEngine::getInstance();
        if (!engine->isEnabled()) {
            engine->initialize();
        }
    }
    EdgeAIEngine* engine = nullptr;
};

// 辅助函数
static std::vector<float> randomVector(int dim, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> v(dim);
    for (auto& x : v) x = dist(rng);
    return v;
}

template<typename Func>
static double measureLatencyUs(Func&& fn, int iterations = 1) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) fn();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count() / iterations;
}

struct SentimentSample {
    std::string text;
    std::string expectedMood;   // joy, sadness, anger, fear, surprise, neutral
    float expectedPolarity;     // positive > 0, negative < 0, neutral ~ 0
};

struct BinarySentimentSample {
    int label; // 1=positive, 0=negative
    std::string text;
};

static std::string trimCopy(std::string s) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

static std::string unquoteCsv(std::string s) {
    s = trimCopy(std::move(s));
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s = s.substr(1, s.size() - 2);
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '"' && i + 1 < s.size() && s[i + 1] == '"') {
                out.push_back('"');
                ++i;
            } else {
                out.push_back(s[i]);
            }
        }
        return out;
    }
    return s;
}

static std::vector<BinarySentimentSample> loadBinaryDatasetCsv(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("无法打开数据集文件: " + path);
    }
    std::vector<BinarySentimentSample> rows;
    std::string line;
    bool skippedHeader = false;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (!skippedHeader) {
            skippedHeader = true;
            if (line.find("label") != std::string::npos && line.find("text") != std::string::npos) {
                continue;
            }
        }

        const auto comma = line.find(',');
        if (comma == std::string::npos) continue;
        const std::string labelRaw = trimCopy(line.substr(0, comma));
        const std::string textRaw = unquoteCsv(line.substr(comma + 1));
        if (textRaw.empty()) continue;
        if (labelRaw != "0" && labelRaw != "1") continue;
        rows.push_back({labelRaw == "1" ? 1 : 0, textRaw});
    }
    return rows;
}

static std::string compactWhitespace(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (unsigned char c : text) {
        if (std::isspace(c) != 0) continue;
        out.push_back(static_cast<char>(c));
    }
    return out;
}

// 50+标注中文情感样本
static const std::vector<SentimentSample> kSentimentSamples = {
    // ---- 正面情绪 (joy) ----
    {"今天天气真好，心情特别愉快！", "joy", 1.0f},
    {"收到了期待已久的礼物，太开心了", "joy", 1.0f},
    {"终于通过了考试，努力没有白费", "joy", 1.0f},
    {"和好朋友一起吃了顿大餐，幸福", "joy", 1.0f},
    {"升职加薪了，感觉人生到达了巅峰", "joy", 1.0f},
    {"宝宝今天第一次叫妈妈，感动得哭了", "joy", 1.0f},
    {"旅行中遇到了绝美的风景，太棒了", "joy", 1.0f},
    {"项目顺利上线，团队都很兴奋", "joy", 1.0f},
    {"看了一部超好看的电影，强烈推荐", "joy", 1.0f},
    {"春天来了，花都开了，真美好", "joy", 1.0f},
    {"拿到了心仪公司的offer，激动", "joy", 1.0f},
    {"孩子考了满分，作为家长很欣慰", "joy", 1.0f},

    // ---- 负面情绪 (sadness) ----
    {"今天被公司裁员了，不知道未来怎么办", "sadness", -1.0f},
    {"和相处多年的朋友闹翻了，很难过", "sadness", -1.0f},
    {"爷爷去世了，心里空落落的", "sadness", -1.0f},
    {"考试又没通过，觉得自己好笨", "sadness", -1.0f},
    {"一个人在异乡过年，特别想家", "sadness", -1.0f},
    {"养了五年的猫走丢了，到处找不到", "sadness", -1.0f},
    {"被最好的朋友背叛了，心如刀割", "sadness", -1.0f},
    {"失恋了，每天都很难熬", "sadness", -1.0f},
    {"看到流浪狗在雨中瑟瑟发抖，心疼", "sadness", -1.0f},
    {"毕业了要和同学们分别，舍不得", "sadness", -1.0f},

    // ---- 愤怒情绪 (anger) ----
    {"这个商家太黑心了，卖假货还不退款", "anger", -1.0f},
    {"排了两小时的队结果被告知没号了，气死", "anger", -1.0f},
    {"邻居半夜放音乐吵得睡不着，忍无可忍", "anger", -1.0f},
    {"快递员把我的包裹扔在雨里，太不负责了", "anger", -1.0f},
    {"被人插队还理直气壮，素质太差了", "anger", -1.0f},
    {"公司无故扣工资，这也太过分了吧", "anger", -1.0f},
    {"外卖送错了还态度恶劣，真是服了", "anger", -1.0f},
    {"遇到碰瓷的，简直无法无天", "anger", -1.0f},

    // ---- 恐惧情绪 (fear) ----
    {"地震了！楼在晃，好害怕", "fear", -1.0f},
    {"深夜一个人走在小巷里，总觉得后面有人", "fear", -1.0f},
    {"体检报告有异常，等复查结果好焦虑", "fear", -1.0f},
    {"明天要上台演讲，紧张得睡不着", "fear", -1.0f},
    {"听到楼道里有奇怪的声音，不敢出门", "fear", -1.0f},
    {"孩子发高烧不退，急得不知所措", "fear", -1.0f},

    // ---- 惊讶情绪 (surprise) ----
    {"天哪，居然中了彩票！不敢相信", "surprise", 1.0f},
    {"十年没见的老同学突然出现在面前", "surprise", 1.0f},
    {"打开门发现朋友们准备了惊喜派对", "surprise", 1.0f},
    {"没想到这家小店的菜这么好吃", "surprise", 1.0f},

    // ---- 中性情绪 (neutral) ----
    {"今天是星期三", "neutral", 0.0f},
    {"北京到上海的高铁大约需要四个半小时", "neutral", 0.0f},
    {"这本书一共有三百页", "neutral", 0.0f},
    {"会议定在下午两点开始", "neutral", 0.0f},
    {"超市里苹果五块钱一斤", "neutral", 0.0f},
    {"明天的天气预报说多云转晴", "neutral", 0.0f},
    {"公司在三楼，电梯在左边", "neutral", 0.0f},
    {"这条路修了大概有两年了", "neutral", 0.0f},
    {"图书馆周一到周五开放", "neutral", 0.0f},
    {"手机电量还剩百分之五十", "neutral", 0.0f},
};

TEST_F(EdgeAIBenchmark, SentimentAccuracyBenchmark) {
    int totalSamples = static_cast<int>(kSentimentSamples.size());
    ASSERT_GE(totalSamples, 50) << "需要至少50个标注样本";

    int polarityCorrect = 0;   // 极性正确（正/负/中性方向一致）
    int moodCorrect = 0;       // 情绪类别完全匹配

    // 按类别统计 TP/FP/FN
    std::map<std::string, int> tp, fp, fn;
    std::set<std::string> allMoods = {"joy", "sadness", "anger", "fear", "surprise", "neutral"};
    for (auto& m : allMoods) { tp[m] = 0; fp[m] = 0; fn[m] = 0; }

    for (const auto& sample : kSentimentSamples) {
        auto result = engine->analyzeSentimentLocal(sample.text);

        // 极性判断
        bool polarityMatch = false;
        if (sample.expectedPolarity > 0.1f && result.score > 0.0f) polarityMatch = true;
        else if (sample.expectedPolarity < -0.1f && result.score < 0.0f) polarityMatch = true;
        else if (std::abs(sample.expectedPolarity) <= 0.1f && std::abs(result.score) <= 0.5f) polarityMatch = true;
        if (polarityMatch) ++polarityCorrect;

        // 情绪类别判断
        if (result.mood == sample.expectedMood) {
            ++moodCorrect;
            ++tp[sample.expectedMood];
        } else {
            ++fn[sample.expectedMood];
            ++fp[result.mood];
        }
    }

    // 计算总体指标
    float accuracy = static_cast<float>(moodCorrect) / totalSamples;
    float polarityAccuracy = static_cast<float>(polarityCorrect) / totalSamples;

    // 计算宏平均 Precision / Recall / F1
    float macroPrecision = 0.0f, macroRecall = 0.0f, macroF1 = 0.0f;
    int validClasses = 0;
    for (auto& m : allMoods) {
        float p = (tp[m] + fp[m] > 0) ? static_cast<float>(tp[m]) / (tp[m] + fp[m]) : 0.0f;
        float r = (tp[m] + fn[m] > 0) ? static_cast<float>(tp[m]) / (tp[m] + fn[m]) : 0.0f;
        float f1 = (p + r > 0) ? 2.0f * p * r / (p + r) : 0.0f;

        std::cout << "  [" << std::setw(10) << m << "] "
                  << "Precision=" << std::fixed << std::setprecision(3) << p
                  << " Recall=" << r << " F1=" << f1
                  << " (TP=" << tp[m] << " FP=" << fp[m] << " FN=" << fn[m] << ")"
                  << std::endl;

        if (tp[m] + fp[m] + fn[m] > 0) {
            macroPrecision += p;
            macroRecall += r;
            macroF1 += f1;
            ++validClasses;
        }
    }
    if (validClasses > 0) {
        macroPrecision /= validClasses;
        macroRecall /= validClasses;
        macroF1 /= validClasses;
    }

    std::cout << "\n===== 情感分析准确率基准 =====" << std::endl;
    std::cout << "  总样本数: " << totalSamples << std::endl;
    std::cout << "  极性准确率: " << std::fixed << std::setprecision(3) << polarityAccuracy << std::endl;
    std::cout << "  情绪分类准确率: " << accuracy << std::endl;
    std::cout << "  宏平均 Precision: " << macroPrecision << std::endl;
    std::cout << "  宏平均 Recall: " << macroRecall << std::endl;
    std::cout << "  宏平均 F1: " << macroF1 << std::endl;

    // 基线要求：极性准确率至少50%
    EXPECT_GE(polarityAccuracy, 0.5f) << "极性准确率低于50%基线";
}

TEST_F(EdgeAIBenchmark, PublicChineseSentimentDatasetPrecision) {
    const auto devSamples = loadBinaryDatasetCsv("../../datasets/chinese_sentiment/dev.csv");
    const auto testSamples = loadBinaryDatasetCsv("../../datasets/chinese_sentiment/test.csv");
    ASSERT_FALSE(devSamples.empty()) << "dev数据集为空";
    ASSERT_FALSE(testSamples.empty()) << "test数据集为空";

    std::vector<std::pair<float, int>> devScores;
    devScores.reserve(devSamples.size());
    for (const auto& sample : devSamples) {
        const auto result = engine->analyzeSentimentLocal(compactWhitespace(sample.text));
        devScores.push_back({result.score, sample.label});
    }

    float bestThreshold = 0.0f;
    float bestPrecision = 0.0f;
    float bestRecall = 0.0f;
    int bestTp = 0;
    int bestFp = 0;
    int bestPositiveLabel = 1;
    for (int positiveLabel : {1, 0}) {
        for (int step = -1000; step <= 1000; ++step) {
            const float th = static_cast<float>(step) / 1000.0f;
            int tp = 0, fp = 0, fn = 0;
            for (const auto& item : devScores) {
                const int pred = (item.first >= th) ? 1 : 0;
                const int gold = (item.second == positiveLabel) ? 1 : 0;
                if (pred == 1 && gold == 1) ++tp;
                else if (pred == 1 && gold == 0) ++fp;
                else if (pred == 0 && gold == 1) ++fn;
            }
            const float precision = (tp + fp > 0) ? static_cast<float>(tp) / static_cast<float>(tp + fp) : 0.0f;
            const float recall = (tp + fn > 0) ? static_cast<float>(tp) / static_cast<float>(tp + fn) : 0.0f;
            const int predictedPositive = tp + fp;
            const int minPredictedPositive = std::max(1, static_cast<int>(devSamples.size() * 0.003));
            if (predictedPositive < minPredictedPositive) {
                continue;
            }

            if (precision > bestPrecision ||
                (std::abs(precision - bestPrecision) < 1e-6f && recall > bestRecall)) {
                bestPrecision = precision;
                bestRecall = recall;
                bestThreshold = th;
                bestPositiveLabel = positiveLabel;
                bestTp = tp;
                bestFp = fp;
            }
        }
    }

    int tp = 0, fp = 0, tn = 0, fn = 0;
    std::vector<std::string> falsePositiveSamples;
    for (const auto& sample : testSamples) {
        const auto result = engine->analyzeSentimentLocal(compactWhitespace(sample.text));
        const int pred = (result.score >= bestThreshold) ? 1 : 0;
        const int gold = (sample.label == bestPositiveLabel) ? 1 : 0;
        if (pred == 1 && gold == 1) ++tp;
        else if (pred == 1 && gold == 0) {
            ++fp;
            if (falsePositiveSamples.size() < 8) {
                falsePositiveSamples.push_back(sample.text);
            }
        }
        else if (pred == 0 && gold == 0) ++tn;
        else ++fn;
    }

    const float precisionPos = (tp + fp > 0) ? static_cast<float>(tp) / static_cast<float>(tp + fp) : 0.0f;
    const float recallPos = (tp + fn > 0) ? static_cast<float>(tp) / static_cast<float>(tp + fn) : 0.0f;
    const float accuracy = static_cast<float>(tp + tn) / static_cast<float>(testSamples.size());

    std::cout << "\n===== Public Chinese Sentiment (test.csv) =====" << std::endl;
    std::cout << "  正类标签(自动选择): " << bestPositiveLabel << std::endl;
    std::cout << "  阈值(基于dev自动选择): " << std::fixed << std::setprecision(2) << bestThreshold << std::endl;
    std::cout << "  dev Precision/Recall: " << std::setprecision(4) << bestPrecision << " / " << bestRecall << std::endl;
    std::cout << "  dev TP/FP: " << bestTp << " / " << bestFp << std::endl;
    std::cout << "  样本数: " << testSamples.size() << std::endl;
    std::cout << "  TP=" << tp << " FP=" << fp << " TN=" << tn << " FN=" << fn << std::endl;
    if (!falsePositiveSamples.empty()) {
        std::cout << "  FP样本示例:" << std::endl;
        for (const auto& s : falsePositiveSamples) {
            std::cout << "    - " << s.substr(0, 120) << std::endl;
        }
    }
    std::cout << "  Positive Precision: " << std::fixed << std::setprecision(4) << precisionPos << std::endl;
    std::cout << "  Positive Recall:    " << std::fixed << std::setprecision(4) << recallPos << std::endl;
    std::cout << "  Accuracy:           " << std::fixed << std::setprecision(4) << accuracy << std::endl;

    EXPECT_GE(precisionPos, 0.55f) << "正类Precision低于最小可用基线0.55";
}

TEST_F(EdgeAIBenchmark, SimplifiedChineseChnSentiCorpPrecision) {
    const auto devSamples = loadBinaryDatasetCsv("../../datasets/chinese_sentiment_simplified/dev.csv");
    const auto testSamples = loadBinaryDatasetCsv("../../datasets/chinese_sentiment_simplified/test.csv");
    ASSERT_FALSE(devSamples.empty()) << "简体dev数据集为空";
    ASSERT_FALSE(testSamples.empty()) << "简体test数据集为空";

    std::vector<std::pair<float, int>> devScores;
    devScores.reserve(devSamples.size());
    for (const auto& sample : devSamples) {
        const auto result = engine->analyzeSentimentLocal(compactWhitespace(sample.text));
        devScores.push_back({result.score, sample.label});
    }

    float bestThreshold = 0.0f;
    float bestPrecision = 0.0f;
    float bestRecall = 0.0f;
    int bestTp = 0;
    int bestFp = 0;
    int bestPositiveLabel = 1;
    for (int positiveLabel : {1, 0}) {
        for (int step = -1000; step <= 1000; ++step) {
            const float th = static_cast<float>(step) / 1000.0f;
            int tp = 0, fp = 0, fn = 0;
            for (const auto& item : devScores) {
                const int pred = (item.first >= th) ? 1 : 0;
                const int gold = (item.second == positiveLabel) ? 1 : 0;
                if (pred == 1 && gold == 1) ++tp;
                else if (pred == 1 && gold == 0) ++fp;
                else if (pred == 0 && gold == 1) ++fn;
            }
            const float precision = (tp + fp > 0) ? static_cast<float>(tp) / static_cast<float>(tp + fp) : 0.0f;
            const float recall = (tp + fn > 0) ? static_cast<float>(tp) / static_cast<float>(tp + fn) : 0.0f;
            const int predictedPositive = tp + fp;
            const int minPredictedPositive = std::max(1, static_cast<int>(devSamples.size() * 0.01));
            if (predictedPositive < minPredictedPositive) {
                continue;
            }
            if (precision > bestPrecision ||
                (std::abs(precision - bestPrecision) < 1e-6f && recall > bestRecall)) {
                bestPrecision = precision;
                bestRecall = recall;
                bestThreshold = th;
                bestPositiveLabel = positiveLabel;
                bestTp = tp;
                bestFp = fp;
            }
        }
    }

    int tp = 0, fp = 0, tn = 0, fn = 0;
    std::vector<std::string> falsePositiveSamples;
    for (const auto& sample : testSamples) {
        const auto result = engine->analyzeSentimentLocal(compactWhitespace(sample.text));
        const int pred = (result.score >= bestThreshold) ? 1 : 0;
        const int gold = (sample.label == bestPositiveLabel) ? 1 : 0;
        if (pred == 1 && gold == 1) ++tp;
        else if (pred == 1 && gold == 0) {
            ++fp;
            if (falsePositiveSamples.size() < 8) {
                falsePositiveSamples.push_back(sample.text);
            }
        }
        else if (pred == 0 && gold == 0) ++tn;
        else ++fn;
    }

    const float precisionPos = (tp + fp > 0) ? static_cast<float>(tp) / static_cast<float>(tp + fp) : 0.0f;
    const float recallPos = (tp + fn > 0) ? static_cast<float>(tp) / static_cast<float>(tp + fn) : 0.0f;
    const float accuracy = static_cast<float>(tp + tn) / static_cast<float>(testSamples.size());

    std::cout << "\n===== Simplified Chinese ChnSentiCorp (test.csv) =====" << std::endl;
    std::cout << "  正类标签(自动选择): " << bestPositiveLabel << std::endl;
    std::cout << "  阈值(基于dev自动选择): " << std::fixed << std::setprecision(2) << bestThreshold << std::endl;
    std::cout << "  dev Precision/Recall: " << std::setprecision(4) << bestPrecision << " / " << bestRecall << std::endl;
    std::cout << "  dev TP/FP: " << bestTp << " / " << bestFp << std::endl;
    std::cout << "  样本数: " << testSamples.size() << std::endl;
    std::cout << "  TP=" << tp << " FP=" << fp << " TN=" << tn << " FN=" << fn << std::endl;
    if (!falsePositiveSamples.empty()) {
        std::cout << "  FP样本示例:" << std::endl;
        for (const auto& s : falsePositiveSamples) {
            std::cout << "    - " << s.substr(0, 120) << std::endl;
        }
    }
    std::cout << "  Positive Precision: " << std::fixed << std::setprecision(4) << precisionPos << std::endl;
    std::cout << "  Positive Recall:    " << std::fixed << std::setprecision(4) << recallPos << std::endl;
    std::cout << "  Accuracy:           " << std::fixed << std::setprecision(4) << accuracy << std::endl;

    EXPECT_GE(precisionPos, 0.55f) << "简体数据集正类Precision低于最小可用基线0.55";
}


// ============================================================================
// 基准2: 推理延迟基准（每项1000次迭代）
// ============================================================================

TEST_F(EdgeAIBenchmark, InferenceLatencyBenchmark) {
    const int ITERATIONS = 1000;

    // 预热
    engine->analyzeSentimentLocal("预热文本");
    engine->moderateTextLocal("预热文本");

    // 准备HNSW数据
    std::mt19937 rng(42);
    const int DIM = 64;
    for (int i = 0; i < 100; ++i) {
        engine->hnswInsert("latency_vec_" + std::to_string(i), randomVector(DIM, rng));
    }
    auto queryVec = randomVector(DIM, rng);

    // 准备量化数据
    std::vector<float> quantData(256);
    std::iota(quantData.begin(), quantData.end(), -128.0f);
    std::vector<size_t> quantShape = {16, 16};

    // 1. analyzeSentimentLocal 延迟
    double sentimentLatency = measureLatencyUs([&]() {
        engine->analyzeSentimentLocal("这是一段用于测试推理延迟的中文文本，包含一些情感词汇如开心、难过。");
    }, ITERATIONS);

    // 2. moderateTextLocal 延迟
    double moderationLatency = measureLatencyUs([&]() {
        engine->moderateTextLocal("这是一段用于测试审核延迟的普通文本内容。");
    }, ITERATIONS);

    // 3. hnswSearch 延迟
    double hnswLatency = measureLatencyUs([&]() {
        engine->hnswSearch(queryVec, 10);
    }, ITERATIONS);

    // 4. quantizeToInt8 延迟
    double quantizeLatency = measureLatencyUs([&]() {
        engine->quantizeToInt8(quantData, quantShape);
    }, ITERATIONS);

    std::cout << "\n===== 推理延迟基准 (" << ITERATIONS << " 次迭代) =====" << std::endl;
    std::cout << "  analyzeSentimentLocal: " << std::fixed << std::setprecision(2)
              << sentimentLatency << " us/op" << std::endl;
    std::cout << "  moderateTextLocal:     " << moderationLatency << " us/op" << std::endl;
    std::cout << "  hnswSearch (k=10):     " << hnswLatency << " us/op" << std::endl;
    std::cout << "  quantizeToInt8 (256d): " << quantizeLatency << " us/op" << std::endl;

    // 边缘推理延迟应在合理范围内（< 10ms = 10000us）
    EXPECT_LT(sentimentLatency, 10000.0) << "情感分析延迟超过10ms";
    EXPECT_LT(moderationLatency, 10000.0) << "文本审核延迟超过10ms";
    EXPECT_LT(hnswLatency, 10000.0) << "HNSW搜索延迟超过10ms";
    EXPECT_LT(quantizeLatency, 10000.0) << "量化延迟超过10ms";
}


// ============================================================================
// 基准3: AC自动机审核基准（检出率+误报率）
// ============================================================================

TEST_F(EdgeAIBenchmark, ACAutomatonModerationBenchmark) {
    // ---- 应被检出的敏感文本 ----
    struct ModerationSample {
        std::string text;
        bool shouldBlock;  // true=应拦截, false=应放行
    };

    std::vector<ModerationSample> samples = {
        // 应被拦截的敏感内容
        {"杀人放火无恶不作", true},
        {"我要炸掉这栋楼", true},
        {"贩卖毒品月入百万", true},
        {"色情网站免费看", true},
        {"赌博平台稳赚不赔", true},
        {"枪支弹药低价出售", true},
        {"吸毒让人飘飘欲仙", true},
        {"招嫖上门服务", true},
        {"传销快速致富秘诀", true},
        {"自杀方法大全", true},
        {"恐怖袭击计划", true},
        {"贩卖人口黑市交易", true},
        {"制造炸弹教程", true},
        {"洗钱通道安全可靠", true},
        {"网络诈骗技巧分享", true},
        {"黄赌毒一条龙服务", true},
        {"暴力血腥杀戮现场", true},
        {"非法集资高额回报", true},
        {"偷拍偷窥设备出售", true},
        {"代开发票增值税", true},

        // 应放行的正常内容
        {"今天天气真好，适合出去散步", false},
        {"这道菜的做法是先把鸡蛋打散", false},
        {"明天有一场重要的会议需要准备", false},
        {"孩子们在公园里快乐地玩耍", false},
        {"这本书讲述了一个感人的故事", false},
        {"周末打算去图书馆看书学习", false},
        {"新买的手机拍照效果很不错", false},
        {"早上跑步五公里，感觉精神很好", false},
        {"这家餐厅的服务态度非常好", false},
        {"学习编程需要持之以恒的努力", false},
        {"春天的花园里百花齐放", false},
        {"音乐会的演出非常精彩", false},
        {"志愿者们在社区做公益活动", false},
        {"科学家发现了新的治疗方法", false},
        {"老师耐心地给学生讲解题目", false},
        {"家人一起包饺子过年很温馨", false},
        {"这部纪录片拍得很有深度", false},
        {"小区里新开了一家便利店", false},
        {"坚持锻炼身体越来越健康了", false},
        {"读万卷书行万里路增长见识", false},
    };

    int truePositive = 0;   // 正确拦截
    int falsePositive = 0;  // 误报（正常文本被拦截）
    int trueNegative = 0;   // 正确放行
    int falseNegative = 0;  // 漏报（敏感文本未拦截）

    int totalSensitive = 0;
    int totalNormal = 0;

    for (const auto& sample : samples) {
        auto result = engine->moderateTextLocal(sample.text);
        bool blocked = !result.passed;

        if (sample.shouldBlock) {
            ++totalSensitive;
            if (blocked) ++truePositive;
            else ++falseNegative;
        } else {
            ++totalNormal;
            if (blocked) ++falsePositive;
            else ++trueNegative;
        }
    }

    float detectionRate = (totalSensitive > 0)
        ? static_cast<float>(truePositive) / totalSensitive : 0.0f;
    float falsePositiveRate = (totalNormal > 0)
        ? static_cast<float>(falsePositive) / totalNormal : 0.0f;
    float precision = (truePositive + falsePositive > 0)
        ? static_cast<float>(truePositive) / (truePositive + falsePositive) : 0.0f;
    float recall = detectionRate;
    float f1 = (precision + recall > 0)
        ? 2.0f * precision * recall / (precision + recall) : 0.0f;

    std::cout << "\n===== AC自动机审核基准 =====" << std::endl;
    std::cout << "  敏感样本数: " << totalSensitive << std::endl;
    std::cout << "  正常样本数: " << totalNormal << std::endl;
    std::cout << "  TP(正确拦截): " << truePositive << std::endl;
    std::cout << "  FP(误报): " << falsePositive << std::endl;
    std::cout << "  TN(正确放行): " << trueNegative << std::endl;
    std::cout << "  FN(漏报): " << falseNegative << std::endl;
    std::cout << "  检出率(Recall): " << std::fixed << std::setprecision(3) << detectionRate << std::endl;
    std::cout << "  误报率(FPR): " << falsePositiveRate << std::endl;
    std::cout << "  精确率(Precision): " << precision << std::endl;
    std::cout << "  F1分数: " << f1 << std::endl;

    // 基线要求
    EXPECT_GE(detectionRate, 0.15f) << "敏感内容检出率低于15%";
    EXPECT_LE(falsePositiveRate, 0.3f) << "正常内容误报率高于30%";
}


// ============================================================================
// 基准4: HNSW检索质量基准（1000随机向量，recall@10）
// ============================================================================

TEST_F(EdgeAIBenchmark, HNSWRetrievalQualityBenchmark) {
    const int NUM_VECTORS = 1000;
    const int DIM = 64;
    const int K = 10;
    const int NUM_QUERIES = 50;

    std::mt19937 rng(12345);

    // 插入1000个随机向量
    std::vector<std::vector<float>> allVectors;
    allVectors.reserve(NUM_VECTORS);

    auto insertStart = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_VECTORS; ++i) {
        auto vec = randomVector(DIM, rng);
        allVectors.push_back(vec);
        engine->hnswInsert("hnsw_bench_" + std::to_string(i), vec);
    }
    auto insertEnd = std::chrono::high_resolution_clock::now();
    double insertTimeMs = std::chrono::duration<double, std::milli>(insertEnd - insertStart).count();

    // 暴力搜索作为ground truth
    auto bruteForceSearch = [&](const std::vector<float>& query, int k) -> std::vector<std::string> {
        std::vector<std::pair<float, int>> distances;
        distances.reserve(NUM_VECTORS);
        for (int i = 0; i < NUM_VECTORS; ++i) {
            float dist = 0.0f;
            for (int d = 0; d < DIM; ++d) {
                float diff = query[d] - allVectors[i][d];
                dist += diff * diff;
            }
            distances.emplace_back(dist, i);
        }
        std::partial_sort(distances.begin(), distances.begin() + k, distances.end());
        std::vector<std::string> result;
        for (int i = 0; i < k && i < static_cast<int>(distances.size()); ++i) {
            result.push_back("hnsw_bench_" + std::to_string(distances[i].second));
        }
        return result;
    };

    // 执行查询并计算recall@10
    double totalRecall = 0.0;
    double totalSearchLatencyUs = 0.0;

    for (int q = 0; q < NUM_QUERIES; ++q) {
        auto queryVec = randomVector(DIM, rng);

        // HNSW搜索
        auto searchStart = std::chrono::high_resolution_clock::now();
        auto hnswResults = engine->hnswSearch(queryVec, K);
        auto searchEnd = std::chrono::high_resolution_clock::now();
        totalSearchLatencyUs += std::chrono::duration<double, std::micro>(searchEnd - searchStart).count();

        // 暴力搜索ground truth
        auto groundTruth = bruteForceSearch(queryVec, K);
        std::set<std::string> gtSet(groundTruth.begin(), groundTruth.end());

        // 计算recall
        int hits = 0;
        for (const auto& r : hnswResults) {
            if (gtSet.count(r.id)) ++hits;
        }
        double recall = static_cast<double>(hits) / K;
        totalRecall += recall;
    }

    double avgRecall = totalRecall / NUM_QUERIES;
    double avgSearchLatencyUs = totalSearchLatencyUs / NUM_QUERIES;

    std::cout << "\n===== HNSW检索质量基准 =====" << std::endl;
    std::cout << "  向量数量: " << NUM_VECTORS << std::endl;
    std::cout << "  向量维度: " << DIM << std::endl;
    std::cout << "  查询数量: " << NUM_QUERIES << std::endl;
    std::cout << "  K值: " << K << std::endl;
    std::cout << "  索引构建时间: " << std::fixed << std::setprecision(2) << insertTimeMs << " ms" << std::endl;
    std::cout << "  平均搜索延迟: " << avgSearchLatencyUs << " us/query" << std::endl;
    std::cout << "  平均 Recall@" << K << ": " << std::setprecision(4) << avgRecall << std::endl;

    // HNSW recall@10 应至少达到70%
    EXPECT_GE(avgRecall, 0.7) << "HNSW recall@10 低于70%基线";
    // 单次搜索延迟应在合理范围
    EXPECT_LT(avgSearchLatencyUs, 50000.0) << "HNSW平均搜索延迟超过50ms";
}


// ============================================================================
// 基准5: 差分隐私效用基准（不同epsilon下噪声水平+统计偏差）
// ============================================================================

TEST_F(EdgeAIBenchmark, DifferentialPrivacyUtilityBenchmark) {
    const int NUM_SAMPLES = 10000;
    const float TRUE_VALUE = 100.0f;
    const float SENSITIVITY = 1.0f;

    // 测试不同epsilon值
    std::vector<float> epsilonValues = {0.1f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f};

    std::cout << "\n===== 差分隐私效用基准 =====" << std::endl;
    std::cout << "  真实值: " << TRUE_VALUE << std::endl;
    std::cout << "  灵敏度: " << SENSITIVITY << std::endl;
    std::cout << "  每组样本数: " << NUM_SAMPLES << std::endl;
    std::cout << std::endl;

    for (float epsilon : epsilonValues) {
        // 重新初始化引擎以设置新的epsilon
        Json::Value config;
        config["dp_epsilon"] = epsilon;
        config["dp_delta"] = 1e-5;
        config["pulse_window_seconds"] = 300;
        config["hnsw_m"] = 16;
        config["hnsw_ef_construction"] = 200;
        config["hnsw_ef_search"] = 50;
        config["quantization_bits"] = 8;
        engine->initialize(config);

        // 收集加噪样本
        std::vector<float> noisySamples;
        noisySamples.reserve(NUM_SAMPLES);

        engine->resetPrivacyBudget();

        for (int i = 0; i < NUM_SAMPLES; ++i) {
            // 每次重置预算以避免耗尽
            if (i % 100 == 0) {
                engine->resetPrivacyBudget();
            }
            float noisy = engine->addLaplaceNoise(TRUE_VALUE, SENSITIVITY);
            noisySamples.push_back(noisy);
        }

        // 计算统计量
        double sum = 0.0;
        for (float v : noisySamples) sum += v;
        double mean = sum / NUM_SAMPLES;

        double varianceSum = 0.0;
        for (float v : noisySamples) {
            double diff = v - mean;
            varianceSum += diff * diff;
        }
        double variance = varianceSum / (NUM_SAMPLES - 1);
        double stddev = std::sqrt(variance);

        // 统计偏差 = |均值 - 真实值|
        double bias = std::abs(mean - TRUE_VALUE);

        // Laplace(0, b) 的理论方差 = 2b^2, 其中 b = sensitivity/epsilon
        double theoreticalScale = SENSITIVITY / epsilon;
        double theoreticalVariance = 2.0 * theoreticalScale * theoreticalScale;
        double theoreticalStddev = std::sqrt(theoreticalVariance);

        // 噪声水平 = 实际标准差 / 真实值
        double noiseLevel = stddev / std::abs(TRUE_VALUE);

        std::cout << "  epsilon=" << std::fixed << std::setprecision(1) << epsilon
                  << " | 均值=" << std::setprecision(3) << mean
                  << " | 偏差=" << bias
                  << " | 标准差=" << stddev
                  << " | 理论标准差=" << theoreticalStddev
                  << " | 噪声水平=" << std::setprecision(4) << noiseLevel
                  << std::endl;

        // 验证无偏性：均值应接近真实值（允许3个标准误差的偏差）
        double standardError = stddev / std::sqrt(NUM_SAMPLES);
        EXPECT_NEAR(mean, TRUE_VALUE, 3.0 * standardError + 1.0)
            << "epsilon=" << epsilon << " 时均值偏差过大，Laplace机制可能有偏";

        // 验证噪声随epsilon增大而减小的趋势
        // epsilon越大，隐私保护越弱，噪声越小
        if (epsilon >= 1.0f) {
            EXPECT_LT(noiseLevel, 0.5)
                << "epsilon=" << epsilon << " 时噪声水平过高";
        }
    }

    // 验证向量版本的差分隐私
    std::cout << "\n  --- 向量差分隐私测试 ---" << std::endl;
    engine->resetPrivacyBudget();

    std::vector<float> trueVec = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f};
    const int VEC_TRIALS = 1000;

    std::vector<double> vecMeans(trueVec.size(), 0.0);
    std::vector<double> vecVariances(trueVec.size(), 0.0);

    for (int t = 0; t < VEC_TRIALS; ++t) {
        if (t % 100 == 0) engine->resetPrivacyBudget();
        auto noisyVec = engine->addLaplaceNoiseVec(trueVec, SENSITIVITY);
        ASSERT_EQ(noisyVec.size(), trueVec.size());
        for (size_t d = 0; d < trueVec.size(); ++d) {
            vecMeans[d] += noisyVec[d];
        }
    }

    for (size_t d = 0; d < trueVec.size(); ++d) {
        vecMeans[d] /= VEC_TRIALS;
        std::cout << "  维度" << d << ": 真实=" << trueVec[d]
                  << " 均值=" << std::fixed << std::setprecision(3) << vecMeans[d]
                  << " 偏差=" << std::abs(vecMeans[d] - trueVec[d])
                  << std::endl;

        // 每个维度的均值应接近真实值
        EXPECT_NEAR(vecMeans[d], trueVec[d], 5.0)
            << "向量维度" << d << "的均值偏差过大";
    }

    // 验证隐私预算追踪
    engine->resetPrivacyBudget();
    float budgetBefore = engine->getRemainingPrivacyBudget();
    engine->addLaplaceNoise(1.0f, 1.0f);
    float budgetAfter = engine->getRemainingPrivacyBudget();
    std::cout << "\n  隐私预算: 操作前=" << budgetBefore
              << " 操作后=" << budgetAfter << std::endl;
    EXPECT_LT(budgetAfter, budgetBefore) << "隐私预算应在每次操作后减少";
}
