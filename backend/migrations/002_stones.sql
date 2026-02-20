-- Migration: Base stones table
-- UP
CREATE TABLE IF NOT EXISTS stones (
    stone_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_stones_user ON stones(user_id);

-- DOWN
-- DROP INDEX IF EXISTS idx_stones_user;
-- DROP TABLE IF EXISTS stones;
