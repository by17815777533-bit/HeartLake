#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR/build"

# Copy .env.example if .env doesn't exist
if [ ! -f "$SCRIPT_DIR/.env" ]; then
    cp "$SCRIPT_DIR/.env.example" "$SCRIPT_DIR/.env"
    echo "Created .env from .env.example - please configure it"
fi

# Load environment variables
set -a
source "$SCRIPT_DIR/.env"
set +a

# Check required environment variables
if [ -z "$PASETO_KEY" ] || [ "$PASETO_KEY" = "your-very-strong-paseto-key-32bytes" ]; then
    echo "Error: PASETO_KEY must be set to a secure value (>=32 bytes) in .env"
    exit 1
fi

# Start HeartLake backend
exec ./HeartLake
