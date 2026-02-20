/**
 * @file AwsSigner.h
 * @brief AWS Signature V4 签名工具
 */

#pragma once

#include <string>
#include <map>

namespace heartlake {
namespace utils {

class AwsSigner {
public:
    struct SignedRequest {
        std::map<std::string, std::string> headers;
        std::string authorizationHeader;
    };

    static SignedRequest sign(
        const std::string& method,
        const std::string& host,
        const std::string& path,
        const std::string& region,
        const std::string& service,
        const std::string& payload,
        const std::string& accessKeyId,
        const std::string& secretAccessKey
    );

private:
    static std::string hmacSha256(const std::string& key, const std::string& data);
    static std::string sha256Hex(const std::string& data);
    static std::string getTimestamp();
    static std::string getDateStamp();
};

} // namespace utils
} // namespace heartlake
