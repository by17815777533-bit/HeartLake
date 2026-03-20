#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

export COMPOSE_PROJECT_NAME="${COMPOSE_PROJECT_NAME:-heartlake_runtime}"

MODE="${1:-all}"

server_lite_hints() {
  echo "[docker-up] 提示: server-lite 默认面向 2C2G，后台扫描任务建议保持关闭。"
  echo "[docker-up] 提示: server-lite 不会启动 ollama；若 AI_PROVIDER=ollama，请自行保证 AI_BASE_URL 可达。"
}

case "${MODE}" in
  all)
    echo "[docker-up] 启动全部服务（docker-compose.yml 中定义的所有服务）..."
    docker compose up -d --build
    ;;
  full)
    echo "[docker-up] 启动全功能栈兼容别名（等价于 all）..."
    docker compose up -d --build
    ;;
  dev)
    echo "[docker-up] 启动开发栈兼容别名（等价于 all）..."
    docker compose up -d --build
    ;;
  prod)
    echo "[docker-up] 启动交付栈兼容别名（等价于 all）..."
    docker compose up -d --build
    ;;
  lite)
    echo "[docker-up] 启动精简栈（不含 ollama）..."
    docker compose up -d --build postgres redis backend admin gateway
    ;;
  server-lite)
    echo "[docker-up] 启动 2C2G Ubuntu 服务器精简栈（低内存构建 + 本机绑定内部端口）..."
    server_lite_hints
    docker compose -f docker-compose.yml -f docker-compose.server-lite.yml up -d --build postgres redis backend admin gateway
    ;;
  server-lite-admin)
    echo "[docker-up] 2C2G Ubuntu 服务器增量重建 admin（避免顺带重建 backend）..."
    server_lite_hints
    docker compose -f docker-compose.yml -f docker-compose.server-lite.yml build admin
    docker compose -f docker-compose.yml -f docker-compose.server-lite.yml up -d --no-deps admin
    ;;
  server-lite-backend)
    echo "[docker-up] 2C2G Ubuntu 服务器增量重建 backend（避免顺带重建 admin）..."
    server_lite_hints
    docker compose -f docker-compose.yml -f docker-compose.server-lite.yml build backend
    docker compose -f docker-compose.yml -f docker-compose.server-lite.yml up -d --no-deps backend
    ;;
  server-lite-gateway)
    echo "[docker-up] 2C2G Ubuntu 服务器仅重启 gateway..."
    server_lite_hints
    docker compose -f docker-compose.yml -f docker-compose.server-lite.yml up -d --no-deps gateway
    ;;
  db)
    echo "[docker-up] 仅启动数据库与缓存..."
    docker compose up -d postgres redis
    ;;
  *)
    echo "用法: $0 [all|lite|server-lite|server-lite-admin|server-lite-backend|server-lite-gateway|db]"
    echo "兼容别名: full/dev/prod -> all"
    exit 1
    ;;
esac

echo "[docker-up] 完成，服务状态如下："
docker compose ps
