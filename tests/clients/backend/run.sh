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

if [[ ! -f build/CTestTestfile.cmake ]]; then
  echo "[backend] 初始化 CMake 构建目录"
  cmake -S . -B build
fi

if ! find build -maxdepth 1 -type f -name "test_*" -executable | grep -q .; then
  echo "[backend] 未发现测试二进制，开始构建"
  cmake --build build -j"$(cpu_jobs)"
fi

echo "[backend] 执行 ctest"
ctest --test-dir build --output-on-failure
