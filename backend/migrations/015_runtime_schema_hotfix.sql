-- Migration 015: Runtime schema hotfix
-- Purpose:
-- 1) Ensure account-related auxiliary tables exist (to avoid 500 on account APIs)
-- 2) Expand users.recovery_key_hash length to prevent hash truncation failures

BEGIN;

-- account/device/session related tables
CREATE TABLE IF NOT EXISTS user_sessions (
    session_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id VARCHAR(64) NOT NULL,
    device_type VARCHAR(32),
    device_name VARCHAR(128),
    ip_address VARCHAR(45),
    is_active BOOLEAN NOT NULL DEFAULT true,
    last_active_at TIMESTAMPTZ DEFAULT NOW(),
    created_at TIMESTAMPTZ DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_user_sessions_user_active ON user_sessions(user_id, is_active);
CREATE INDEX IF NOT EXISTS idx_user_sessions_created ON user_sessions(created_at);

CREATE TABLE IF NOT EXISTS login_logs (
    log_id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    login_time TIMESTAMPTZ DEFAULT NOW(),
    ip_address VARCHAR(45),
    device_type VARCHAR(32),
    location VARCHAR(128),
    success BOOLEAN NOT NULL DEFAULT true
);
CREATE INDEX IF NOT EXISTS idx_login_logs_user_time ON login_logs(user_id, login_time DESC);

CREATE TABLE IF NOT EXISTS security_events (
    event_id SERIAL PRIMARY KEY,
    user_id VARCHAR(64),
    event_type VARCHAR(64) NOT NULL,
    severity VARCHAR(16) NOT NULL DEFAULT 'low',
    description TEXT,
    ip_address VARCHAR(45),
    user_agent TEXT,
    metadata JSONB,
    created_at TIMESTAMPTZ DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_security_events_user_time ON security_events(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_security_events_user_severity_time ON security_events(user_id, severity, created_at);

CREATE TABLE IF NOT EXISTS user_privacy_settings (
    user_id VARCHAR(64) PRIMARY KEY,
    profile_visibility VARCHAR(16) NOT NULL DEFAULT 'public',
    show_online_status BOOLEAN NOT NULL DEFAULT true,
    allow_friend_request BOOLEAN NOT NULL DEFAULT true,
    allow_message_from_stranger BOOLEAN NOT NULL DEFAULT false
);

-- compatibility view used by temp friend code paths
CREATE OR REPLACE VIEW temp_connections AS
SELECT * FROM connections;

-- widen recovery key hash column (idempotent)
DO $$
BEGIN
    IF EXISTS (
        SELECT 1
        FROM information_schema.columns c
        WHERE c.table_schema = 'public'
          AND c.table_name = 'users'
          AND c.column_name = 'recovery_key_hash'
    ) THEN
        BEGIN
            ALTER TABLE users
                ALTER COLUMN recovery_key_hash TYPE VARCHAR(256);
        EXCEPTION WHEN OTHERS THEN
            RAISE NOTICE 'skip alter users.recovery_key_hash: %', SQLERRM;
        END;
    END IF;
END $$;

COMMIT;
