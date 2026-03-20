#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "${ROOT_DIR}/admin"

ADMIN_TEST_MAX_OLD_SPACE_MB="${ADMIN_TEST_MAX_OLD_SPACE_MB:-512}"
export CI="${CI:-1}"
export NODE_OPTIONS="${NODE_OPTIONS:-} --max-old-space-size=${ADMIN_TEST_MAX_OLD_SPACE_MB}"

echo "[admin] 执行 vitest (Node 内存上限 ${ADMIN_TEST_MAX_OLD_SPACE_MB}MB)"
npm run test
