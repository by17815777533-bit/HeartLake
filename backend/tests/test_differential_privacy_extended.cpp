/**
 * @file test_differential_privacy_extended.cpp
 * @brief DifferentialPrivacyEngine 扩展测试
 */

#include <gtest/gtest.h>
#include "infrastructure/privacy/DifferentialPrivacyEngine.h"
#include <cmath>
#include <numeric>
#include <vector>
#include <algorithm>

using namespace heartlake::privacy;

class DPExtTest : public ::testing::Test {
protected:
    DifferentialPrivacyEngine* engine;
    void SetUp() override {
        engine = &DifferentialPrivacyEngine::getInstance();
    }
};

// =====================================================================
// Laplace 噪声统计验证
// =====================================================================

TEST_F(DPExtTest, Laplace_MedianNearOriginal) {
    const double val = 42.0;
    const int N = 5000;
    std::vector<double> samples(N);
    for (int i = 0; i < N; ++i) {
        samples[i] = engine->addLaplaceNoise(val, 1.0, 1.0);
    }
    std::sort(samples.begin(), samples.end());
    double median = samples[N / 2];
    EXPECT_NEAR(median, val, 2.0);
}

TEST_F(DPExtTest, Laplace_VarianceMatchesTheory) {
    // Var(Laplace) = 2 * b^2 where b = sensitivity/epsilon
    const double val = 0.0, sens = 1.0, eps = 1.0;
    const double expectedVar = 2.0 * (sens / eps) * (sens / eps);
    const int N = 10000;
    double sum = 0.0, sumSq = 0.0;
    for (int i = 0; i < N; ++i) {
        double noisy = engine->addLaplaceNoise(val, sens, eps);
        sum += noisy;
        sumSq += noisy * noisy;
    }
    double mean = sum / N;
    double variance = sumSq / N - mean * mean;
    EXPECT_NEAR(variance, expectedVar, 1.0);
}

TEST_F(DPExtTest, Laplace_Eps01_MoreNoiseThanEps10) {
    const double val = 50.0;
    const int N = 3000;
    double var01 = 0.0, var10 = 0.0;
    for (int i = 0; i < N; ++i) {
        double n1 = engine->addLaplaceNoise(val, 1.0, 0.1) - val;
        var01 += n1 * n1;
        double n2 = engine->addLaplaceNoise(val, 1.0, 10.0) - val;
        var10 += n2 * n2;
    }
    EXPECT_GT(var01 / N, var10 / N);
}

TEST_F(DPExtTest, Laplace_Sensitivity01_LessNoise) {
    const double val = 50.0;
    const int N = 3000;
    double varLow = 0.0, varHigh = 0.0;
    for (int i = 0; i < N; ++i) {
        double n1 = engine->addLaplaceNoise(val, 0.1, 1.0) - val;
        varLow += n1 * n1;
        double n2 = engine->addLaplaceNoise(val, 5.0, 1.0) - val;
        varHigh += n2 * n2;
    }
    EXPECT_GT(varHigh / N, varLow / N);
}

TEST_F(DPExtTest, Laplace_NegativeValue) {
    double result = engine->addLaplaceNoise(-100.0, 1.0, 1.0);
    EXPECT_TRUE(std::isfinite(result));
    // Mean should be near -100
    double sum = 0.0;
    for (int i = 0; i < 3000; ++i) {
        sum += engine->addLaplaceNoise(-100.0, 1.0, 1.0);
    }
    EXPECT_NEAR(sum / 3000, -100.0, 2.0);
}

TEST_F(DPExtTest, Laplace_ZeroValue) {
    double sum = 0.0;
    for (int i = 0; i < 5000; ++i) {
        sum += engine->addLaplaceNoise(0.0, 1.0, 1.0);
    }
    EXPECT_NEAR(sum / 5000, 0.0, 1.0);
}

TEST_F(DPExtTest, Laplace_LargeEpsilon_SmallNoise) {
    const double val = 100.0;
    const int N = 1000;
    double maxDiff = 0.0;
    for (int i = 0; i < N; ++i) {
        double diff = std::abs(engine->addLaplaceNoise(val, 1.0, 100.0) - val);
        maxDiff = std::max(maxDiff, diff);
    }
    // With eps=100, noise should be very small
    EXPECT_LT(maxDiff, 5.0);
}

