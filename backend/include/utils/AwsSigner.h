/**
 * AWS Signature V4 请求签名工具
 *
 * 实现 AWS SigV4 签名流程，用于调用 AWS 服务（如 Comprehend 情感分析）。
 * 签名过程：Canonical Request -> StringToSign -> 四层 HMAC-SHA256 派生签名密钥 -> 最终签名。
 * 密钥材料仅在内存中短暂存在，不做持久化。
 */

#pragma once

#include <string>
#include <map>

namespace heartlake {
namespace utils {

class AwsSigner {
public:
    /// 签名后的请求，包含需要附加到 HTTP 请求的头部
    struct SignedRequest {
        std::map<std::string, std::string> headers;
        std::string authorizationHeader;
    };

    /**
     * 对 HTTP 请求进行 AWS SigV4 签名
     *
     * @param method HTTP 方法（GET/POST 等）
     * @param host 目标服务主机名
     * @param path 请求路径
     * @param region AWS 区域（如 us-east-1）
     * @param service AWS 服务名（如 comprehend）
     * @param payload 请求体原文，用于计算 payload hash
     * @param accessKeyId AWS Access Key ID
     * @param secretAccessKey AWS Secret Access Key
     * @return 包含 Authorization 头和必要请求头的签名结果
     */
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
    /// HMAC-SHA256 消息认证码
    static std::string hmacSha256(const std::string& key, const std::string& data);
    /// SHA-256 哈希并转为小写十六进制
    static std::string sha256Hex(const std::string& data);
    /// 获取 ISO 8601 格式的 UTC 时间戳（如 20260228T120000Z）
    static std::string getTimestamp();
    /// 获取日期戳（如 20260228）
    static std::string getDateStamp();
};

} // namespace utils
} // namespace heartlake
