/**
 * @brief 安全审计评分系统 — 实时量化评估系统安全状态
 *
 * 对 12 项安全维度（PASETO 配置、E2E 加密、身份匿名化、限流、
 * 输入校验、SQL 注入防护、XSS 防护、CSRF 防护、安全响应头、
 * 密码哈希、会话管理、审计日志）逐项检查并加权评分。
 * 每项检查返回通过/未通过及详情，最终汇总为 0~100 分。
 *
 * @note 单例模式，runAudit() 会刷新 currentScore_ 并返回完整报告 JSON
 */

#pragma once

#include <string>
#include <vector>
#include <json/json.h>

namespace heartlake {
namespace utils {

/// 单项安全检查结果
struct SecurityCheckItem {
    std::string name;       ///< 检查项名称
    std::string category;   ///< 所属类别（如 "authentication"、"encryption"）
    double weight;          ///< 权重（0.0~1.0），反映该项对总分的影响程度
    bool passed;            ///< 是否通过
    std::string detail;     ///< 检查详情或未通过原因
};

/**
 * @brief 安全审计评分器（单例）
 *
 * 对 12 项安全维度逐项检查并加权评分，汇总为 0~100 分。
 * runAudit() 刷新 currentScore_ 并返回完整报告 JSON。
 */
class SecurityAuditScore {
public:
    /// 获取单例实例
    static SecurityAuditScore& getInstance();

    /**
     * @brief 执行全部 12 项安全检查，计算加权总分
     * @return JSON 对象，包含 score（总分）、items（各项详情）、timestamp
     */
    Json::Value runAudit();

    /// 获取最近一次审计的总分（0.0~100.0）
    double getScore() const { return currentScore_; }

private:
    SecurityAuditScore() = default;

    // ---- 各维度检查 ----
    SecurityCheckItem checkPasetoConfig();           ///< PASETO token 配置完整性
    SecurityCheckItem checkE2EEncryption();           ///< 端到端加密可用性
    SecurityCheckItem checkIdentityAnonymization();   ///< 身份匿名化机制
    SecurityCheckItem checkRateLimiting();             ///< 限流器配置
    SecurityCheckItem checkInputValidation();          ///< 输入校验覆盖度
    SecurityCheckItem checkSqlInjectionProtection();   ///< SQL 注入防护（参数化查询）
    SecurityCheckItem checkXssProtection();            ///< XSS 输出编码
    SecurityCheckItem checkCsrfProtection();           ///< CSRF token 验证
    SecurityCheckItem checkSecurityHeaders();           ///< 安全响应头（CSP、HSTS 等）
    SecurityCheckItem checkPasswordHashing();           ///< 密码哈希算法强度
    SecurityCheckItem checkSessionManagement();         ///< 会话生命周期管理
    SecurityCheckItem checkAuditLogging();              ///< 安全审计日志完整性

    double currentScore_ = 0.0;
};

} // namespace utils
} // namespace heartlake
