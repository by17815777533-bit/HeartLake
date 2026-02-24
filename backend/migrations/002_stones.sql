-- 002: 石头表（心湖核心实体）
-- 注意: 生产环境建议将 CREATE INDEX 改为 CREATE INDEX CONCURRENTLY 以避免锁表
BEGIN;

CREATE TABLE IF NOT EXISTS stones (
    stone_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    content TEXT NOT NULL,
    stone_type VARCHAR(20) DEFAULT 'medium',
    stone_color VARCHAR(10) DEFAULT '#ADA59E',
    mood_type VARCHAR(20) DEFAULT 'neutral',
    is_anonymous BOOLEAN DEFAULT TRUE,
    nickname VARCHAR(64),
    emotion_score FLOAT,
    sentiment_score FLOAT,
    ripple_count INT DEFAULT 0,
    boat_count INT DEFAULT 0,
    view_count INT DEFAULT 0,
    status VARCHAR(20) DEFAULT 'published',
    tags TEXT[],
    media_ids TEXT[],
    ai_tags TEXT[],
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP
);

ALTER TABLE stones ADD COLUMN IF NOT EXISTS deleted_at TIMESTAMP;

CREATE INDEX IF NOT EXISTS idx_stones_user ON stones(user_id);
CREATE INDEX IF NOT EXISTS idx_stones_created ON stones(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_stones_mood ON stones(mood_type);
CREATE INDEX IF NOT EXISTS idx_stones_status ON stones(status);
CREATE INDEX IF NOT EXISTS idx_stones_status_created ON stones(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_stones_status_mood_created ON stones(status, mood_type, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_stones_user_status_created ON stones(user_id, status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_stones_deleted_at ON stones(deleted_at);

-- 石头向量嵌入表（HNSW向量索引用）
CREATE TABLE IF NOT EXISTS stone_embeddings (
    stone_id VARCHAR(64) PRIMARY KEY REFERENCES stones(stone_id) ON DELETE CASCADE,
    embedding TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_stone_embeddings_created ON stone_embeddings(created_at);

COMMIT;
