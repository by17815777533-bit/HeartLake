-- 003: 社交系统（好友、临时好友、消息）
-- 注意: 生产环境建议将 CREATE INDEX 改为 CREATE INDEX CONCURRENTLY 以避免锁表

-- 好友关系表
CREATE TABLE IF NOT EXISTS friends (
    friendship_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    friend_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT NOW(),
    CONSTRAINT unique_friendship UNIQUE (user_id, friend_id)
);

CREATE INDEX IF NOT EXISTS idx_friends_user_status ON friends(user_id, status);
CREATE INDEX IF NOT EXISTS idx_friends_friend_status ON friends(friend_id, status);

-- 临时好友表（纸船/石头互动产生的临时关系，有过期时间）
CREATE TABLE IF NOT EXISTS temp_friends (
    temp_friend_id VARCHAR(64) PRIMARY KEY,
    user1_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    user2_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    requester_id VARCHAR(64),
    source VARCHAR(32) NOT NULL DEFAULT 'chat',
    source_id VARCHAR(64),
    status VARCHAR(20) NOT NULL DEFAULT 'active',
    upgraded_to_friend BOOLEAN DEFAULT FALSE,
    expires_at TIMESTAMP NOT NULL,
    created_at TIMESTAMP DEFAULT NOW(),
    CONSTRAINT unique_temp_friendship UNIQUE (user1_id, user2_id)
);

CREATE INDEX IF NOT EXISTS idx_temp_friends_user1_status ON temp_friends(user1_id, status);
CREATE INDEX IF NOT EXISTS idx_temp_friends_user2_status ON temp_friends(user2_id, status);
CREATE INDEX IF NOT EXISTS idx_temp_friends_expires ON temp_friends(expires_at) WHERE status = 'active';

-- 好友消息表
CREATE TABLE IF NOT EXISTS friend_messages (
    id SERIAL PRIMARY KEY,
    sender_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    receiver_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_friend_messages_sender ON friend_messages(sender_id);
CREATE INDEX IF NOT EXISTS idx_friend_messages_receiver ON friend_messages(receiver_id);
