-- 025: 湖神AI对话消息表
-- 存储用户与湖神的对话历史，支持消息持久化和历史回溯

CREATE TABLE IF NOT EXISTS lake_god_messages (
    id BIGSERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    role VARCHAR(20) NOT NULL CHECK (role IN ('user', 'assistant')),
    content TEXT NOT NULL,
    mood VARCHAR(50),
    emotion_score REAL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_lake_god_messages_user_id ON lake_god_messages(user_id);
CREATE INDEX idx_lake_god_messages_user_created ON lake_god_messages(user_id, created_at DESC);
