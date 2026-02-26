#!/usr/bin/env bash
# =============================================================================
# HeartLake 数据库迁移回滚脚本
#
# 用法:
#   ./rollback.sh <migration_number> [db_options]
#
# 示例:
#   ./rollback.sh 013                          # 回滚到 012（撤销 013）
#   ./rollback.sh 010                          # 回滚到 009（依次撤销 013→012→011→010）
#   ./rollback.sh 013 -h localhost -U postgres  # 指定数据库连接参数
#
# 环境变量:
#   DB_HOST     数据库主机（默认 localhost）
#   DB_PORT     数据库端口（默认 5432）
#   DB_NAME     数据库名（默认 heartlake）
#   DB_USER     数据库用户（默认 postgres）
#   PGPASSWORD  数据库密码（psql 标准环境变量）
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MIGRATIONS_DIR="$(dirname "$SCRIPT_DIR")/migrations"
ROLLBACK_DIR="$MIGRATIONS_DIR/rollback"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

usage() {
    echo "用法: $0 <target_migration_number> [psql_options]"
    echo ""
    echo "将数据库回滚到指定迁移版本（撤销该版本及之后的所有迁移）。"
    echo ""
    echo "示例:"
    echo "  $0 013                    # 仅撤销 013"
    echo "  $0 010                    # 依次撤销 013, 012, 011, 010"
    echo "  $0 005 -h db.example.com  # 指定数据库主机"
    exit 1
}

if [[ $# -lt 1 ]]; then
    usage
fi

TARGET="$1"
shift

# 数据库连接参数（优先使用环境变量，其次使用命令行参数）
DB_HOST="${DB_HOST:-localhost}"
DB_PORT="${DB_PORT:-5432}"
DB_NAME="${DB_NAME:-heartlake}"
DB_USER="${DB_USER:-postgres}"

PSQL_OPTS="-h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME $*"

# 去掉前导零，方便数值比较
target_num=$((10#$TARGET))

# 收集所有需要回滚的迁移编号（从最新到 target，降序）
rollback_files=()
for f in "$ROLLBACK_DIR"/*_down.sql; do
    [[ -f "$f" ]] || continue
    basename="$(basename "$f")"
    # 提取迁移编号，如 013_schema_fixes_down.sql → 013
    num_str="${basename%%_*}"
    num=$((10#$num_str))
    if [[ $num -ge $target_num ]]; then
        rollback_files+=("$num:$f")
    fi
done

# 按编号降序排列（先回滚最新的）
IFS=$'\n' sorted=($(sort -t: -k1 -nr <<<"${rollback_files[*]}")); unset IFS

if [[ ${#sorted[@]} -eq 0 ]]; then
    echo -e "${RED}错误: 未找到编号 >= $TARGET 的回滚文件${NC}"
    echo "可用的回滚文件:"
    ls "$ROLLBACK_DIR"/ 2>/dev/null || echo "  (无)"
    exit 1
fi

echo -e "${YELLOW}即将回滚以下迁移（降序执行）:${NC}"
for item in "${sorted[@]}"; do
    file="${item#*:}"
    echo "  - $(basename "$file")"
done
echo ""

# 确认操作（非交互模式下跳过）
if [[ -t 0 ]]; then
    read -rp "确认执行回滚? (y/N) " confirm
    if [[ "$confirm" != "y" && "$confirm" != "Y" ]]; then
        echo "已取消。"
        exit 0
    fi
fi

# 逐个执行回滚
failed=0
for item in "${sorted[@]}"; do
    file="${item#*:}"
    basename="$(basename "$file")"
    echo -e "${YELLOW}回滚: $basename ...${NC}"

    if psql $PSQL_OPTS -f "$file" 2>&1; then
        echo -e "${GREEN}  成功${NC}"
    else
        echo -e "${RED}  失败!${NC}"
        failed=1
        echo -e "${RED}回滚中断，请手动检查数据库状态。${NC}"
        exit 1
    fi
done

if [[ $failed -eq 0 ]]; then
    echo ""
    echo -e "${GREEN}全部回滚完成。数据库已回退到迁移 $(printf '%03d' $((target_num - 1))) 的状态。${NC}"
else
    echo ""
    echo -e "${RED}部分回滚失败，请检查上方错误信息。${NC}"
    exit 1
fi
