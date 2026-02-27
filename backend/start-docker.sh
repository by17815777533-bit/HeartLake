#!/bin/bash
set -euo pipefail

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
    "connection_number": ${DB_POOL_SIZE:-10},
    "timeout": ${DB_TIMEOUT:-30}
  }],
  "redis_clients": [{
    "name": "default",
    "host": "${REDIS_HOST:-127.0.0.1}",
    "port": ${REDIS_PORT:-6379},
    "passwd": "${REDIS_PASSWORD:-}",
    "connection_number": ${REDIS_POOL_SIZE:-10}
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
    MIGRATION_FAILED=0
    for f in $(ls migrations/*.sql 2>/dev/null | sort); do
        echo "  Applying $(basename "$f")..."
        if ! PGPASSWORD="$DB_PASSWORD" psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" \
            -v ON_ERROR_STOP=1 -f "$f" 2>&1; then
            echo "  WARNING: Migration $(basename "$f") had errors (may already be applied)"
            MIGRATION_FAILED=$((MIGRATION_FAILED + 1))
        fi
    done
    if [ "$MIGRATION_FAILED" -gt 0 ]; then
        echo "Migrations complete with $MIGRATION_FAILED warnings"
    else
        echo "All migrations applied successfully"
    fi
fi

exec ./HeartLake
