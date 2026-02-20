/**
 * @file PasswordUtil.cpp
 * @brief PasswordUtil 模块实现
 * Created by 林子怡
 */
#include "utils/PasswordUtil.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <regex>
#include <vector>

using namespace heartlake::utils;

std::string PasswordUtil::generateSalt() {
    unsigned char salt[SALT_LENGTH];
    if (RAND_bytes(salt, SALT_LENGTH) != 1) {
        throw std::runtime_error("Failed to generate random salt");
    }
    return bytesToHex(salt, SALT_LENGTH);
}

std::string PasswordUtil::pbkdf2(const std::string& password,
                                 const std::string& salt,
                                 int iterations,
                                 int keyLength) {
    std::vector<unsigned char> key(keyLength);
    std::string saltBytes = hexToBytes(salt);

    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                          reinterpret_cast<const unsigned char*>(saltBytes.c_str()),
                          saltBytes.length(),
                          iterations,
                          EVP_sha256(),
                          keyLength,
                          key.data()) != 1) {
        throw std::runtime_error("PBKDF2 key derivation failed");
    }

    return bytesToHex(key.data(), keyLength);
}

std::string PasswordUtil::hashPassword(const std::string& password, const std::string& salt) {
    return pbkdf2(password, salt, ITERATIONS, HASH_LENGTH);
}

bool PasswordUtil::verifyPassword(const std::string& password,
                                  const std::string& salt,
                                  const std::string& hashedPassword) {
    try {
        std::string computedHash = hashPassword(password, salt);
        // 使用时间恒定比较防止时序攻击
        if (computedHash.length() != hashedPassword.length()) {
            return false;
        }
        volatile int result = 0;
        for (size_t i = 0; i < computedHash.length(); ++i) {
            result |= computedHash[i] ^ hashedPassword[i];
        }
        return result == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

void PasswordUtil::generatePasswordHash(const std::string& password,
                                       std::string& outSalt,
                                       std::string& outHash) {
    outSalt = generateSalt();
    outHash = hashPassword(password, outSalt);
}

std::string PasswordUtil::bytesToHex(const unsigned char* bytes, size_t length) {
    std::stringstream ss;
    for (size_t i = 0; i < length; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

std::string PasswordUtil::hexToBytes(const std::string& hex) {
    if (hex.length() % 2 != 0) {
        throw std::runtime_error("Invalid hex string length");
    }
    std::string bytes;
    bytes.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
        if (!std::isxdigit(hex[i]) || !std::isxdigit(hex[i + 1])) {
            throw std::runtime_error("Invalid hex character");
        }
        std::string byteString = hex.substr(i, 2);
        char byte = static_cast<char>(std::strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}
