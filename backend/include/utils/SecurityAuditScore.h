/**
 * @file SecurityAuditScore.h
 * @brief 安全审计评分系统 - 实时计算安全评分
 * Created by 白洋
 */

#pragma once

#include <string>
#include <vector>
#include <json/json.h>

namespace heartlake {
namespace utils {

struct SecurityCheckItem {
    std::string name;
    std::string category;
    double weight;
    bool passed;
    std::string detail;
};

class SecurityAuditScore {
public:
    static SecurityAuditScore& getInstance();

    // 执行全面安全审计并返回评分
    Json::Value runAudit();

    // 获取当前评分
    double getScore() const { return currentScore_; }

private:
    SecurityAuditScore() = default;

    // 各项安全检查
    SecurityCheckItem checkPasetoConfig();
    SecurityCheckItem checkE2EEncryption();
    SecurityCheckItem checkIdentityAnonymization();
    SecurityCheckItem checkRateLimiting();
    SecurityCheckItem checkInputValidation();
    SecurityCheckItem checkSqlInjectionProtection();
    SecurityCheckItem checkXssProtection();
    SecurityCheckItem checkCsrfProtection();
    SecurityCheckItem checkSecurityHeaders();
    SecurityCheckItem checkPasswordHashing();
    SecurityCheckItem checkSessionManagement();
    SecurityCheckItem checkAuditLogging();

    double currentScore_ = 0.0;
};

} // namespace utils
} // namespace heartlake
