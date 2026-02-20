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
