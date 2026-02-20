-- Migration: Base users table
-- UP
CREATE TABLE IF NOT EXISTS users (
    id VARCHAR(64) PRIMARY KEY,
    shadow_id VARCHAR(64) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_users_shadow ON users(shadow_id);

-- DOWN
-- DROP INDEX IF EXISTS idx_users_shadow;
-- DROP TABLE IF EXISTS users;
