-- Migration 013: Schema fixes
-- 1. Create missing tables (user_sessions, login_logs, security_events, user_privacy_settings)
-- 2. Add missing indexes (lamp_transfers, user_interaction_history)
-- 3. Convert TIMESTAMP → TIMESTAMPTZ for consistency

BEGIN;

-- ============================================================
-- 1. Missing tables
-- ============================================================

-- 1.1 user_sessions (referenced by AccountController, AdminController)
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

-- 1.2 login_logs (referenced by AccountController)
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

-- 1.3 security_events (referenced by AccountController, SecurityLogger)
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

-- 1.4 user_privacy_settings (referenced by AccountController)
CREATE TABLE IF NOT EXISTS user_privacy_settings (
    user_id VARCHAR(64) PRIMARY KEY,
    profile_visibility VARCHAR(16) NOT NULL DEFAULT 'public',
    show_online_status BOOLEAN NOT NULL DEFAULT true,
    allow_friend_request BOOLEAN NOT NULL DEFAULT true,
    allow_message_from_stranger BOOLEAN NOT NULL DEFAULT false
);

-- 1.5 temp_connections view (code uses temp_connections, migration 004 created connections)
CREATE OR REPLACE VIEW temp_connections AS SELECT * FROM connections;

-- ============================================================
-- 2. Missing indexes + foreign keys
-- ============================================================

-- Foreign keys for core tables (IF NOT EXISTS via DO block)
DO $$ BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'fk_stones_user') THEN
        ALTER TABLE stones ADD CONSTRAINT fk_stones_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'fk_notifications_user') THEN
        ALTER TABLE notifications ADD CONSTRAINT fk_notifications_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'fk_emotion_tracking_user') THEN
        ALTER TABLE emotion_tracking ADD CONSTRAINT fk_emotion_tracking_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'fk_uih_user') THEN
        ALTER TABLE user_interaction_history ADD CONSTRAINT fk_uih_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'fk_uih_stone') THEN
        ALTER TABLE user_interaction_history ADD CONSTRAINT fk_uih_stone FOREIGN KEY (stone_id) REFERENCES stones(stone_id) ON DELETE CASCADE;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'fk_resonance_points_user') THEN
        ALTER TABLE resonance_points ADD CONSTRAINT fk_resonance_points_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'fk_vip_upgrade_logs_user') THEN
        ALTER TABLE vip_upgrade_logs ADD CONSTRAINT fk_vip_upgrade_logs_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'fk_data_export_tasks_user') THEN
        ALTER TABLE data_export_tasks ADD CONSTRAINT fk_data_export_tasks_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE;
    END IF;
END $$;

-- lamp_transfers: queries filter by from_user_id and to_user_id
CREATE INDEX IF NOT EXISTS idx_lamp_transfers_from ON lamp_transfers(from_user_id);
CREATE INDEX IF NOT EXISTS idx_lamp_transfers_to ON lamp_transfers(to_user_id);

-- user_interaction_history: composite indexes for recommendation queries
-- itemBasedCF CTE: WHERE user_id = $1 ORDER BY interaction_weight DESC, created_at DESC
CREATE INDEX IF NOT EXISTS idx_uih_user_weight_time ON user_interaction_history(user_id, interaction_weight DESC, created_at DESC);
-- NOT EXISTS subquery: WHERE stone_id = ... AND user_id = ...
CREATE INDEX IF NOT EXISTS idx_uih_stone_user ON user_interaction_history(stone_id, user_id);

-- ============================================================
-- 3. TIMESTAMP → TIMESTAMPTZ conversion
-- ============================================================

-- 001_users.sql: users
ALTER TABLE users
    ALTER COLUMN vip_expires_at TYPE TIMESTAMPTZ USING vip_expires_at AT TIME ZONE 'UTC',
    ALTER COLUMN guardian_since TYPE TIMESTAMPTZ USING guardian_since AT TIME ZONE 'UTC',
    ALTER COLUMN last_login_at TYPE TIMESTAMPTZ USING last_login_at AT TIME ZONE 'UTC',
    ALTER COLUMN last_active_at TYPE TIMESTAMPTZ USING last_active_at AT TIME ZONE 'UTC',
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC',
    ALTER COLUMN updated_at TYPE TIMESTAMPTZ USING updated_at AT TIME ZONE 'UTC';

-- 001_users.sql: user_similarity
ALTER TABLE user_similarity
    ALTER COLUMN updated_at TYPE TIMESTAMPTZ USING updated_at AT TIME ZONE 'UTC';

-- 002_stones.sql: stones
ALTER TABLE stones
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC',
    ALTER COLUMN updated_at TYPE TIMESTAMPTZ USING updated_at AT TIME ZONE 'UTC',
    ALTER COLUMN deleted_at TYPE TIMESTAMPTZ USING deleted_at AT TIME ZONE 'UTC';

-- 002_stones.sql: stone_embeddings
ALTER TABLE stone_embeddings
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 003_social.sql: friends
ALTER TABLE friends
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 003_social.sql: temp_friends
ALTER TABLE temp_friends
    ALTER COLUMN expires_at TYPE TIMESTAMPTZ USING expires_at AT TIME ZONE 'UTC',
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 003_social.sql: friend_messages
ALTER TABLE friend_messages
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 004_interactions.sql: ripples
ALTER TABLE ripples
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 004_interactions.sql: paper_boats
ALTER TABLE paper_boats
    ALTER COLUMN response_at TYPE TIMESTAMPTZ USING response_at AT TIME ZONE 'UTC',
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC',
    ALTER COLUMN updated_at TYPE TIMESTAMPTZ USING updated_at AT TIME ZONE 'UTC';

-- 004_interactions.sql: connections
ALTER TABLE connections
    ALTER COLUMN expires_at TYPE TIMESTAMPTZ USING expires_at AT TIME ZONE 'UTC',
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 004_interactions.sql: connection_messages
ALTER TABLE connection_messages
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 005_ai_system.sql: user_interaction_history
ALTER TABLE user_interaction_history
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 006_guardian.sql: emotion_tracking
ALTER TABLE emotion_tracking
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 006_guardian.sql: resonance_points
ALTER TABLE resonance_points
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 006_guardian.sql: lamp_transfers
ALTER TABLE lamp_transfers
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 006_guardian.sql: intervention_log
ALTER TABLE intervention_log
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 007_admin.sql: admin_users
ALTER TABLE admin_users
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC',
    ALTER COLUMN last_login_at TYPE TIMESTAMPTZ USING last_login_at AT TIME ZONE 'UTC';

-- 007_admin.sql: sensitive_words
ALTER TABLE sensitive_words
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 007_admin.sql: operation_logs
ALTER TABLE operation_logs
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 008_notifications.sql: notifications
ALTER TABLE notifications
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 009_consultation.sql: consultation_sessions
ALTER TABLE consultation_sessions
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC',
    ALTER COLUMN ended_at TYPE TIMESTAMPTZ USING ended_at AT TIME ZONE 'UTC';

-- 009_consultation.sql: consultation_messages
ALTER TABLE consultation_messages
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

-- 010_data_export.sql: data_export_tasks
ALTER TABLE data_export_tasks
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC',
    ALTER COLUMN completed_at TYPE TIMESTAMPTZ USING completed_at AT TIME ZONE 'UTC',
    ALTER COLUMN expires_at TYPE TIMESTAMPTZ USING expires_at AT TIME ZONE 'UTC';

-- 011_performance_schema_hardening.sql: user_followups
ALTER TABLE user_followups
    ALTER COLUMN created_at TYPE TIMESTAMPTZ USING created_at AT TIME ZONE 'UTC';

COMMIT;
