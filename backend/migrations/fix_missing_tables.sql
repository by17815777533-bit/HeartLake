-- Fix missing tables (corrected foreign key references: users(id) is the actual PK)

-- stone_embeddings
CREATE TABLE IF NOT EXISTS stone_embeddings (
    stone_id VARCHAR(64) PRIMARY KEY REFERENCES stones(stone_id) ON DELETE CASCADE,
    embedding TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_stone_embeddings_created ON stone_embeddings(created_at);

-- intervention_log
CREATE TABLE IF NOT EXISTS intervention_log (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    reason TEXT,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_intervention_log_user ON intervention_log(user_id, created_at);

-- consultation_sessions (fixed: user_id references users(id))
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

-- consultation_messages
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

-- data_export_tasks (fixed: user_id references users(id))
CREATE TABLE IF NOT EXISTS data_export_tasks (
    task_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id),
    status VARCHAR(20) NOT NULL DEFAULT 'pending',
    download_url TEXT,
    file_size BIGINT,
    checksum VARCHAR(64),
    created_at TIMESTAMP DEFAULT NOW(),
    completed_at TIMESTAMP,
    expires_at TIMESTAMP,
    error_message TEXT
);
CREATE INDEX IF NOT EXISTS idx_export_tasks_user ON data_export_tasks(user_id);
CREATE INDEX IF NOT EXISTS idx_export_tasks_status ON data_export_tasks(status);

-- friend_messages (fixed: references users(id))
CREATE TABLE IF NOT EXISTS friend_messages (
    id SERIAL PRIMARY KEY,
    sender_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    receiver_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_friend_messages_sender ON friend_messages(sender_id);
CREATE INDEX IF NOT EXISTS idx_friend_messages_receiver ON friend_messages(receiver_id);
