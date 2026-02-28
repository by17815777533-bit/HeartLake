#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

echo "[tests] 开始执行 admin 测试"
./tests/clients/admin/run.sh

echo "[tests] 开始执行 frontend 测试"
./tests/clients/frontend/run.sh

echo "[tests] 开始执行 backend 测试"
./tests/clients/backend/run.sh

echo "[tests] 全部测试通过"
