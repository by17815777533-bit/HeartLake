-- Migration: Complete schema for stones table + missing tables
-- Adds columns referenced by backend code but missing from 002_stones.sql

-- ============================================================
-- 1. stones 表补充列
-- ============================================================
ALTER TABLE stones ADD COLUMN IF NOT EXISTS stone_type VARCHAR(20) DEFAULT 'medium';
ALTER TABLE stones ADD COLUMN IF NOT EXISTS stone_color VARCHAR(10) DEFAULT '#ADA59E';
ALTER TABLE stones ADD COLUMN IF NOT EXISTS mood_type VARCHAR(20);
ALTER TABLE stones ADD COLUMN IF NOT EXISTS is_anonymous BOOLEAN DEFAULT TRUE;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS emotion_score FLOAT;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS sentiment_score FLOAT;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS ripple_count INT DEFAULT 0;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS boat_count INT DEFAULT 0;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS view_count INT DEFAULT 0;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS status VARCHAR(20) DEFAULT 'published';
ALTER TABLE stones ADD COLUMN IF NOT EXISTS updated_at TIMESTAMP;
ALTER TABLE stones ADD COLUMN IF NOT EXISTS tags TEXT[];
ALTER TABLE stones ADD COLUMN IF NOT EXISTS media_ids TEXT[];
ALTER TABLE stones ADD COLUMN IF NOT EXISTS ai_tags TEXT[];
ALTER TABLE stones ADD COLUMN IF NOT EXISTS nickname VARCHAR(64);

