/**
 * @file EmailService.cpp
 * @brief EmailService 模块实现
 * Created by 白洋
 */
#include "infrastructure/services/EmailService.h"
#include <drogon/drogon.h>
#include <curl/curl.h>
#include <random>
#include <regex>
#include <sstream>
#include <iomanip>
#include <vector>

using namespace heartlake::services;
using namespace drogon;

namespace {
    // SMTP payload structure for libcurl
    struct UploadStatus {
        const std::vector<std::string>* lines;
        size_t lines_read;
    };

    // Callback function for reading email data
    static size_t payload_source(char* ptr, size_t size, size_t nmemb, void* userp) {
        auto* upload_ctx = static_cast<UploadStatus*>(userp);

        if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
            return 0;
        }

        if (upload_ctx->lines_read < upload_ctx->lines->size()) {
            const std::string& line = (*upload_ctx->lines)[upload_ctx->lines_read];
            size_t len = line.length();
            if (len > size * nmemb) {
                len = size * nmemb;
            }
            memcpy(ptr, line.c_str(), len);
            upload_ctx->lines_read++;
            return len;
        }

        return 0;
    }
}

EmailService& EmailService::getInstance() {
    static EmailService instance;
    return instance;
}

bool EmailService::initialize() {
    if (m_initialized) {
        return true;
    }
    
    try {
        auto config = app().getCustomConfig();
        auto emailConfig = config["email"];
        
        if (!emailConfig["enabled"].asBool()) {
            LOG_INFO << "邮件服务未启用";
            return false;
        }
        
        m_smtpHost = emailConfig["smtp_host"].asString();
        m_smtpPort = emailConfig["smtp_port"].asInt();
        m_smtpUser = emailConfig["smtp_user"].asString();
        m_smtpPassword = emailConfig["smtp_password"].asString();
        m_fromAddress = emailConfig["from_address"].asString();
        m_fromName = emailConfig["from_name"].asString();
        
        // 替换环境变量
        if (m_smtpUser.find("${") != std::string::npos) {
            auto envVar = std::getenv("EMAIL_USER");
            if (envVar) m_smtpUser = envVar;
        }
        if (m_smtpPassword.find("${") != std::string::npos) {
            auto envVar = std::getenv("EMAIL_PASSWORD");
            if (envVar) m_smtpPassword = envVar;
        }
        
        m_initialized = true;
        LOG_INFO << "邮件服务初始化成功";
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR << "邮件服务初始化失败: " << e.what();
        return false;
    }
}

std::string EmailService::generateVerificationCode(int length) {
    static const std::string digits = "0123456789";
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, digits.size() - 1);

    std::string code;
    code.reserve(length);
    for (int i = 0; i < length; ++i) {
        code += digits[dis(gen)];
    }
    return code;
}

bool EmailService::isValidEmail(const std::string& email) {
    static const std::regex pattern(
        R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)"
    );
    return std::regex_match(email, pattern);
}

bool EmailService::sendVerificationCode(const std::string& email, 
                                       const std::string& code,
                                       const std::string& purpose) {
    if (!m_initialized) {
        LOG_WARN << "邮件服务未初始化";
        return false;
    }
    
    std::string subject;
    if (purpose == "register") {
        subject = "【心湖】注册验证码";
    } else if (purpose == "reset_password") {
        subject = "【心湖】密码重置验证码";
    } else {
        subject = "【心湖】验证码";
    }
    
    std::string htmlContent = getVerificationEmailTemplate(code, purpose);
    std::string textContent = "您的验证码是: " + code + "，10分钟内有效。";
    
    return sendEmail(email, subject, htmlContent, textContent);
}

bool EmailService::sendWelcomeEmail(const std::string& email, 
                                   const std::string& nickname) {
    if (!m_initialized) {
        return false;
    }
    
    std::string subject = "欢迎来到心湖 🌞";
    std::string htmlContent = getWelcomeEmailTemplate(nickname);
    std::string textContent = "欢迎来到心湖，" + nickname + "！";
    
    return sendEmail(email, subject, htmlContent, textContent);
}

bool EmailService::sendPasswordResetEmail(const std::string& email, 
                                         const std::string& code) {
    if (!m_initialized) {
        return false;
    }
    
    std::string subject = "【心湖】密码重置";
    std::string htmlContent = getPasswordResetEmailTemplate(code);
    std::string textContent = "您的密码重置验证码: " + code;
    
    return sendEmail(email, subject, htmlContent, textContent);
}

