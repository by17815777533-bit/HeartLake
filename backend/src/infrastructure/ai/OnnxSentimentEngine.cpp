/**
 * @file OnnxSentimentEngine.cpp
 * @brief ONNX Runtime 中文情感分析引擎实现
 *
 * 包含完整的 BERT WordPiece Tokenizer 和 ONNX 推理逻辑。
 *
 * Created by 王璐瑶
 */

#ifdef HEARTLAKE_USE_ONNX

#include "infrastructure/ai/OnnxSentimentEngine.h"
#include "utils/EnvUtils.h"
#include <trantor/utils/Logger.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cctype>
#include <cstdlib>

namespace heartlake {
namespace ai {

namespace {

int parseNonNegativeIntEnv(const char* envName, int defaultValue) {
    const char* raw = std::getenv(envName);
    if (!raw) {
        return defaultValue;
    }
    char* end = nullptr;
    long parsed = std::strtol(raw, &end, 10);
    if (end == raw || *end != '\0' || parsed < 0) {
        return defaultValue;
    }
    return static_cast<int>(parsed);
}

int parseIntEnvClamped(const char* envName, int defaultValue, int minValue, int maxValue) {
    const char* raw = std::getenv(envName);
    if (!raw || *raw == '\0') {
        return defaultValue;
    }
    char* end = nullptr;
    const long parsed = std::strtol(raw, &end, 10);
    if (end == raw || *end != '\0') {
        return defaultValue;
    }
    return std::clamp(static_cast<int>(parsed), minValue, maxValue);
}

}  // namespace

// ============================================================================
// 初始化
// ============================================================================

bool OnnxSentimentEngine::initialize(const std::string& modelPath,
                                      const std::string& vocabPath,
                                      int numThreads) {
    if (initialized_.load()) return true;

    try {
        // 加载词表
        if (!loadVocab(vocabPath)) {
            LOG_ERROR << "[OnnxSentiment] Failed to load vocab: " << vocabPath;
            return false;
        }

        // 创建 ONNX Runtime 环境
        env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "HeartLakeSentiment");

        maxSeqLen_ = parseIntEnvClamped("EDGE_AI_ONNX_MAX_SEQ_LEN", maxSeqLen_, 32, 512);
        const int intraThreads = std::max(1, numThreads);
        const int targetSessionPool = parseIntEnvClamped("EDGE_AI_ONNX_SESSION_POOL", 1, 1, 64);

        // 配置 Session
        sessionOptions_ = std::make_unique<Ort::SessionOptions>();
        sessionOptions_->SetIntraOpNumThreads(intraThreads);
        sessionOptions_->SetInterOpNumThreads(1);
        sessionOptions_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        // 默认优先尝试 CUDA EP（若当前 ORT 不支持会自动降级到 CPU）
        const bool forceGpu = heartlake::utils::parseBoolEnv(
            std::getenv("EDGE_AI_ONNX_FORCE_GPU"), true);
        const bool hardFail = heartlake::utils::parseBoolEnv(
            std::getenv("EDGE_AI_ONNX_GPU_HARD_FAIL"), false);
        const int gpuDeviceId = parseNonNegativeIntEnv("EDGE_AI_ONNX_GPU_DEVICE", 0);
        bool gpuEnabled = false;

        if (forceGpu) {
            try {
                OrtCUDAProviderOptions cudaOptions{};
                cudaOptions.device_id = gpuDeviceId;
                sessionOptions_->AppendExecutionProvider_CUDA(cudaOptions);
                gpuEnabled = true;
                LOG_INFO << "[OnnxSentiment] CUDA EP enabled, device=" << gpuDeviceId;
            } catch (const Ort::Exception& e) {
                LOG_WARN << "[OnnxSentiment] CUDA EP unavailable, fallback to CPU: " << e.what();
                if (hardFail) {
                    return false;
                }
            }
        } else {
            LOG_INFO << "[OnnxSentiment] CUDA EP disabled by EDGE_AI_ONNX_FORCE_GPU=false";
        }

        sessions_.clear();
        sessions_.reserve(static_cast<size_t>(targetSessionPool));
        for (int i = 0; i < targetSessionPool; ++i) {
            try {
                auto session = std::make_unique<Ort::Session>(*env_, modelPath.c_str(), *sessionOptions_);
                if (i == 0) {
                    const size_t numInputs = session->GetInputCount();
                    const size_t numOutputs = session->GetOutputCount();
                    LOG_INFO << "[OnnxSentiment] Model loaded: " << numInputs << " inputs, "
                             << numOutputs << " outputs";

                    for (size_t inputIdx = 0; inputIdx < numInputs; ++inputIdx) {
                        auto name = session->GetInputNameAllocated(inputIdx, allocator_);
                        LOG_INFO << "[OnnxSentiment] Input[" << inputIdx << "]: " << name.get();
                    }
                    for (size_t outputIdx = 0; outputIdx < numOutputs; ++outputIdx) {
                        auto name = session->GetOutputNameAllocated(outputIdx, allocator_);
                        LOG_INFO << "[OnnxSentiment] Output[" << outputIdx << "]: " << name.get();
                    }
                }
                sessions_.push_back(std::move(session));
            } catch (const Ort::Exception& e) {
                if (sessions_.empty()) {
                    throw;
                }
                LOG_WARN << "[OnnxSentiment] Failed to create extra session " << i
                         << ", keep pool size " << sessions_.size() << ": " << e.what();
                break;
            }
        }
        if (sessions_.empty()) {
            LOG_ERROR << "[OnnxSentiment] Session pool init failed";
            return false;
        }
        sessionPoolSize_ = sessions_.size();
        nextSessionIndex_.store(0, std::memory_order_relaxed);

        initialized_.store(true);
        LOG_INFO << "[OnnxSentiment] Engine initialized successfully. "
                 << "Vocab size: " << vocab_.size()
                 << ", Threads: " << intraThreads
                 << ", MaxSeqLen: " << maxSeqLen_
                 << ", SessionPool: " << sessionPoolSize_
                 << ", GPU: " << (gpuEnabled ? "on" : "off");
        return true;

    } catch (const Ort::Exception& e) {
        LOG_ERROR << "[OnnxSentiment] ONNX Runtime error: " << e.what();
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR << "[OnnxSentiment] Init error: " << e.what();
        return false;
    }
}

bool OnnxSentimentEngine::loadVocab(const std::string& vocabPath) {
    std::ifstream file(vocabPath);
    if (!file.is_open()) return false;

    std::string line;
    int id = 0;
    while (std::getline(file, line)) {
        // 去除行尾空白
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
            line.pop_back();
        }
        if (!line.empty()) {
            vocab_[line] = id;
        }
        ++id;
    }

