-- 006: 守望者系统（情绪追踪、共鸣积分、明灯传递、干预日志）

-- 情绪追踪表
CREATE TABLE IF NOT EXISTS emotion_tracking (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    score FLOAT NOT NULL,
    content_hash VARCHAR(64),
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_emotion_tracking_user ON emotion_tracking(user_id, created_at);

-- 共鸣积分表
CREATE TABLE IF NOT EXISTS resonance_points (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    points INT NOT NULL,
    reason VARCHAR(255),
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_resonance_points_user ON resonance_points(user_id);

-- 明灯传递表
CREATE TABLE IF NOT EXISTS lamp_transfers (
    id SERIAL PRIMARY KEY,
    from_user_id VARCHAR(64) NOT NULL,
    to_user_id VARCHAR(64) NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

-- 干预日志表（防止重复干预）
CREATE TABLE IF NOT EXISTS intervention_log (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    reason TEXT,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_intervention_log_user ON intervention_log(user_id, created_at);

-- VIP升级日志表
CREATE TABLE IF NOT EXISTS vip_upgrade_logs (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    old_vip_level INT DEFAULT 0,
    new_vip_level INT DEFAULT 0,
    upgrade_type VARCHAR(50) DEFAULT 'auto_emotion',
    reason TEXT,
    expires_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_vip_upgrade_logs_user ON vip_upgrade_logs(user_id);
