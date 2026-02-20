-- Migration 019: 修复代码与数据库 schema 不匹配的列名问题
-- UP

-- 1. stones 表: 代码使用 view_count，但 014 创建的是 views_count
-- 添加 view_count 列（如果不存在），并从 views_count 迁移数据
ALTER TABLE stones ADD COLUMN IF NOT EXISTS view_count INT DEFAULT 0;
UPDATE stones SET view_count = COALESCE(views_count, 0) WHERE view_count = 0 AND views_count IS NOT NULL AND views_count > 0;

-- 2. stones 表: 代码使用 deleted_at 进行软删除，但没有 migration 添加此列
ALTER TABLE stones ADD COLUMN IF NOT EXISTS deleted_at TIMESTAMP;
CREATE INDEX IF NOT EXISTS idx_stones_deleted_at ON stones(deleted_at) WHERE deleted_at IS NULL;

-- 3. stones 表: 代码使用 tags 数组列（getTrendingTopics 查询 UNNEST(tags)）
ALTER TABLE stones ADD COLUMN IF NOT EXISTS tags TEXT[];

-- 4. paper_boats 表: 确保表存在并包含所有代码引用的列
CREATE TABLE IF NOT EXISTS paper_boats (
    boat_id VARCHAR(64) PRIMARY KEY,
    stone_id VARCHAR(64) REFERENCES stones(stone_id) ON DELETE SET NULL,
    sender_id VARCHAR(64) NOT NULL,
    receiver_id VARCHAR(64),
    catcher_id VARCHAR(64),
    content TEXT NOT NULL,
    mood VARCHAR(32) DEFAULT 'hopeful',
    boat_style VARCHAR(32) DEFAULT 'paper',
    boat_color VARCHAR(32) DEFAULT '#F5EFE7',
    is_anonymous BOOLEAN DEFAULT TRUE,
    is_ai_reply BOOLEAN DEFAULT FALSE,
    status VARCHAR(20) DEFAULT 'drifting',
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- 确保 paper_boats 表有所有代码引用的列
ALTER TABLE paper_boats ADD COLUMN IF NOT EXISTS receiver_id VARCHAR(64);
ALTER TABLE paper_boats ADD COLUMN IF NOT EXISTS catcher_id VARCHAR(64);
ALTER TABLE paper_boats ADD COLUMN IF NOT EXISTS mood VARCHAR(32) DEFAULT 'hopeful';
ALTER TABLE paper_boats ADD COLUMN IF NOT EXISTS boat_style VARCHAR(32) DEFAULT 'paper';
ALTER TABLE paper_boats ADD COLUMN IF NOT EXISTS boat_color VARCHAR(32) DEFAULT '#F5EFE7';
ALTER TABLE paper_boats ADD COLUMN IF NOT EXISTS is_ai_reply BOOLEAN DEFAULT FALSE;
ALTER TABLE paper_boats ADD COLUMN IF NOT EXISTS updated_at TIMESTAMP DEFAULT NOW();

-- 5. 确保缺失的表存在
-- ripples 表（涟漪/互动）
CREATE TABLE IF NOT EXISTS ripples (
    ripple_id VARCHAR(64) PRIMARY KEY,
    stone_id VARCHAR(64) NOT NULL REFERENCES stones(stone_id) ON DELETE CASCADE,
    user_id VARCHAR(64) NOT NULL,
    type VARCHAR(32) DEFAULT 'ripple',
    content TEXT,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_ripples_stone ON ripples(stone_id);
CREATE INDEX IF NOT EXISTS idx_ripples_user ON ripples(user_id);

-- notifications 表
CREATE TABLE IF NOT EXISTS notifications (
    notification_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    type VARCHAR(32) NOT NULL,
    content TEXT,
    related_id VARCHAR(64),
    related_type VARCHAR(32),
    is_read BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_notifications_user ON notifications(user_id, created_at DESC);

-- reports 表
CREATE TABLE IF NOT EXISTS reports (
    report_id VARCHAR(64) PRIMARY KEY,
    reporter_id VARCHAR(64) NOT NULL,
    target_type VARCHAR(32) NOT NULL,
    target_id VARCHAR(64) NOT NULL,
    reason VARCHAR(255),
    description TEXT,
    status VARCHAR(20) DEFAULT 'pending',
    handled_by VARCHAR(64),
    handled_at TIMESTAMP,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_reports_status ON reports(status, created_at DESC);

-- moderation_logs 表
CREATE TABLE IF NOT EXISTS moderation_logs (
    log_id VARCHAR(64) PRIMARY KEY,
    target_type VARCHAR(32),
    target_id VARCHAR(64),
    action VARCHAR(32),
    reason TEXT,
    operator_id VARCHAR(64),
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_moderation_logs_created ON moderation_logs(created_at DESC);

-- sensitive_words 表
CREATE TABLE IF NOT EXISTS sensitive_words (
    id SERIAL PRIMARY KEY,
    word VARCHAR(255) NOT NULL,
    level VARCHAR(20) DEFAULT 'medium',
    category VARCHAR(64) DEFAULT 'other',
    action VARCHAR(32) DEFAULT 'block',
    created_at TIMESTAMP DEFAULT NOW()
);

-- user_sessions 表
CREATE TABLE IF NOT EXISTS user_sessions (
    session_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    device_type VARCHAR(32),
    device_name VARCHAR(128),
    ip_address VARCHAR(45),
    is_active BOOLEAN DEFAULT TRUE,
    last_active_at TIMESTAMP DEFAULT NOW(),
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_user_sessions_active ON user_sessions(user_id, is_active) WHERE is_active = true;

-- login_logs 表
CREATE TABLE IF NOT EXISTS login_logs (
    log_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    login_time TIMESTAMP DEFAULT NOW(),
    ip_address VARCHAR(45),
    device_type VARCHAR(32),
    location VARCHAR(255),
    success BOOLEAN DEFAULT TRUE
);

-- security_events 表
CREATE TABLE IF NOT EXISTS security_events (
    event_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    event_type VARCHAR(64) NOT NULL,
    description TEXT,
    ip_address VARCHAR(45),
    severity VARCHAR(20) DEFAULT 'low',
    created_at TIMESTAMP DEFAULT NOW()
);

-- user_privacy_settings 表
CREATE TABLE IF NOT EXISTS user_privacy_settings (
    user_id VARCHAR(64) PRIMARY KEY,
    profile_visibility VARCHAR(20) DEFAULT 'public',
    show_online_status BOOLEAN DEFAULT TRUE,
    allow_friend_request BOOLEAN DEFAULT TRUE,
    allow_message_from_stranger BOOLEAN DEFAULT FALSE
);

-- user_blocks 表
CREATE TABLE IF NOT EXISTS user_blocks (
    user_id VARCHAR(64) NOT NULL,
    blocked_user_id VARCHAR(64) NOT NULL,
    created_at TIMESTAMP DEFAULT NOW(),
    PRIMARY KEY (user_id, blocked_user_id)
);

-- 6. users 表: 确保所有代码引用的列存在
ALTER TABLE users ADD COLUMN IF NOT EXISTS password_hash VARCHAR(255);
ALTER TABLE users ADD COLUMN IF NOT EXISTS salt VARCHAR(255);
ALTER TABLE users ADD COLUMN IF NOT EXISTS avatar_url TEXT;
ALTER TABLE users ADD COLUMN IF NOT EXISTS bio TEXT;
ALTER TABLE users ADD COLUMN IF NOT EXISTS gender VARCHAR(10);
ALTER TABLE users ADD COLUMN IF NOT EXISTS birthday VARCHAR(20);
ALTER TABLE users ADD COLUMN IF NOT EXISTS device_id VARCHAR(128);
ALTER TABLE users ADD COLUMN IF NOT EXISTS updated_at TIMESTAMP;

-- 7. 心理风险评估相关表
CREATE TABLE IF NOT EXISTS psychological_assessments (
    assessment_id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    content_id VARCHAR(64),
    content_type VARCHAR(32),
    risk_level INT DEFAULT 0,
    risk_score FLOAT DEFAULT 0,
    primary_concern VARCHAR(255),
    needs_immediate_attention BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_psych_assessments_user ON psychological_assessments(user_id, created_at DESC);

CREATE TABLE IF NOT EXISTS high_risk_events (
    event_id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    content_id VARCHAR(64),
    content_type VARCHAR(32),
    risk_level INT DEFAULT 0,
    risk_score FLOAT DEFAULT 0,
    intervention_sent BOOLEAN DEFAULT FALSE,
    admin_notified BOOLEAN DEFAULT FALSE,
    status VARCHAR(20) DEFAULT 'pending',
    handled_by VARCHAR(64),
    handled_at TIMESTAMP,
    notes TEXT,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_high_risk_events_status ON high_risk_events(status, created_at DESC);

CREATE TABLE IF NOT EXISTS admin_interventions (
    id SERIAL PRIMARY KEY,
    admin_id VARCHAR(64) NOT NULL,
    user_id VARCHAR(64) NOT NULL,
    event_id INT,
    action_type VARCHAR(32),
    action_details TEXT,
    outcome VARCHAR(32),
    created_at TIMESTAMP DEFAULT NOW()
);

-- 8. 高风险用户监控视图
CREATE OR REPLACE VIEW high_risk_users_monitor AS
SELECT
    u.user_id,
    u.nickname,
    u.email,
    COUNT(pa.assessment_id) AS total_assessments,
    COUNT(CASE WHEN pa.risk_level >= 3 THEN 1 END) AS high_risk_count,
    COALESCE(MAX(pa.risk_score), 0) AS max_risk_score,
    MAX(pa.created_at) AS last_assessment_at,
    COUNT(CASE WHEN hre.status = 'pending' THEN 1 END) AS pending_events,
    COUNT(hre.event_id) AS high_risk_events,
    ubp.negative_post_frequency,
    ubp.social_isolation_score,
    ubp.consecutive_negative_days
FROM users u
LEFT JOIN psychological_assessments pa ON u.user_id = pa.user_id
LEFT JOIN high_risk_events hre ON u.user_id = hre.user_id
LEFT JOIN user_behavior_patterns ubp ON u.user_id = ubp.user_id
WHERE pa.risk_level >= 2
GROUP BY u.user_id, u.nickname, u.email,
    ubp.negative_post_frequency, ubp.social_isolation_score, ubp.consecutive_negative_days;

-- 情绪趋势分析视图
CREATE OR REPLACE VIEW emotion_trend_analysis AS
SELECT
    et.user_id,
    DATE(et.created_at) AS date,
    AVG(et.intensity) AS avg_sentiment,
    MIN(et.intensity) AS min_sentiment,
    MAX(et.intensity) AS max_sentiment,
    COUNT(*) AS entry_count,
    COUNT(CASE WHEN et.intensity < 0.3 THEN 1 END) AS negative_count
FROM emotion_tracking et
GROUP BY et.user_id, DATE(et.created_at);

-- 用户行为模式表
CREATE TABLE IF NOT EXISTS user_behavior_patterns (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    analysis_date DATE DEFAULT CURRENT_DATE,
    negative_post_frequency FLOAT DEFAULT 0,
    engagement_decline FLOAT DEFAULT 0,
    social_isolation_score FLOAT DEFAULT 0,
    consecutive_negative_days INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_user_behavior_user ON user_behavior_patterns(user_id, analysis_date DESC);

-- 9. 管理后台相关表
CREATE TABLE IF NOT EXISTS broadcast_messages (
    id SERIAL PRIMARY KEY,
    title VARCHAR(255),
    content TEXT NOT NULL,
    level VARCHAR(20) DEFAULT 'info',
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS admin_operation_logs (
    id SERIAL PRIMARY KEY,
    admin_id VARCHAR(64) NOT NULL,
    action VARCHAR(64) NOT NULL,
    target_type VARCHAR(32),
    target_id VARCHAR(64),
    details TEXT,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS admins (
    admin_id VARCHAR(64) PRIMARY KEY,
    username VARCHAR(64) NOT NULL,
    role VARCHAR(32) DEFAULT 'admin',
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT NOW()
);

-- 插入默认管理员（如果不存在）
INSERT INTO admins (admin_id, username, role, is_active)
VALUES ('admin_001', 'admin', 'admin', true)
ON CONFLICT (admin_id) DO NOTHING;

-- DOWN
-- DROP VIEW IF EXISTS emotion_trend_analysis;
-- DROP VIEW IF EXISTS high_risk_users_monitor;
-- ALTER TABLE stones DROP COLUMN IF EXISTS view_count;
-- ALTER TABLE stones DROP COLUMN IF EXISTS deleted_at;
-- ALTER TABLE stones DROP COLUMN IF EXISTS tags;
