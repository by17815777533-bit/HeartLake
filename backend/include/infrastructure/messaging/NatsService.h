/**
 * @file NatsService.h
 * @brief NATS JetStream消息队列服务 (HTTP REST API实现)
 *
 * 前沿技术：NATS JetStream - 微秒级延迟消息队列
 */

#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <future>
#include <ctime>
#include <json/json.h>
#include <drogon/drogon.h>

namespace heartlake::infrastructure::messaging {

struct NatsConfig {
    std::string host = "127.0.0.1";
    int port = 8222;
    std::string stream = "HEARTLAKE";
};

class NatsService {
public:
    static NatsService& getInstance() {
        static NatsService instance;
        return instance;
    }

    void initialize(const NatsConfig& config = NatsConfig{}) {
        config_ = config;
        baseUrl_ = "http://" + config_.host + ":" + std::to_string(config_.port);
        initialized_ = true;
    }

    bool publish(const std::string& subject, const std::string& data) {
        if (!initialized_) return false;

        auto client = drogon::HttpClient::newHttpClient(baseUrl_);
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setPath("/publish");
        req->setMethod(drogon::Post);
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        Json::Value body;
        body["subject"] = subject;
        body["payload"] = data;
        Json::StreamWriterBuilder writer;
        req->setBody(Json::writeString(writer, body));

        std::promise<bool> promise;
        auto future = promise.get_future();

        client->sendRequest(req, [&promise](drogon::ReqResult result, const drogon::HttpResponsePtr& resp) {
            promise.set_value(result == drogon::ReqResult::Ok && resp && resp->statusCode() == drogon::k200OK);
        });

        // 带超时等待，避免永久阻塞
        if (future.wait_for(std::chrono::seconds(5)) == std::future_status::ready) {
            return future.get();
        }
        LOG_WARN << "NATS publish timeout for subject: " << subject;
        return false;
    }

    void publishStoneEvent(const std::string& stoneId, const std::string& userId,
                           const std::string& content, const std::string& mood) {
        Json::Value json;
        json["stoneId"] = stoneId;
        json["userId"] = userId;
        json["content"] = content;
        json["mood"] = mood;
        json["timestamp"] = static_cast<Json::Int64>(std::time(nullptr));
        Json::StreamWriterBuilder writer;
        publish("heartlake.stone.published", Json::writeString(writer, json));
    }

    void publishRippleEvent(const std::string& rippleId, const std::string& stoneId,
                            const std::string& userId) {
        Json::Value json;
        json["rippleId"] = rippleId;
        json["stoneId"] = stoneId;
        json["userId"] = userId;
        json["timestamp"] = static_cast<Json::Int64>(std::time(nullptr));
        Json::StreamWriterBuilder writer;
        publish("heartlake.ripple.created", Json::writeString(writer, json));
    }

    void publishBoatEvent(const std::string& boatId, const std::string& senderId,
                          const std::string& content) {
        Json::Value json;
        json["boatId"] = boatId;
        json["senderId"] = senderId;
        json["content"] = content;
        json["timestamp"] = static_cast<Json::Int64>(std::time(nullptr));
        Json::StreamWriterBuilder writer;
        publish("heartlake.boat.sent", Json::writeString(writer, json));
    }

    bool isInitialized() const { return initialized_; }

private:
    NatsService() = default;
    ~NatsService() = default;

    NatsConfig config_;
    std::string baseUrl_;
    std::atomic<bool> initialized_{false};
};

} // namespace heartlake::infrastructure::messaging
