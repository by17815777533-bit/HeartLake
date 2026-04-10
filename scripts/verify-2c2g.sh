#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

export BACKEND_BUILD_DIR="${BACKEND_BUILD_DIR:-build-2c2g}"
export BACKEND_CMAKE_BUILD_TYPE="${BACKEND_CMAKE_BUILD_TYPE:-Release}"
export BACKEND_BUILD_JOBS="${BACKEND_BUILD_JOBS:-2}"
export ADMIN_BUILD_MAX_OLD_SPACE_MB="${ADMIN_BUILD_MAX_OLD_SPACE_MB:-512}"
export VERIFY_ONNX_SMOKE="${VERIFY_ONNX_SMOKE:-0}"

echo "[2c2g] Backend: ${BACKEND_CMAKE_BUILD_TYPE}, build jobs=${BACKEND_BUILD_JOBS}"
echo "[2c2g] Admin:   node memory=${ADMIN_BUILD_MAX_OLD_SPACE_MB}MB"
echo "[2c2g] Frontend: flutter analyze"
echo "[2c2g] ONNX smoke: ${VERIFY_ONNX_SMOKE}"

cmake -S "${ROOT_DIR}/backend" -B "${ROOT_DIR}/backend/${BACKEND_BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BACKEND_CMAKE_BUILD_TYPE}" >/dev/null
cmake --build "${ROOT_DIR}/backend/${BACKEND_BUILD_DIR}" -j"${BACKEND_BUILD_JOBS}"

(
  cd "${ROOT_DIR}/admin"
  export CI="${CI:-1}"
  export NODE_OPTIONS="${NODE_OPTIONS:-} --max-old-space-size=${ADMIN_BUILD_MAX_OLD_SPACE_MB}"
  npm run build
)

(
  cd "${ROOT_DIR}/frontend"
  flutter analyze
)

CTEST_LISTING="$(ctest --test-dir "${ROOT_DIR}/backend/${BACKEND_BUILD_DIR}" -N)"
printf '%s\n' "${CTEST_LISTING}"
CTEST_TOTAL_TESTS="$(printf '%s\n' "${CTEST_LISTING}" | sed -n 's/^Total Tests: //p' | tail -n1)"
if [[ -z "${CTEST_TOTAL_TESTS}" ]]; then
  echo "[2c2g] 无法解析 ctest 注册数量"
  exit 1
fi
if [[ "${CTEST_TOTAL_TESTS}" -gt 0 ]]; then
  echo "[2c2g] 发现 ${CTEST_TOTAL_TESTS} 个真实 ctest 目标，开始执行..."
  ctest --test-dir "${ROOT_DIR}/backend/${BACKEND_BUILD_DIR}" --output-on-failure
else
  echo "[2c2g] 当前未注册 backend CTest 目标，跳过执行"
fi

if [[ "${VERIFY_ONNX_SMOKE}" == "1" ]]; then
  ./scripts/verify-onnx-smoke.sh
fi
