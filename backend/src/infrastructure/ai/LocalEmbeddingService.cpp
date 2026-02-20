/**
 * @file LocalEmbeddingService.cpp
 * @brief 本地Embedding服务实现
 */

#include "infrastructure/ai/LocalEmbeddingService.h"
#include <drogon/HttpClient.h>
#include <drogon/drogon.h>

using namespace drogon;

namespace heartlake {
namespace ai {

LocalEmbeddingService& LocalEmbeddingService::getInstance() {
    static LocalEmbeddingService instance;
    return instance;
}

void LocalEmbeddingService::initialize(
    const std::string& localUrl,
    const std::string& remoteUrl,
    const std::string& remoteApiKey
) {
    localUrl_ = localUrl;
    remoteUrl_ = remoteUrl;
    remoteApiKey_ = remoteApiKey;
    healthCheck();
    LOG_INFO << "LocalEmbeddingService initialized, local=" << localUrl_;
}

void LocalEmbeddingService::healthCheck() {
    auto client = HttpClient::newHttpClient(localUrl_);
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/health");

    client->sendRequest(req,
        [this](ReqResult result, const HttpResponsePtr& resp) {
            localAvailable_ = (result == ReqResult::Ok && resp &&
                              resp->getStatusCode() == k200OK);
            LOG_INFO << "Local embedding service available: " << localAvailable_.load();
        }, 2.0);
}

void LocalEmbeddingService::generateEmbedding(
    const std::string& text,
    std::function<void(const std::vector<float>&, const std::string&, bool)> callback
) {
    if (localAvailable_.load()) {
        callLocalService(text, [this, callback, text](const std::vector<float>& emb, const std::string& err) {
            if (err.empty()) {
                localCalls_++;
                callback(emb, "", true);
            } else {
                localFailures_++;
                localAvailable_ = false;
                // 降级到远程
                if (!remoteUrl_.empty()) {
                    callRemoteService(text, [callback](const std::vector<float>& e, const std::string& er) {
                        callback(e, er, false);
                    });
                } else {
                    callback({}, err, false);
                }
            }
        });
    } else if (!remoteUrl_.empty()) {
        remoteCalls_++;
        callRemoteService(text, [callback](const std::vector<float>& e, const std::string& er) {
            callback(e, er, false);
        });
    } else {
        callback({}, "No embedding service available", false);
    }
}

void LocalEmbeddingService::generateEmbeddingBatch(
    const std::vector<std::string>& texts,
    std::function<void(const std::vector<std::vector<float>>&, const std::string&, bool)> callback
) {
    if (texts.empty()) {
        callback({}, "", true);
        return;
    }

    auto results = std::make_shared<std::vector<std::vector<float>>>(texts.size());
    auto counter = std::make_shared<std::atomic<size_t>>(0);
    auto hasError = std::make_shared<std::atomic<bool>>(false);

    for (size_t i = 0; i < texts.size(); ++i) {
        generateEmbedding(texts[i], [=](const std::vector<float>& emb, const std::string& err, bool isLocal) {
            if (!err.empty()) hasError->store(true);
            (*results)[i] = emb;
            if (++(*counter) == texts.size()) {
                callback(*results, hasError->load() ? "Some embeddings failed" : "", isLocal);
            }
        });
    }
}

void LocalEmbeddingService::callLocalService(
    const std::string& text,
    std::function<void(const std::vector<float>&, const std::string&)> callback
) {
    auto client = HttpClient::newHttpClient(localUrl_);

    Json::Value payload;
    payload["text"] = text;

    auto req = HttpRequest::newHttpJsonRequest(payload);
    req->setMethod(drogon::Post);
    req->setPath("/embed");

    client->sendRequest(req,
        [callback](ReqResult result, const HttpResponsePtr& resp) {
            if (result != ReqResult::Ok || !resp || resp->getStatusCode() != k200OK) {
                callback({}, "Local service error");
                return;
            }

            auto json = resp->getJsonObject();
            if (!json || !json->isMember("embedding")) {
                callback({}, "Invalid response");
                return;
            }

            std::vector<float> embedding;
            for (const auto& v : (*json)["embedding"]) {
                embedding.push_back(v.asFloat());
            }
            callback(embedding, "");
        }, 5.0);
}

void LocalEmbeddingService::callRemoteService(
    const std::string& text,
    std::function<void(const std::vector<float>&, const std::string&)> callback
) {
    remoteCalls_++;
    auto client = HttpClient::newHttpClient(remoteUrl_);

    Json::Value payload;
    payload["input"] = text;
    payload["model"] = "text-embedding-3-small";

    auto req = HttpRequest::newHttpJsonRequest(payload);
    req->setMethod(drogon::Post);
    req->setPath("/v1/embeddings");
    req->addHeader("Authorization", "Bearer " + remoteApiKey_);

    client->sendRequest(req,
        [callback](ReqResult result, const HttpResponsePtr& resp) {
            if (result != ReqResult::Ok || !resp || resp->getStatusCode() != k200OK) {
                callback({}, "Remote API error");
                return;
            }

            auto json = resp->getJsonObject();
            if (!json || !json->isMember("data")) {
                callback({}, "Invalid response");
                return;
            }

            std::vector<float> embedding;
            for (const auto& v : (*json)["data"][0]["embedding"]) {
                embedding.push_back(v.asFloat());
            }
            callback(embedding, "");
        }, 30.0);
}

LocalEmbeddingService::Stats LocalEmbeddingService::getStats() const {
    Stats s;
    s.localCalls = localCalls_.load();
    s.remoteCalls = remoteCalls_.load();
    s.localFailures = localFailures_.load();
    return s;
}

} // namespace ai
} // namespace heartlake
