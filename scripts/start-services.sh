#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

MODE="${1:-all}"

case "${MODE}" in
  all)
    ./scripts/docker-up.sh all
    ;;
  full)
    ./scripts/docker-up.sh full
    ;;
  db)
    ./scripts/docker-up.sh db
    ;;
  dev)
    ./scripts/docker-up.sh dev
    ;;
  stop)
    ./scripts/docker-down.sh
    ;;
  *)
    echo "用法: $0 [all|full|db|dev|stop]"
    echo "  all/full - 启动全部功能环境（默认启动全部服务）"
    echo "  db   - 仅启动 postgres + redis"
    echo "  dev  - 启动开发环境（与 full 等价）"
    echo "  stop - 停止服务"
    exit 1
    ;;
esac
