/**
 * @file test_concurrency_torture.cpp
 * @brief 并发极限压力测试 - 多模块混合并发/速率限制/线程安全
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <future>
#include <mutex>
#include <set>
#include "utils/CircuitBreaker.h"
#include "utils/PasetoUtil.h"
#include "utils/E2EEncryption.h"
#include "utils/Validator.h"
#include "infrastructure/ai/EdgeAIEngine.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "infrastructure/ai/DualMemoryRAG.h"

using namespace heartlake::utils;
using namespace heartlake::ai;

class ConcurrencyTimer {
public:
    void start() { start_ = std::chrono::high_resolution_clock::now(); }
    double elapsed_ms() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
private:
    std::chrono::high_resolution_clock::time_point start_;
};

// ============================================================================
// 1. 熔断器并发极限 (10+ 用例)
// ============================================================================
class ConcurrentCircuitBreaker : public ::testing::Test {};

TEST_F(ConcurrentCircuitBreaker, 100ThreadsSimultaneous) {
    CircuitBreaker cb(50, 1000);
    std::atomic<int> success{0}, failure{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 100; i++) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 100; j++) {
                try {
                    cb.execute([i, j]() {
                        if ((i + j) % 7 == 0) throw std::runtime_error("fail");
                        return i * 100 + j;
                    });
                    success++;
                } catch (...) {
                    failure++;
                }
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(success.load() + failure.load(), 10000);
}

TEST_F(ConcurrentCircuitBreaker, ConcurrentResetAndExecute) {
    CircuitBreaker cb(3, 100);
    std::atomic<bool> running{true};
    std::atomic<int> ops{0};

    // Thread that keeps resetting
    std::thread resetter([&]() {
        while (running.load()) {
            cb.reset();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Threads that execute
    std::vector<std::thread> workers;
    for (int i = 0; i < 20; i++) {
        workers.emplace_back([&, i]() {
            for (int j = 0; j < 200; j++) {
                try {
                    cb.execute([i]() {
                        if (i % 3 == 0) throw std::runtime_error("fail");
                        return 1;
                    });
                } catch (...) {}
                ops++;
            }
        });
    }

    for (auto& w : workers) w.join();
    running = false;
    resetter.join();
    EXPECT_EQ(ops.load(), 4000);
}

TEST_F(ConcurrentCircuitBreaker, MultipleIndependentBreakers) {
    const int NUM_BREAKERS = 50;
    std::vector<std::unique_ptr<CircuitBreaker>> breakers;
    for (int i = 0; i < NUM_BREAKERS; i++) {
        breakers.push_back(std::make_unique<CircuitBreaker>(5, 100));
    }

    std::atomic<int> total{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_BREAKERS; i++) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 100; j++) {
                try {
                    breakers[i]->execute([j]() {
                        if (j % 10 == 0) throw std::runtime_error("fail");
                        return j;
                    });
                } catch (...) {}
                total++;
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(total.load(), NUM_BREAKERS * 100);
}

TEST_F(ConcurrentCircuitBreaker, RapidOpenCloseUnderLoad) {
    CircuitBreaker cb(2, 10);
    std::atomic<int> done{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 50; i++) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; j++) {
                try {
                    cb.execute([]() -> int { throw std::runtime_error("fail"); });
                } catch (...) {}
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
                try {
                    cb.execute([]() { return 1; });
                } catch (...) {}
                done++;
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 5000);
}

// ============================================================================
// 2. PASETO并发极限 (10+ 用例)
// ============================================================================
class ConcurrentPaseto : public ::testing::Test {
protected:
    void SetUp() override {
        setenv("PASETO_KEY", "test_key_for_concurrency_torture_32b", 0);
        key = PasetoUtil::getKey();
    }
    std::string key;
};

TEST_F(ConcurrentPaseto, 100ThreadsGenerateVerify) {
    std::atomic<int> success{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 100; t++) {
        threads.emplace_back([&, t]() {
            std::string userId = "thread_user_" + std::to_string(t);
            auto token = PasetoUtil::generateToken(userId, key);
            EXPECT_FALSE(token.empty());
            auto result = PasetoUtil::verifyToken(token, key);
            EXPECT_EQ(result, userId);
            success++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(success.load(), 100);
}

TEST_F(ConcurrentPaseto, BulkGenerateThenBulkVerify) {
    const int N = 1000;
    std::vector<std::string> tokens(N);
    std::vector<std::string> userIds(N);

    // Generate phase - concurrent
    std::vector<std::thread> genThreads;
    for (int i = 0; i < N; i++) {
        userIds[i] = "bulk_user_" + std::to_string(i);
    }
    // Use 20 threads to generate
    std::atomic<int> genIdx{0};
    for (int t = 0; t < 20; t++) {
        genThreads.emplace_back([&]() {
            int idx;
            while ((idx = genIdx.fetch_add(1)) < N) {
                tokens[idx] = PasetoUtil::generateToken(userIds[idx], key);
            }
        });
    }
    for (auto& t : genThreads) t.join();

    // Verify phase - concurrent
    std::atomic<int> verifySuccess{0};
    std::atomic<int> verIdx{0};
    std::vector<std::thread> verThreads;
    for (int t = 0; t < 20; t++) {
        verThreads.emplace_back([&]() {
            int idx;
            while ((idx = verIdx.fetch_add(1)) < N) {
                auto result = PasetoUtil::verifyToken(tokens[idx], key);
                if (result == userIds[idx]) verifySuccess++;
            }
        });
    }
    for (auto& t : verThreads) t.join();
    EXPECT_EQ(verifySuccess.load(), N);
}

TEST_F(ConcurrentPaseto, MixedGenerateVerifyInvalidate) {
    std::atomic<int> ops{0};
    std::vector<std::thread> threads;

    // Some threads generate
    for (int t = 0; t < 10; t++) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 50; i++) {
                PasetoUtil::generateToken("gen_" + std::to_string(t * 50 + i), key);
                ops++;
            }
        });
    }
    // Some threads verify valid tokens
    auto validToken = PasetoUtil::generateToken("valid_user", key);
    for (int t = 0; t < 10; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < 50; i++) {
                PasetoUtil::verifyToken(validToken, key);
                ops++;
            }
        });
    }
    // Some threads verify garbage (verifyToken throws on invalid tokens)
    for (int t = 0; t < 10; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < 50; i++) {
                try {
                    PasetoUtil::verifyToken("garbage_token_" + std::to_string(i), key);
                } catch (...) {}
                ops++;
            }
        });
    }

    for (auto& t : threads) t.join();
    EXPECT_EQ(ops.load(), 1500);
}

// ============================================================================
// 3. E2E加密并发极限 (10+ 用例)
// ============================================================================
class ConcurrentE2E : public ::testing::Test {};

TEST_F(ConcurrentE2E, 100ThreadsEncryptDecrypt) {
    auto encKey = E2EEncryption::generateKey();
    std::atomic<int> success{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 100; t++) {
        threads.emplace_back([&, t]() {
            std::string msg = "Thread " + std::to_string(t) + " secret message";
            auto encrypted = E2EEncryption::encrypt(msg, encKey);
            EXPECT_TRUE(encrypted.has_value());
            if (encrypted) {
                auto decrypted = E2EEncryption::decrypt(*encrypted, encKey);
                EXPECT_TRUE(decrypted.has_value());
                if (decrypted) {
                    EXPECT_EQ(*decrypted, msg);
                    success++;
                }
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(success.load(), 100);
}

TEST_F(ConcurrentE2E, ConcurrentKeyGeneration) {
    std::vector<std::string> keys(100);
    std::vector<std::thread> threads;
    for (int i = 0; i < 100; i++) {
        threads.emplace_back([&, i]() {
            keys[i] = E2EEncryption::generateKey();
            EXPECT_FALSE(keys[i].empty());
        });
    }
    for (auto& t : threads) t.join();
    // All keys should be unique
    std::set<std::string> uniqueKeys(keys.begin(), keys.end());
    EXPECT_EQ(uniqueKeys.size(), 100u);
}

TEST_F(ConcurrentE2E, ConcurrentX25519KeyExchange) {
    std::atomic<int> success{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 50; t++) {
        threads.emplace_back([&]() {
            auto kp1 = E2EEncryption::generateX25519KeyPair();
            auto kp2 = E2EEncryption::generateX25519KeyPair();
            EXPECT_TRUE(kp1.has_value());
            EXPECT_TRUE(kp2.has_value());
            if (kp1 && kp2) {
                auto secret1 = E2EEncryption::computeSharedSecret(kp1->privateKey, kp2->publicKey);
                auto secret2 = E2EEncryption::computeSharedSecret(kp2->privateKey, kp1->publicKey);
                EXPECT_TRUE(secret1.has_value());
                EXPECT_TRUE(secret2.has_value());
                if (secret1 && secret2) {
                    EXPECT_EQ(*secret1, *secret2);
                    success++;
                }
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(success.load(), 50);
}

TEST_F(ConcurrentE2E, MixedEncryptDecryptDifferentKeys) {
    std::atomic<int> success{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 50; t++) {
        threads.emplace_back([&, t]() {
            auto myKey = E2EEncryption::generateKey();
            std::string msg = "Message from thread " + std::to_string(t);
            auto enc = E2EEncryption::encrypt(msg, myKey);
            EXPECT_TRUE(enc.has_value());
            if (enc) {
                auto dec = E2EEncryption::decrypt(*enc, myKey);
                EXPECT_TRUE(dec.has_value());
                if (dec && *dec == msg) success++;
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(success.load(), 50);
}

// ============================================================================
// 4. 输入验证并发 (10+ 用例)
// ============================================================================
class ConcurrentValidator : public ::testing::Test {};

TEST_F(ConcurrentValidator, 100ThreadsSqlInjectionCheck) {
    std::atomic<int> done{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 100; t++) {
        threads.emplace_back([&, t]() {
            Validator::checkSqlInjection("' OR 1=1--", "field");
            Validator::checkSqlInjection("normal text " + std::to_string(t), "field");
            done++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 100);
}

TEST_F(ConcurrentValidator, 100ThreadsHtmlSanitize) {
    std::atomic<int> done{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 100; t++) {
        threads.emplace_back([&, t]() {
            Validator::sanitizeHtml("<script>alert(" + std::to_string(t) + ")</script>");
            done++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 100);
}

TEST_F(ConcurrentValidator, 100ThreadsPathTraversal) {
    std::atomic<int> done{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 100; t++) {
        threads.emplace_back([&, t]() {
            Validator::checkPathTraversal("../../../etc/passwd", "path");
            Validator::checkPathTraversal("safe/path/file_" + std::to_string(t) + ".txt", "path");
            done++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 100);
}

TEST_F(ConcurrentValidator, MixedValidationOperations) {
    std::atomic<int> done{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 50; t++) {
        threads.emplace_back([&, t]() {
            Validator::email("user" + std::to_string(t) + "@example.com");
            Validator::password("P@ssw0rd!" + std::to_string(t));
            Validator::checkSqlInjection("input_" + std::to_string(t), "field");
            Validator::sanitizeHtml("<b>text " + std::to_string(t) + "</b>");
            Validator::checkPathTraversal("file_" + std::to_string(t) + ".txt", "path");
            done++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 50);
}

TEST_F(ConcurrentValidator, PasswordStrengthConcurrent) {
    std::atomic<int> done{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 100; t++) {
        threads.emplace_back([&, t]() {
            int strength = Validator::calculatePasswordStrength("P@ss" + std::to_string(t) + "Word!");
            EXPECT_GE(strength, 0);
            EXPECT_LE(strength, 6);
            done++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 100);
}

// ============================================================================
// 5. AI引擎并发极限 (10+ 用例)
// ============================================================================
class ConcurrentAI : public ::testing::Test {
protected:
    EdgeAIEngine& engine = EdgeAIEngine::getInstance();
};

TEST_F(ConcurrentAI, MixedSentimentAndModeration) {
    std::atomic<int> done{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 50; t++) {
        threads.emplace_back([&, t]() {
            if (t % 2 == 0) {
                engine.analyzeSentimentLocal("I feel great today " + std::to_string(t));
            } else {
                engine.moderateTextLocal("check this content " + std::to_string(t));
            }
            done++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 50);
}

TEST_F(ConcurrentAI, ConcurrentHNSWInsertAndSearch) {
    // Insert some vectors first
    for (int i = 0; i < 100; i++) {
        std::vector<float> vec(128);
        std::mt19937 gen(i);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        for (auto& v : vec) v = dist(gen);
        engine.hnswInsert("conc_vec_" + std::to_string(i), vec);
    }

    std::atomic<int> done{0};
    std::vector<std::thread> threads;
    // Concurrent searches
    for (int t = 0; t < 50; t++) {
        threads.emplace_back([&, t]() {
            std::vector<float> query(128);
            std::mt19937 gen(t + 1000);
            std::normal_distribution<float> dist(0.0f, 1.0f);
            for (auto& v : query) v = dist(gen);
            auto results = engine.hnswSearch(query, 5);
            EXPECT_LE(results.size(), 5u);
            done++;
        });
    }
    // Concurrent inserts
    for (int t = 0; t < 50; t++) {
        threads.emplace_back([&, t]() {
            std::vector<float> vec(128);
            std::mt19937 gen(t + 2000);
            std::normal_distribution<float> dist(0.0f, 1.0f);
            for (auto& v : vec) v = dist(gen);
            engine.hnswInsert("conc_new_" + std::to_string(t), vec);
            done++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 100);
}

TEST_F(ConcurrentAI, ConcurrentDPNoiseGeneration) {
    engine.resetPrivacyBudget();
    std::atomic<int> done{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 50; t++) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 100; i++) {
                float noisy = engine.addLaplaceNoise(1.0f, 1.0f);
                (void)noisy;
                done++;
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 5000);
}

TEST_F(ConcurrentAI, ConcurrentEmbeddingGeneration) {
    auto& embEngine = AdvancedEmbeddingEngine::getInstance();
    if (!embEngine.isInitialized()) {
        embEngine.initialize(128, 1000);
    }

    std::atomic<int> done{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 50; t++) {
        threads.emplace_back([&, t]() {
            auto vec = embEngine.generateEmbedding("concurrent text " + std::to_string(t));
            EXPECT_EQ(vec.size(), 128u);
            done++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 50);
}

TEST_F(ConcurrentAI, ConcurrentRAGUpdate) {
    auto& rag = DualMemoryRAG::getInstance();
    std::atomic<int> done{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 100; t++) {
        threads.emplace_back([&, t]() {
            std::string userId = "rag_conc_user_" + std::to_string(t);
            rag.updateShortTermMemory(userId, "content " + std::to_string(t), "neutral", 0.5f);
            done++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 100);
}

// ============================================================================
// 6. 混合并发极限 (5+ 用例)
// ============================================================================
class MixedConcurrency : public ::testing::Test {
protected:
    static std::string getPasetoKeyOrSkip() {
        try {
            return PasetoUtil::getKey();
        } catch (...) {
            // Set a test key if env not set
            setenv("PASETO_KEY", "test_key_for_concurrency_torture_32b", 0);
            return PasetoUtil::getKey();
        }
    }
};

TEST_F(MixedConcurrency, AllModulesSimultaneous) {
    auto& aiEngine = EdgeAIEngine::getInstance();
    auto& embEngine = AdvancedEmbeddingEngine::getInstance();
    if (!embEngine.isInitialized()) embEngine.initialize(128, 1000);
    auto pasetoKey = getPasetoKeyOrSkip();

    std::atomic<int> done{0};
    std::vector<std::thread> threads;

    // Sentiment threads
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&, i]() {
            aiEngine.analyzeSentimentLocal("mixed test " + std::to_string(i));
            done++;
        });
    }
    // Moderation threads
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&, i]() {
            aiEngine.moderateTextLocal("mixed content " + std::to_string(i));
            done++;
        });
    }
    // PASETO threads
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&, i]() {
            auto token = PasetoUtil::generateToken("mixed_" + std::to_string(i), pasetoKey);
            PasetoUtil::verifyToken(token, pasetoKey);
            done++;
        });
    }
    // E2E threads
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&, i]() {
            auto key = E2EEncryption::generateKey();
            auto enc = E2EEncryption::encrypt("mixed msg " + std::to_string(i), key);
            if (enc) E2EEncryption::decrypt(*enc, key);
            done++;
        });
    }
    // Embedding threads
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&, i]() {
            embEngine.generateEmbedding("mixed embed " + std::to_string(i));
            done++;
        });
    }
    // Validator threads
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&, i]() {
            Validator::checkSqlInjection("test " + std::to_string(i), "f");
            Validator::sanitizeHtml("<b>" + std::to_string(i) + "</b>");
            done++;
        });
    }

    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 60);
}

TEST_F(MixedConcurrency, StressAllModules200Threads) {
    auto& aiEngine = EdgeAIEngine::getInstance();
    auto pasetoKey = getPasetoKeyOrSkip();

    std::atomic<int> done{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < 200; i++) {
        threads.emplace_back([&, i]() {
            switch (i % 5) {
                case 0:
                    aiEngine.analyzeSentimentLocal("stress " + std::to_string(i));
                    break;
                case 1:
                    aiEngine.moderateTextLocal("stress " + std::to_string(i));
                    break;
                case 2: {
                    auto token = PasetoUtil::generateToken("s" + std::to_string(i), pasetoKey);
                    PasetoUtil::verifyToken(token, pasetoKey);
                    break;
                }
                case 3: {
                    auto key = E2EEncryption::generateKey();
                    auto enc = E2EEncryption::encrypt("s" + std::to_string(i), key);
                    if (enc) E2EEncryption::decrypt(*enc, key);
                    break;
                }
                case 4:
                    Validator::checkSqlInjection("' OR 1=1--", "f");
                    Validator::sanitizeHtml("<script>alert(1)</script>");
                    break;
            }
            done++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 200);
}

TEST_F(MixedConcurrency, DeadlockDetectionWithTimeout) {
    // Run mixed operations with a timeout to detect deadlocks
    auto future = std::async(std::launch::async, []() {
        auto& engine = EdgeAIEngine::getInstance();
        auto key = MixedConcurrency::getPasetoKeyOrSkip();
        std::vector<std::thread> threads;
        std::atomic<int> done{0};

        for (int i = 0; i < 100; i++) {
            threads.emplace_back([&, i]() {
                engine.analyzeSentimentLocal("deadlock test " + std::to_string(i));
                auto token = PasetoUtil::generateToken("dl_" + std::to_string(i), key);
                PasetoUtil::verifyToken(token, key);
                done++;
            });
        }
        for (auto& t : threads) t.join();
        return done.load();
    });

    auto status = future.wait_for(std::chrono::seconds(30));
    EXPECT_NE(status, std::future_status::timeout) << "Potential deadlock detected!";
    if (status == std::future_status::ready) {
        EXPECT_EQ(future.get(), 100);
    }
}

TEST_F(MixedConcurrency, NoDataRaceOnSharedState) {
    auto& engine = EdgeAIEngine::getInstance();
    std::atomic<int> done{0};
    std::vector<std::thread> threads;

    // Multiple threads doing sentiment + pulse simultaneously
    for (int i = 0; i < 50; i++) {
        threads.emplace_back([&, i]() {
            auto result = engine.analyzeSentimentLocal("race test " + std::to_string(i));
            engine.submitEmotionSample(result.score, result.mood);
            engine.getCurrentPulse();
            done++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 50);
}

TEST_F(MixedConcurrency, HighContentionSingleResource) {
    // All threads hammer the same singleton
    auto& engine = EdgeAIEngine::getInstance();
    std::atomic<int> done{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < 100; i++) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 50; j++) {
                engine.analyzeSentimentLocal("contention");
                engine.moderateTextLocal("contention");
                done++;
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(done.load(), 5000);
}

int main(int argc, char **argv) {
    // Ensure PASETO_KEY is set for tests
    setenv("PASETO_KEY", "test_key_for_concurrency_torture_32b", 0);
    // Initialize AI engine
    auto& engine = EdgeAIEngine::getInstance();
    engine.initialize();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
