/**
 * Validator 模块接口定义
 */

#pragma once
#include <string>
#include <optional>
#include <json/json.h>
#include <drogon/HttpRequest.h>

namespace heartlake {
namespace utils {

/**
 * 验证结果
 */
struct ValidationResult {
    bool isValid;
    std::string errorMessage;
    
    /**
     * valid方法
     * @return 返回值说明
     */
    static ValidationResult valid() {
        return {true, ""};
    }
    
    /**
     * invalid方法
     *
     * @param message 参数说明
     * @return 返回值说明
     */
    static ValidationResult invalid(const std::string& message) {
        return {false, message};
    }
    
    /**
     * bool方法
     * @return 返回值说明
     */
    operator bool() const { return isValid; }
};

/**
 * 统一验证器
 * 减少控制器中重复的验证代码
 */
/**
 * 验证器，用于数据验证
 *
 * 详细说明
 *
 * @note 注意事项
 */
class Validator {
public:
    /**
     * 验证请求JSON
     */
    static ValidationResult validateJson(const drogon::HttpRequestPtr& req);
    
    /**
     * 验证字段非空
     */
    static ValidationResult required(const Json::Value& value, const std::string& fieldName);
    
    /**
     * 验证字符串非空
     */
    static ValidationResult requiredString(const std::string& value, const std::string& fieldName);
    
    /**
     * 验证字符串长度
     */
    static ValidationResult length(const std::string& value, size_t min, size_t max, 
                                   const std::string& fieldName);
    
    /**
     * 验证邮箱格式
     */
    static ValidationResult email(const std::string& value);
    
    /**
     * 验证密码强度
     */
    static ValidationResult password(const std::string& value);

    /**
     * 验证密码复杂度（增强版）
     * 要求：至少8个字符，包含大小写字母、数字、特殊字符中的至少3种
     */
    static ValidationResult passwordStrong(const std::string& value);

    /**
     * 计算密码强度分数（0-6分）
     */
    static int calculatePasswordStrength(const std::string& value);

    /**
     * 验证用户ID格式
     */
    static ValidationResult userId(const std::string& value);

    /**
     * XSS防护：清理HTML标签和危险字符
     */
    static std::string sanitizeHtml(const std::string& input);

    /**
     * SQL注入防护：检测危险SQL关键字
     */
    static ValidationResult checkSqlInjection(const std::string& input, const std::string& fieldName);

    /**
     * 路径遍历防护：检测 ../ 等危险路径
     */
    static ValidationResult checkPathTraversal(const std::string& path, const std::string& fieldName);

    /**
     * URL验证
     */
    static ValidationResult url(const std::string& value, const std::string& fieldName = "URL");

    /**
     * 手机号验证（中国大陆）
     */
    static ValidationResult phoneNumber(const std::string& value);

    /**
     * 验证码格式验证
     */
    static ValidationResult verificationCode(const std::string& code);

    /**
     * 文件扩展名验证
     */
    static ValidationResult fileExtension(const std::string& filename,
                                          const std::vector<std::string>& allowedExtensions,
                                          const std::string& fieldName = "文件");

    /**
     * JSON字段存在性验证
     */
    static ValidationResult hasField(const Json::Value& json, const std::string& fieldName);

    /**
     * 枚举值验证
     */
    static ValidationResult inEnum(const std::string& value,
                                   const std::vector<std::string>& allowedValues,
                                   const std::string& fieldName);
    
    /**
     * 验证数组长度
     */
    static ValidationResult arrayLength(const Json::Value& array, size_t max, 
                                       const std::string& fieldName);
    
    /**
     * 验证数字范围
     */
    static ValidationResult numberRange(int value, int min, int max, 
                                       const std::string& fieldName);
    
    /**
     * 验证分页参数
     */
    static ValidationResult paginationParams(int page, int pageSize);
    
    /**
     * 组合多个验证结果
     */
    static ValidationResult combine(const std::vector<ValidationResult>& results);
    
    /**
     * 从请求中获取用户ID并验证
     */
    static std::optional<std::string> getUserId(const drogon::HttpRequestPtr& req);
};

/**
 * 常用验证规则
 */
/**
 * ValidationRules类
 *
 * 详细说明
 *
 * @note 注意事项
 */
class ValidationRules {
public:
    /**
     * content方法
     *
     * @param value 参数说明
     * @return 返回值说明
     */
    static ValidationResult content(const std::string& value);
    
    /**
     * nickname方法
     *
     * @param value 参数说明
     * @return 返回值说明
     */
    static ValidationResult nickname(const std::string& value);
    
    /**
     * tags方法
     *
     * @param tags 参数说明
     * @return 返回值说明
     */
    static ValidationResult tags(const Json::Value& tags);
    
    /**
     * mediaIds方法
     *
     * @param mediaIds 参数说明
     * @return 返回值说明
     */
    static ValidationResult mediaIds(const Json::Value& mediaIds);
};

} // namespace utils
} // namespace heartlake
