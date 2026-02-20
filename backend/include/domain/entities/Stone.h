/**
 * @file Stone.h
 * @brief Stone 模块接口定义
 * Created by 林子怡
 */

#pragma once

#include <string>
#include <vector>
#include <ctime>
#include <json/json.h>

namespace heartlake {
namespace models {

/**
 * @brief 石头模型
 *
 * 详细说明
 *
 * @note 注意事项
 */
class Stone {
public:
    Stone() = default;
    ~Stone() = default;

    std::string getStoneId() const { return stone_id_; }
    std::string getUserId() const { return user_id_; }
    std::string getContent() const { return content_; }
    std::string getStoneType() const { return stone_type_; }
    std::string getStoneColor() const { return stone_color_; }
    /**
     * @brief 检查用户是否匿名
     * @return 返回值说明
     */
    bool isAnonymous() const { return is_anonymous_; }
    std::string getStatus() const { return status_; }
    /**
     * @brief getViewCount方法
     * @return 返回值说明
     */
    int getViewCount() const { return view_count_; }
    /**
     * @brief getRipplesCount方法
     * @return 返回值说明
     */
    int getRippleCount() const { return ripple_count_; }
    /**
     * @brief getBoatCount方法
     * @return 返回值说明
     */
    int getBoatCount() const { return boat_count_; }
    const std::vector<std::string>& getTags() const { return tags_; }
    /**
     * @brief 获取创建时间
     * @return 返回值说明
     */
    time_t getCreatedAt() const { return created_at_; }

    /**
     * @brief setStoneId方法
     *
     * @param id 参数说明
     */
    void setStoneId(const std::string& id) { stone_id_ = id; }
    /**
     * @brief 设置用户ID
     *
     * @param user_id 参数说明
     */
    void setUserId(const std::string& user_id) { user_id_ = user_id; }
    /**
     * @brief setContent方法
     *
     * @param content 参数说明
     */
    void setContent(const std::string& content) { content_ = content; }
    /**
     * @brief setStoneType方法
     *
     * @param type 参数说明
     */
    void setStoneType(const std::string& type) { stone_type_ = type; }
    /**
     * @brief setStoneColor方法
     *
     * @param color 参数说明
     */
    void setStoneColor(const std::string& color) { stone_color_ = color; }
    /**
     * @brief 设置用户是否匿名
     *
     * @param is_anonymous 参数说明
     */
    void setIsAnonymous(bool is_anonymous) { is_anonymous_ = is_anonymous; }
    /**
     * @brief 设置用户状态
     *
     * @param status 参数说明
     */
    void setStatus(const std::string& status) { status_ = status; }
    /**
     * @brief setViewCount方法
     *
     * @param count 参数说明
     */
    void setViewCount(int count) { view_count_ = count; }
    /**
     * @brief setRippleCount方法
     *
     * @param count 参数说明
     */
    void setRippleCount(int count) { ripple_count_ = count; }
    /**
     * @brief setBoatCount方法
     *
     * @param count 参数说明
     */
    void setBoatCount(int count) { boat_count_ = count; }
    /**
     * @brief setTags方法
     *
     * @param tags 参数说明
     */
    void setTags(const std::vector<std::string>& tags) { tags_ = tags; }
    /**
     * @brief 设置创建时间
     *
     * @param time 参数说明
     */
    void setCreatedAt(time_t time) { created_at_ = time; }

    Json::Value toJson() const {
        Json::Value json;
        json["stone_id"] = stone_id_;
        json["user_id"] = user_id_;
        json["content"] = content_;
        json["stone_type"] = stone_type_;
        json["stone_color"] = stone_color_;
        json["is_anonymous"] = is_anonymous_;
        json["status"] = status_;
        json["view_count"] = view_count_;
        json["ripple_count"] = ripple_count_;
        json["boat_count"] = boat_count_;
        json["created_at"] = static_cast<Json::Int64>(created_at_);
        
        Json::Value tagsArray(Json::arrayValue);
        for (const auto& tag : tags_) {
            tagsArray.append(tag);
        }
        json["tags"] = tagsArray;
        
        return json;
    }

    /**
     * @brief 从JSON格式解析对象
     *
     * @param json 参数说明
     */
    void fromJson(const Json::Value& json) {
        if (json.isMember("stone_id")) stone_id_ = json["stone_id"].asString();
        if (json.isMember("user_id")) user_id_ = json["user_id"].asString();
        if (json.isMember("content")) content_ = json["content"].asString();
        if (json.isMember("stone_type")) stone_type_ = json["stone_type"].asString();
        if (json.isMember("stone_color")) stone_color_ = json["stone_color"].asString();
        if (json.isMember("is_anonymous")) is_anonymous_ = json["is_anonymous"].asBool();
        if (json.isMember("status")) status_ = json["status"].asString();
        if (json.isMember("view_count")) view_count_ = json["view_count"].asInt();
        if (json.isMember("ripple_count")) ripple_count_ = json["ripple_count"].asInt();
        if (json.isMember("boat_count")) boat_count_ = json["boat_count"].asInt();
        if (json.isMember("created_at")) created_at_ = json["created_at"].asInt64();
        
        if (json.isMember("tags") && json["tags"].isArray()) {
            tags_.clear();
            for (const auto& tag : json["tags"]) {
                tags_.push_back(tag.asString());
            }
        }
    }

private:
    std::string stone_id_;
    std::string user_id_;
    std::string content_;
    std::string stone_type_ = "medium";  // light/medium/heavy
    std::string stone_color_ = "#7A92A3";
    bool is_anonymous_ = true;
    std::string status_ = "published";
    int view_count_ = 0;
    int ripple_count_ = 0;
    int boat_count_ = 0;
    std::vector<std::string> tags_;
    time_t created_at_ = 0;
};

} // namespace models
} // namespace heartlake
