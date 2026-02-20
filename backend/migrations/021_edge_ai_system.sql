-- Edge AI System Tables
-- 边缘AI系统数据表

-- 边缘AI推理日志
CREATE TABLE IF NOT EXISTS edge_ai_inference_logs (
    id BIGSERIAL PRIMARY KEY,
    user_id VARCHAR(64),
    inference_type VARCHAR(32) NOT NULL,  -- sentiment, moderation, embedding, vector_search
    input_hash VARCHAR(64) NOT NULL,
    result JSONB NOT NULL,
    latency_ms INTEGER NOT NULL,
    model_version VARCHAR(32),
    is_fallback BOOLEAN DEFAULT FALSE,  -- 是否降级到云端
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

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
    started_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    completed_at TIMESTAMP WITH TIME ZONE
);

-- 差分隐私预算追踪
CREATE TABLE IF NOT EXISTS differential_privacy_budget (
    id BIGSERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    query_type VARCHAR(64) NOT NULL,
    epsilon_spent DOUBLE PRECISION NOT NULL,
    delta_spent DOUBLE PRECISION DEFAULT 0,
    noise_mechanism VARCHAR(32) DEFAULT 'laplace',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- 社区情绪脉搏快照
CREATE TABLE IF NOT EXISTS community_emotion_snapshots (
    id BIGSERIAL PRIMARY KEY,
    snapshot_time TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    total_posts INTEGER DEFAULT 0,
    positive_count INTEGER DEFAULT 0,
    neutral_count INTEGER DEFAULT 0,
    negative_count INTEGER DEFAULT 0,
    avg_sentiment DOUBLE PRECISION DEFAULT 0,
    dominant_emotion VARCHAR(32),
    emotion_distribution JSONB,
    window_minutes INTEGER DEFAULT 60
);

-- 边缘节点状态
CREATE TABLE IF NOT EXISTS edge_nodes (
    id BIGSERIAL PRIMARY KEY,
    node_id VARCHAR(64) UNIQUE NOT NULL,
    node_name VARCHAR(128),
    status VARCHAR(16) DEFAULT 'active',  -- active, degraded, offline
    last_heartbeat TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    capabilities JSONB,
    load_score DOUBLE PRECISION DEFAULT 0,
    total_inferences BIGINT DEFAULT 0,
    avg_latency_ms DOUBLE PRECISION DEFAULT 0,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- 向量索引元数据
CREATE TABLE IF NOT EXISTS vector_index_metadata (
    id BIGSERIAL PRIMARY KEY,
    index_name VARCHAR(128) UNIQUE NOT NULL,
    dimension INTEGER NOT NULL,
    total_vectors BIGINT DEFAULT 0,
    index_type VARCHAR(32) DEFAULT 'hnsw',
    build_params JSONB,
    last_rebuilt_at TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- 索引优化
CREATE INDEX IF NOT EXISTS idx_edge_ai_logs_type ON edge_ai_inference_logs(inference_type);
CREATE INDEX IF NOT EXISTS idx_edge_ai_logs_created ON edge_ai_inference_logs(created_at);
CREATE INDEX IF NOT EXISTS idx_edge_ai_logs_user ON edge_ai_inference_logs(user_id);
CREATE INDEX IF NOT EXISTS idx_fl_rounds_number ON federated_learning_rounds(round_number);
CREATE INDEX IF NOT EXISTS idx_dp_budget_user ON differential_privacy_budget(user_id);
CREATE INDEX IF NOT EXISTS idx_dp_budget_created ON differential_privacy_budget(created_at);
CREATE INDEX IF NOT EXISTS idx_emotion_snapshots_time ON community_emotion_snapshots(snapshot_time);
CREATE INDEX IF NOT EXISTS idx_edge_nodes_status ON edge_nodes(status);
CREATE INDEX IF NOT EXISTS idx_edge_nodes_heartbeat ON edge_nodes(last_heartbeat);
