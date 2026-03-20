-- Rollback 017: account schema alignment

BEGIN;

DROP INDEX IF EXISTS idx_connection_messages_conn_created;
DROP INDEX IF EXISTS idx_user_blocks_blocked_created;
DROP INDEX IF EXISTS idx_user_blocks_user_created;
DROP TABLE IF EXISTS user_blocks;

DROP INDEX IF EXISTS idx_users_email;
ALTER TABLE users
    DROP COLUMN IF EXISTS location,
    DROP COLUMN IF EXISTS birthday,
    DROP COLUMN IF EXISTS gender,
    DROP COLUMN IF EXISTS email;

COMMIT;
