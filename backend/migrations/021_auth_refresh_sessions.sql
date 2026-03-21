BEGIN;

ALTER TABLE user_sessions
    ADD COLUMN IF NOT EXISTS refresh_token_hash TEXT,
    ADD COLUMN IF NOT EXISTS refresh_expires_at TIMESTAMPTZ;

CREATE INDEX IF NOT EXISTS idx_user_sessions_refresh_token_hash
    ON user_sessions(refresh_token_hash)
    WHERE refresh_token_hash IS NOT NULL AND is_active = true;

CREATE INDEX IF NOT EXISTS idx_user_sessions_user_refresh_expiry
    ON user_sessions(user_id, refresh_expires_at DESC)
    WHERE is_active = true;

COMMIT;
