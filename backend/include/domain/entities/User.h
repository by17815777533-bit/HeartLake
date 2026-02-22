/**
 * @file User.h
 * @brief User 模块接口定义
 * Created by 林子怡
 */

#pragma once

#include <string>
#include <ctime>
#include <json/json.h>

namespace heartlake {
namespace domain {

/**
 * @brief 用户模型
 *
 * 详细说明
 *
 * @note 注意事项
 */
class User {
public:
    User() = default;
    ~User() = default;

    std::string getUserId() const { return user_id_; }
    std::string getUsername() const { return username_; }
    std::string getNickname() const { return nickname_; }
    std::string getEmail() const { return email_; }
    std::string getPhone() const { return phone_; }
    std::string getDeviceId() const { return device_id_; }

    std::string getAvatarUrl() const { return avatar_url_; }
    std::string getCoverUrl() const { return cover_url_; }
    std::string getGender() const { return gender_; }
    std::string getBirthday() const { return birthday_; }
    std::string getRegion() const { return region_; }
    std::string getBio() const { return bio_; }
    std::string getQrCode() const { return qr_code_; }

    /**
     * @brief 检查用户是否匿名
     * @return 返回值说明
     */
    bool isAnonymous() const { return is_anonymous_; }
    /**
     * @brief 检查用户是否在线
     * @return 返回值说明
     */
    bool isOnline() const { return is_online_; }
    /**
     * @brief 检查邮箱是否已验证
     * @return 返回值说明
     */
    bool isEmailVerified() const { return email_verified_; }
    /**
     * @brief 检查电话是否已验证
     * @return 返回值说明
     */
    bool isPhoneVerified() const { return phone_verified_; }
    std::string getStatus() const { return status_; }

    /**
     * @brief 获取账户等级
     * @return 返回值说明
     */
    int getAccountLevel() const { return account_level_; }
    /**
     * @brief 获取VIP等级
     * @return 返回值说明
     */
    int getVipLevel() const { return vip_level_; }
    /**
     * @brief 获取VIP过期时间
     * @return 返回值说明
     */
    time_t getVipExpiresAt() const { return vip_expires_at_; }

    /**
     * @brief 获取创建时间
     * @return 返回值说明
     */
    time_t getCreatedAt() const { return created_at_; }
    /**
     * @brief 获取更新时间
     * @return 返回值说明
     */
    time_t getUpdatedAt() const { return updated_at_; }
    /**
     * @brief 获取最后活跃时间
     * @return 返回值说明
     */
    time_t getLastActiveAt() const { return last_active_at_; }

    /**
     * @brief 设置用户ID
     *
     * @param id 参数说明
     */
    void setUserId(const std::string& id) { user_id_ = id; }
    /**
     * @brief 设置用户名
     *
     * @param username 参数说明
     */
    void setUsername(const std::string& username) { username_ = username; }
    /**
     * @brief 设置用户昵称
     *
     * @param nickname 参数说明
     */
    void setNickname(const std::string& nickname) { nickname_ = nickname; }
    /**
     * @brief 设置用户邮箱
     *
     * @param email 参数说明
     */
    void setEmail(const std::string& email) { email_ = email; }
    /**
     * @brief 设置用户电话
     *
     * @param phone 参数说明
     */
    void setPhone(const std::string& phone) { phone_ = phone; }
    /**
     * @brief 设置设备ID
     *
     * @param device_id 参数说明
     */
    void setDeviceId(const std::string& device_id) { device_id_ = device_id; }

    /**
     * @brief 设置用户头像URL
     *
     * @param url 参数说明
     */
    void setAvatarUrl(const std::string& url) { avatar_url_ = url; }
    /**
     * @brief 设置用户封面URL
     *
     * @param url 参数说明
     */
    void setCoverUrl(const std::string& url) { cover_url_ = url; }
    /**
     * @brief 设置用户性别
     *
     * @param gender 参数说明
     */
    void setGender(const std::string& gender) { gender_ = gender; }
    /**
     * @brief 设置用户生日
     *
     * @param birthday 参数说明
     */
    void setBirthday(const std::string& birthday) { birthday_ = birthday; }
    /**
     * @brief 设置用户地区
     *
     * @param region 参数说明
     */
    void setRegion(const std::string& region) { region_ = region; }
    /**
     * @brief 设置用户个性签名
     *
     * @param bio 参数说明
     */
    void setBio(const std::string& bio) { bio_ = bio; }
    /**
     * @brief 设置用户二维码
     *
     * @param qr_code 参数说明
     */
    void setQrCode(const std::string& qr_code) { qr_code_ = qr_code; }

    /**
     * @brief 设置用户是否匿名
     *
     * @param is_anonymous 参数说明
     */
    void setIsAnonymous(bool is_anonymous) { is_anonymous_ = is_anonymous; }
    /**
     * @brief 设置用户是否在线
     *
     * @param is_online 参数说明
     */
    void setIsOnline(bool is_online) { is_online_ = is_online; }
    /**
     * @brief 设置邮箱是否已验证
     *
     * @param verified 参数说明
     */
    void setEmailVerified(bool verified) { email_verified_ = verified; }
    /**
     * @brief 设置电话是否已验证
     *
     * @param verified 参数说明
     */
    void setPhoneVerified(bool verified) { phone_verified_ = verified; }
    /**
     * @brief 设置用户状态
     *
     * @param status 参数说明
     */
    void setStatus(const std::string& status) { status_ = status; }

