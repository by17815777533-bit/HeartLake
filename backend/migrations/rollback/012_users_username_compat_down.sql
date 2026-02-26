-- 012 回滚: 移除 username 和 recovery_key_hash 兼容列
BEGIN;

DROP INDEX IF EXISTS idx_users_username;
ALTER TABLE users DROP COLUMN IF EXISTS recovery_key_hash;
ALTER TABLE users DROP COLUMN IF EXISTS username;

COMMIT;
