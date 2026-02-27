/**
 * AWS Signature V4 签名实现
 */

#include "utils/AwsSigner.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

namespace heartlake {
namespace utils {

static std::string toHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; ++i) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::string AwsSigner::sha256Hex(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    return toHex(hash, SHA256_DIGEST_LENGTH);
}

std::string AwsSigner::hmacSha256(const std::string& key, const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(EVP_sha256(), key.c_str(), key.size(),
         reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash, &len);
    return std::string(reinterpret_cast<char*>(hash), len);
}

std::string AwsSigner::getTimestamp() {
    time_t now = time(nullptr);
    struct tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &now);
#else
    gmtime_r(&now, &tm);
#endif
    char buf[32];
    strftime(buf, sizeof(buf), "%Y%m%dT%H%M%SZ", &tm);
    return buf;
}

std::string AwsSigner::getDateStamp() {
    time_t now = time(nullptr);
    struct tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &now);
#else
    gmtime_r(&now, &tm);
#endif
    char buf[16];
    strftime(buf, sizeof(buf), "%Y%m%d", &tm);
    return buf;
}

AwsSigner::SignedRequest AwsSigner::sign(
    const std::string& method,
    const std::string& host,
    const std::string& path,
    const std::string& region,
    const std::string& service,
    const std::string& payload,
    const std::string& accessKeyId,
    const std::string& secretAccessKey
) {
    std::string timestamp = getTimestamp();
    std::string datestamp = timestamp.substr(0, 8);
    std::string payloadHash = sha256Hex(payload);

    // Canonical request
    std::string canonicalHeaders = "content-type:application/x-amz-json-1.1\n"
                                   "host:" + host + "\n"
                                   "x-amz-date:" + timestamp + "\n";
    std::string signedHeaders = "content-type;host;x-amz-date";

    std::string canonicalRequest = method + "\n" + path + "\n\n" +
                                   canonicalHeaders + "\n" +
                                   signedHeaders + "\n" + payloadHash;

    // String to sign
    std::string algorithm = "AWS4-HMAC-SHA256";
    std::string credentialScope = datestamp + "/" + region + "/" + service + "/aws4_request";
    std::string stringToSign = algorithm + "\n" + timestamp + "\n" +
                               credentialScope + "\n" + sha256Hex(canonicalRequest);

    // Signing key
    std::string kDate = hmacSha256("AWS4" + secretAccessKey, datestamp);
    std::string kRegion = hmacSha256(kDate, region);
    std::string kService = hmacSha256(kRegion, service);
    std::string kSigning = hmacSha256(kService, "aws4_request");
    std::string signature = toHex(reinterpret_cast<const unsigned char*>(
        hmacSha256(kSigning, stringToSign).c_str()), 32);

    // Authorization header
    std::string authorization = algorithm + " Credential=" + accessKeyId + "/" +
                                credentialScope + ", SignedHeaders=" + signedHeaders +
                                ", Signature=" + signature;

    SignedRequest result;
    result.headers["Content-Type"] = "application/x-amz-json-1.1";
    result.headers["X-Amz-Date"] = timestamp;
    result.headers["Host"] = host;
    result.authorizationHeader = authorization;
    return result;
}

} // namespace utils
} // namespace heartlake
