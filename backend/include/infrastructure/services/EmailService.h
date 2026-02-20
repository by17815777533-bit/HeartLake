/**
 * @file EmailService.h
 * @brief EmailService 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <string>
#include <memory>
#include <functional>

namespace heartlake {
namespace services {

/**
 * @brief 邮件服务，用于发送邮件
 *
 * 详细说明
 *
 * @note 注意事项
 */
class EmailService {
public:
    static EmailService& getInstance();
    
    /**
     * @brief initialize方法
     * @return 返回值说明
     */
    bool initialize();
    
    bool sendVerificationCode(const std::string& email, 
                             const std::string& code,
                             const std::string& purpose = "register");
    
    bool sendWelcomeEmail(const std::string& email, 
                         const std::string& nickname);
    
    bool sendPasswordResetEmail(const std::string& email, 
                                const std::string& code);
    
    bool sendNotificationEmail(const std::string& email,
                              const std::string& subject,
                              const std::string& content);
    
    std::string generateVerificationCode(int length = 6);
    
    /**
     * @brief isValidEmail方法
     *
     * @param email 参数说明
     * @return 返回值说明
     */
    static bool isValidEmail(const std::string& email);
    
private:
    EmailService() = default;
    ~EmailService() = default;
    EmailService(const EmailService&) = delete;
    EmailService& operator=(const EmailService&) = delete;
    
    bool sendEmail(const std::string& to,
                   const std::string& subject,
                   const std::string& htmlContent,
                   const std::string& textContent = "");
    
    std::string getVerificationEmailTemplate(const std::string& code, 
                                             const std::string& purpose);
    std::string getWelcomeEmailTemplate(const std::string& nickname);
    std::string getPasswordResetEmailTemplate(const std::string& code);
    
    bool m_initialized = false;
    std::string m_smtpHost;
    int m_smtpPort = 587;
    std::string m_smtpUser;
    std::string m_smtpPassword;
    std::string m_fromAddress;
    std::string m_fromName;
};

} // namespace services
} // namespace heartlake
