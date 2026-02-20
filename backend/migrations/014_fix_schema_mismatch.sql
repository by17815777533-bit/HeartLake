-- Migration: Fix schema mismatches between migrations and Repository code
-- UP

-- 1. Fix users table: add missing columns expected by UserRepository
ALTER TABLE users ADD COLUMN IF NOT EXISTS user_id VARCHAR(64) UNIQUE;
ALTER TABLE users ADD COLUMN IF NOT EXISTS username VARCHAR(64) UNIQUE;
ALTER TABLE users ADD COLUMN IF NOT EXISTS nickname VARCHAR(128);
ALTER TABLE users ADD COLUMN IF NOT EXISTS email VARCHAR(255);
ALTER TABLE users ADD COLUMN IF NOT EXISTS is_anonymous BOOLEAN DEFAULT TRUE;
ALTER TABLE users ADD COLUMN IF NOT EXISTS status VARCHAR(20) DEFAULT 'active';
ALTER TABLE users ADD COLUMN IF NOT EXISTS last_active_at TIMESTAMP;

-- Populate user_id from id for existing records
UPDATE users SET user_id = id WHERE user_id IS NULL;

-- 2. Fix stones table: add missing columns expected by StoneRepository
ALTER TABLE stones ADD COLUMN IF NOT EXISTS stone_type VARCHAR(32) DEFAULT 'text';
ALTER TABLE stones ADD COLUMN IF NOT EXISTS stone_color VARCHAR(32) DEFAULT 'blue';
ALTER TABLE stones ADD COLUMN IF NOT EXISTS mood_type VARCHAR(32) DEFAULT 'neutral';
ALTER TABLE stones ADD COLUMN IF NOT EXISTS is_anonymous BOOLEAN DEFAULT TRUE;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS ripple_count INT DEFAULT 0;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS boat_count INT DEFAULT 0;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS views_count INT DEFAULT 0;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS status VARCHAR(20) DEFAULT 'published';
ALTER TABLE stones ADD COLUMN IF NOT EXISTS emotion_score FLOAT;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS updated_at TIMESTAMP DEFAULT NOW();

-- 3. Create friends table expected by FriendRepository
CREATE TABLE IF NOT EXISTS friends (
    friendship_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    friend_id VARCHAR(64) NOT NULL,
    status VARCHAR(20) DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT NOW(),
    CONSTRAINT unique_friendship UNIQUE (user_id, friend_id)
);

CREATE INDEX IF NOT EXISTS idx_friends_user_status ON friends(user_id, status);
CREATE INDEX IF NOT EXISTS idx_friends_friend_status ON friends(friend_id, status);

-- DOWN
-- DROP INDEX IF EXISTS idx_friends_friend_status;
-- DROP INDEX IF EXISTS idx_friends_user_status;
-- DROP TABLE IF EXISTS friends;
-- ALTER TABLE stones DROP COLUMN IF EXISTS updated_at;
-- ALTER TABLE stones DROP COLUMN IF EXISTS emotion_score;
-- ALTER TABLE stones DROP COLUMN IF EXISTS status;
-- ALTER TABLE stones DROP COLUMN IF EXISTS views_count;
-- ALTER TABLE stones DROP COLUMN IF EXISTS boat_count;
-- ALTER TABLE stones DROP COLUMN IF EXISTS ripple_count;
-- ALTER TABLE stones DROP COLUMN IF EXISTS is_anonymous;
-- ALTER TABLE stones DROP COLUMN IF EXISTS mood_type;
-- ALTER TABLE stones DROP COLUMN IF EXISTS stone_color;
-- ALTER TABLE stones DROP COLUMN IF EXISTS stone_type;
-- ALTER TABLE users DROP COLUMN IF EXISTS last_active_at;
-- ALTER TABLE users DROP COLUMN IF EXISTS status;
-- ALTER TABLE users DROP COLUMN IF EXISTS is_anonymous;
-- ALTER TABLE users DROP COLUMN IF EXISTS email;
-- ALTER TABLE users DROP COLUMN IF EXISTS nickname;
-- ALTER TABLE users DROP COLUMN IF EXISTS username;
-- ALTER TABLE users DROP COLUMN IF EXISTS user_id;