    // 查找特殊 token ID
    auto findToken = [this](const std::string& token, int defaultId) -> int {
        auto it = vocab_.find(token);
        return it != vocab_.end() ? it->second : defaultId;
    };

    clsTokenId_ = findToken("[CLS]", 101);
    sepTokenId_ = findToken("[SEP]", 102);
    unkTokenId_ = findToken("[UNK]", 100);
    padTokenId_ = findToken("[PAD]", 0);

    LOG_INFO << "[OnnxSentiment] Vocab loaded: " << vocab_.size() << " tokens"
             << " [CLS]=" << clsTokenId_ << " [SEP]=" << sepTokenId_
             << " [UNK]=" << unkTokenId_ << " [PAD]=" << padTokenId_;
    return !vocab_.empty();
}

// ============================================================================
// BERT WordPiece Tokenizer
// ============================================================================

uint32_t OnnxSentimentEngine::decodeUTF8(const std::string& s, size_t& pos) {
    uint32_t cp = 0;
    unsigned char c = static_cast<unsigned char>(s[pos]);

    if (c < 0x80) {
        cp = c;
        pos += 1;
    } else if ((c & 0xE0) == 0xC0) {
        cp = c & 0x1F;
        if (pos + 1 < s.size()) cp = (cp << 6) | (static_cast<unsigned char>(s[pos + 1]) & 0x3F);
        pos += 2;
    } else if ((c & 0xF0) == 0xE0) {
        cp = c & 0x0F;
        if (pos + 1 < s.size()) cp = (cp << 6) | (static_cast<unsigned char>(s[pos + 1]) & 0x3F);
        if (pos + 2 < s.size()) cp = (cp << 6) | (static_cast<unsigned char>(s[pos + 2]) & 0x3F);
        pos += 3;
    } else if ((c & 0xF8) == 0xF0) {
        cp = c & 0x07;
        if (pos + 1 < s.size()) cp = (cp << 6) | (static_cast<unsigned char>(s[pos + 1]) & 0x3F);
        if (pos + 2 < s.size()) cp = (cp << 6) | (static_cast<unsigned char>(s[pos + 2]) & 0x3F);
        if (pos + 3 < s.size()) cp = (cp << 6) | (static_cast<unsigned char>(s[pos + 3]) & 0x3F);
        pos += 4;
    } else {
        pos += 1; // 无效字节，跳过
    }
    return cp;
}

