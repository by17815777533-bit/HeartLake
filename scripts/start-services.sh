#!/bin/bash
# HeartLake 服务启动脚本
# 用法: ./scripts/start-services.sh [all|db|dev]
#   all - 启动所有服务
#   db  - 只启动数据库和缓存 (开发模式)
#   dev - 启动数据库+缓存+ollama (本地开发)

set -e

cd "$(dirname "$0")/.."

case "${1:-dev}" in
  all)
    echo "🚀 启动所有服务..."
    docker compose up -d
    ;;
  db)
    echo "🗄️  启动数据库和缓存..."
    docker compose up -d postgres redis
    ;;
  dev)
    echo "🔧 启动开发环境 (postgres + redis + ollama)..."
    docker compose up -d postgres redis ollama
    ;;
  stop)
    echo "⏹️  停止所有服务..."
    docker compose down
    ;;
  *)
    echo "用法: $0 [all|db|dev|stop]"
    echo "  all  - 启动所有服务 (含nginx/backend/admin)"
    echo "  db   - 只启动 postgres + redis"
    echo "  dev  - 启动 postgres + redis + ollama (默认)"
    echo "  stop - 停止所有服务"
    exit 1
    ;;
esac

echo "✅ 完成！使用 'docker compose ps' 查看服务状态"
