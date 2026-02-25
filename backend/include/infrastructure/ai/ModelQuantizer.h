/**
 * @file ModelQuantizer.h
 * @brief 模型量化推理子系统 - INT8量化/反量化与量化矩阵运算
 *
 * 从 EdgeAIEngine 提取的独立量化推理模块。
 * 纯函数式、无外部依赖、线程安全（仅 atomic 计数器）。
 *
 * 功能：
 * - float32 → INT8 对称量化
 * - INT8 量化矩阵乘法（手动4路展开）
 * - 量化前向推理（量化→矩阵乘→加偏置→ReLU）
 */

#pragma once

#include <vector>
#include <cstdint>
#include <atomic>
#include <cstddef>

namespace heartlake {
namespace ai {

class ModelQuantizer {
public:
    /**
     * @brief INT8量化后的张量
     */
    struct QuantizedTensor {
        std::vector<int8_t> data;   ///< 量化后的INT8数据
        float scale;                ///< 量化缩放因子
        int8_t zeroPoint;           ///< 零点偏移
        std::vector<size_t> shape;  ///< 张量形状

        /**
         * @brief 反量化为float32
         * @return float32向量
         */
        std::vector<float> dequantize() const {
            std::vector<float> result(data.size());
            for (size_t i = 0; i < data.size(); ++i) {
                result[i] = scale * (static_cast<float>(data[i]) - static_cast<float>(zeroPoint));
            }
            return result;
        }
    };

    /**
     * @brief 将float32张量量化为INT8（对称量化）
     * @param tensor 输入float32数据
     * @param shape 张量形状
     * @return 量化后的QuantizedTensor
     */
    QuantizedTensor quantizeToInt8(const std::vector<float>& tensor,
                                   const std::vector<size_t>& shape);

    /**
     * @brief INT8量化矩阵乘法 (M×K) × (K×N) → (M×N)
     * @param a 左矩阵（量化）
     * @param b 右矩阵（量化）
     * @param M 左矩阵行数
     * @param K 内维度
     * @param N 右矩阵列数
     * @return float32结果矩阵（已反量化）
     */
    std::vector<float> quantizedMatMul(const QuantizedTensor& a,
                                       const QuantizedTensor& b,
                                       size_t M, size_t K, size_t N);

    /**
     * @brief 量化前向推理（量化输入→矩阵乘→加偏置→ReLU）
     * @param input 输入向量 (inDim)
     * @param weights 权重矩阵 (outDim × inDim)，已量化
     * @param biases 偏置向量 (outDim)
     * @return 输出向量 (outDim)
     */
    std::vector<float> quantizedForward(const std::vector<float>& input,
                                        const QuantizedTensor& weights,
                                        const std::vector<float>& biases);

    /**
     * @brief 获取累计量化操作次数
     */
    size_t getTotalOps() const { return totalQuantizedOps_.load(); }

private:
    std::atomic<size_t> totalQuantizedOps_{0};
};

} // namespace ai
} // namespace heartlake
