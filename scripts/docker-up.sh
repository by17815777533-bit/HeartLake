#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

export COMPOSE_PROJECT_NAME="${COMPOSE_PROJECT_NAME:-heartlake_runtime}"

MODE="${1:-all}"

case "${MODE}" in
  all)
    echo "[docker-up] 启动全部服务（docker-compose.yml 中定义的所有服务）..."
    docker compose up -d --build
    ;;
  full)
    echo "[docker-up] 启动全功能栈（等价于 all）..."
    docker compose up -d --build
    ;;
  dev)
    echo "[docker-up] 启动开发栈（与 all/full 等价，保留兼容参数）..."
    docker compose up -d --build
    ;;
  prod)
    echo "[docker-up] 启动交付栈（与 all/full 等价）..."
    docker compose up -d --build
    ;;
  lite)
    echo "[docker-up] 启动精简栈（不含 ollama）..."
    docker compose up -d --build postgres redis backend admin gateway
    ;;
  db)
    echo "[docker-up] 仅启动数据库与缓存..."
    docker compose up -d postgres redis
    ;;
  *)
    echo "用法: $0 [all|full|dev|prod|lite|db]"
    exit 1
    ;;
esac

echo "[docker-up] 完成，服务状态如下："
docker compose ps
