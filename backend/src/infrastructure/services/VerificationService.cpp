/**
 * @file VerificationService.cpp
 * @brief VerificationService 模块实现
 * Created by 白洋
 */

#include "infrastructure/services/VerificationService.h"
#include "infrastructure/cache/RedisCache.h"
#include "utils/StructuredLogger.h"
#include <random>
#include <stdexcept>
#include <drogon/drogon.h>

using namespace heartlake::services;

std::string VerificationService::generateCode() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    return std::to_string(dis(gen));
}

std::string VerificationService::buildRedisKey(const std::string& email,
                                             const std::string& purpose,
                                             const std::string& prefix) {
    return prefix + purpose + ":" + email;
}

bool VerificationService::checkRateLimit(const std::string& email) {
    try {
        auto& redisCache = heartlake::cache::RedisCache::getInstance();
        std::string rateLimitKey = buildRedisKey(email, "rate", RATE_LIMIT_PREFIX);

        std::string lastSendTime = redisCache.getSync(rateLimitKey);
        if (lastSendTime.empty()) {
            return true; // 没有记录，可以发送
        }

        // 检查是否超过冷却时间
        auto now = std::chrono::system_clock::now();
        auto lastTime = std::chrono::system_clock::from_time_t(std::stoll(lastSendTime));
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime).count();

        return elapsed >= RATE_LIMIT_SECONDS;
    } catch (const std::exception& e) {
        LOG_ERROR << "Rate limit check failed: " << e.what();
        return false; // 出错时拒绝发送
    }
}

int VerificationService::getRemainingCooldown(const std::string& email) {
    try {
        auto& redisCache = heartlake::cache::RedisCache::getInstance();
        std::string rateLimitKey = buildRedisKey(email, "rate", RATE_LIMIT_PREFIX);

        std::string lastSendTime = redisCache.getSync(rateLimitKey);
        if (lastSendTime.empty()) {
            return 0; // 没有记录，可以立即发送
        }

        auto now = std::chrono::system_clock::now();
        auto lastTime = std::chrono::system_clock::from_time_t(std::stoll(lastSendTime));
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime).count();

        int remaining = RATE_LIMIT_SECONDS - static_cast<int>(elapsed);
        return remaining > 0 ? remaining : 0;
    } catch (const std::exception& e) {
        LOG_ERROR << "Get remaining cooldown failed: " << e.what();
        return RATE_LIMIT_SECONDS; // 出错时返回最大冷却时间
    }
}

bool VerificationService::checkDailyLimit(const std::string& email) {
    try {
        auto& redisCache = heartlake::cache::RedisCache::getInstance();
        std::string countKey = buildRedisKey(email, "daily", COUNT_PREFIX);

        std::string countStr = redisCache.getSync(countKey);
        if (countStr.empty()) {
            return false; // 没有记录，未超过限制
        }

        int count = std::stoi(countStr);
        return count >= MAX_DAILY_COUNT;
    } catch (const std::exception& e) {
        LOG_ERROR << "Daily limit check failed: " << e.what();
        return true; // 出错时拒绝发送
    }
}

void VerificationService::incrementDailyCount(const std::string& email) {
    try {
        auto& redisCache = heartlake::cache::RedisCache::getInstance();
        std::string countKey = buildRedisKey(email, "daily", COUNT_PREFIX);

        std::string countStr = redisCache.getSync(countKey);
        int count = countStr.empty() ? 0 : std::stoi(countStr);
        count++;

        redisCache.setexSync(countKey, std::to_string(count), DAILY_TTL_SECONDS);
    } catch (const std::exception& e) {
        LOG_ERROR << "Increment daily count failed: " << e.what();
        // 不抛出异常，避免影响主流程
    }
}

std::string VerificationService::generateAndStoreCode(const std::string& email,
                                                    const std::string& purpose) {
    // 检查速率限制
    if (!checkRateLimit(email)) {
        int remaining = getRemainingCooldown(email);
        throw std::runtime_error("发送过于频繁，请等待 " + std::to_string(remaining) + " 秒后重试");
    }

    // 检查每日限制
    if (checkDailyLimit(email)) {
        throw std::runtime_error("今日发送次数已达上限，请明天再试");
    }

    try {
        auto& redisCache = heartlake::cache::RedisCache::getInstance();

        // 生成验证码
        std::string code = generateCode();

        // 存储验证码到Redis
        std::string codeKey = buildRedisKey(email, purpose, CODE_PREFIX);
        redisCache.setexSync(codeKey, code, CODE_TTL_SECONDS);

        // 更新速率限制记录
        std::string rateLimitKey = buildRedisKey(email, "rate", RATE_LIMIT_PREFIX);
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        redisCache.setexSync(rateLimitKey, std::to_string(timestamp), RATE_LIMIT_SECONDS);

        // 增加每日计数
        incrementDailyCount(email);

        // 安全日志：邮箱脱敏
        LOG_INFO << "Verification code generated for email: " << heartlake::utils::StructuredLogger::maskEmail(email)
                 << ", purpose: " << purpose;

        return code;

    } catch (const std::exception& e) {
        LOG_ERROR << "Generate and store verification code failed: " << e.what();
        throw std::runtime_error("验证码生成失败，请稍后重试");
    }
}

bool VerificationService::verifyCode(const std::string& email,
                                   const std::string& code,
                                   const std::string& purpose) {
    try {
        auto& redisCache = heartlake::cache::RedisCache::getInstance();

        // 从Redis获取存储的验证码
        std::string codeKey = buildRedisKey(email, purpose, CODE_PREFIX);
        std::string storedCode = redisCache.getSync(codeKey);

        if (storedCode.empty()) {
            LOG_WARN << "Verification code not found or expired for email: " << heartlake::utils::StructuredLogger::maskEmail(email);
            return false;
        }

        // 验证码匹配检查
        bool isValid = (code == storedCode);

        if (isValid) {
            // 验证成功，删除验证码（一次性使用）
            redisCache.del(codeKey);
            LOG_INFO << "Verification code verified successfully for email: " << heartlake::utils::StructuredLogger::maskEmail(email);
        } else {
            LOG_WARN << "Invalid verification code for email: " << heartlake::utils::StructuredLogger::maskEmail(email);
        }

        return isValid;

    } catch (const std::exception& e) {
        LOG_ERROR << "Verify code failed: " << e.what();
        return false;
    }
}

void VerificationService::cleanupExpiredCodes() {
    // Redis的TTL机制会自动清理过期的键，这里主要用于日志记录
    LOG_DEBUG << "Verification code cleanup triggered (Redis TTL handles expiration)";
}

void VerificationService::deleteCode(const std::string& email, const std::string& purpose) {
    try {
        auto& redisCache = heartlake::cache::RedisCache::getInstance();
        std::string codeKey = buildRedisKey(email, purpose, CODE_PREFIX);
        redisCache.del(codeKey);
        LOG_DEBUG << "Verification code deleted for email: " << heartlake::utils::StructuredLogger::maskEmail(email) << ", purpose: " << purpose;
    } catch (const std::exception& e) {
        LOG_ERROR << "Delete verification code failed: " << e.what();
    }
}
