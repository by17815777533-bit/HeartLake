#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

PROJECT_NAME="${COMPOSE_PROJECT_NAME:-heartlake_runtime}"
REMOTE_HOST="${2:-heartlake-server}"
REMOTE_APP_DIR="${REMOTE_APP_DIR:-/root/HeartLake}"
MODE="${1:-admin}"
SHORT_SHA="$(git rev-parse --short HEAD 2>/dev/null || date +%s)"
TMP_DIR="$(mktemp -d)"
REMOTE_TMP_DIR="/tmp/${PROJECT_NAME}-deploy-${SHORT_SHA}-$$"

cleanup() {
  rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "缺少命令: $1" >&2
    exit 1
  }
}

need_cmd docker
need_cmd ssh
need_cmd rsync
need_cmd gzip

build_and_pack() {
  local service="$1"
  local context="$2"
  local dockerfile="$3"
  shift 3

  local image_latest="${PROJECT_NAME}-${service}:latest"
  local image_sha="${PROJECT_NAME}-${service}:${SHORT_SHA}"
  local archive_path="${TMP_DIR}/${PROJECT_NAME}-${service}-${SHORT_SHA}.tar.gz"

  echo "[deploy-local] 本地构建 ${service} 镜像..."
  docker build -f "${dockerfile}" -t "${image_latest}" -t "${image_sha}" "$@" "${context}"

  echo "[deploy-local] 打包 ${service} 镜像..."
  docker save "${image_latest}" "${image_sha}" | gzip -1 > "${archive_path}"
}

case "${MODE}" in
  admin)
    SERVICES=(admin)
    build_and_pack admin ./admin ./admin/Dockerfile
    ;;
  backend)
    SERVICES=(backend)
    build_and_pack backend ./backend ./backend/Dockerfile \
      --build-arg "BUILD_JOBS=${BACKEND_BUILD_JOBS:-2}"
    ;;
  all)
    SERVICES=(backend admin)
    build_and_pack backend ./backend ./backend/Dockerfile \
      --build-arg "BUILD_JOBS=${BACKEND_BUILD_JOBS:-2}"
    build_and_pack admin ./admin ./admin/Dockerfile
    ;;
  *)
    echo "用法: $0 [admin|backend|all] [remote-host]" >&2
    exit 1
    ;;
esac

LOCAL_ARCH="$(uname -m)"
REMOTE_ARCH="$(ssh -o ConnectTimeout=8 -o ConnectionAttempts=1 "${REMOTE_HOST}" 'uname -m')"

if [[ "${LOCAL_ARCH}" != "${REMOTE_ARCH}" ]]; then
  echo "本地架构 ${LOCAL_ARCH} 与远端架构 ${REMOTE_ARCH} 不一致，停止部署。" >&2
  exit 1
fi

echo "[deploy-local] 上传镜像包到 ${REMOTE_HOST}:${REMOTE_TMP_DIR} ..."
ssh "${REMOTE_HOST}" "mkdir -p '${REMOTE_TMP_DIR}'"
rsync -az "${TMP_DIR}/" "${REMOTE_HOST}:${REMOTE_TMP_DIR}/"

echo "[deploy-local] 远端加载镜像并增量更新服务..."
ssh "${REMOTE_HOST}" "
  set -euo pipefail
  cd '${REMOTE_APP_DIR}'
  for archive in '${REMOTE_TMP_DIR}'/*.tar.gz; do
    gunzip -c \"\${archive}\" | docker load >/dev/null
  done
  COMPOSE_PROJECT_NAME='${PROJECT_NAME}' docker compose -f docker-compose.yml -f docker-compose.server-lite.yml up -d --no-deps ${SERVICES[*]}
  docker image prune -f >/dev/null 2>&1 || true
  rm -rf '${REMOTE_TMP_DIR}'
  COMPOSE_PROJECT_NAME='${PROJECT_NAME}' docker compose -f docker-compose.yml -f docker-compose.server-lite.yml ps ${SERVICES[*]}
"

echo "[deploy-local] 远端健康检查..."
if [[ " ${SERVICES[*]} " == *" backend "* ]]; then
  ssh "${REMOTE_HOST}" "curl -fsS http://127.0.0.1/api/health"
  printf '\n'
fi

if [[ " ${SERVICES[*]} " == *" admin "* ]]; then
  ssh "${REMOTE_HOST}" "curl -I -fsS http://127.0.0.1/admin/" >/dev/null
  echo "[deploy-local] admin 已返回 200"
fi

echo "[deploy-local] 完成: ${SERVICES[*]} @ ${SHORT_SHA}"
