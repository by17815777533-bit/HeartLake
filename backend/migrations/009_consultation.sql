-- 009: 心理咨询E2EE加密会话
-- 注意: 生产环境建议将 CREATE INDEX 改为 CREATE INDEX CONCURRENTLY 以避免锁表

-- 咨询会话表
CREATE TABLE IF NOT EXISTS consultation_sessions (
    id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    counselor_id VARCHAR(64) NOT NULL,
    server_key TEXT NOT NULL,
    client_key TEXT,
    key_salt TEXT,
    status VARCHAR(20) DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT NOW(),
    ended_at TIMESTAMP,
    CONSTRAINT fk_consultation_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_consultation_sessions_user ON consultation_sessions(user_id);

-- 咨询消息表（端到端加密）
CREATE TABLE IF NOT EXISTS consultation_messages (
    id SERIAL PRIMARY KEY,
    session_id VARCHAR(64) NOT NULL REFERENCES consultation_sessions(id) ON DELETE CASCADE,
    sender_shadow_id VARCHAR(64) NOT NULL,
    ciphertext TEXT NOT NULL,
    iv VARCHAR(32) NOT NULL,
    tag VARCHAR(32) NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_consultation_messages_session ON consultation_messages(session_id);
