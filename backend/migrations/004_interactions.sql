-- 004: 互动系统（涟漪、纸船、临时连接）
-- 注意: 生产环境建议将 CREATE INDEX 改为 CREATE INDEX CONCURRENTLY 以避免锁表
BEGIN;

-- 涟漪表（类似点赞）
CREATE TABLE IF NOT EXISTS ripples (
    ripple_id VARCHAR(64) PRIMARY KEY,
    stone_id VARCHAR(64) NOT NULL REFERENCES stones(stone_id) ON DELETE CASCADE,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(stone_id, user_id)
);

CREATE INDEX IF NOT EXISTS idx_ripples_stone ON ripples(stone_id);
CREATE INDEX IF NOT EXISTS idx_ripples_user ON ripples(user_id);

-- 纸船表（评论/漂流瓶）
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

-- 临时连接表（石头互动产生的匿名连接）
CREATE TABLE IF NOT EXISTS connections (
    connection_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    target_user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    stone_id VARCHAR(64) REFERENCES stones(stone_id) ON DELETE SET NULL,
    status VARCHAR(20) DEFAULT 'pending',
    expires_at TIMESTAMP,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_connections_user ON connections(user_id);
CREATE INDEX IF NOT EXISTS idx_connections_target ON connections(target_user_id);

-- 连接消息表
CREATE TABLE IF NOT EXISTS connection_messages (
    id SERIAL PRIMARY KEY,
    connection_id VARCHAR(64) NOT NULL REFERENCES connections(connection_id) ON DELETE CASCADE,
    sender_id VARCHAR(64) NOT NULL,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_connection_messages_conn ON connection_messages(connection_id);

COMMIT;
