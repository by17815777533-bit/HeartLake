#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
RUNTIME_DIR="${ROOT_DIR}/.runtime"
LOG_DIR="${RUNTIME_DIR}/logs"
mkdir -p "${RUNTIME_DIR}" "${LOG_DIR}" "${RUNTIME_DIR}/redis"

BOOTSTRAP_LIB="${ROOT_DIR}/scripts/lib/env-bootstrap.sh"
if [[ ! -f "${BOOTSTRAP_LIB}" ]]; then
    echo "FATAL: bootstrap helper missing: ${BOOTSTRAP_LIB}"
    exit 1
fi
source "${BOOTSTRAP_LIB}"

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

if bootstrap_msg="$(heartlake_bootstrap_env "${ENV_FILE}" "${RUNTIME_DIR}/bootstrap-secrets.txt" "backend/start.sh")"; then
    if [[ -n "${bootstrap_msg}" ]]; then
        echo "${bootstrap_msg}"
    fi
else
    exit 1
fi

set -a
source "${ENV_FILE}"
set +a
export HEARTLAKE_ENV_PATH="${ENV_FILE}"

clamp_int() {
    local raw="${1:-0}"
    local min="${2:-1}"
    local max="${3:-999999}"
    if [[ ! "${raw}" =~ ^[0-9]+$ ]]; then
        echo "${min}"
        return
    fi
    if (( raw < min )); then
        echo "${min}"
        return
    fi
    if (( raw > max )); then
        echo "${max}"
        return
    fi
    echo "${raw}"
}

detect_server_threads() {
    if [[ -n "${SERVER_THREADS:-}" ]]; then
        clamp_int "${SERVER_THREADS}" 2 8
        return
    fi
    local detected="2"
    if command -v nproc >/dev/null 2>&1; then
        detected="$(nproc)"
    elif command -v sysctl >/dev/null 2>&1; then
        detected="$(sysctl -n hw.ncpu 2>/dev/null || echo 2)"
    fi
    clamp_int "${detected}" 2 8
}

resolve_compiler() {
    local preferred="${1:-}"
    local fallback="${2:-}"
    local resolved=""

    if [[ -n "${preferred}" ]]; then
        resolved="$(command -v "${preferred}" 2>/dev/null || printf '%s' "${preferred}")"
        if [[ "${resolved}" == *openharmony* ]]; then
            resolved=""
        fi
    fi

    if [[ -z "${resolved}" && -n "${fallback}" ]]; then
        resolved="$(command -v "${fallback}" 2>/dev/null || true)"
    fi

    printf '%s' "${resolved}"
}

DEFAULT_SERVER_THREADS="$(detect_server_threads)"
DEFAULT_DB_POOL="$(( DEFAULT_SERVER_THREADS * 2 ))"
DEFAULT_REDIS_POOL="$(( DEFAULT_SERVER_THREADS * 2 ))"
DEFAULT_DB_POOL="$(clamp_int "${DEFAULT_DB_POOL}" 4 12)"
DEFAULT_REDIS_POOL="$(clamp_int "${DEFAULT_REDIS_POOL}" 4 8)"
NATIVE_CC="$(resolve_compiler "${BACKEND_CC:-${CC:-}}" gcc)"
NATIVE_CXX="$(resolve_compiler "${BACKEND_CXX:-${CXX:-}}" g++)"

# --------- Defaults (single source of truth) ---------
export SERVER_HOST="${SERVER_HOST:-0.0.0.0}"
export SERVER_PORT="${SERVER_PORT:-8080}"
export SERVER_THREADS="${SERVER_THREADS:-${DEFAULT_SERVER_THREADS}}"
LOW_RESOURCE_PROFILE=false
if [[ "${SERVER_THREADS}" =~ ^[0-9]+$ ]] && (( SERVER_THREADS <= 2 )); then
    LOW_RESOURCE_PROFILE=true
