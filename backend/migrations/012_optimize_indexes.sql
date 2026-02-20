-- Migration: Add indexes for query optimization
-- UP

-- Friends table indexes (if table exists)
CREATE INDEX IF NOT EXISTS idx_friends_user_status ON friends(user_id, status);
CREATE INDEX IF NOT EXISTS idx_friends_friend_status ON friends(friend_id, status);
CREATE INDEX IF NOT EXISTS idx_friends_friendship ON friends(friendship_id);

-- Stones table composite indexes for common queries
CREATE INDEX IF NOT EXISTS idx_stones_status_created ON stones(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_stones_status_mood_created ON stones(status, mood_type, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_stones_user_status_created ON stones(user_id, status, created_at DESC);

-- DOWN
-- DROP INDEX IF EXISTS idx_friends_user_status;
-- DROP INDEX IF EXISTS idx_friends_friend_status;
-- DROP INDEX IF EXISTS idx_friends_friendship;
-- DROP INDEX IF EXISTS idx_stones_status_created;
-- DROP INDEX IF EXISTS idx_stones_status_mood_created;
-- DROP INDEX IF EXISTS idx_stones_user_status_created;
