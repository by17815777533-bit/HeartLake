#!/bin/bash
set -euo pipefail

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

SERVER_THREADS_VALUE="$(clamp_int "${SERVER_THREADS:-4}" 2 8)"
DB_POOL_SIZE_VALUE="$(clamp_int "${DB_POOL_SIZE:-$(( SERVER_THREADS_VALUE * 2 ))}" 4 12)"
REDIS_POOL_SIZE_VALUE="$(clamp_int "${REDIS_POOL_SIZE:-$(( SERVER_THREADS_VALUE * 2 ))}" 4 8)"
REDIS_MAX_POOL_SIZE_VALUE="$(clamp_int "${REDIS_MAX_POOL_SIZE:-$(( REDIS_POOL_SIZE_VALUE * 2 ))}" 4 8)"
LOW_RESOURCE_PROFILE=false
if (( SERVER_THREADS_VALUE <= 2 )); then
    LOW_RESOURCE_PROFILE=true
fi

DEFAULT_EMBEDDING_CACHE_SIZE=20000
DEFAULT_AI_SEMANTIC_CACHE_MAX_SIZE=5000
DEFAULT_AI_SENTIMENT_CACHE_MAX_SIZE=30000
DEFAULT_EDGE_AI_SENTIMENT_CACHE_MAX_SIZE=8192
DEFAULT_LAKE_GOD_ENABLED=true
DEFAULT_EMOTION_TRACKING_ENABLED=true
DEFAULT_USER_FOLLOWUP_ENABLED=true
if [[ "${LOW_RESOURCE_PROFILE}" == "true" ]]; then
    DEFAULT_EMBEDDING_CACHE_SIZE=6000
    DEFAULT_AI_SEMANTIC_CACHE_MAX_SIZE=2000
    DEFAULT_AI_SENTIMENT_CACHE_MAX_SIZE=8000
    DEFAULT_EDGE_AI_SENTIMENT_CACHE_MAX_SIZE=4096
    DEFAULT_LAKE_GOD_ENABLED=false
    DEFAULT_EMOTION_TRACKING_ENABLED=false
    DEFAULT_USER_FOLLOWUP_ENABLED=false
fi

export DB_CONNECTION_POOL_SIZE="${DB_CONNECTION_POOL_SIZE:-${DB_POOL_SIZE_VALUE}}"
export REDIS_CONNECTION_POOL_SIZE="${REDIS_CONNECTION_POOL_SIZE:-${REDIS_POOL_SIZE_VALUE}}"
export REDIS_MAX_POOL_SIZE="${REDIS_MAX_POOL_SIZE:-${REDIS_MAX_POOL_SIZE_VALUE}}"
export EMBEDDING_CACHE_SIZE="${EMBEDDING_CACHE_SIZE:-${DEFAULT_EMBEDDING_CACHE_SIZE}}"
export AI_SEMANTIC_CACHE_MAX_SIZE="${AI_SEMANTIC_CACHE_MAX_SIZE:-${DEFAULT_AI_SEMANTIC_CACHE_MAX_SIZE}}"
export AI_SENTIMENT_CACHE_MAX_SIZE="${AI_SENTIMENT_CACHE_MAX_SIZE:-${DEFAULT_AI_SENTIMENT_CACHE_MAX_SIZE}}"
export EDGE_AI_SENTIMENT_CACHE_MAX_SIZE="${EDGE_AI_SENTIMENT_CACHE_MAX_SIZE:-${DEFAULT_EDGE_AI_SENTIMENT_CACHE_MAX_SIZE}}"
export ENABLE_LAKE_GOD_GUARDIAN="${ENABLE_LAKE_GOD_GUARDIAN:-${DEFAULT_LAKE_GOD_ENABLED}}"
export ENABLE_EMOTION_TRACKING="${ENABLE_EMOTION_TRACKING:-${DEFAULT_EMOTION_TRACKING_ENABLED}}"
export ENABLE_USER_FOLLOWUP="${ENABLE_USER_FOLLOWUP:-${DEFAULT_USER_FOLLOWUP_ENABLED}}"
export REALTIME_QUIC_ENABLED="${REALTIME_QUIC_ENABLED:-false}"
export AI_OLLAMA_NUM_THREAD="${AI_OLLAMA_NUM_THREAD:-$([[ "${LOW_RESOURCE_PROFILE}" == "true" ]] && echo 2 || echo 0)}"
export AI_OLLAMA_NUM_CTX="${AI_OLLAMA_NUM_CTX:-$([[ "${LOW_RESOURCE_PROFILE}" == "true" ]] && echo 2048 || echo 0)}"

# Generate config.json from environment variables
cat > config.json <<CONF
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
    "host": "${DB_HOST:-127.0.0.1}",
    "port": ${DB_PORT:-5432},
    "dbname": "${DB_NAME:-heartlake}",
    "user": "${DB_USER:-postgres}",
    "passwd": "${DB_PASSWORD}",
    "connection_number": ${DB_POOL_SIZE_VALUE},
    "timeout": ${DB_TIMEOUT:-30}
  }],
  "redis_clients": [{
    "name": "default",
    "host": "${REDIS_HOST:-127.0.0.1}",
    "port": ${REDIS_PORT:-6379},
    "passwd": "${REDIS_PASSWORD:-}",
    "connection_number": ${REDIS_POOL_SIZE_VALUE}
  }]
}
CONF

# Validate critical environment variables
if [ -z "${DB_PASSWORD:-}" ]; then
    echo "FATAL: DB_PASSWORD is not set"
    exit 1
fi

if [ -z "${PASETO_KEY:-}" ] || [ "${#PASETO_KEY}" -lt 32 ]; then
    echo "FATAL: PASETO_KEY must be at least 32 bytes"
    exit 1
fi

if [ -z "${ADMIN_PASETO_KEY:-}" ] || [ "${#ADMIN_PASETO_KEY}" -lt 32 ]; then
    echo "FATAL: ADMIN_PASETO_KEY must be at least 32 bytes"
    exit 1
fi

# Wait for PostgreSQL with timeout
echo "Waiting for PostgreSQL at ${DB_HOST}:${DB_PORT}..."
for i in $(seq 1 30); do
    if pg_isready -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" 2>/dev/null; then
        echo "PostgreSQL is ready"
        break
    fi
    if [ "$i" -eq 30 ]; then
        echo "FATAL: PostgreSQL not ready after 30 seconds"
        exit 1
    fi
    sleep 1
done

# Run migrations with proper error handling
if [ -d "migrations" ]; then
    echo "Running migrations..."
    for f in $(ls migrations/*.sql 2>/dev/null | sort); do
        echo "  Applying $(basename "$f")..."
        if ! PGPASSWORD="$DB_PASSWORD" psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" \
            -v ON_ERROR_STOP=1 -f "$f" 2>&1; then
            echo "  FATAL: Migration $(basename "$f") failed"
            exit 1
        fi
    done
    echo "All migrations applied successfully"
fi

exec ./HeartLake
