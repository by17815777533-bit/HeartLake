-- 用户相似度表
CREATE TABLE IF NOT EXISTS user_similarity (
    user1_id VARCHAR(64) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    user2_id VARCHAR(64) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    similarity_score DECIMAL(5,4) NOT NULL DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user1_id, user2_id)
);

CREATE INDEX IF NOT EXISTS idx_user_similarity_score ON user_similarity(user1_id, similarity_score DESC);
