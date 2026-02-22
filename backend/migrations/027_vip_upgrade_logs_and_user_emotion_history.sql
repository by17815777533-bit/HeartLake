-- VIP升级日志表
CREATE TABLE IF NOT EXISTS vip_upgrade_logs (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(100) NOT NULL,
    old_vip_level INT DEFAULT 0,
    new_vip_level INT DEFAULT 0,
    upgrade_type VARCHAR(50) DEFAULT 'auto_emotion',
    reason TEXT,
    expires_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_vip_upgrade_logs_user_id ON vip_upgrade_logs(user_id);
CREATE INDEX IF NOT EXISTS idx_vip_upgrade_logs_created_at ON vip_upgrade_logs(created_at);

-- 用户情绪历史表
CREATE TABLE IF NOT EXISTS user_emotion_history (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(100) NOT NULL,
    sentiment_score FLOAT DEFAULT 0,
    mood_type VARCHAR(50) DEFAULT 'neutral',
    content_snippet TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_user_emotion_history_user_id ON user_emotion_history(user_id);
CREATE INDEX IF NOT EXISTS idx_user_emotion_history_created_at ON user_emotion_history(created_at);