TEST_F(DPExtTest, Laplace_ZeroEpsilon_HandledGracefully) {
    double result = engine->addLaplaceNoise(50.0, 1.0, 0.0);
    EXPECT_TRUE(std::isfinite(result));
}

TEST_F(DPExtTest, Laplace_NegativeEpsilon_HandledGracefully) {
    double result = engine->addLaplaceNoise(50.0, 1.0, -1.0);
    EXPECT_TRUE(std::isfinite(result));
}

TEST_F(DPExtTest, Laplace_VerySmallEpsilon_LargeNoise) {
    const double val = 50.0;
    double var = 0.0;
    const int N = 1000;
    for (int i = 0; i < N; ++i) {
        double diff = engine->addLaplaceNoise(val, 1.0, 0.01) - val;
        var += diff * diff;
    }
    EXPECT_GT(var / N, 100.0);  // Should have large variance
}

TEST_F(DPExtTest, Laplace_ZeroSensitivity_NoNoise) {
    const double val = 42.0;
    // With sensitivity=0, noise scale = 0/eps = 0
    double result = engine->addLaplaceNoise(val, 0.0, 1.0);
    EXPECT_NEAR(result, val, 0.001);
}

// =====================================================================
// 隐私报告
// =====================================================================

TEST_F(DPExtTest, PrivacyReport_HasAllFields) {
    auto report = engine->getPrivacyReport();
    EXPECT_TRUE(report.isMember("total_epsilon_consumed"));
    EXPECT_TRUE(report.isMember("query_count"));
    EXPECT_TRUE(report.isMember("privacy_level"));
    EXPECT_TRUE(report.isMember("recommendation"));
}

TEST_F(DPExtTest, PrivacyReport_LevelIsValid) {
    auto report = engine->getPrivacyReport();
    std::string level = report["privacy_level"].asString();
    EXPECT_TRUE(level == "strong" || level == "moderate" || level == "weak");
}

TEST_F(DPExtTest, PrivacyReport_QueryCountNonNegative) {
    auto report = engine->getPrivacyReport();
    EXPECT_GE(report["query_count"].asInt(), 0);
}

TEST_F(DPExtTest, PrivacyReport_EpsilonNonNegative) {
    auto report = engine->getPrivacyReport();
    EXPECT_GE(report["total_epsilon_consumed"].asDouble(), 0.0);
}

TEST_F(DPExtTest, PrivacyReport_RecommendationNotEmpty) {
    auto report = engine->getPrivacyReport();
    EXPECT_FALSE(report["recommendation"].asString().empty());
}

// =====================================================================
// 多次查询后预算累积
// =====================================================================

TEST_F(DPExtTest, Budget_IncreasesAfterQueries) {
    auto r1 = engine->getPrivacyReport();
    double eps1 = r1["total_epsilon_consumed"].asDouble();
    int q1 = r1["query_count"].asInt();

    // Perform some queries
    for (int i = 0; i < 10; ++i) {
        engine->addLaplaceNoise(1.0, 1.0, 1.0);
    }

    auto r2 = engine->getPrivacyReport();
    double eps2 = r2["total_epsilon_consumed"].asDouble();
    int q2 = r2["query_count"].asInt();

    EXPECT_GE(eps2, eps1);
    EXPECT_GE(q2, q1);
}

TEST_F(DPExtTest, Laplace_LargeValue) {
    double result = engine->addLaplaceNoise(1e10, 1.0, 1.0);
    EXPECT_TRUE(std::isfinite(result));
    EXPECT_NEAR(result, 1e10, 1e8);
}

TEST_F(DPExtTest, Laplace_VerySmallValue) {
    double result = engine->addLaplaceNoise(1e-10, 1.0, 1.0);
    EXPECT_TRUE(std::isfinite(result));
}

TEST_F(DPExtTest, Laplace_LargeSensitivity) {
    double result = engine->addLaplaceNoise(0.0, 1000.0, 1.0);
    EXPECT_TRUE(std::isfinite(result));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    _exit(ret);
}