    /**
     * @brief 设置账户等级
     *
     * @param level 参数说明
     */
    void setAccountLevel(int level) { account_level_ = level; }
    /**
     * @brief 设置VIP等级
     *
     * @param level 参数说明
     */
    void setVipLevel(int level) { vip_level_ = level; }
    /**
     * @brief 设置VIP过期时间
     *
     * @param time 参数说明
     */
    void setVipExpiresAt(time_t time) { vip_expires_at_ = time; }

    /**
     * @brief 设置创建时间
     *
     * @param time 参数说明
     */
    void setCreatedAt(time_t time) { created_at_ = time; }
    /**
     * @brief 设置更新时间
     *
     * @param time 参数说明
     */
    void setUpdatedAt(time_t time) { updated_at_ = time; }
    /**
     * @brief 设置最后活跃时间
     *
     * @param time 参数说明
     */
    void setLastActiveAt(time_t time) { last_active_at_ = time; }

    Json::Value toJson() const {
        Json::Value json;
        json["user_id"] = user_id_;
        if (!username_.empty()) json["username"] = username_;
        json["nickname"] = nickname_;
        if (!email_.empty()) json["email"] = email_;
        if (!phone_.empty()) json["phone"] = phone_;

        if (!avatar_url_.empty()) json["avatar_url"] = avatar_url_;
        if (!cover_url_.empty()) json["cover_url"] = cover_url_;
        if (!gender_.empty()) json["gender"] = gender_;
        if (!birthday_.empty()) json["birthday"] = birthday_;
        if (!region_.empty()) json["region"] = region_;
        if (!bio_.empty()) json["bio"] = bio_;
        if (!qr_code_.empty()) json["qr_code"] = qr_code_;

        json["is_anonymous"] = is_anonymous_;
        json["is_online"] = is_online_;
        json["email_verified"] = email_verified_;
        json["phone_verified"] = phone_verified_;
        json["status"] = status_;

        json["account_level"] = account_level_;
        json["vip_level"] = vip_level_;
        if (vip_expires_at_ > 0) {
            json["vip_expires_at"] = static_cast<Json::Int64>(vip_expires_at_);
        }

        json["created_at"] = static_cast<Json::Int64>(created_at_);
        json["updated_at"] = static_cast<Json::Int64>(updated_at_);
        json["last_active_at"] = static_cast<Json::Int64>(last_active_at_);

        return json;
    }

    /**
     * @brief 从JSON格式解析对象
     *
     * @param json 参数说明
     */
    void fromJson(const Json::Value& json) {
        if (json.isMember("user_id")) user_id_ = json["user_id"].asString();
        if (json.isMember("username")) username_ = json["username"].asString();
        if (json.isMember("nickname")) nickname_ = json["nickname"].asString();
        if (json.isMember("email")) email_ = json["email"].asString();
        if (json.isMember("phone")) phone_ = json["phone"].asString();
        if (json.isMember("device_id")) device_id_ = json["device_id"].asString();

        if (json.isMember("avatar_url")) avatar_url_ = json["avatar_url"].asString();
        if (json.isMember("cover_url")) cover_url_ = json["cover_url"].asString();
        if (json.isMember("gender")) gender_ = json["gender"].asString();
        if (json.isMember("birthday")) birthday_ = json["birthday"].asString();
        if (json.isMember("region")) region_ = json["region"].asString();
        if (json.isMember("bio")) bio_ = json["bio"].asString();
        if (json.isMember("qr_code")) qr_code_ = json["qr_code"].asString();

        if (json.isMember("is_anonymous")) is_anonymous_ = json["is_anonymous"].asBool();
        if (json.isMember("is_online")) is_online_ = json["is_online"].asBool();
        if (json.isMember("email_verified")) email_verified_ = json["email_verified"].asBool();
        if (json.isMember("phone_verified")) phone_verified_ = json["phone_verified"].asBool();
        if (json.isMember("status")) status_ = json["status"].asString();

        if (json.isMember("account_level")) account_level_ = json["account_level"].asInt();
        if (json.isMember("vip_level")) vip_level_ = json["vip_level"].asInt();
        if (json.isMember("vip_expires_at")) vip_expires_at_ = json["vip_expires_at"].asInt64();

        if (json.isMember("created_at")) created_at_ = json["created_at"].asInt64();
        if (json.isMember("updated_at")) updated_at_ = json["updated_at"].asInt64();
        if (json.isMember("last_active_at")) last_active_at_ = json["last_active_at"].asInt64();
    }

private:
    std::string user_id_;
    std::string username_;        // 系统账号 user001, user002...
    std::string nickname_;        // 用户昵称
    std::string email_;
    std::string phone_;
    std::string device_id_;

    std::string avatar_url_;
    std::string cover_url_;       // 个人主页封面
    std::string gender_ = "unknown";  // male, female, unknown
    std::string birthday_;        // YYYY-MM-DD
    std::string region_;          // 地区
    std::string bio_;             // 个性签名
    std::string qr_code_;         // 二维码URL

    bool is_anonymous_ = true;
    bool is_online_ = false;
    bool email_verified_ = false;
    bool phone_verified_ = false;
    std::string status_ = "active";  // active, suspended, deleted

    int account_level_ = 1;
    int vip_level_ = 0;           // 0=普通用户
    time_t vip_expires_at_ = 0;

    time_t created_at_ = 0;
    time_t updated_at_ = 0;
    time_t last_active_at_ = 0;
};

} // namespace domain
} // namespace heartlake
