#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
RUNTIME_DIR="${ROOT_DIR}/.runtime"
LOG_DIR="${RUNTIME_DIR}/logs"
mkdir -p "${RUNTIME_DIR}" "${LOG_DIR}" "${RUNTIME_DIR}/redis"

ENV_FILE="${HEARTLAKE_ENV_PATH:-}"
if [[ -z "${ENV_FILE}" ]]; then
    if [[ -f "${SCRIPT_DIR}/.env" ]]; then
        ENV_FILE="${SCRIPT_DIR}/.env"
    elif [[ -f "${ROOT_DIR}/.env" ]]; then
        ENV_FILE="${ROOT_DIR}/.env"
    else
        cp "${SCRIPT_DIR}/.env.example" "${SCRIPT_DIR}/.env"
        ENV_FILE="${SCRIPT_DIR}/.env"
        echo "Created ${ENV_FILE} from .env.example"
    fi
fi

if [[ ! -f "${ENV_FILE}" ]]; then
    echo "FATAL: env file not found: ${ENV_FILE}"
    exit 1
fi

set -a
source "${ENV_FILE}"
set +a
export HEARTLAKE_ENV_PATH="${ENV_FILE}"

# --------- Defaults (single source of truth) ---------
export SERVER_HOST="${SERVER_HOST:-0.0.0.0}"
export SERVER_PORT="${SERVER_PORT:-8080}"
export DB_HOST="${DB_HOST:-127.0.0.1}"
export DB_PORT="${DB_PORT:-5432}"
export DB_NAME="${DB_NAME:-heartlake}"
export DB_USER="${DB_USER:-postgres}"
export DB_CONNECTION_POOL_SIZE="${DB_CONNECTION_POOL_SIZE:-${DB_POOL_SIZE:-12}}"
export DB_TIMEOUT="${DB_TIMEOUT:-30}"
export REDIS_HOST="${REDIS_HOST:-127.0.0.1}"
export REDIS_PORT="${REDIS_PORT:-6379}"
export REDIS_CONNECTION_POOL_SIZE="${REDIS_CONNECTION_POOL_SIZE:-${REDIS_POOL_SIZE:-12}}"
export AI_PROVIDER="${AI_PROVIDER:-ollama}"
export AI_BASE_URL="${AI_BASE_URL:-http://127.0.0.1:11434}"
export AI_MODEL="${AI_MODEL:-heartlake-qwen}"

# GPU strategy: default force GPU for heavy inference.
export AI_OLLAMA_FORCE_GPU="${AI_OLLAMA_FORCE_GPU:-true}"
export AI_OLLAMA_NUM_GPU="${AI_OLLAMA_NUM_GPU:-999}"
export AI_OLLAMA_MAIN_GPU="${AI_OLLAMA_MAIN_GPU:-0}"
export AI_OLLAMA_LOW_VRAM="${AI_OLLAMA_LOW_VRAM:-false}"
export EDGE_AI_ONNX_FORCE_GPU="${EDGE_AI_ONNX_FORCE_GPU:-true}"
export EDGE_AI_ONNX_GPU_DEVICE="${EDGE_AI_ONNX_GPU_DEVICE:-0}"
export EDGE_AI_ONNX_GPU_HARD_FAIL="${EDGE_AI_ONNX_GPU_HARD_FAIL:-false}"

if [[ -z "${DB_PASSWORD:-}" ]]; then
    echo "FATAL: DB_PASSWORD is required"
    exit 1
fi
if [[ -z "${PASETO_KEY:-}" || "${#PASETO_KEY}" -lt 32 ]]; then
    echo "FATAL: PASETO_KEY must be at least 32 bytes"
    exit 1
fi

# --------- Generate runtime config.json from env ---------
RUNTIME_CONFIG="${SCRIPT_DIR}/config.runtime.json"
cat > "${RUNTIME_CONFIG}" <<CONF
{
  "app": {
    "session": { "timeout": 1200 },
    "log": {
      "log_path": "./logs",
      "logfile_base_name": "heartlake",
      "log_size_limit": 100000000,
      "max_files": 10,
      "log_level": "${LOG_LEVEL:-INFO}"
    }
  },
  "db_clients": [{
    "name": "default",
    "rdbms": "postgresql",
    "host": "${DB_HOST}",
    "port": ${DB_PORT},
    "dbname": "${DB_NAME}",
    "user": "${DB_USER}",
    "passwd": "${DB_PASSWORD}",
    "connection_number": ${DB_CONNECTION_POOL_SIZE},
    "timeout": ${DB_TIMEOUT}
  }],
  "redis_clients": [{
    "name": "default",
    "host": "${REDIS_HOST}",
    "port": ${REDIS_PORT},
    "passwd": "${REDIS_PASSWORD:-}",
    "connection_number": ${REDIS_CONNECTION_POOL_SIZE}
  }]
}
CONF
export HEARTLAKE_CONFIG_PATH="${RUNTIME_CONFIG}"

wait_pg() {
    for _ in $(seq 1 30); do
        if pg_isready -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" >/dev/null 2>&1; then
            return 0
        fi
        sleep 1
    done
    return 1
}

local_addr() {
    [[ "$1" == "127.0.0.1" || "$1" == "localhost" ]]
}

