-- 咨询室E2EE端到端加密表结构
-- Created by engineer-4

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
    CONSTRAINT fk_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS consultation_messages (
    id SERIAL PRIMARY KEY,
    session_id VARCHAR(64) NOT NULL REFERENCES consultation_sessions(id) ON DELETE CASCADE,
    sender_shadow_id VARCHAR(64) NOT NULL,
    ciphertext TEXT NOT NULL,
    iv VARCHAR(32) NOT NULL,
    tag VARCHAR(32) NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_consultation_sessions_user ON consultation_sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_consultation_messages_session ON consultation_messages(session_id);

-- DOWN
-- DROP INDEX IF EXISTS idx_consultation_messages_session;
-- DROP INDEX IF EXISTS idx_consultation_sessions_user;
-- DROP TABLE IF EXISTS consultation_messages;
-- DROP TABLE IF EXISTS consultation_sessions;