bool EmailService::sendNotificationEmail(const std::string& email,
                                        const std::string& subject,
                                        const std::string& content) {
    if (!m_initialized) {
        return false;
    }
    
    return sendEmail(email, subject, content);
}

bool EmailService::sendEmail(const std::string& to,
                             const std::string& subject,
                             const std::string& htmlContent,
                             const std::string& textContent) {
    constexpr int maxRetries = 3;

    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            LOG_ERROR << "Failed to initialize CURL";
            return false;
        }

        struct curl_slist* recipients = nullptr;
        auto cleanup = [&]() {
            if (recipients) curl_slist_free_all(recipients);
            curl_easy_cleanup(curl);
        };

        try {
            std::vector<std::string> payload_lines;
            payload_lines.push_back("From: " + m_fromName + " <" + m_fromAddress + ">\r\n");
            payload_lines.push_back("To: <" + to + ">\r\n");
            payload_lines.push_back("Subject: " + subject + "\r\n");
            payload_lines.push_back("MIME-Version: 1.0\r\n");
            payload_lines.push_back("Content-Type: multipart/alternative; boundary=\"boundary123\"\r\n");
            payload_lines.push_back("\r\n");

            if (!textContent.empty()) {
                payload_lines.push_back("--boundary123\r\n");
                payload_lines.push_back("Content-Type: text/plain; charset=UTF-8\r\n");
                payload_lines.push_back("\r\n");
                payload_lines.push_back(textContent + "\r\n");
                payload_lines.push_back("\r\n");
            }

            payload_lines.push_back("--boundary123\r\n");
            payload_lines.push_back("Content-Type: text/html; charset=UTF-8\r\n");
            payload_lines.push_back("\r\n");
            payload_lines.push_back(htmlContent + "\r\n");
            payload_lines.push_back("\r\n");
            payload_lines.push_back("--boundary123--\r\n");

            UploadStatus upload_ctx{&payload_lines, 0};

            std::string smtp_url = "smtp://" + m_smtpHost + ":" + std::to_string(m_smtpPort);
            curl_easy_setopt(curl, CURLOPT_URL, smtp_url.c_str());
            curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
            curl_easy_setopt(curl, CURLOPT_USERNAME, m_smtpUser.c_str());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, m_smtpPassword.c_str());
            curl_easy_setopt(curl, CURLOPT_MAIL_FROM, m_fromAddress.c_str());

            recipients = curl_slist_append(recipients, to.c_str());
            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
            curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

            CURLcode res = curl_easy_perform(curl);
            cleanup();

            if (res == CURLE_OK) {
                LOG_INFO << "邮件发送成功: " << to;
                return true;
            }

            LOG_WARN << "邮件发送失败 (尝试 " << attempt << "/" << maxRetries << "): "
                     << to << ", 错误: " << curl_easy_strerror(res);

        } catch (const std::exception& e) {
            cleanup();
            LOG_ERROR << "发送邮件异常: " << e.what();
            return false;
        }
    }

    LOG_ERROR << "邮件发送最终失败: " << to;
    return false;
}

