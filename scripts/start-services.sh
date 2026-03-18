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
  server-lite)
    ./scripts/docker-up.sh server-lite
    ;;
  dev)
    ./scripts/docker-up.sh dev
    ;;
  stop)
    ./scripts/docker-down.sh
    ;;
  *)
    echo "用法: $0 [all|db|server-lite|stop]"
    echo "  all - 启动全部功能环境（默认启动全部服务）"
    echo "  db   - 仅启动 postgres + redis"
    echo "  server-lite - 2C2G Ubuntu 服务器精简环境（不含 Ollama）"
    echo "  stop - 停止服务"
    echo "  兼容别名: full/dev -> all"
    exit 1
    ;;
esac