# --------- PostgreSQL (persistent local runtime) ---------
if ! wait_pg; then
    if local_addr "${DB_HOST}"; then
        PG_DATA_DIR="${RUNTIME_DIR}/postgres"
        PG_PW_FILE="${RUNTIME_DIR}/postgres_pw"
        printf '%s' "${DB_PASSWORD}" > "${PG_PW_FILE}"
        chmod 600 "${PG_PW_FILE}"

        if [[ ! -s "${PG_DATA_DIR}/PG_VERSION" ]]; then
            rm -rf "${PG_DATA_DIR}"
            initdb -D "${PG_DATA_DIR}" --username="${DB_USER}" --pwfile="${PG_PW_FILE}" > "${LOG_DIR}/initdb.log" 2>&1
        fi

        pg_ctl -D "${PG_DATA_DIR}" -l "${LOG_DIR}/postgres.log" -o "-h 127.0.0.1 -p ${DB_PORT}" start >/dev/null
    else
        echo "FATAL: PostgreSQL is not ready at ${DB_HOST}:${DB_PORT}"
        exit 1
    fi
fi

if ! wait_pg; then
    echo "FATAL: PostgreSQL startup failed"
    exit 1
fi

if ! PGPASSWORD="${DB_PASSWORD}" psql -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" -d postgres -tAc "SELECT 1 FROM pg_database WHERE datname='${DB_NAME}'" | grep -q 1; then
    PGPASSWORD="${DB_PASSWORD}" createdb -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" "${DB_NAME}"
fi

# --------- Redis (persistent local runtime) ---------
if ! redis-cli -h "${REDIS_HOST}" -p "${REDIS_PORT}" -a "${REDIS_PASSWORD:-}" ping >/dev/null 2>&1; then
    if local_addr "${REDIS_HOST}"; then
        REDIS_CONF="${RUNTIME_DIR}/redis/redis.conf"
        cat > "${REDIS_CONF}" <<RC
bind 127.0.0.1
port ${REDIS_PORT}
dir ${RUNTIME_DIR}/redis
appendonly yes
protected-mode yes
daemonize yes
logfile ${LOG_DIR}/redis.log
requirepass ${REDIS_PASSWORD:-}
RC
        redis-server "${REDIS_CONF}" >/dev/null 2>&1
        sleep 1
    else
        echo "FATAL: Redis is not ready at ${REDIS_HOST}:${REDIS_PORT}"
        exit 1
    fi
fi

if ! redis-cli -h "${REDIS_HOST}" -p "${REDIS_PORT}" -a "${REDIS_PASSWORD:-}" ping >/dev/null 2>&1; then
    echo "FATAL: Redis startup failed"
    exit 1
fi

# --------- Ollama + Qwen model ---------
if [[ "${AI_PROVIDER}" == "ollama" ]]; then
    OLLAMA_ENDPOINT="${AI_BASE_URL%/}/api/tags"
    if ! curl -fsS "${OLLAMA_ENDPOINT}" >/dev/null 2>&1; then
        OLLAMA_HOST_VALUE="${AI_BASE_URL#http://}"
        OLLAMA_HOST_VALUE="${OLLAMA_HOST_VALUE#https://}"
        OLLAMA_HOST_VALUE="${OLLAMA_HOST_VALUE%%/*}"
        CUDA_VISIBLE_DEVICES_VALUE="${AI_GPU_DEVICES:-0}"
        nohup env \
            OLLAMA_HOST="${OLLAMA_HOST_VALUE}" \
            OLLAMA_NUM_GPU="${AI_OLLAMA_NUM_GPU}" \
            CUDA_VISIBLE_DEVICES="${CUDA_VISIBLE_DEVICES_VALUE}" \
            OLLAMA_FLASH_ATTENTION=1 \
            ollama serve > "${LOG_DIR}/ollama.log" 2>&1 &
        echo $! > "${RUNTIME_DIR}/ollama.pid"
        for _ in $(seq 1 30); do
            if curl -fsS "${OLLAMA_ENDPOINT}" >/dev/null 2>&1; then
                break
            fi
            sleep 1
        done
    fi

    if ! ollama list 2>/dev/null | awk 'NR>1 {print $1}' | grep -Eq "^${AI_MODEL}(:|$)"; then
        ollama pull "${AI_MODEL}"
    fi
fi

# --------- Migrations ---------
if [[ -d "${SCRIPT_DIR}/migrations" ]]; then
    for f in $(ls "${SCRIPT_DIR}"/migrations/*.sql 2>/dev/null | sort); do
        echo "Applying $(basename "$f")"
        if ! PGPASSWORD="${DB_PASSWORD}" psql -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" -d "${DB_NAME}" -v ON_ERROR_STOP=1 -f "$f" >> "${LOG_DIR}/migrations.log" 2>&1; then
            echo "WARN: migration failed or already applied: $(basename "$f")"
        fi
    done
fi

# --------- Build if needed ---------
if [[ ! -x "${SCRIPT_DIR}/build/HeartLake" ]]; then
    cmake -S "${SCRIPT_DIR}" -B "${SCRIPT_DIR}/build" -DCMAKE_BUILD_TYPE=Release -DHEARTLAKE_USE_ONNX=ON
    cmake --build "${SCRIPT_DIR}/build" -j"$(nproc)"
fi

# Prefer bundled ONNX Runtime libs to avoid system mismatch.
if [[ -d "${SCRIPT_DIR}/third_party/onnxruntime-linux-x64-1.22.0/lib" ]]; then
    export LD_LIBRARY_PATH="${SCRIPT_DIR}/third_party/onnxruntime-linux-x64-1.22.0/lib:${LD_LIBRARY_PATH:-}"
fi

echo "HeartLake runtime ready. Starting backend..."
cd "${SCRIPT_DIR}/build"
exec ./HeartLake