fi
DEFAULT_EMBEDDING_CACHE_SIZE=20000
DEFAULT_AI_SEMANTIC_CACHE_MAX_SIZE=5000
DEFAULT_AI_SENTIMENT_CACHE_MAX_SIZE=30000
DEFAULT_EDGE_AI_SENTIMENT_CACHE_MAX_SIZE=8192
DEFAULT_LAKE_GOD_ENABLED=true
DEFAULT_EMOTION_TRACKING_ENABLED=true
DEFAULT_USER_FOLLOWUP_ENABLED=true
DEFAULT_REALTIME_QUIC_ENABLED=false
if [[ "${LOW_RESOURCE_PROFILE}" == "true" ]]; then
    DEFAULT_EMBEDDING_CACHE_SIZE=6000
    DEFAULT_AI_SEMANTIC_CACHE_MAX_SIZE=2000
    DEFAULT_AI_SENTIMENT_CACHE_MAX_SIZE=8000
    DEFAULT_EDGE_AI_SENTIMENT_CACHE_MAX_SIZE=4096
    DEFAULT_LAKE_GOD_ENABLED=false
    DEFAULT_EMOTION_TRACKING_ENABLED=false
    DEFAULT_USER_FOLLOWUP_ENABLED=false
fi
export DB_HOST="${DB_HOST:-127.0.0.1}"
export DB_PORT="${DB_PORT:-5432}"
export DB_NAME="${DB_NAME:-heartlake}"
export DB_USER="${DB_USER:-postgres}"
export DB_POOL_SIZE="${DB_POOL_SIZE:-${DEFAULT_DB_POOL}}"
export DB_CONNECTION_POOL_SIZE="${DB_CONNECTION_POOL_SIZE:-${DB_POOL_SIZE:-${DEFAULT_DB_POOL}}}"
export DB_TIMEOUT="${DB_TIMEOUT:-30}"
export REDIS_HOST="${REDIS_HOST:-127.0.0.1}"
export REDIS_PORT="${REDIS_PORT:-6379}"
export REDIS_POOL_SIZE="${REDIS_POOL_SIZE:-${DEFAULT_REDIS_POOL}}"
export REDIS_CONNECTION_POOL_SIZE="${REDIS_CONNECTION_POOL_SIZE:-${REDIS_POOL_SIZE:-${DEFAULT_REDIS_POOL}}}"
export REDIS_MAX_POOL_SIZE="${REDIS_MAX_POOL_SIZE:-$(clamp_int "$(( REDIS_CONNECTION_POOL_SIZE * 2 ))" 4 8)}"
export REALTIME_QUIC_ENABLED="${REALTIME_QUIC_ENABLED:-${DEFAULT_REALTIME_QUIC_ENABLED}}"
export QUIC_GATEWAY_BIND="${QUIC_GATEWAY_BIND:-0.0.0.0:8443}"
export AI_PROVIDER="${AI_PROVIDER:-ollama}"
export AI_BASE_URL="${AI_BASE_URL:-http://127.0.0.1:11434}"
export AI_MODEL="${AI_MODEL:-heartlake-qwen}"
export AI_OLLAMA_AUTOSTART="${AI_OLLAMA_AUTOSTART:-false}"
export EMBEDDING_DIM="${EMBEDDING_DIM:-256}"
export EMBEDDING_CACHE_SIZE="${EMBEDDING_CACHE_SIZE:-${DEFAULT_EMBEDDING_CACHE_SIZE}}"
export AI_SEMANTIC_CACHE_MAX_SIZE="${AI_SEMANTIC_CACHE_MAX_SIZE:-${DEFAULT_AI_SEMANTIC_CACHE_MAX_SIZE}}"
export AI_SENTIMENT_CACHE_MAX_SIZE="${AI_SENTIMENT_CACHE_MAX_SIZE:-${DEFAULT_AI_SENTIMENT_CACHE_MAX_SIZE}}"
export EDGE_AI_SENTIMENT_CACHE_MAX_SIZE="${EDGE_AI_SENTIMENT_CACHE_MAX_SIZE:-${DEFAULT_EDGE_AI_SENTIMENT_CACHE_MAX_SIZE}}"
export ENABLE_LAKE_GOD_GUARDIAN="${ENABLE_LAKE_GOD_GUARDIAN:-${DEFAULT_LAKE_GOD_ENABLED}}"
export ENABLE_EMOTION_TRACKING="${ENABLE_EMOTION_TRACKING:-${DEFAULT_EMOTION_TRACKING_ENABLED}}"
export ENABLE_USER_FOLLOWUP="${ENABLE_USER_FOLLOWUP:-${DEFAULT_USER_FOLLOWUP_ENABLED}}"

