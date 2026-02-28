#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

export COMPOSE_PROJECT_NAME="${COMPOSE_PROJECT_NAME:-heartlake_runtime}"

PURGE="${1:-}"

if [[ "${PURGE}" == "--purge" ]]; then
  echo "[docker-down] 停止并清理容器、网络、卷..."
  docker compose down -v --remove-orphans
else
  echo "[docker-down] 停止并清理容器、网络..."
  docker compose down --remove-orphans
fi

echo "[docker-down] 完成"
