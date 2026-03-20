#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

export BACKEND_BUILD_DIR="${BACKEND_BUILD_DIR:-build-2c2g}"
export BACKEND_CMAKE_BUILD_TYPE="${BACKEND_CMAKE_BUILD_TYPE:-Release}"
export BACKEND_BUILD_JOBS="${BACKEND_BUILD_JOBS:-2}"
export BACKEND_TEST_JOBS="${BACKEND_TEST_JOBS:-2}"
export ADMIN_TEST_MAX_OLD_SPACE_MB="${ADMIN_TEST_MAX_OLD_SPACE_MB:-512}"
export FLUTTER_TEST_CONCURRENCY="${FLUTTER_TEST_CONCURRENCY:-2}"
export VERIFY_ONNX_SMOKE="${VERIFY_ONNX_SMOKE:-0}"

echo "[2c2g] Backend: ${BACKEND_CMAKE_BUILD_TYPE}, build jobs=${BACKEND_BUILD_JOBS}, test jobs=${BACKEND_TEST_JOBS}"
echo "[2c2g] Admin:   node memory=${ADMIN_TEST_MAX_OLD_SPACE_MB}MB"
echo "[2c2g] Frontend:flutter concurrency=${FLUTTER_TEST_CONCURRENCY}"
echo "[2c2g] ONNX smoke: ${VERIFY_ONNX_SMOKE}"

./tests/clients/run_all.sh

if [[ "${VERIFY_ONNX_SMOKE}" == "1" ]]; then
  ./scripts/verify-onnx-smoke.sh
fi
