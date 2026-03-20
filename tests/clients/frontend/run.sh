#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "${ROOT_DIR}/frontend"

FLUTTER_TEST_CONCURRENCY="${FLUTTER_TEST_CONCURRENCY:-2}"

echo "[frontend] 执行 flutter test (并发 ${FLUTTER_TEST_CONCURRENCY})"
flutter test --concurrency="${FLUTTER_TEST_CONCURRENCY}"
