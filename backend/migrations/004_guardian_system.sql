-- Migration: Guardian incentive and emotion tracking system
-- Created for Task #12: 负重救助与生态运营

-- Emotion tracking table
CREATE TABLE IF NOT EXISTS emotion_tracking (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    score FLOAT NOT NULL,
    content_hash VARCHAR(64),
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_emotion_tracking_user ON emotion_tracking(user_id, created_at);

-- Resonance points table
CREATE TABLE IF NOT EXISTS resonance_points (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    points INT NOT NULL,
    reason VARCHAR(255),
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_resonance_points_user ON resonance_points(user_id);

-- Lamp transfers table
CREATE TABLE IF NOT EXISTS lamp_transfers (
    id SERIAL PRIMARY KEY,
    from_user_id VARCHAR(64) NOT NULL,
    to_user_id VARCHAR(64) NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Intervention log table (prevents duplicate interventions)
CREATE TABLE IF NOT EXISTS intervention_log (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    reason TEXT,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_intervention_log_user ON intervention_log(user_id, created_at);

-- Add guardian columns to users table
ALTER TABLE users ADD COLUMN IF NOT EXISTS resonance_total INT DEFAULT 0;
ALTER TABLE users ADD COLUMN IF NOT EXISTS is_guardian BOOLEAN DEFAULT FALSE;
ALTER TABLE users ADD COLUMN IF NOT EXISTS guardian_since TIMESTAMP;

-- DOWN
-- ALTER TABLE users DROP COLUMN IF EXISTS guardian_since;
-- ALTER TABLE users DROP COLUMN IF EXISTS is_guardian;
-- ALTER TABLE users DROP COLUMN IF EXISTS resonance_total;
-- DROP INDEX IF EXISTS idx_intervention_log_user;
-- DROP TABLE IF EXISTS intervention_log;
-- DROP TABLE IF EXISTS lamp_transfers;
-- DROP INDEX IF EXISTS idx_resonance_points_user;
-- DROP TABLE IF EXISTS resonance_points;
-- DROP INDEX IF EXISTS idx_emotion_tracking_user;
-- DROP TABLE IF EXISTS emotion_tracking;
