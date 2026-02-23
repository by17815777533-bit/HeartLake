#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
RUNTIME_DIR="${ROOT_DIR}/.runtime"

stop_pid_file() {
    local pid_file="$1"
    if [[ -f "${pid_file}" ]]; then
        local pid
        pid="$(cat "${pid_file}" || true)"
        if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
            kill "${pid}" || true
        fi
        rm -f "${pid_file}"
    fi
}

# 停 backend
pkill -f '/build/HeartLake' >/dev/null 2>&1 || true
stop_pid_file "${RUNTIME_DIR}/backend.pid"

# 停 ollama（仅停止脚本拉起的实例）
stop_pid_file "${RUNTIME_DIR}/ollama.pid"

# 停本地 runtime postgres
if [[ -d "${RUNTIME_DIR}/postgres" ]]; then
    pg_ctl -D "${RUNTIME_DIR}/postgres" stop >/dev/null 2>&1 || true
fi

# 停本地 runtime redis（只关当前端口）
if command -v redis-cli >/dev/null 2>&1; then
    redis-cli -p "${REDIS_PORT:-6379}" -a "${REDIS_PASSWORD:-}" shutdown >/dev/null 2>&1 || true
fi

echo "HeartLake local runtime stopped."
