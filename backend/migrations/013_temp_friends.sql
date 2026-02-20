-- Migration: Create temp_friends table for temporary friendship feature
-- UP

CREATE TABLE IF NOT EXISTS temp_friends (
    temp_friend_id VARCHAR(64) PRIMARY KEY,
    user_id_1 VARCHAR(64) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    user_id_2 VARCHAR(64) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    requester_id VARCHAR(64),
    source VARCHAR(32) NOT NULL DEFAULT 'chat',
    source_id VARCHAR(64),
    status VARCHAR(20) NOT NULL DEFAULT 'active',
    upgraded_to_friend BOOLEAN DEFAULT FALSE,
    expires_at TIMESTAMP NOT NULL,
    created_at TIMESTAMP DEFAULT NOW(),
    CONSTRAINT unique_temp_friendship UNIQUE (user_id_1, user_id_2)
);

CREATE INDEX IF NOT EXISTS idx_temp_friends_user1_status ON temp_friends(user_id_1, status);
CREATE INDEX IF NOT EXISTS idx_temp_friends_user2_status ON temp_friends(user_id_2, status);
CREATE INDEX IF NOT EXISTS idx_temp_friends_expires ON temp_friends(expires_at) WHERE status = 'active';

-- DOWN
-- DROP INDEX IF EXISTS idx_temp_friends_user1_status;
-- DROP INDEX IF EXISTS idx_temp_friends_user2_status;
-- DROP INDEX IF EXISTS idx_temp_friends_expires;
-- DROP TABLE IF EXISTS temp_friends;