CREATE INDEX IF NOT EXISTS idx_stones_created ON stones(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_stones_mood ON stones(mood_type);
CREATE INDEX IF NOT EXISTS idx_stones_status ON stones(status);

-- ============================================================
-- 2. users 表补充列（后端代码引用）
-- ============================================================
ALTER TABLE users ADD COLUMN IF NOT EXISTS nickname VARCHAR(64);
ALTER TABLE users ADD COLUMN IF NOT EXISTS avatar_url TEXT;
ALTER TABLE users ADD COLUMN IF NOT EXISTS bio TEXT;
ALTER TABLE users ADD COLUMN IF NOT EXISTS device_id VARCHAR(128);
ALTER TABLE users ADD COLUMN IF NOT EXISTS recovery_key VARCHAR(128);
ALTER TABLE users ADD COLUMN IF NOT EXISTS user_id VARCHAR(64);
ALTER TABLE users ADD COLUMN IF NOT EXISTS role VARCHAR(20) DEFAULT 'user';
ALTER TABLE users ADD COLUMN IF NOT EXISTS status VARCHAR(20) DEFAULT 'active';
ALTER TABLE users ADD COLUMN IF NOT EXISTS last_login_at TIMESTAMP;
ALTER TABLE users ADD COLUMN IF NOT EXISTS updated_at TIMESTAMP;

-- 确保 user_id 列与 id 同步（兼容两种引用方式）
UPDATE users SET user_id = id WHERE user_id IS NULL;
CREATE UNIQUE INDEX IF NOT EXISTS idx_users_user_id ON users(user_id);

-- ============================================================
-- 3. ripples 涟漪表
-- ============================================================
CREATE TABLE IF NOT EXISTS ripples (
    ripple_id VARCHAR(64) PRIMARY KEY,
    stone_id VARCHAR(64) NOT NULL REFERENCES stones(stone_id) ON DELETE CASCADE,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(stone_id, user_id)
);
CREATE INDEX IF NOT EXISTS idx_ripples_stone ON ripples(stone_id);
CREATE INDEX IF NOT EXISTS idx_ripples_user ON ripples(user_id);

-- ============================================================
-- 4. paper_boats 纸船表
-- ============================================================
CREATE TABLE IF NOT EXISTS paper_boats (
    boat_id VARCHAR(64) PRIMARY KEY,
    stone_id VARCHAR(64) REFERENCES stones(stone_id) ON DELETE SET NULL,
    sender_id VARCHAR(64) NOT NULL,
    receiver_id VARCHAR(64),
    content TEXT NOT NULL,
    mood VARCHAR(20) DEFAULT 'hopeful',
    drift_mode VARCHAR(20) DEFAULT 'random',
    boat_style VARCHAR(20) DEFAULT 'paper',
    is_anonymous BOOLEAN DEFAULT TRUE,
    status VARCHAR(20) DEFAULT 'active',
    response_content TEXT,
    response_at TIMESTAMP,
    drift_delay_seconds INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_paper_boats_stone ON paper_boats(stone_id);
CREATE INDEX IF NOT EXISTS idx_paper_boats_sender ON paper_boats(sender_id);
CREATE INDEX IF NOT EXISTS idx_paper_boats_receiver ON paper_boats(receiver_id);
CREATE INDEX IF NOT EXISTS idx_paper_boats_status ON paper_boats(status);

-- ============================================================
-- 5. lake_god_messages 湖神消息表
-- ============================================================
CREATE TABLE IF NOT EXISTS lake_god_messages (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    role VARCHAR(20) NOT NULL,  -- 'user' or 'assistant'
    content TEXT NOT NULL,
    mood VARCHAR(20),
    emotion_score FLOAT,
    rag_context TEXT,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_lake_god_messages_user ON lake_god_messages(user_id, created_at);

-- ============================================================
-- 6. user_emotion_profile 用户情绪画像表
-- ============================================================
CREATE TABLE IF NOT EXISTS user_emotion_profile (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    date DATE NOT NULL DEFAULT CURRENT_DATE,
    mood_type VARCHAR(20),
    avg_emotion_score FLOAT DEFAULT 0.0,
    stone_count INT DEFAULT 0,
    dominant_emotion VARCHAR(20),
    updated_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(user_id, date)
);
CREATE INDEX IF NOT EXISTS idx_user_emotion_profile_user ON user_emotion_profile(user_id, date DESC);

-- ============================================================
-- 7. temp_connections 临时连接表（后端代码使用 temp_connections）
-- ============================================================
CREATE TABLE IF NOT EXISTS temp_connections (
    connection_id VARCHAR(64) PRIMARY KEY,
    stone_id VARCHAR(64) REFERENCES stones(stone_id) ON DELETE SET NULL,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    target_user_id VARCHAR(64) REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) DEFAULT 'active',
    created_at TIMESTAMP DEFAULT NOW(),
    expires_at TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_temp_connections_user ON temp_connections(user_id);
CREATE INDEX IF NOT EXISTS idx_temp_connections_target ON temp_connections(target_user_id);
CREATE INDEX IF NOT EXISTS idx_temp_connections_status ON temp_connections(status);

-- ============================================================
-- 8. connection_messages 连接消息表
-- ============================================================
CREATE TABLE IF NOT EXISTS connection_messages (
    message_id VARCHAR(64) PRIMARY KEY,
    connection_id VARCHAR(64) NOT NULL REFERENCES temp_connections(connection_id) ON DELETE CASCADE,
    sender_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_connection_messages_conn ON connection_messages(connection_id, created_at);

-- ============================================================
-- 9. friends 好友关系表
-- ============================================================
CREATE TABLE IF NOT EXISTS friends (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    friend_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(user_id, friend_id)
);
CREATE INDEX IF NOT EXISTS idx_friends_user ON friends(user_id);
CREATE INDEX IF NOT EXISTS idx_friends_friend ON friends(friend_id);

-- ============================================================
-- 10. reports 举报表
-- ============================================================
CREATE TABLE IF NOT EXISTS reports (
    report_id VARCHAR(64) PRIMARY KEY,
    reporter_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    target_type VARCHAR(20) NOT NULL,
    target_id VARCHAR(64) NOT NULL,
    reason VARCHAR(255),
    description TEXT,
    status VARCHAR(20) DEFAULT 'pending',
    handled_by VARCHAR(64),
    handled_at TIMESTAMP,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_reports_status ON reports(status);
CREATE INDEX IF NOT EXISTS idx_reports_reporter ON reports(reporter_id);

-- ============================================================
-- 11. vip_subscriptions VIP订阅表
-- ============================================================
CREATE TABLE IF NOT EXISTS vip_subscriptions (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    plan_type VARCHAR(20) NOT NULL DEFAULT 'monthly',
    started_at TIMESTAMP DEFAULT NOW(),
    expires_at TIMESTAMP NOT NULL,
    status VARCHAR(20) DEFAULT 'active',
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_vip_user ON vip_subscriptions(user_id);

-- ============================================================
-- 12. media_files 媒体文件表
-- ============================================================
CREATE TABLE IF NOT EXISTS media_files (
    media_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    file_type VARCHAR(20) NOT NULL,
    file_url TEXT NOT NULL,
    file_size BIGINT,
    mime_type VARCHAR(64),
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_media_user ON media_files(user_id);

-- ============================================================
-- 13. admin_users 管理员表
-- ============================================================
CREATE TABLE IF NOT EXISTS admin_users (
    id VARCHAR(64) PRIMARY KEY,
    username VARCHAR(64) UNIQUE NOT NULL,
    password_hash VARCHAR(256) NOT NULL,
    role VARCHAR(20) DEFAULT 'admin',
    created_at TIMESTAMP DEFAULT NOW(),
    last_login_at TIMESTAMP
);

-- ============================================================
-- 14. sensitive_words 敏感词表
-- ============================================================
CREATE TABLE IF NOT EXISTS sensitive_words (
    id SERIAL PRIMARY KEY,
    word VARCHAR(255) NOT NULL,
    category VARCHAR(20) DEFAULT 'general',
    level INT DEFAULT 1,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_sensitive_words_active ON sensitive_words(is_active);

-- ============================================================
-- 15. operation_logs 操作日志表
-- ============================================================
CREATE TABLE IF NOT EXISTS operation_logs (
    id SERIAL PRIMARY KEY,
    admin_id VARCHAR(64),
    action VARCHAR(64) NOT NULL,
    target_type VARCHAR(32),
    target_id VARCHAR(64),
    detail TEXT,
    ip_address VARCHAR(45),
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_operation_logs_admin ON operation_logs(admin_id, created_at DESC);
