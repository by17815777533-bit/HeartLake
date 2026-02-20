/**
 * @file TempFriendController.h
 * @brief TempFriendController 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * 临时好友控制器
 * 临时好友会在24小时后自动失效，可以升级为永久好友
 */
/**
 * @brief 临时好友相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class TempFriendController : public drogon::HttpController<TempFriendController> {
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(TempFriendController::createTempFriend,
                  "/api/temp-friends", Post, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(TempFriendController::createTempFriend,
                  "/api/friends/temp", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(TempFriendController::getMyTempFriends,
                  "/api/temp-friends", Get, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(TempFriendController::getMyTempFriends,
                  "/api/friends/temp", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(TempFriendController::getTempFriendDetail,
                  "/api/temp-friends/{1}", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(TempFriendController::upgradeToPermanent,
                  "/api/temp-friends/{1}/upgrade", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(TempFriendController::deleteTempFriend,
                  "/api/temp-friends/{1}", Delete, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(TempFriendController::checkTempFriendStatus,
                  "/api/temp-friends/check/{1}", Get, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END
    
    void createTempFriend(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getMyTempFriends(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getTempFriendDetail(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback,
                            const std::string &tempFriendId);
    
    void upgradeToPermanent(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback,
                           const std::string &tempFriendId);
    
    void deleteTempFriend(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback,
                         const std::string &tempFriendId);
    
    void checkTempFriendStatus(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback,
                              const std::string &targetUserId);
};

} // namespace controllers
} // namespace heartlake
