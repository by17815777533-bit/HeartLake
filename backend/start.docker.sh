#!/bin/bash
set -e

# In Docker, env vars are passed via docker-compose
# Validate required vars
if [ -z "$PASETO_KEY" ] || [ "$PASETO_KEY" = "your-very-strong-paseto-key-32bytes" ]; then
    echo "Error: PASETO_KEY must be set to a secure value (>=32 bytes)"
    exit 1
fi

exec ./HeartLake
