/**
 * AdminConfigStore 单元测试 - 配置读写、默认值、并发安全
 */

#include <gtest/gtest.h>
#include "utils/AdminConfigStore.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

using namespace heartlake::utils;

// =====================================================================
// AdminConfigStore 测试
// =====================================================================

class AdminConfigStoreTest : public ::testing::Test {
protected:
    std::string testConfigPath_;

    void SetUp() override {
        // 使用临时目录隔离测试，避免污染真实配置
        testConfigPath_ = "/tmp/heartlake_test_admin_config_" +
                          std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ".json";
        setenv("ADMIN_CONFIG_FILE", testConfigPath_.c_str(), 1);
        // 确保测试开始时没有残留文件
        std::filesystem::remove(testConfigPath_);
    }

    void TearDown() override {
        std::filesystem::remove(testConfigPath_);
        unsetenv("ADMIN_CONFIG_FILE");
    }
};

// -------------------- configFilePath --------------------

TEST_F(AdminConfigStoreTest, ConfigFilePath_UsesEnvVariable) {
    auto path = AdminConfigStore::configFilePath();
    EXPECT_EQ(path, testConfigPath_);
}

// -------------------- defaultConfig --------------------

TEST_F(AdminConfigStoreTest, DefaultConfig_HasSystemSection) {
    auto config = AdminConfigStore::defaultConfig();
    EXPECT_TRUE(config.isMember("system"));
    EXPECT_EQ(config["system"]["name"].asString(), "HeartLake");
}

TEST_F(AdminConfigStoreTest, DefaultConfig_HasAiSection) {
    auto config = AdminConfigStore::defaultConfig();
    EXPECT_TRUE(config.isMember("ai"));
    EXPECT_TRUE(config["ai"]["enableSentiment"].asBool());
    EXPECT_TRUE(config["ai"]["enableModeration"].asBool());
}

TEST_F(AdminConfigStoreTest, DefaultConfig_HasRateSection) {
    auto config = AdminConfigStore::defaultConfig();
    EXPECT_TRUE(config.isMember("rate"));
    EXPECT_EQ(config["rate"]["stonePerHour"].asInt(), 15);
    EXPECT_EQ(config["rate"]["messagePerMinute"].asInt(), 60);
    EXPECT_EQ(config["rate"]["maxContentLength"].asInt(), 2000);
}

TEST_F(AdminConfigStoreTest, DefaultConfig_AllowRegisterIsTrue) {
    auto config = AdminConfigStore::defaultConfig();
    EXPECT_TRUE(config["system"]["allowRegister"].asBool());
    EXPECT_TRUE(config["system"]["allowAnonymous"].asBool());
}

// -------------------- load --------------------

TEST_F(AdminConfigStoreTest, Load_NoFile_ReturnsDefault) {
    // 文件不存在时应返回默认配置
    auto config = AdminConfigStore::load();
    EXPECT_TRUE(config.isMember("system"));
    EXPECT_EQ(config["system"]["name"].asString(), "HeartLake");
}

TEST_F(AdminConfigStoreTest, Load_ValidFile_ReturnsFileContent) {
    // 先写入一个自定义配置
    Json::Value custom;
    custom["system"]["name"] = "TestApp";
    custom["custom_key"] = "custom_value";
    ASSERT_TRUE(AdminConfigStore::save(custom));

    auto loaded = AdminConfigStore::load();
    EXPECT_EQ(loaded["system"]["name"].asString(), "TestApp");
    EXPECT_EQ(loaded["custom_key"].asString(), "custom_value");
}

TEST_F(AdminConfigStoreTest, Load_InvalidJson_ReturnsDefault) {
    // 写入非法JSON内容
    std::filesystem::create_directories(std::filesystem::path(testConfigPath_).parent_path());
    std::ofstream out(testConfigPath_);
    out << "this is not valid json {{{";
    out.close();

    auto config = AdminConfigStore::load();
    // 解析失败应返回默认配置
    EXPECT_TRUE(config.isMember("system"));
    EXPECT_EQ(config["system"]["name"].asString(), "HeartLake");
}

TEST_F(AdminConfigStoreTest, Load_EmptyFile_ReturnsDefault) {
    std::filesystem::create_directories(std::filesystem::path(testConfigPath_).parent_path());
    std::ofstream out(testConfigPath_);
    out << "";
    out.close();

    auto config = AdminConfigStore::load();
    EXPECT_TRUE(config.isMember("system"));
}

// -------------------- save --------------------

TEST_F(AdminConfigStoreTest, Save_CreatesFile) {
    Json::Value config;
    config["test"] = "value";
    EXPECT_TRUE(AdminConfigStore::save(config));
    EXPECT_TRUE(std::filesystem::exists(testConfigPath_));
}

TEST_F(AdminConfigStoreTest, Save_ThenLoad_RoundTrip) {
    Json::Value original;
    original["system"]["name"] = "RoundTripTest";
    original["rate"]["stonePerHour"] = 42;
    original["flag"] = true;

    ASSERT_TRUE(AdminConfigStore::save(original));
    auto loaded = AdminConfigStore::load();

    EXPECT_EQ(loaded["system"]["name"].asString(), "RoundTripTest");
    EXPECT_EQ(loaded["rate"]["stonePerHour"].asInt(), 42);
    EXPECT_TRUE(loaded["flag"].asBool());
}

TEST_F(AdminConfigStoreTest, Save_CreatesParentDirectories) {
    std::string nestedPath = "/tmp/heartlake_test_nested/sub/dir/config.json";
    setenv("ADMIN_CONFIG_FILE", nestedPath.c_str(), 1);

    Json::Value config;
    config["nested"] = true;
    EXPECT_TRUE(AdminConfigStore::save(config));
    EXPECT_TRUE(std::filesystem::exists(nestedPath));

    // 清理
    std::filesystem::remove_all("/tmp/heartlake_test_nested");
}

// -------------------- 并发读写安全 --------------------

TEST_F(AdminConfigStoreTest, ConcurrentReadWrite_NoCorruption) {
    // 先写入初始配置
    Json::Value initial;
    initial["counter"] = 0;
    AdminConfigStore::save(initial);

    constexpr int numThreads = 8;
    constexpr int opsPerThread = 20;
    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < opsPerThread; ++i) {
                if (t % 2 == 0) {
                    // 写线程
                    Json::Value cfg;
                    cfg["thread"] = t;
                    cfg["iteration"] = i;
                    AdminConfigStore::save(cfg);
                } else {
                    // 读线程
                    auto cfg = AdminConfigStore::load();
                    // 只要不崩溃、返回有效JSON就算通过
                    EXPECT_TRUE(cfg.isObject());
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // 最终应能正常读取
    auto final_cfg = AdminConfigStore::load();
    EXPECT_TRUE(final_cfg.isObject());
}

// =====================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