bool OnnxSentimentEngine::isChineseChar(uint32_t cp) {
    return (cp >= 0x4E00 && cp <= 0x9FFF) ||
           (cp >= 0x3400 && cp <= 0x4DBF) ||
           (cp >= 0x20000 && cp <= 0x2A6DF) ||
           (cp >= 0x2A700 && cp <= 0x2B73F) ||
           (cp >= 0x2B740 && cp <= 0x2B81F) ||
           (cp >= 0x2B820 && cp <= 0x2CEAF) ||
           (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0x2F800 && cp <= 0x2FA1F);
}

bool OnnxSentimentEngine::isPunctuation(uint32_t cp) {
    if ((cp >= 33 && cp <= 47) || (cp >= 58 && cp <= 64) ||
        (cp >= 91 && cp <= 96) || (cp >= 123 && cp <= 126)) {
        return true;
    }
    // 中文标点
    if ((cp >= 0x3000 && cp <= 0x303F) || (cp >= 0xFF00 && cp <= 0xFFEF)) {
        return true;
    }
    return false;
}

std::vector<std::string> OnnxSentimentEngine::basicTokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current;

    size_t pos = 0;
    while (pos < text.size()) {
        uint32_t cp = decodeUTF8(text, pos);

        if (cp == 0) continue;

        // 空白字符：flush 当前 token
        if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' ||
            cp == 0x00A0 || cp == 0x3000) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }

        // 中文字符：每个字独立成 token
        if (isChineseChar(cp)) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            // 编码回 UTF-8
            std::string ch;
            if (cp < 0x80) {
                ch += static_cast<char>(cp);
            } else if (cp < 0x800) {
                ch += static_cast<char>(0xC0 | (cp >> 6));
                ch += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                ch += static_cast<char>(0xE0 | (cp >> 12));
                ch += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                ch += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                ch += static_cast<char>(0xF0 | (cp >> 18));
                ch += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                ch += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                ch += static_cast<char>(0x80 | (cp & 0x3F));
            }
            tokens.push_back(ch);
            continue;
        }

        // 标点：独立成 token
        if (isPunctuation(cp)) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            std::string ch;
            if (cp < 0x80) {
                ch += static_cast<char>(cp);
            } else if (cp < 0x800) {
                ch += static_cast<char>(0xC0 | (cp >> 6));
                ch += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                ch += static_cast<char>(0xE0 | (cp >> 12));
                ch += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                ch += static_cast<char>(0x80 | (cp & 0x3F));
            }
            tokens.push_back(ch);
            continue;
        }

        // ASCII 字母/数字：转小写，累积
        if (cp < 0x80) {
            current += static_cast<char>(std::tolower(static_cast<int>(cp)));
        } else {
            // 其他 Unicode 字符：编码回 UTF-8 累积
            if (cp < 0x800) {
                current += static_cast<char>(0xC0 | (cp >> 6));
                current += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                current += static_cast<char>(0xE0 | (cp >> 12));
                current += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                current += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                current += static_cast<char>(0xF0 | (cp >> 18));
                current += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                current += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                current += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

std::vector<std::string> OnnxSentimentEngine::wordPieceTokenize(const std::string& word) {
    std::vector<std::string> subTokens;

    if (word.empty()) return subTokens;

    // 如果整个词在词表中，直接返回
    if (vocab_.count(word)) {
        subTokens.push_back(word);
        return subTokens;
    }

    // WordPiece 贪心最长匹配
    size_t start = 0;
    while (start < word.size()) {
        size_t end = word.size();
        std::string bestMatch;

        while (start < end) {
            std::string substr = word.substr(start, end - start);
            if (start > 0) {
                substr = "##" + substr;
            }

            if (vocab_.count(substr)) {
                bestMatch = substr;
                break;
            }

            // 缩短：回退一个 UTF-8 字符
            size_t newEnd = end - 1;
            while (newEnd > start && (static_cast<unsigned char>(word[newEnd]) & 0xC0) == 0x80) {
                --newEnd;
            }
            if (newEnd == end) break; // 防止死循环
            end = newEnd;
        }

        if (bestMatch.empty()) {
            // 无法匹配，用 [UNK]
            subTokens.push_back("[UNK]");
            // 跳过当前 UTF-8 字符
            size_t skip = start + 1;
            while (skip < word.size() && (static_cast<unsigned char>(word[skip]) & 0xC0) == 0x80) {
                ++skip;
            }
            start = skip;
        } else {
            subTokens.push_back(bestMatch);
            start += bestMatch.size();
            if (bestMatch.size() > 2 && bestMatch[0] == '#' && bestMatch[1] == '#') {
                start -= 2; // ## 前缀不算在原始 word 的偏移中
            }
        }
    }

    return subTokens;
}

OnnxSentimentEngine::TokenizerOutput OnnxSentimentEngine::tokenize(const std::string& text) {
    TokenizerOutput output;

    // Step 1: 基础分词
    auto basicTokens = basicTokenize(text);

    // Step 2: WordPiece 子词切分
    std::vector<int> tokenIds;
    tokenIds.push_back(clsTokenId_); // [CLS]

    for (const auto& token : basicTokens) {
        auto subTokens = wordPieceTokenize(token);
        for (const auto& sub : subTokens) {
            auto it = vocab_.find(sub);
            tokenIds.push_back(it != vocab_.end() ? it->second : unkTokenId_);

            // 限制长度（留一个位置给 [SEP]）
            if (static_cast<int>(tokenIds.size()) >= maxSeqLen_ - 1) break;
        }
        if (static_cast<int>(tokenIds.size()) >= maxSeqLen_ - 1) break;
    }

    tokenIds.push_back(sepTokenId_); // [SEP]

    // Step 3: Padding
    int seqLen = static_cast<int>(tokenIds.size());
    output.inputIds.resize(maxSeqLen_, padTokenId_);
    output.attentionMask.resize(maxSeqLen_, 0);
    output.tokenTypeIds.resize(maxSeqLen_, 0);

    for (int i = 0; i < seqLen; ++i) {
        output.inputIds[i] = tokenIds[i];
        output.attentionMask[i] = 1;
    }

    return output;
}

// ============================================================================
// 推理
// ============================================================================

OnnxSentimentResult OnnxSentimentEngine::analyze(const std::string& text) {
    OnnxSentimentResult result{0.0f, 0.5f, "neutral"};

    if (!initialized_.load() || text.empty() || sessions_.empty()) {
        return result;
    }

    auto startTime = std::chrono::steady_clock::now();
    ++totalCalls_;

    try {
        // Tokenize
        auto encoded = tokenize(text);

        // 创建输入 tensors
        std::array<int64_t, 2> inputShape = {1, static_cast<int64_t>(maxSeqLen_)};
        auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        auto inputIdsTensor = Ort::Value::CreateTensor<int64_t>(
            memoryInfo, encoded.inputIds.data(), encoded.inputIds.size(),
            inputShape.data(), inputShape.size());

        auto attentionMaskTensor = Ort::Value::CreateTensor<int64_t>(
            memoryInfo, encoded.attentionMask.data(), encoded.attentionMask.size(),
            inputShape.data(), inputShape.size());

        auto tokenTypeIdsTensor = Ort::Value::CreateTensor<int64_t>(
            memoryInfo, encoded.tokenTypeIds.data(), encoded.tokenTypeIds.size(),
            inputShape.data(), inputShape.size());

        // 输入输出名称
        const char* inputNames[] = {"input_ids", "attention_mask", "token_type_ids"};
        const char* outputNames[] = {"logits"};

        std::vector<Ort::Value> inputTensors;
        inputTensors.push_back(std::move(inputIdsTensor));
        inputTensors.push_back(std::move(attentionMaskTensor));
        inputTensors.push_back(std::move(tokenTypeIdsTensor));

        const uint64_t sessionIndex = nextSessionIndex_.fetch_add(1, std::memory_order_relaxed);
        Ort::Session* activeSession = sessions_[static_cast<size_t>(sessionIndex % sessionPoolSize_)].get();

        // 推理
        auto outputTensors = activeSession->Run(
            Ort::RunOptions{nullptr},
            inputNames, inputTensors.data(), inputTensors.size(),
            outputNames, 1);

        // 解析输出: logits [1, 2] -> softmax -> [neg_prob, pos_prob]
        float* logits = outputTensors[0].GetTensorMutableData<float>();
        float negLogit = logits[0];
        float posLogit = logits[1];

        // Softmax
        float maxLogit = std::max(negLogit, posLogit);
        float expNeg = std::exp(negLogit - maxLogit);
        float expPos = std::exp(posLogit - maxLogit);
        float sumExp = expNeg + expPos;
        float negProb = expNeg / sumExp;
        float posProb = expPos / sumExp;

        // 映射到 [-1, 1]
        result.score = posProb - negProb;
        result.confidence = std::max(negProb, posProb);
        result.mood = scoresToMood(result.score);

        auto endTime = std::chrono::steady_clock::now();
        double latencyMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        // CAS 循环实现原子 read-modify-write，避免并发推理时丢失延迟统计
        double oldVal = totalLatencyMs_.load(std::memory_order_relaxed);
        while (!totalLatencyMs_.compare_exchange_weak(
            oldVal, oldVal + latencyMs,
            std::memory_order_relaxed, std::memory_order_relaxed)) {
            // oldVal 被自动更新为当前值，重试
        }

    } catch (const Ort::Exception& e) {
        LOG_ERROR << "[OnnxSentiment] Inference error: " << e.what();
    } catch (const std::exception& e) {
        LOG_ERROR << "[OnnxSentiment] Error: " << e.what();
    }

    return result;
}

std::string OnnxSentimentEngine::scoresToMood(float score) {
    if (score > 0.6f) return "happy";
    if (score > 0.3f) return "calm";
    if (score > -0.3f) return "neutral";
    if (score > -0.6f) return "anxious";
    return "sad";
}

double OnnxSentimentEngine::getAvgLatencyMs() const {
    size_t calls = totalCalls_.load();
    if (calls == 0) return 0.0;
    return totalLatencyMs_.load() / static_cast<double>(calls);
}

} // namespace ai
} // namespace heartlake

#endif // HEARTLAKE_USE_ONNX