# GPU strategy: default to stable CPU path unless explicitly enabled.
export AI_OLLAMA_FORCE_GPU="${AI_OLLAMA_FORCE_GPU:-false}"
export AI_OLLAMA_NUM_GPU="${AI_OLLAMA_NUM_GPU:-0}"
export AI_OLLAMA_MAIN_GPU="${AI_OLLAMA_MAIN_GPU:-0}"
export AI_OLLAMA_LOW_VRAM="${AI_OLLAMA_LOW_VRAM:-false}"
export AI_OLLAMA_NUM_THREAD="${AI_OLLAMA_NUM_THREAD:-$([[ "${LOW_RESOURCE_PROFILE}" == "true" ]] && echo 2 || echo 0)}"
export AI_OLLAMA_NUM_CTX="${AI_OLLAMA_NUM_CTX:-$([[ "${LOW_RESOURCE_PROFILE}" == "true" ]] && echo 2048 || echo 0)}"
export EDGE_AI_ONNX_FORCE_GPU="${EDGE_AI_ONNX_FORCE_GPU:-false}"
export EDGE_AI_ONNX_GPU_DEVICE="${EDGE_AI_ONNX_GPU_DEVICE:-0}"
export EDGE_AI_ONNX_GPU_HARD_FAIL="${EDGE_AI_ONNX_GPU_HARD_FAIL:-false}"
ORT_GPU_ROOT="${SCRIPT_DIR}/third_party/onnxruntime-linux-x64-gpu-1.22.0"
ORT_CPU_ROOT="${SCRIPT_DIR}/third_party/onnxruntime-linux-x64-1.22.0"

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
    local retries="${1:-10}"
    local timeout_sec="${2:-1}"
    for _ in $(seq 1 "${retries}"); do
        if command -v pg_isready >/dev/null 2>&1; then
            if pg_isready -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" -t "${timeout_sec}" >/dev/null 2>&1; then
                return 0
            fi
        elif command -v psql >/dev/null 2>&1; then
            if PGPASSWORD="${DB_PASSWORD}" psql -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" -d postgres -tAc "SELECT 1" >/dev/null 2>&1; then
                return 0
            fi
        fi
        sleep 1
    done
    return 1
}

local_addr() {
    [[ "$1" == "127.0.0.1" || "$1" == "localhost" ]]
}

prepend_ld_library_path() {
    local dir="$1"
    if [[ ! -d "${dir}" ]]; then
        return 0
    fi
    if [[ -z "${LD_LIBRARY_PATH:-}" ]]; then
        export LD_LIBRARY_PATH="${dir}"
        return 0
    fi
    if [[ ":${LD_LIBRARY_PATH}:" != *":${dir}:"* ]]; then
        export LD_LIBRARY_PATH="${dir}:${LD_LIBRARY_PATH}"
    fi
}

missing_commands=()
require_command() {
    local cmd="$1"
    if ! command -v "${cmd}" >/dev/null 2>&1; then
        missing_commands+=("${cmd}")
    fi
}

require_command "cmake"
require_command "psql"
require_command "redis-cli"
if [[ -z "${NATIVE_CXX}" ]]; then
    missing_commands+=("g++")
fi
if local_addr "${DB_HOST}"; then
    require_command "initdb"
    require_command "pg_ctl"
fi
if local_addr "${REDIS_HOST}"; then
    require_command "redis-server"
fi

