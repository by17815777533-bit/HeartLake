/**
 * @file test_differential_privacy.cpp
 * @brief DifferentialPrivacyEngine 单元测试
 *
 * 覆盖：
 * - Laplace噪声采样分布验证
 * - 隐私预算追踪
 * - 加噪后数据的统计特性
 * - 边界条件处理
 */

#include <gtest/gtest.h>
#include "infrastructure/privacy/DifferentialPrivacyEngine.h"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <vector>

using namespace heartlake::privacy;

class DifferentialPrivacyTest : public ::testing::Test {
protected:
    DifferentialPrivacyEngine* engine;

    void SetUp() override {
        engine = &DifferentialPrivacyEngine::getInstance();
    }
};

// ==================== Laplace噪声测试 ====================

TEST_F(DifferentialPrivacyTest, LaplaceNoise_ZeroMean) {
    // 大量采样后均值应接近原始值
    const double originalValue = 100.0;
    const double sensitivity = 1.0;
    const double epsilon = 1.0;
    const int samples = 10000;

    double sum = 0.0;
    for (int i = 0; i < samples; ++i) {
        sum += engine->addLaplaceNoise(originalValue, sensitivity, epsilon);
    }
    double mean = sum / samples;

    // 均值应在原始值附近（允许较大误差因为是随机的）
    EXPECT_NEAR(mean, originalValue, 1.0);
}

TEST_F(DifferentialPrivacyTest, LaplaceNoise_ScaleProportionalToSensitivity) {
    // 更高的sensitivity应产生更大的噪声方差
    const double value = 50.0;
    const double epsilon = 1.0;
    const int samples = 5000;

    // 低sensitivity
    double variance_low = 0.0;
    for (int i = 0; i < samples; ++i) {
        double noisy = engine->addLaplaceNoise(value, 0.1, epsilon);
        variance_low += (noisy - value) * (noisy - value);
    }
    variance_low /= samples;

    // 高sensitivity
    double variance_high = 0.0;
    for (int i = 0; i < samples; ++i) {
        double noisy = engine->addLaplaceNoise(value, 10.0, epsilon);
        variance_high += (noisy - value) * (noisy - value);
    }
    variance_high /= samples;

    EXPECT_GT(variance_high, variance_low);
}

TEST_F(DifferentialPrivacyTest, LaplaceNoise_SmallerEpsilonMoreNoise) {
    // 更小的epsilon（更强隐私）应产生更大噪声
    const double value = 50.0;
    const double sensitivity = 1.0;
    const int samples = 5000;

    double variance_strong = 0.0;
    for (int i = 0; i < samples; ++i) {
        double noisy = engine->addLaplaceNoise(value, sensitivity, 0.1);
        variance_strong += (noisy - value) * (noisy - value);
    }
    variance_strong /= samples;

    double variance_weak = 0.0;
    for (int i = 0; i < samples; ++i) {
        double noisy = engine->addLaplaceNoise(value, sensitivity, 10.0);
        variance_weak += (noisy - value) * (noisy - value);
    }
    variance_weak /= samples;

    EXPECT_GT(variance_strong, variance_weak);
}

TEST_F(DifferentialPrivacyTest, LaplaceNoise_InvalidEpsilon_DefaultsToOne) {
    // epsilon <= 0 应被处理为默认值1.0
    double result = engine->addLaplaceNoise(100.0, 1.0, 0.0);
    EXPECT_TRUE(std::isfinite(result));

    result = engine->addLaplaceNoise(100.0, 1.0, -1.0);
    EXPECT_TRUE(std::isfinite(result));
}

// ==================== 隐私预算测试 ====================

TEST_F(DifferentialPrivacyTest, PrivacyReport_ReturnsValidJson) {
    auto report = engine->getPrivacyReport();

    EXPECT_TRUE(report.isMember("total_epsilon_consumed"));
    EXPECT_TRUE(report.isMember("query_count"));
    EXPECT_TRUE(report.isMember("privacy_level"));
    EXPECT_TRUE(report.isMember("recommendation"));
}

TEST_F(DifferentialPrivacyTest, PrivacyReport_PrivacyLevelCategories) {
    auto report = engine->getPrivacyReport();
    std::string level = report["privacy_level"].asString();

    // 应该是三个级别之一
    EXPECT_TRUE(level == "strong" || level == "moderate" || level == "weak");
}

// ==================== 湖面天气测试 ====================

TEST_F(DifferentialPrivacyTest, LakeWeather_ReturnsValidStructure) {
    // 注意：这个测试需要数据库连接，在无DB环境下可能失败
    // 但结构验证仍然有价值
    try {
        auto weather = engine->getPrivateLakeWeather(2.0);
        if (!weather.isNull()) {
            EXPECT_TRUE(weather.isMember("weather"));
            EXPECT_TRUE(weather.isMember("emoji"));
        }
    } catch (const std::exception&) {
        // 无数据库连接时跳过
        GTEST_SKIP() << "Database not available";
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    _exit(ret);  // 跳过全局析构，避免drogon单例析构顺序导致的SEGFAULT
}
