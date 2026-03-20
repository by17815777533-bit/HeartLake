#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

export COMPOSE_PROJECT_NAME="${COMPOSE_PROJECT_NAME:-heartlake_runtime}"

GATEWAY_PORT="${GATEWAY_PORT:-3000}"
API_BASE="http://127.0.0.1:${GATEWAY_PORT}"
RUN_EXISTING_TESTS="${RUN_EXISTING_TESTS:-1}"
TEST_RUNNER="${TEST_RUNNER:-./scripts/verify-2c2g.sh}"

extract_json_string_field() {
  local json="$1"
  local field="$2"
  printf '%s' "${json}" | sed -n "s/.*\"${field}\"[[:space:]]*:[[:space:]]*\"\\([^\"]*\\)\".*/\\1/p" | head -n1
}

assert_code_zero() {
  local json="$1"
  if ! printf '%s' "${json}" | grep -Eq '"code"[[:space:]]*:[[:space:]]*0'; then
    echo "[docker-test] 接口返回非成功业务码: ${json}"
    return 1
  fi
}

authed_json() {
  local method="$1"
  local url="$2"
  local data="${3:-}"
  if [[ -n "${data}" ]]; then
    curl -fsS -X "${method}" "${url}" \
      -H "Content-Type: application/json" \
      -H "Authorization: Bearer ${TOKEN}" \
      -H "X-User-Id: ${USER_ID}" \
      -d "${data}"
  else
    curl -fsS -X "${method}" "${url}" \
      -H "Authorization: Bearer ${TOKEN}" \
      -H "X-User-Id: ${USER_ID}"
  fi
}

echo "[docker-test] 检查容器状态..."
docker compose ps

echo "[docker-test] 等待后端健康检查通过..."
for i in $(seq 1 60); do
  if curl -fsS "${API_BASE}/api/health" >/dev/null 2>&1; then
    echo "[docker-test] 后端健康检查通过"
    break
  fi
  if [[ "${i}" -eq 60 ]]; then
    echo "[docker-test] 后端健康检查超时"
    exit 1
  fi
  sleep 2
done

echo "[docker-test] 验证网关健康接口..."
curl -fsS "${API_BASE}/healthz" | grep -q "ok"

echo "[docker-test] 等待管理后台可用..."
for i in $(seq 1 60); do
  if curl -fsS "${API_BASE}/admin/" >/dev/null 2>&1; then
    echo "[docker-test] 管理后台已可用"
    break
  fi
  if [[ "${i}" -eq 60 ]]; then
    echo "[docker-test] 管理后台启动超时"
    exit 1
  fi
  sleep 2
done

echo "[docker-test] 验证管理后台首页..."
curl -fsS "${API_BASE}/admin/" >/dev/null

if command -v node >/dev/null 2>&1; then
  echo "[docker-test] 验证客户端 HTTP + WebSocket 联调链路..."
  node ./scripts/smoke-client-connect.mjs "${API_BASE}"
fi

if [[ "${RUN_EXISTING_TESTS}" == "1" ]]; then
  echo "[docker-test] 执行测试集合（${TEST_RUNNER}）..."
  "${TEST_RUNNER}"
fi

echo "[docker-test] 验证匿名登录接口..."
DEVICE_ID="docker_smoke_$(date +%s)"
AUTH_JSON="$(curl -fsS -X POST "${API_BASE}/api/auth/anonymous" \
  -H "Content-Type: application/json" \
  -d "{\"device_id\":\"${DEVICE_ID}\"}")"
echo "${AUTH_JSON}" | grep -Eq '"success"\s*:\s*true|"code"\s*:\s*0'
TOKEN="$(extract_json_string_field "${AUTH_JSON}" "token")"
USER_ID="$(extract_json_string_field "${AUTH_JSON}" "user_id")"
if [[ -z "${TOKEN}" || -z "${USER_ID}" ]]; then
  echo "[docker-test] 匿名登录返回缺少 token/user_id"
  exit 1
fi

echo "[docker-test] 验证账号隐私设置读取..."
PRIVACY_GET_JSON="$(authed_json GET "${API_BASE}/api/account/privacy")"
assert_code_zero "${PRIVACY_GET_JSON}"

echo "[docker-test] 验证账号隐私设置更新..."
PRIVACY_PUT_JSON="$(authed_json PUT "${API_BASE}/api/account/privacy" '{"profile_visibility":"friends","show_online_status":true,"allow_friend_request":true,"allow_message_from_stranger":false}')"
assert_code_zero "${PRIVACY_PUT_JSON}"

echo "[docker-test] 验证湖面石头列表..."
STONES_JSON="$(authed_json GET "${API_BASE}/api/lake/stones?page=1&page_size=5&sort=latest")"
assert_code_zero "${STONES_JSON}"

echo "[docker-test] 验证投石主流程..."
STONE_CREATE_JSON="$(authed_json POST "${API_BASE}/api/stones" '{"content":"[docker-smoke] 愿今天的你被温柔接住。","stone_type":"medium","stone_color":"#7A92A3","mood_type":"sad","is_anonymous":true}')"
assert_code_zero "${STONE_CREATE_JSON}"
STONE_ID="$(extract_json_string_field "${STONE_CREATE_JSON}" "stone_id")"
if [[ -z "${STONE_ID}" ]]; then
  echo "[docker-test] 创建石头后未返回 stone_id"
  exit 1
fi

echo "[docker-test] 验证涟漪互动..."
RIPPLE_JSON="$(authed_json POST "${API_BASE}/api/stones/${STONE_ID}/ripples" '{}')"
assert_code_zero "${RIPPLE_JSON}"

echo "[docker-test] 验证我的涟漪列表..."
MY_RIPPLES_JSON="$(authed_json GET "${API_BASE}/api/interactions/my/ripples?page=1&page_size=5")"
assert_code_zero "${MY_RIPPLES_JSON}"

echo "[docker-test] 验证情绪日历与热力图..."
EMOTION_CALENDAR_JSON="$(authed_json GET "${API_BASE}/api/users/my/emotion-calendar?year=2026&month=2")"
assert_code_zero "${EMOTION_CALENDAR_JSON}"
EMOTION_HEATMAP_JSON="$(authed_json GET "${API_BASE}/api/users/my/emotion-heatmap")"
assert_code_zero "${EMOTION_HEATMAP_JSON}"

echo "[docker-test] 验证好友列表接口..."
FRIENDS_JSON="$(authed_json GET "${API_BASE}/api/friends")"
assert_code_zero "${FRIENDS_JSON}"

echo "[docker-test] 验证推荐接口..."
TRENDING_JSON="$(authed_json GET "${API_BASE}/api/recommendations/trending?limit=3")"
assert_code_zero "${TRENDING_JSON}"

echo "[docker-test] 所有 Docker Smoke Test 通过"
