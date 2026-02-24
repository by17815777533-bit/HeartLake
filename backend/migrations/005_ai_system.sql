-- 005: AI系统（情感分析、向量索引、湖神对话、推荐、联邦学习、差分隐私）
-- 注意: 生产环境建议将 CREATE INDEX 改为 CREATE INDEX CONCURRENTLY 以避免锁表
BEGIN;

-- 湖神AI对话消息表
CREATE TABLE IF NOT EXISTS lake_god_messages (
    id BIGSERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    role VARCHAR(20) NOT NULL CHECK (role IN ('user', 'assistant')),
    content TEXT NOT NULL,
    mood VARCHAR(50),
    emotion_score REAL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_lake_god_messages_user ON lake_god_messages(user_id);
CREATE INDEX IF NOT EXISTS idx_lake_god_messages_user_created ON lake_god_messages(user_id, created_at DESC);

-- 用户情绪历史表
CREATE TABLE IF NOT EXISTS user_emotion_history (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    sentiment_score FLOAT DEFAULT 0,
    mood_type VARCHAR(50) DEFAULT 'neutral',
    content_snippet TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_user_emotion_history_user ON user_emotion_history(user_id);
CREATE INDEX IF NOT EXISTS idx_user_emotion_history_created ON user_emotion_history(created_at);

-- 用户情绪画像视图（基于stones聚合）
CREATE OR REPLACE VIEW user_emotion_profile AS
SELECT
    s.user_id,
    s.mood_type,
    COALESCE(AVG(s.emotion_score), 0.5) AS avg_emotion_score,
    DATE(s.created_at) AS date
FROM stones s
WHERE s.status = 'published'
GROUP BY s.user_id, s.mood_type, DATE(s.created_at)
ORDER BY date DESC;

-- 用户互动历史表（推荐系统用）
CREATE TABLE IF NOT EXISTS user_interaction_history (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    stone_id VARCHAR(64) NOT NULL,
    interaction_type VARCHAR(32) NOT NULL DEFAULT 'view',
    interaction_weight DOUBLE PRECISION DEFAULT 1.0,
    dwell_time_seconds INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(user_id, stone_id, interaction_type)
);

CREATE INDEX IF NOT EXISTS idx_uih_user ON user_interaction_history(user_id);
CREATE INDEX IF NOT EXISTS idx_uih_stone ON user_interaction_history(stone_id);

-- 情绪兼容性表（推荐系统用）
CREATE TABLE IF NOT EXISTS emotion_compatibility (
    id SERIAL PRIMARY KEY,
    mood_type_1 VARCHAR(32) NOT NULL,
    mood_type_2 VARCHAR(32) NOT NULL,
    compatibility_score DOUBLE PRECISION NOT NULL DEFAULT 0.5,
    relationship_type VARCHAR(32) DEFAULT 'neutral',
    UNIQUE(mood_type_1, mood_type_2)
);

INSERT INTO emotion_compatibility (mood_type_1, mood_type_2, compatibility_score, relationship_type) VALUES
('happy', 'happy', 0.95, 'resonance'), ('happy', 'calm', 0.80, 'harmony'),
('happy', 'hopeful', 0.90, 'uplift'), ('happy', 'grateful', 0.85, 'warmth'),
('happy', 'sad', 0.65, 'comfort'), ('happy', 'anxious', 0.60, 'soothe'),
('happy', 'angry', 0.50, 'contrast'), ('happy', 'lonely', 0.70, 'companion'),
('calm', 'calm', 0.90, 'resonance'), ('calm', 'sad', 0.75, 'gentle'),
('calm', 'anxious', 0.70, 'soothe'), ('calm', 'hopeful', 0.85, 'harmony'),
('calm', 'grateful', 0.80, 'warmth'), ('calm', 'angry', 0.55, 'balance'),
('calm', 'lonely', 0.75, 'companion'), ('sad', 'sad', 0.85, 'empathy'),
('sad', 'lonely', 0.80, 'empathy'), ('sad', 'hopeful', 0.75, 'uplift'),
('sad', 'grateful', 0.65, 'warmth'), ('sad', 'anxious', 0.70, 'empathy'),
('sad', 'angry', 0.55, 'contrast'), ('anxious', 'anxious', 0.80, 'empathy'),
('anxious', 'hopeful', 0.75, 'uplift'), ('anxious', 'lonely', 0.70, 'empathy'),
('anxious', 'grateful', 0.65, 'soothe'), ('anxious', 'angry', 0.60, 'tension'),
('angry', 'angry', 0.70, 'empathy'), ('angry', 'lonely', 0.60, 'contrast'),
('angry', 'hopeful', 0.65, 'uplift'), ('angry', 'grateful', 0.55, 'balance'),
('lonely', 'lonely', 0.85, 'empathy'), ('lonely', 'hopeful', 0.80, 'uplift'),
('lonely', 'grateful', 0.70, 'warmth'), ('hopeful', 'hopeful', 0.90, 'resonance'),
('hopeful', 'grateful', 0.85, 'warmth'), ('grateful', 'grateful', 0.90, 'resonance')
ON CONFLICT DO NOTHING;

-- 边缘AI推理日志
CREATE TABLE IF NOT EXISTS edge_ai_inference_logs (
    id BIGSERIAL PRIMARY KEY,
    user_id VARCHAR(64),
    inference_type VARCHAR(32) NOT NULL,
    input_hash VARCHAR(64) NOT NULL,
    result JSONB NOT NULL,
    latency_ms INTEGER NOT NULL,
    model_version VARCHAR(32),
    is_fallback BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_edge_ai_logs_type ON edge_ai_inference_logs(inference_type);
CREATE INDEX IF NOT EXISTS idx_edge_ai_logs_created ON edge_ai_inference_logs(created_at);
CREATE INDEX IF NOT EXISTS idx_edge_ai_logs_user ON edge_ai_inference_logs(user_id);

-- 联邦学习轮次记录
CREATE TABLE IF NOT EXISTS federated_learning_rounds (
    id BIGSERIAL PRIMARY KEY,
    round_number INTEGER NOT NULL,
    participants_count INTEGER NOT NULL,
    model_type VARCHAR(64) NOT NULL,
    aggregation_method VARCHAR(32) DEFAULT 'fedavg',
    global_model_hash VARCHAR(128),
    metrics JSONB,
    privacy_budget_spent DOUBLE PRECISION DEFAULT 0,
    started_at TIMESTAMPTZ DEFAULT NOW(),
    completed_at TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS idx_fl_rounds_number ON federated_learning_rounds(round_number);

-- 差分隐私预算追踪
CREATE TABLE IF NOT EXISTS differential_privacy_budget (
    id BIGSERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    query_type VARCHAR(64) NOT NULL,
    epsilon_spent DOUBLE PRECISION NOT NULL,
    delta_spent DOUBLE PRECISION DEFAULT 0,
    noise_mechanism VARCHAR(32) DEFAULT 'laplace',
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_dp_budget_user ON differential_privacy_budget(user_id);
CREATE INDEX IF NOT EXISTS idx_dp_budget_created ON differential_privacy_budget(created_at);

-- 社区情绪脉搏快照
CREATE TABLE IF NOT EXISTS community_emotion_snapshots (
    id BIGSERIAL PRIMARY KEY,
    snapshot_time TIMESTAMPTZ DEFAULT NOW(),
    total_posts INTEGER DEFAULT 0,
    positive_count INTEGER DEFAULT 0,
    neutral_count INTEGER DEFAULT 0,
    negative_count INTEGER DEFAULT 0,
    avg_sentiment DOUBLE PRECISION DEFAULT 0,
    dominant_emotion VARCHAR(32),
    emotion_distribution JSONB,
    window_minutes INTEGER DEFAULT 60
);

CREATE INDEX IF NOT EXISTS idx_emotion_snapshots_time ON community_emotion_snapshots(snapshot_time);

-- 边缘节点状态
CREATE TABLE IF NOT EXISTS edge_nodes (
    id BIGSERIAL PRIMARY KEY,
    node_id VARCHAR(64) UNIQUE NOT NULL,
    node_name VARCHAR(128),
    status VARCHAR(16) DEFAULT 'active',
    last_heartbeat TIMESTAMPTZ DEFAULT NOW(),
    capabilities JSONB,
    load_score DOUBLE PRECISION DEFAULT 0,
    total_inferences BIGINT DEFAULT 0,
    avg_latency_ms DOUBLE PRECISION DEFAULT 0,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_edge_nodes_status ON edge_nodes(status);
CREATE INDEX IF NOT EXISTS idx_edge_nodes_heartbeat ON edge_nodes(last_heartbeat);

-- 向量索引元数据
CREATE TABLE IF NOT EXISTS vector_index_metadata (
    id BIGSERIAL PRIMARY KEY,
    index_name VARCHAR(128) UNIQUE NOT NULL,
    dimension INTEGER NOT NULL,
    total_vectors BIGINT DEFAULT 0,
    index_type VARCHAR(32) DEFAULT 'hnsw',
    build_params JSONB,
    last_rebuilt_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

COMMIT;
