/**
 * @file ModelQuantizer.cpp
 * @brief 模型量化推理子系统实现
 *
 * 从 EdgeAIEngine.cpp 提取的量化推理实现。
 * INT8对称量化、量化矩阵乘法（4路展开）、量化前向推理。
 */

#include "infrastructure/ai/ModelQuantizer.h"

#include <trantor/utils/Logger.h>
#include <cmath>
#include <algorithm>

namespace heartlake {
namespace ai {

ModelQuantizer::QuantizedTensor ModelQuantizer::quantizeToInt8(
    const std::vector<float>& tensor,
    const std::vector<size_t>& shape) {

    totalQuantizedOps_.fetch_add(1);

    QuantizedTensor result;
    result.shape = shape;
    result.zeroPoint = 0;

    if (tensor.empty()) {
        result.scale = 1.0f;
        return result;
    }

    // 8路展开找 absMax
    float absMax = 0.0f;
    const size_t n = tensor.size();
    const float* __restrict__ p = tensor.data();
    size_t i = 0;
    float m0 = 0.0f, m1 = 0.0f;
    for (; i + 7 < n; i += 8) {
        m0 = std::max(m0, std::max(std::abs(p[i]),     std::max(std::abs(p[i+1]),
                         std::max(std::abs(p[i+2]),     std::abs(p[i+3])))));
        m1 = std::max(m1, std::max(std::abs(p[i+4]),   std::max(std::abs(p[i+5]),
                         std::max(std::abs(p[i+6]),     std::abs(p[i+7])))));
    }
    absMax = std::max(m0, m1);
    for (; i < n; ++i) {
        float av = std::abs(p[i]);
        if (av > absMax) absMax = av;
    }

    result.scale = (absMax < 1e-8f) ? 1e-8f : (absMax / 127.0f);
    float invScale = 1.0f / result.scale;

    result.data.resize(n);
    int8_t* __restrict__ out = result.data.data();
    i = 0;
    for (; i + 3 < n; i += 4) {
        out[i]     = static_cast<int8_t>(std::clamp(std::round(p[i]     * invScale), -128.0f, 127.0f));
        out[i + 1] = static_cast<int8_t>(std::clamp(std::round(p[i + 1] * invScale), -128.0f, 127.0f));
        out[i + 2] = static_cast<int8_t>(std::clamp(std::round(p[i + 2] * invScale), -128.0f, 127.0f));
        out[i + 3] = static_cast<int8_t>(std::clamp(std::round(p[i + 3] * invScale), -128.0f, 127.0f));
    }
    for (; i < n; ++i) {
        out[i] = static_cast<int8_t>(std::clamp(std::round(p[i] * invScale), -128.0f, 127.0f));
    }

    return result;
}

std::vector<float> ModelQuantizer::quantizedMatMul(
    const QuantizedTensor& a,
    const QuantizedTensor& b,
    size_t M, size_t K, size_t N) {

    totalQuantizedOps_.fetch_add(1);

    if (a.data.size() != M * K || b.data.size() != K * N) {
        LOG_ERROR << "[ModelQuantizer] quantizedMatMul dimension mismatch";
        return std::vector<float>(M * N, 0.0f);
    }

    float combinedScale = a.scale * b.scale;
    std::vector<float> result(M * N, 0.0f);

    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            int32_t acc = 0;
            const int8_t* aRow = a.data.data() + i * K;
            const int8_t* bCol = b.data.data() + j;

            size_t k = 0;
            for (; k + 3 < K; k += 4) {
                acc += static_cast<int32_t>(aRow[k])     * static_cast<int32_t>(bCol[k * N]);
                acc += static_cast<int32_t>(aRow[k + 1]) * static_cast<int32_t>(bCol[(k + 1) * N]);
                acc += static_cast<int32_t>(aRow[k + 2]) * static_cast<int32_t>(bCol[(k + 2) * N]);
                acc += static_cast<int32_t>(aRow[k + 3]) * static_cast<int32_t>(bCol[(k + 3) * N]);
            }
            for (; k < K; ++k) {
                acc += static_cast<int32_t>(aRow[k]) * static_cast<int32_t>(bCol[k * N]);
            }

            result[i * N + j] = combinedScale * static_cast<float>(acc);
        }
    }

    return result;
}

std::vector<float> ModelQuantizer::quantizedForward(
    const std::vector<float>& input,
    const QuantizedTensor& weights,
    const std::vector<float>& biases) {

    totalQuantizedOps_.fetch_add(1);

    if (weights.shape.size() < 2) {
        LOG_ERROR << "[ModelQuantizer] weights must be 2D (outDim x inDim)";
        return {};
    }

    size_t outDim = weights.shape[0];
    size_t inDim = weights.shape[1];

    if (input.size() != inDim) {
        LOG_ERROR << "[ModelQuantizer] input dim mismatch";
        return {};
    }

    if (!biases.empty() && biases.size() != outDim) {
        LOG_ERROR << "[ModelQuantizer] bias size mismatch";
        return {};
    }

    // 量化输入
    auto qInput = quantizeToInt8(input, {1, inDim});

    // output = input(1 x inDim) * weights^T(inDim x outDim)
    // weights 存储为 (outDim x inDim)，逐行点积
    float combinedScale = qInput.scale * weights.scale;
    std::vector<float> output(outDim, 0.0f);

    for (size_t o = 0; o < outDim; ++o) {
        int32_t acc = 0;
        const int8_t* wRow = weights.data.data() + o * inDim;

        size_t k = 0;
        for (; k + 3 < inDim; k += 4) {
            acc += static_cast<int32_t>(qInput.data[k])     * static_cast<int32_t>(wRow[k]);
            acc += static_cast<int32_t>(qInput.data[k + 1]) * static_cast<int32_t>(wRow[k + 1]);
            acc += static_cast<int32_t>(qInput.data[k + 2]) * static_cast<int32_t>(wRow[k + 2]);
            acc += static_cast<int32_t>(qInput.data[k + 3]) * static_cast<int32_t>(wRow[k + 3]);
        }
        for (; k < inDim; ++k) {
            acc += static_cast<int32_t>(qInput.data[k]) * static_cast<int32_t>(wRow[k]);
        }

        output[o] = combinedScale * static_cast<float>(acc);

        if (!biases.empty()) {
            output[o] += biases[o];
        }
    }

    // ReLU
    for (float& v : output) {
        if (v < 0.0f) v = 0.0f;
    }

    return output;
}

} // namespace ai
} // namespace heartlake
