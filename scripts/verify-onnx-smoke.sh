#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKEND_DIR="${ROOT_DIR}/backend"

resolve_compiler() {
  local preferred="${1:-}"
  local fallback="${2:-}"
  local resolved=""

  if [[ -n "${preferred}" ]]; then
    resolved="$(command -v "${preferred}" 2>/dev/null || printf '%s' "${preferred}")"
    if [[ "${resolved}" == *openharmony* ]]; then
      resolved=""
    fi
  fi

  if [[ -z "${resolved}" && -n "${fallback}" ]]; then
    resolved="$(command -v "${fallback}" 2>/dev/null || true)"
  fi

  printf '%s' "${resolved}"
}

BUILD_DIR="${BACKEND_ONNX_BUILD_DIR:-build-onnx-2c2g}"
BUILD_JOBS="${BACKEND_ONNX_BUILD_JOBS:-2}"
MODEL_ROOT="${EDGE_AI_MODEL_PATH:-${BACKEND_DIR}/models}"
VOCAB_PATH="${EDGE_AI_VOCAB_PATH:-${BACKEND_DIR}/models/vocab.txt}"
ONNX_ROOT="${BACKEND_ONNX_ROOT:-${OnnxRuntime_ROOT:-${ONNXRUNTIME_ROOT:-${BACKEND_DIR}/third_party/onnxruntime-linux-x64-1.22.0}}}"
CXX_BIN="$(resolve_compiler "${BACKEND_CXX:-${CXX:-}}" g++)"
GTEST_DIR="${BACKEND_GTEST_DIR:-}"

if [[ -z "${GTEST_DIR}" && -f "/usr/lib64/cmake/GTest/GTestConfig.cmake" ]]; then
  GTEST_DIR="/usr/lib64/cmake/GTest"
fi

if [[ ! -f "${MODEL_ROOT}/sentiment_zh.onnx" ]]; then
  echo "[onnx-smoke] skip: missing ${MODEL_ROOT}/sentiment_zh.onnx"
  exit 0
fi

if [[ ! -f "${VOCAB_PATH}" ]]; then
  echo "[onnx-smoke] skip: missing ${VOCAB_PATH}"
  exit 0
fi

if [[ ! -d "${ONNX_ROOT}/lib" ]]; then
  echo "[onnx-smoke] skip: missing ONNX runtime lib dir ${ONNX_ROOT}/lib"
  exit 0
fi

echo "[onnx-smoke] configure ${BUILD_DIR} with OnnxRuntime_ROOT=${ONNX_ROOT}"
cmake -S "${BACKEND_DIR}" -B "${BACKEND_DIR}/${BUILD_DIR}" \
  ${CXX_BIN:+-DCMAKE_CXX_COMPILER="${CXX_BIN}"} \
  ${GTEST_DIR:+-DGTest_DIR="${GTEST_DIR}"} \
  -DCMAKE_BUILD_TYPE=Release \
  -DHEARTLAKE_USE_ONNX=ON \
  -DOnnxRuntime_ROOT="${ONNX_ROOT}" >/dev/null

echo "[onnx-smoke] build benchmark_edge_ai (-j${BUILD_JOBS})"
cmake --build "${BACKEND_DIR}/${BUILD_DIR}" -j"${BUILD_JOBS}" --target benchmark_edge_ai >/dev/null

export EDGE_AI_ONNX_ENABLED=true
export EDGE_AI_ONNX_THREADS="${EDGE_AI_ONNX_THREADS:-1}"
export EDGE_AI_ONNX_SESSION_POOL="${EDGE_AI_ONNX_SESSION_POOL:-1}"
export EDGE_AI_MODEL_PATH="${MODEL_ROOT}"
export EDGE_AI_VOCAB_PATH="${VOCAB_PATH}"
export LD_LIBRARY_PATH="${ONNX_ROOT}/lib:${LD_LIBRARY_PATH:-}"

echo "[onnx-smoke] run SentimentAccuracyBenchmark"
"${BACKEND_DIR}/${BUILD_DIR}/benchmark_edge_ai" \
  --gtest_filter=EdgeAIBenchmark.SentimentAccuracyBenchmark
