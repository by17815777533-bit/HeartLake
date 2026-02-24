-- 001: 用户表（匿名登录，无密码）
-- 注意: 生产环境建议将 CREATE INDEX 改为 CREATE INDEX CONCURRENTLY 以避免锁表
CREATE TABLE IF NOT EXISTS users (
    id VARCHAR(64) PRIMARY KEY,
    shadow_id VARCHAR(64) UNIQUE NOT NULL,
    user_id VARCHAR(64) UNIQUE,
    nickname VARCHAR(64),
    avatar_url TEXT,
    bio TEXT,
    device_id VARCHAR(128),
    recovery_key VARCHAR(128),
    role VARCHAR(20) DEFAULT 'user',
    status VARCHAR(20) DEFAULT 'active',
    is_anonymous BOOLEAN DEFAULT TRUE,
    vip_level INT DEFAULT 0,
    vip_expires_at TIMESTAMP,
    resonance_total INT DEFAULT 0,
    is_guardian BOOLEAN DEFAULT FALSE,
    guardian_since TIMESTAMP,
    last_login_at TIMESTAMP,
    last_active_at TIMESTAMP,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_users_shadow ON users(shadow_id);
CREATE INDEX IF NOT EXISTS idx_users_device ON users(device_id);
CREATE INDEX IF NOT EXISTS idx_users_status ON users(status);

-- 用户相似度表（推荐系统用）
CREATE TABLE IF NOT EXISTS user_similarity (
    user1_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    user2_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    similarity_score DECIMAL(5,4) NOT NULL DEFAULT 0,
    updated_at TIMESTAMP DEFAULT NOW(),
    PRIMARY KEY (user1_id, user2_id)
);

CREATE INDEX IF NOT EXISTS idx_user_similarity_score ON user_similarity(user1_id, similarity_score DESC);
