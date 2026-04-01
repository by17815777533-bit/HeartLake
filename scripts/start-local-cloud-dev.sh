#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PUBLIC_ORIGIN="${PUBLIC_ORIGIN:-http://121.41.195.165}"
ADMIN_HOST="${ADMIN_HOST:-0.0.0.0}"
ADMIN_PORT="${ADMIN_PORT:-5173}"
FRONTEND_HOST="${FRONTEND_HOST:-0.0.0.0}"
FRONTEND_PORT="${FRONTEND_PORT:-3001}"

to_ws_origin() {
  local origin="$1"
  origin="${origin%/}"
  origin="${origin/#http:\/\//ws://}"
  origin="${origin/#https:\/\//wss://}"
  printf '%s' "$origin"
}

WS_ORIGIN="${WS_ORIGIN:-$(to_ws_origin "$PUBLIC_ORIGIN")}"

for bin in npm flutter; do
  if ! command -v "$bin" >/dev/null 2>&1; then
    echo "[local-cloud-dev] 缺少依赖: $bin" >&2
    exit 1
  fi
done

echo "[local-cloud-dev] admin  -> http://127.0.0.1:${ADMIN_PORT}/admin/"
echo "[local-cloud-dev] flutter -> http://127.0.0.1:${FRONTEND_PORT}"
echo "[local-cloud-dev] cloud api -> ${PUBLIC_ORIGIN}/api"
echo "[local-cloud-dev] cloud ws  -> ${WS_ORIGIN}/ws/broadcast"

(
  cd "${ROOT_DIR}/admin"
  VITE_DEV_API_ORIGIN="${PUBLIC_ORIGIN}" \
  VITE_DEV_WS_ORIGIN="${WS_ORIGIN}" \
  npm run dev -- --host "${ADMIN_HOST}" --port "${ADMIN_PORT}"
) &
ADMIN_PID=$!

(
  cd "${ROOT_DIR}/frontend"
  flutter run -d web-server \
    --web-hostname "${FRONTEND_HOST}" \
    --web-port "${FRONTEND_PORT}" \
    --dart-define=PUBLIC_ORIGIN="${PUBLIC_ORIGIN}"
) &
FRONTEND_PID=$!

cleanup() {
  kill "${ADMIN_PID}" "${FRONTEND_PID}" 2>/dev/null || true
}

trap cleanup EXIT INT TERM
wait
