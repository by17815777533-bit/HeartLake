/**
 * @file OnnxSentimentEngine.h
 * @brief ONNX Runtime 中文情感分析引擎
 *
 * 使用 Erlangshen-Roberta-110M-Sentiment 模型进行高精度中文情感分析。
 * 内置 BERT WordPiece Tokenizer，无需额外依赖。
 *
 * Created by 王璐瑶
 */

#pragma once

#ifdef HEARTLAKE_USE_ONNX

#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

namespace heartlake {
namespace ai {

/**
 * @brief ONNX 情感分析结果
 */
struct OnnxSentimentResult {
    float score;          ///< 情感分数 [-1.0, 1.0]
    float confidence;     ///< 置信度 [0.0, 1.0]
    std::string mood;     ///< 情绪标签
};

/**
 * @brief ONNX 中文情感分析引擎
 *
 * 封装 ONNX Runtime 推理和 BERT WordPiece Tokenizer。
 * 线程安全：OrtSession::Run() 本身支持并发调用。
 */
class OnnxSentimentEngine {
public:
    OnnxSentimentEngine() = default;
    ~OnnxSentimentEngine() = default;

    /**
     * @brief 初始化引擎
     * @param modelPath ONNX 模型文件路径
     * @param vocabPath vocab.txt 词表文件路径
     * @param numThreads 推理线程数
     * @return 是否初始化成功
     */
    bool initialize(const std::string& modelPath,
                    const std::string& vocabPath,
                    int numThreads = 2);

    /**
     * @brief 是否已初始化
     */
    bool isInitialized() const { return initialized_.load(); }

    /**
     * @brief 分析文本情感
     * @param text 输入文本
     * @return 情感分析结果
     */
    OnnxSentimentResult analyze(const std::string& text);

    /**
     * @brief 获取统计信息
     */
    size_t getTotalCalls() const { return totalCalls_.load(); }
    double getAvgLatencyMs() const;

private:
    // ---- ONNX Runtime ----
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::SessionOptions> sessionOptions_;
    Ort::AllocatorWithDefaultOptions allocator_;

    // ---- BERT WordPiece Tokenizer ----
    std::unordered_map<std::string, int> vocab_;
    int clsTokenId_ = 101;    // [CLS]
    int sepTokenId_ = 102;    // [SEP]
    int unkTokenId_ = 100;    // [UNK]
    int padTokenId_ = 0;      // [PAD]
    int maxSeqLen_ = 128;

    /**
     * @brief 加载词表
     */
    bool loadVocab(const std::string& vocabPath);

    /**
     * @brief 完整 tokenize 流程
     * @return {input_ids, attention_mask, token_type_ids}
     */
    struct TokenizerOutput {
        std::vector<int64_t> inputIds;
        std::vector<int64_t> attentionMask;
        std::vector<int64_t> tokenTypeIds;
    };
    TokenizerOutput tokenize(const std::string& text);

    /**
     * @brief 基础分词：按空格/标点/中文字符切分
     */
    std::vector<std::string> basicTokenize(const std::string& text);

    /**
     * @brief WordPiece 子词切分
     */
    std::vector<std::string> wordPieceTokenize(const std::string& word);

    /**
     * @brief 判断是否为中文字符
     */
    static bool isChineseChar(uint32_t cp);

    /**
     * @brief 判断是否为标点
     */
    static bool isPunctuation(uint32_t cp);

    /**
     * @brief UTF-8 解码单个码点
     */
    static uint32_t decodeUTF8(const std::string& s, size_t& pos);

    /**
     * @brief 分数映射到情绪标签
     */
    static std::string scoresToMood(float score);

    // ---- 状态 ----
    std::atomic<bool> initialized_{false};
    std::atomic<size_t> totalCalls_{0};
    std::atomic<double> totalLatencyMs_{0.0};
};

} // namespace ai
} // namespace heartlake

#endif // HEARTLAKE_USE_ONNX
