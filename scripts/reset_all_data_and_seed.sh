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

if ! command -v psql >/dev/null 2>&1; then
  echo "FATAL: psql is required"
  exit 1
fi

WITH_SEED=true
if [[ "${1:-}" == "--schema-only" ]]; then
  WITH_SEED=false
fi

export PGPASSWORD="${DB_PASSWORD}"

echo "[1/5] Drop & recreate database: ${DB_NAME}"
psql -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" -d postgres -v ON_ERROR_STOP=1 <<SQL
SELECT pg_terminate_backend(pid)
FROM pg_stat_activity
WHERE datname = '${DB_NAME}'
  AND pid <> pg_backend_pid();

DROP DATABASE IF EXISTS "${DB_NAME}";
CREATE DATABASE "${DB_NAME}";
SQL

echo "[2/5] Apply migrations"
for file in "${BACKEND_DIR}"/migrations/*.sql; do
  echo "  - $(basename "${file}")"
  psql -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" -d "${DB_NAME}" -v ON_ERROR_STOP=1 -f "${file}" >/dev/null
done

if [[ "${WITH_SEED}" == "true" ]]; then
  echo "[3/5] Seed showcase content"
  psql -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" -d "${DB_NAME}" -v ON_ERROR_STOP=1 -f "${ROOT_DIR}/scripts/seed_core_content_only.sql" >/dev/null
else
  echo "[3/5] Skip seed (--schema-only)"
fi

echo "[4/5] Create full access account"
"${ROOT_DIR}/scripts/create_full_access_account.sh" >/dev/null

echo "[5/5] Completed"
echo "Database reset done. Target DB: ${DB_NAME}"
