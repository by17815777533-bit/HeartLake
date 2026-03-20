-- Migration 017: account schema alignment
-- 目的：
-- 1. 补齐账号资料字段，收敛用户资料读写口径
-- 2. 创建 user_blocks，修复账户拉黑链路缺表问题
-- 3. 为连接消息和拉黑列表补充热路径索引

BEGIN;

ALTER TABLE users
    ADD COLUMN IF NOT EXISTS email VARCHAR(255),
    ADD COLUMN IF NOT EXISTS gender VARCHAR(32),
    ADD COLUMN IF NOT EXISTS birthday DATE,
    ADD COLUMN IF NOT EXISTS location VARCHAR(128);

CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);

CREATE TABLE IF NOT EXISTS user_blocks (
    user_id VARCHAR(64) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    blocked_user_id VARCHAR(64) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    PRIMARY KEY (user_id, blocked_user_id),
    CONSTRAINT chk_user_blocks_not_self CHECK (user_id <> blocked_user_id)
);

CREATE INDEX IF NOT EXISTS idx_user_blocks_user_created
    ON user_blocks(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_user_blocks_blocked_created
    ON user_blocks(blocked_user_id, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_connection_messages_conn_created
    ON connection_messages(connection_id, created_at ASC);

COMMIT;
