BEGIN;

DROP INDEX IF EXISTS idx_user_sessions_user_refresh_expiry;
DROP INDEX IF EXISTS idx_user_sessions_refresh_token_hash;

ALTER TABLE user_sessions
    DROP COLUMN IF EXISTS refresh_expires_at,
    DROP COLUMN IF EXISTS refresh_token_hash;

COMMIT;
