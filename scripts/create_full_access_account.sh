#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKEND_DIR="${ROOT_DIR}/backend"
ENV_FILE="${HEARTLAKE_ENV_PATH:-${BACKEND_DIR}/.env}"

if [[ ! -f "${ENV_FILE}" ]]; then
  echo "FATAL: env file not found: ${ENV_FILE}"
  exit 1
fi

set -a
source "${ENV_FILE}"
set +a

: "${DB_HOST:?DB_HOST is required}"
: "${DB_PORT:?DB_PORT is required}"
: "${DB_NAME:?DB_NAME is required}"
: "${DB_USER:?DB_USER is required}"
: "${DB_PASSWORD:?DB_PASSWORD is required}"

ACCOUNT_ID="${1:-heartlake_full_access}"
ACCOUNT_USERNAME="${2:-heartlake_guardian}"
ACCOUNT_NICKNAME="${3:-灯火守护员}"
ACCOUNT_KEYWORD="${4:-峰峦-霜降-海棠-书香-赤金-月光-绿水-岛屿}"
export ACCOUNT_KEYWORD

if ! command -v psql >/dev/null 2>&1; then
  echo "FATAL: psql is required"
  exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo "FATAL: python3 is required"
  exit 1
fi

RECOVERY_HASH="$(python3 - <<'PY'
import hashlib, os
keyword = os.environ["ACCOUNT_KEYWORD"]
salt = os.urandom(16).hex()
digest = hashlib.pbkdf2_hmac("sha256", keyword.encode("utf-8"), bytes.fromhex(salt), 100000, dklen=32).hex()
print(f"{salt}:{digest}")
PY
)"

export PGPASSWORD="${DB_PASSWORD}"

psql -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" -d "${DB_NAME}" -v ON_ERROR_STOP=1 <<SQL
INSERT INTO users (
    id, shadow_id, user_id, username, nickname, avatar_url, bio, device_id,
    role, status, is_anonymous, vip_level, vip_expires_at, resonance_total,
    is_guardian, guardian_since, last_login_at, last_active_at, created_at, updated_at,
    recovery_key_hash
) VALUES (
    '${ACCOUNT_ID}',
    'shadow_${ACCOUNT_ID}',
    '${ACCOUNT_ID}',
    '${ACCOUNT_USERNAME}',
    '${ACCOUNT_NICKNAME}',
    'https://api.dicebear.com/7.x/glass/svg?seed=${ACCOUNT_ID}',
    '全功能演示账号（永久灯火 + 守护者权限）',
    'device_${ACCOUNT_ID}',
    'user',
    'active',
    true,
    1,
    NULL,
    9999,
    true,
    NOW(),
    NOW(),
    NOW(),
    NOW(),
    NOW(),
    '${RECOVERY_HASH}'
)
ON CONFLICT (id) DO UPDATE SET
    user_id = EXCLUDED.user_id,
    username = EXCLUDED.username,
    nickname = EXCLUDED.nickname,
    status = 'active',
    vip_level = 1,
    vip_expires_at = NULL,
    resonance_total = GREATEST(users.resonance_total, 9999),
    is_guardian = true,
    guardian_since = COALESCE(users.guardian_since, NOW()),
    recovery_key_hash = EXCLUDED.recovery_key_hash,
    updated_at = NOW();

INSERT INTO user_privacy_settings (
    user_id, profile_visibility, show_online_status, allow_friend_request, allow_message_from_stranger
) VALUES (
    '${ACCOUNT_ID}', 'public', true, true, true
)
ON CONFLICT (user_id) DO UPDATE SET
    profile_visibility = EXCLUDED.profile_visibility,
    show_online_status = EXCLUDED.show_online_status,
    allow_friend_request = EXCLUDED.allow_friend_request,
    allow_message_from_stranger = EXCLUDED.allow_message_from_stranger;

INSERT INTO vip_upgrade_logs (
    user_id, old_vip_level, new_vip_level, upgrade_type, reason, expires_at, created_at
) VALUES (
    '${ACCOUNT_ID}', 0, 1, 'system_bootstrap', 'system:full_access_bootstrap', NULL, NOW()
);
SQL

echo "Full access account ready:"
echo "  user_id: ${ACCOUNT_ID}"
echo "  username: ${ACCOUNT_USERNAME}"
echo "  keyword: ${ACCOUNT_KEYWORD}"
