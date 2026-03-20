#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"
mkdir -p "${ROOT_DIR}/.runtime"

BOOTSTRAP_LIB="${ROOT_DIR}/scripts/lib/env-bootstrap.sh"
if [[ ! -f "${BOOTSTRAP_LIB}" ]]; then
  echo "[docker-up] FATAL: bootstrap helper missing: ${BOOTSTRAP_LIB}"
  exit 1
fi
source "${BOOTSTRAP_LIB}"

ENV_FILE="${HEARTLAKE_ENV_PATH:-${ROOT_DIR}/.env}"
if [[ ! -f "${ENV_FILE}" ]]; then
  cp "${ROOT_DIR}/.env.example" "${ENV_FILE}"
  echo "[docker-up] Created ${ENV_FILE} from .env.example"
fi

if bootstrap_msg="$(heartlake_bootstrap_env "${ENV_FILE}" "${ROOT_DIR}/.runtime/compose-bootstrap-secrets.txt" "scripts/docker-up.sh")"; then
  if [[ -n "${bootstrap_msg}" ]]; then
    echo "[docker-up] ${bootstrap_msg}"
  fi
else
  exit 1
fi

if ! command -v docker >/dev/null 2>&1; then
  echo "[docker-up] FATAL: docker is required but was not found in PATH"
  echo "[docker-up] 提示: 仅调试本地 backend 可先执行 ./backend/start.sh"
  exit 127
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "[docker-up] FATAL: docker compose plugin is required"
  exit 1
fi

export COMPOSE_PROJECT_NAME="${COMPOSE_PROJECT_NAME:-heartlake_runtime}"
BASE_COMPOSE=(docker compose --env-file "${ENV_FILE}")
SERVER_LITE_COMPOSE=("${BASE_COMPOSE[@]}" -f docker-compose.yml -f docker-compose.server-lite.yml)

MODE="${1:-all}"
STATUS_COMPOSE=("${BASE_COMPOSE[@]}")

server_lite_hints() {
  echo "[docker-up] 提示: server-lite 默认面向 2C2G，后台扫描任务建议保持关闭。"
  echo "[docker-up] 提示: server-lite 不会启动 ollama；若 AI_PROVIDER=ollama，请自行保证 AI_BASE_URL 可达。"
}

case "${MODE}" in
  all)
    echo "[docker-up] 启动全部服务（docker-compose.yml 中定义的所有服务）..."
    "${BASE_COMPOSE[@]}" up -d --build
    ;;
  full)
    echo "[docker-up] 启动全功能栈兼容别名（等价于 all）..."
    "${BASE_COMPOSE[@]}" up -d --build
    ;;
  dev)
    echo "[docker-up] 启动开发栈兼容别名（等价于 all）..."
    "${BASE_COMPOSE[@]}" up -d --build
    ;;
  prod)
    echo "[docker-up] 启动交付栈兼容别名（等价于 all）..."
    "${BASE_COMPOSE[@]}" up -d --build
    ;;
  lite)
    echo "[docker-up] 启动精简栈（不含 ollama）..."
    "${BASE_COMPOSE[@]}" up -d --build postgres redis backend admin gateway
    ;;
  server-lite)
    echo "[docker-up] 启动 2C2G Ubuntu 服务器精简栈（低内存构建 + 本机绑定内部端口）..."
    server_lite_hints
    STATUS_COMPOSE=("${SERVER_LITE_COMPOSE[@]}")
    "${SERVER_LITE_COMPOSE[@]}" up -d --build postgres redis backend admin gateway
    ;;
  server-lite-admin)
    echo "[docker-up] 2C2G Ubuntu 服务器增量重建 admin（避免顺带重建 backend）..."
    server_lite_hints
    STATUS_COMPOSE=("${SERVER_LITE_COMPOSE[@]}")
    "${SERVER_LITE_COMPOSE[@]}" build admin
    "${SERVER_LITE_COMPOSE[@]}" up -d --no-deps admin
    ;;
  server-lite-backend)
    echo "[docker-up] 2C2G Ubuntu 服务器增量重建 backend（避免顺带重建 admin）..."
    server_lite_hints
    STATUS_COMPOSE=("${SERVER_LITE_COMPOSE[@]}")
    "${SERVER_LITE_COMPOSE[@]}" build backend
    "${SERVER_LITE_COMPOSE[@]}" up -d --no-deps backend
    ;;
  server-lite-gateway)
    echo "[docker-up] 2C2G Ubuntu 服务器仅重启 gateway..."
    server_lite_hints
    STATUS_COMPOSE=("${SERVER_LITE_COMPOSE[@]}")
    "${SERVER_LITE_COMPOSE[@]}" up -d --no-deps gateway
    ;;
  db)
    echo "[docker-up] 仅启动数据库与缓存..."
    "${BASE_COMPOSE[@]}" up -d postgres redis
    ;;
  *)
    echo "用法: $0 [all|lite|server-lite|server-lite-admin|server-lite-backend|server-lite-gateway|db]"
    echo "兼容别名: full/dev/prod -> all"
    exit 1
    ;;
esac

echo "[docker-up] 完成，服务状态如下："
"${STATUS_COMPOSE[@]}" ps
