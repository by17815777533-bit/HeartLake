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
  case "${1:-all}" in
    server-lite-local|server-lite-admin-local|server-lite-backend-local)
      :
      ;;
    *)
      echo "[docker-up] FATAL: docker is required but was not found in PATH"
      echo "[docker-up] 提示: 仅调试本地 backend 可先执行 ./backend/start.sh"
      exit 127
      ;;
  esac
fi

if command -v docker >/dev/null 2>&1; then
  if ! docker compose version >/dev/null 2>&1; then
    case "${1:-all}" in
      server-lite-local|server-lite-admin-local|server-lite-backend-local)
        :
        ;;
      *)
        echo "[docker-up] FATAL: docker compose plugin is required"
        exit 1
        ;;
    esac
  fi
fi

export COMPOSE_PROJECT_NAME="${COMPOSE_PROJECT_NAME:-heartlake_runtime}"
BASE_COMPOSE=(docker compose --env-file "${ENV_FILE}")
SERVER_LITE_COMPOSE=("${BASE_COMPOSE[@]}" -f docker-compose.yml -f docker-compose.server-lite.yml)

MODE="${1:-all}"
REMOTE_HOST="${2:-${HEARTLAKE_REMOTE_HOST:-heartlake-server}}"
STATUS_COMPOSE=("${BASE_COMPOSE[@]}")

server_lite_hints() {
  echo "[docker-up] 提示: server-lite 默认面向 2C2G，后台扫描任务建议保持关闭。"
  echo "[docker-up] 提示: server-lite 不会启动 ollama；若 AI_PROVIDER=ollama，请自行保证 AI_BASE_URL 可达。"
}

server_lite_admin_build_guard() {
  local blocked_mode="$1"
  echo "[docker-up] 为避免 2C2G 云机在 admin 构建时失联，已默认禁用 ${blocked_mode} 的远端现编。"
  echo "[docker-up] 推荐命令:"
  echo "[docker-up]   ./scripts/docker-up.sh server-lite-admin-local ${REMOTE_HOST}"
  echo "[docker-up]   ./scripts/docker-up.sh server-lite-local ${REMOTE_HOST}"
  echo "[docker-up] 或直接使用:"
  echo "[docker-up]   ./scripts/deploy-server-lite-local.sh admin ${REMOTE_HOST}"
  echo "[docker-up] 如确认要冒险在服务器上现编 admin，请显式设置 HEARTLAKE_ALLOW_REMOTE_ADMIN_BUILD=1。"
  exit 2
}

run_server_lite_local_deploy() {
  local deploy_mode="$1"
  echo "[docker-up] 使用本地构建并推送到 ${REMOTE_HOST} ..."
  bash "${ROOT_DIR}/scripts/deploy-server-lite-local.sh" "${deploy_mode}" "${REMOTE_HOST}"
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
    if [[ "${HEARTLAKE_ALLOW_REMOTE_ADMIN_BUILD:-0}" != "1" ]]; then
      server_lite_admin_build_guard "server-lite"
    fi
    echo "[docker-up] 启动 2C2G Ubuntu 服务器精简栈（低内存构建 + 本机绑定内部端口）..."
    server_lite_hints
    STATUS_COMPOSE=("${SERVER_LITE_COMPOSE[@]}")
    "${SERVER_LITE_COMPOSE[@]}" up -d --build postgres redis backend admin gateway
    ;;
  server-lite-local)
    run_server_lite_local_deploy all
    ;;
  server-lite-admin)
    if [[ "${HEARTLAKE_ALLOW_REMOTE_ADMIN_BUILD:-0}" != "1" ]]; then
      server_lite_admin_build_guard "server-lite-admin"
    fi
    echo "[docker-up] 2C2G Ubuntu 服务器增量重建 admin（避免顺带重建 backend）..."
    server_lite_hints
    STATUS_COMPOSE=("${SERVER_LITE_COMPOSE[@]}")
    "${SERVER_LITE_COMPOSE[@]}" build admin
    "${SERVER_LITE_COMPOSE[@]}" up -d --no-deps admin
    ;;
  server-lite-admin-local)
    run_server_lite_local_deploy admin
    ;;
  server-lite-backend)
    echo "[docker-up] 2C2G Ubuntu 服务器增量重建 backend（避免顺带重建 admin）..."
    server_lite_hints
    STATUS_COMPOSE=("${SERVER_LITE_COMPOSE[@]}")
    "${SERVER_LITE_COMPOSE[@]}" build backend
    "${SERVER_LITE_COMPOSE[@]}" up -d --no-deps backend
    ;;
  server-lite-backend-local)
    run_server_lite_local_deploy backend
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
    echo "用法: $0 [all|lite|server-lite|server-lite-local|server-lite-admin|server-lite-admin-local|server-lite-backend|server-lite-backend-local|server-lite-gateway|db] [remote-host]"
    echo "兼容别名: full/dev/prod -> all"
    exit 1
    ;;
esac

echo "[docker-up] 完成，服务状态如下："
"${STATUS_COMPOSE[@]}" ps
