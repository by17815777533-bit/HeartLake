-- 012: users 表兼容列补齐
-- 注意: 生产环境建议将 CREATE INDEX 改为 CREATE INDEX CONCURRENTLY 以避免锁表
-- 目标：
-- 1) 补齐 username 列，兼容历史代码中对 users.username 的查询
-- 2) 补齐 recovery_key_hash 列，兼容关键词恢复流程
-- 3) 回填 username，避免 NULL 导致前端展示异常

ALTER TABLE users
    ADD COLUMN IF NOT EXISTS username VARCHAR(64);

ALTER TABLE users
    ADD COLUMN IF NOT EXISTS recovery_key_hash VARCHAR(64);

UPDATE users
SET username = COALESCE(NULLIF(user_id, ''), id)
WHERE username IS NULL OR username = '';

CREATE INDEX IF NOT EXISTS idx_users_username
    ON users(username);