if (( ${#missing_commands[@]} > 0 )); then
    printf 'FATAL: missing required commands: %s\n' "$(IFS=', '; echo "${missing_commands[*]}")"
    echo "Hint: install local PostgreSQL/Redis toolchains, or use ./scripts/docker-up.sh once Docker is available."
    exit 1
fi

# --------- PostgreSQL (persistent local runtime) ---------
if ! wait_pg 5 1; then
    if local_addr "${DB_HOST}"; then
        PG_DATA_DIR="${RUNTIME_DIR}/postgres"
        PG_PW_FILE="${RUNTIME_DIR}/postgres_pw"
        PG_SOCKET_DIR="${RUNTIME_DIR}/postgres_socket"
        printf '%s' "${DB_PASSWORD}" > "${PG_PW_FILE}"
        chmod 600 "${PG_PW_FILE}"
        mkdir -p "${PG_SOCKET_DIR}"

        if [[ ! -s "${PG_DATA_DIR}/PG_VERSION" ]]; then
            rm -rf "${PG_DATA_DIR}"
            initdb -D "${PG_DATA_DIR}" --username="${DB_USER}" --pwfile="${PG_PW_FILE}" > "${LOG_DIR}/initdb.log" 2>&1
        fi

        if ! pg_ctl -D "${PG_DATA_DIR}" -l "${LOG_DIR}/postgres.log" -o "-h 127.0.0.1 -p ${DB_PORT} -k ${PG_SOCKET_DIR}" start >/dev/null; then
            echo "FATAL: PostgreSQL startup command failed"
            tail -n 40 "${LOG_DIR}/postgres.log" || true
            exit 1
        fi
    else
        echo "FATAL: PostgreSQL is not ready at ${DB_HOST}:${DB_PORT}"
        exit 1
    fi
fi

if ! wait_pg 20 1; then
    echo "FATAL: PostgreSQL startup failed"
    tail -n 40 "${LOG_DIR}/postgres.log" || true
    exit 1
fi

if ! PGPASSWORD="${DB_PASSWORD}" psql -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" -d postgres -tAc "SELECT 1 FROM pg_database WHERE datname='${DB_NAME}'" | grep -q 1; then
    if ! command -v createdb >/dev/null 2>&1; then
        echo "FATAL: createdb is required to create missing database ${DB_NAME}"
        exit 1
    fi
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
    if [[ "${AI_OLLAMA_AUTOSTART}" != "true" ]]; then
        echo "AI_PROVIDER=ollama but AI_OLLAMA_AUTOSTART=false, skipping ollama bootstrap."
    else
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
fi

# --------- Migrations ---------
if [[ -d "${SCRIPT_DIR}/migrations" ]]; then
    for f in $(ls "${SCRIPT_DIR}"/migrations/*.sql 2>/dev/null | sort); do
        echo "Applying $(basename "$f")"
        if ! PGPASSWORD="${DB_PASSWORD}" psql -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" -d "${DB_NAME}" -v ON_ERROR_STOP=1 -f "$f" >> "${LOG_DIR}/migrations.log" 2>&1; then
            echo "FATAL: migration failed: $(basename "$f")"
            echo "See ${LOG_DIR}/migrations.log for details"
            exit 1
        fi
    done
fi

# --------- ONNX Runtime / CUDA loader path ---------
if [[ -d "${ORT_GPU_ROOT}/lib" ]]; then
    export ONNXRUNTIME_ROOT="${ORT_GPU_ROOT}"
    prepend_ld_library_path "${ORT_GPU_ROOT}/lib"
elif [[ -d "${ORT_CPU_ROOT}/lib" ]]; then
    export ONNXRUNTIME_ROOT="${ORT_CPU_ROOT}"
    prepend_ld_library_path "${ORT_CPU_ROOT}/lib"
fi

# Reuse existing local CUDA runtime shipped by Ollama first.
prepend_ld_library_path "/usr/local/lib/ollama/cuda_v12"
prepend_ld_library_path "/usr/local/lib/ollama/cuda_v13"
prepend_ld_library_path "/usr/local/lib/ollama/mlx_cuda_v13"

# Optional local CUDA deps (if user preinstalled into runtime venv).
shopt -s nullglob
for nvidia_lib_dir in "${ROOT_DIR}"/.runtime/venv-onnx/lib/python*/site-packages/nvidia/*/lib; do
    prepend_ld_library_path "${nvidia_lib_dir}"
done
shopt -u nullglob

# --------- Configure & Build ---------
if [[ -f "${SCRIPT_DIR}/build/CMakeCache.txt" && -n "${NATIVE_CXX}" ]]; then
    CURRENT_CXX_COMPILER="$(sed -n 's#^CMAKE_CXX_COMPILER:FILEPATH=##p' "${SCRIPT_DIR}/build/CMakeCache.txt" | head -n 1)"
    if [[ -n "${CURRENT_CXX_COMPILER}" && "${CURRENT_CXX_COMPILER}" != "${NATIVE_CXX}" ]]; then
        echo "Compiler changed (${CURRENT_CXX_COMPILER} -> ${NATIVE_CXX}), recreating build directory"
        rm -rf "${SCRIPT_DIR}/build"
    fi
fi

cmake -S "${SCRIPT_DIR}" -B "${SCRIPT_DIR}/build" \
    ${NATIVE_CC:+-DCMAKE_C_COMPILER="${NATIVE_CC}"} \
    ${NATIVE_CXX:+-DCMAKE_CXX_COMPILER="${NATIVE_CXX}"} \
    -DCMAKE_BUILD_TYPE=Release \
    -DHEARTLAKE_USE_ONNX=ON \
    -DONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT:-}"

if [[ "${HEARTLAKE_SKIP_BUILD:-false}" != "true" ]]; then
    cmake --build "${SCRIPT_DIR}/build" -j"${SERVER_THREADS}"
fi

# --------- Rust QUIC gateway (dual-channel realtime) ---------
if [[ "${REALTIME_QUIC_ENABLED}" == "true" ]]; then
    QUIC_DIR="${SCRIPT_DIR}/quic_gateway"
    QUIC_BIN="${QUIC_DIR}/target/release/heartlake-quic-gateway"
    QUIC_LOG="${LOG_DIR}/quic_gateway.log"
    QUIC_PID_FILE="${RUNTIME_DIR}/quic_gateway.pid"

    if [[ ! -f "${QUIC_DIR}/Cargo.toml" ]]; then
        echo "FATAL: QUIC gateway project missing: ${QUIC_DIR}"
        exit 1
    fi

    if ! command -v cargo >/dev/null 2>&1; then
        echo "FATAL: cargo is required when REALTIME_QUIC_ENABLED=true"
        exit 1
    fi

    if [[ "${HEARTLAKE_SKIP_QUIC_BUILD:-false}" != "true" ]]; then
        cargo build --release --manifest-path "${QUIC_DIR}/Cargo.toml" >> "${LOG_DIR}/quic_build.log" 2>&1
    fi

    if [[ ! -x "${QUIC_BIN}" ]]; then
        echo "FATAL: QUIC gateway binary not found: ${QUIC_BIN}"
        exit 1
    fi

    if [[ -f "${QUIC_PID_FILE}" ]]; then
        old_pid="$(cat "${QUIC_PID_FILE}" || true)"
        if [[ -n "${old_pid}" ]] && kill -0 "${old_pid}" 2>/dev/null; then
            kill "${old_pid}" || true
        fi
        rm -f "${QUIC_PID_FILE}"
    fi

    nohup env \
        PASETO_KEY="${PASETO_KEY}" \
        REDIS_HOST="${REDIS_HOST}" \
        REDIS_PORT="${REDIS_PORT}" \
        REDIS_PASSWORD="${REDIS_PASSWORD:-}" \
        REDIS_URL="${REDIS_URL:-}" \
        QUIC_GATEWAY_BIND="${QUIC_GATEWAY_BIND}" \
        "${QUIC_BIN}" >> "${QUIC_LOG}" 2>&1 &
    echo $! > "${QUIC_PID_FILE}"
    sleep 1
    if ! kill -0 "$(cat "${QUIC_PID_FILE}")" 2>/dev/null; then
        echo "FATAL: QUIC gateway startup failed"
        tail -n 60 "${QUIC_LOG}" || true
        exit 1
    fi
fi

echo "HeartLake runtime ready. Starting backend..."
cd "${SCRIPT_DIR}/build"
echo $$ > "${RUNTIME_DIR}/backend.pid"
exec ./HeartLake
