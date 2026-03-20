#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "${ROOT_DIR}/backend"

cpu_jobs() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
    return
  fi
  if command -v sysctl >/dev/null 2>&1; then
    sysctl -n hw.ncpu 2>/dev/null || true
    return
  fi
  echo 4
}

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

cap_jobs() {
  local raw="${1:-2}"
  if [[ ! "${raw}" =~ ^[0-9]+$ ]] || (( raw < 1 )); then
    echo 2
    return
  fi
  if (( raw > 2 )); then
    echo 2
    return
  fi
  echo "${raw}"
}

DEFAULT_JOBS="$(cap_jobs "$(cpu_jobs)")"
BUILD_DIR="${BACKEND_BUILD_DIR:-build-2c2g}"
BUILD_TYPE="${BACKEND_CMAKE_BUILD_TYPE:-${CMAKE_BUILD_TYPE:-Release}}"
BUILD_JOBS="${BACKEND_BUILD_JOBS:-${BUILD_JOBS:-${DEFAULT_JOBS}}}"
TEST_JOBS="${BACKEND_TEST_JOBS:-${TEST_JOBS:-${DEFAULT_JOBS}}}"
USE_ONNX="${HEARTLAKE_USE_ONNX:-OFF}"
ONNX_ROOT="${BACKEND_ONNX_ROOT:-${OnnxRuntime_ROOT:-${ONNXRUNTIME_ROOT:-}}}"
CXX_BIN="$(resolve_compiler "${BACKEND_CXX:-${CXX:-}}" g++)"
GTEST_DIR="${BACKEND_GTEST_DIR:-}"

if [[ -z "${GTEST_DIR}" && -f "/usr/lib64/cmake/GTest/GTestConfig.cmake" ]]; then
  GTEST_DIR="/usr/lib64/cmake/GTest"
fi

if [[ -f "${BUILD_DIR}/CMakeCache.txt" && -n "${CXX_BIN}" ]]; then
  CURRENT_COMPILER="$(sed -n 's#^CMAKE_CXX_COMPILER:FILEPATH=##p' "${BUILD_DIR}/CMakeCache.txt" | head -n 1)"
  if [[ -n "${CURRENT_COMPILER}" && "${CURRENT_COMPILER}" != "${CXX_BIN}" ]]; then
    echo "[backend] 检测到编译器切换 (${CURRENT_COMPILER} -> ${CXX_BIN})，重建 ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
  fi
fi

echo "[backend] 配置 ${BUILD_TYPE} 构建目录 ${BUILD_DIR}"
echo "[backend] 使用编译器 CXX=${CXX_BIN:-auto}"
if [[ -n "${GTEST_DIR}" ]]; then
  echo "[backend] 使用系统 GTest: ${GTEST_DIR}"
fi
cmake -S . -B "${BUILD_DIR}" \
  ${CXX_BIN:+-DCMAKE_CXX_COMPILER="${CXX_BIN}"} \
  ${GTEST_DIR:+-DGTest_DIR="${GTEST_DIR}"} \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  ${ONNX_ROOT:+-DOnnxRuntime_ROOT="${ONNX_ROOT}"} \
  -DHEARTLAKE_USE_ONNX="${USE_ONNX}" >/dev/null

if ! find "${BUILD_DIR}" -maxdepth 1 -type f -name "test_*" -executable | grep -q .; then
  echo "[backend] 未发现测试二进制，开始构建"
else
  echo "[backend] 增量构建测试目标"
fi
cmake --build "${BUILD_DIR}" -j"${BUILD_JOBS}"

echo "[backend] 执行 ctest"
ctest --test-dir "${BUILD_DIR}" --output-on-failure -j"${TEST_JOBS}"