std::string EmailService::getVerificationEmailTemplate(const std::string& code, 
                                                       const std::string& purpose) {
    std::stringstream html;
    html << R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial, sans-serif; background-color: #f5f5f5; padding: 20px; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 40px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; margin-bottom: 30px; }
        .header h1 { color: #2E7D9A; margin: 0; }
        .code-box { background: #E8F4F8; padding: 20px; border-radius: 8px; text-align: center; margin: 30px 0; }
        .code { font-size: 32px; font-weight: bold; color: #2E7D9A; letter-spacing: 8px; }
        .footer { text-align: center; color: #999; font-size: 12px; margin-top: 30px; }
        .tips { color: #666; font-size: 14px; line-height: 1.6; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌊 心湖 HeartLake</h1>
            <p>一个温暖治愈的情感交流空间</p>
        </div>
        
        <p>您好！</p>
        <p>您正在)";
    
    if (purpose == "register") {
        html << "注册心湖账号";
    } else if (purpose == "reset_password") {
        html << "重置密码";
    } else {
        html << "进行验证";
    }
    
    html << R"(，您的验证码为：</p>
        
        <div class="code-box">
            <div class="code">)" << code << R"(</div>
        </div>
        
        <div class="tips">
            <p><strong>温馨提示：</strong></p>
            <ul>
                <li>验证码10分钟内有效，请尽快使用</li>
                <li>如果不是您本人操作，请忽略此邮件</li>
                <li>请勿将验证码透露给他人</li>
            </ul>
        </div>
        
        <div class="footer">
            <p>此邮件由系统自动发送，请勿回复</p>
            <p>© 2025 心湖 HeartLake. All rights reserved.</p>
        </div>
    </div>
</body>
</html>
)";
    
    return html.str();
}

std::string EmailService::getWelcomeEmailTemplate(const std::string& nickname) {
    std::stringstream html;
    html << R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial, sans-serif; background-color: #f5f5f5; padding: 20px; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 40px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; margin-bottom: 30px; }
        .header h1 { color: #2E7D9A; margin: 0; font-size: 36px; }
        .welcome-box { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); padding: 30px; border-radius: 8px; color: white; text-align: center; margin: 20px 0; }
        .feature { margin: 15px 0; padding: 15px; background: #f8f9fa; border-left: 4px solid #2E7D9A; border-radius: 4px; }
        .feature h3 { margin: 0 0 10px 0; color: #2E7D9A; }
        .footer { text-align: center; color: #999; font-size: 12px; margin-top: 30px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌊 欢迎来到心湖</h1>
        </div>
        
        <div class="welcome-box">
            <h2>你好，)" << nickname << R"( ！</h2>
            <p style="font-size: 18px; margin-top: 20px;">感谢你加入心湖大家庭 ❤️</p>
        </div>
        
        <p>心湖是一个温暖治愈的情感交流空间，在这里你可以：</p>
        
        <div class="feature">
            <h3>🪨 投掷石头</h3>
            <p>分享你的心情、烦恼或喜悦，让情感在湖面荡起涟漪</p>
        </div>
        
        <div class="feature">
            <h3>🛶 放飞纸船</h3>
            <p>给他人的石头留下温暖的评论，传递治愈的力量</p>
        </div>
        
        <div class="feature">
            <h3>👥 结识好友</h3>
            <p>通过互动建立24小时临时好友，深入交流可升级为永久好友</p>
        </div>
        
        <div class="feature">
            <h3>🤖 AI陪伴</h3>
            <p>当你需要倾诉时，AI助手会给予智能且温暖的回应</p>
        </div>
        
        <p style="text-align: center; margin-top: 30px; font-size: 16px;">
            <strong>愿你在心湖找到属于自己的那份平静与温暖</strong>
        </p>
        
        <div class="footer">
            <p>此邮件由系统自动发送，请勿回复</p>
            <p>© 2025 心湖 HeartLake. All rights reserved.</p>
        </div>
    </div>
</body>
</html>
)";
    
    return html.str();
}

std::string EmailService::getPasswordResetEmailTemplate(const std::string& code) {
    std::stringstream html;
    html << R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial, sans-serif; background-color: #f5f5f5; padding: 20px; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 40px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; margin-bottom: 30px; }
        .header h1 { color: #2E7D9A; margin: 0; }
        .code-box { background: #FFF3E0; padding: 20px; border-radius: 8px; text-align: center; margin: 30px 0; border: 2px dashed #FF9800; }
        .code { font-size: 32px; font-weight: bold; color: #FF6F00; letter-spacing: 8px; }
        .warning { background: #FFF3CD; padding: 15px; border-radius: 4px; border-left: 4px solid #FFC107; color: #856404; margin: 20px 0; }
        .footer { text-align: center; color: #999; font-size: 12px; margin-top: 30px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔒 密码重置</h1>
        </div>
        
        <p>您好！</p>
        <p>我们收到了您重置密码的请求。您的验证码为：</p>
        
        <div class="code-box">
            <div class="code">)" << code << R"(</div>
        </div>
        
        <div class="warning">
            <p><strong>⚠️ 安全提示：</strong></p>
            <ul style="margin: 10px 0;">
                <li>验证码10分钟内有效</li>
                <li>如果不是您本人操作，请立即联系我们</li>
                <li>请勿将验证码透露给任何人</li>
            </ul>
        </div>
        
        <div class="footer">
            <p>此邮件由系统自动发送，请勿回复</p>
            <p>© 2025 心湖 HeartLake. All rights reserved.</p>
        </div>
    </div>
</body>
</html>
)";
    
    return html.str();
}
