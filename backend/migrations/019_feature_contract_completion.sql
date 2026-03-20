-- Migration 019: feature contract completion
-- 目的：
-- 1. 补齐推荐、VIP、风险监控链路缺失的真实表结构
-- 2. 用现有主链路数据构建分析视图，消除运行时幽灵表
-- 3. 清理已废弃的 temp_connections 兼容视图，统一走 connections

BEGIN;

-- ============================================================
-- 1. Recommendation contracts
-- ============================================================

CREATE TABLE IF NOT EXISTS user_preferences (
    user_id VARCHAR(64) PRIMARY KEY REFERENCES users(user_id) ON DELETE CASCADE,
    preferred_moods TEXT[] NOT NULL DEFAULT '{}'::text[],
    preferred_tags TEXT[] NOT NULL DEFAULT '{}'::text[],
    last_updated TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_user_preferences_updated
    ON user_preferences(last_updated DESC);

-- ============================================================
-- 2. VIP contracts
-- ============================================================

CREATE TABLE IF NOT EXISTS vip_privileges (
    privilege_key VARCHAR(64) PRIMARY KEY,
    privilege_name VARCHAR(128) NOT NULL DEFAULT '',
    description TEXT NOT NULL DEFAULT '',
    vip_level_required INTEGER NOT NULL DEFAULT 1,
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    config JSONB NOT NULL DEFAULT '{}'::jsonb,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS vip_privilege_usage (
    user_id VARCHAR(64) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    privilege_key VARCHAR(64) NOT NULL REFERENCES vip_privileges(privilege_key) ON DELETE CASCADE,
    usage_count INTEGER NOT NULL DEFAULT 1,
    metadata JSONB NOT NULL DEFAULT '{}'::jsonb,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    last_used_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    PRIMARY KEY (user_id, privilege_key)
);

CREATE INDEX IF NOT EXISTS idx_vip_privilege_usage_last_used
    ON vip_privilege_usage(user_id, last_used_at DESC);

CREATE TABLE IF NOT EXISTS counseling_appointments (
    appointment_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    appointment_time TIMESTAMPTZ NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'scheduled',
    is_free_vip BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_counseling_appointments_user_status
    ON counseling_appointments(user_id, status, appointment_time DESC);
CREATE INDEX IF NOT EXISTS idx_counseling_appointments_time
    ON counseling_appointments(appointment_time DESC);

INSERT INTO vip_privileges (
    privilege_key, privilege_name, description, vip_level_required, is_active, config
) VALUES
    (
        'ai_comment_frequent',
        '高频 AI 陪伴',
        '将 AI 陪伴评论频率从 2 小时提升到 30 分钟',
        1,
        TRUE,
        '{"frequency_minutes":30}'::jsonb
    ),
    (
        'free_counseling',
        '免费心理咨询',
        'VIP 有效期内可获得 1 次免费心理咨询额度',
        1,
        TRUE,
        '{"max_usage":1,"window_days":30}'::jsonb
    )
ON CONFLICT (privilege_key) DO UPDATE
SET privilege_name = EXCLUDED.privilege_name,
    description = EXCLUDED.description,
    vip_level_required = EXCLUDED.vip_level_required,
    is_active = EXCLUDED.is_active,
    config = EXCLUDED.config,
    updated_at = NOW();

-- ============================================================
-- 3. Risk monitoring contracts
-- ============================================================

CREATE TABLE IF NOT EXISTS psychological_assessments (
    assessment_id BIGSERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    content_id VARCHAR(64),
    content_type VARCHAR(32) NOT NULL DEFAULT 'stone',
    risk_level INTEGER NOT NULL DEFAULT 0,
    risk_score DOUBLE PRECISION NOT NULL DEFAULT 0,
    primary_concern VARCHAR(64),
    needs_immediate_attention BOOLEAN NOT NULL DEFAULT FALSE,
    keywords TEXT[] NOT NULL DEFAULT '{}'::text[],
    factors JSONB NOT NULL DEFAULT '[]'::jsonb,
    support_message TEXT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_psychological_assessments_user_created
    ON psychological_assessments(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_psychological_assessments_content
    ON psychological_assessments(content_type, content_id);
CREATE INDEX IF NOT EXISTS idx_psychological_assessments_risk
    ON psychological_assessments(risk_level, created_at DESC);

CREATE TABLE IF NOT EXISTS high_risk_events (
    event_id BIGSERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    content_id VARCHAR(64),
    content_type VARCHAR(32) NOT NULL DEFAULT 'stone',
    risk_level INTEGER NOT NULL,
    risk_score DOUBLE PRECISION NOT NULL,
    intervention_sent BOOLEAN NOT NULL DEFAULT FALSE,
    admin_notified BOOLEAN NOT NULL DEFAULT FALSE,
    status VARCHAR(20) NOT NULL DEFAULT 'pending',
    handled_by VARCHAR(64),
    handled_at TIMESTAMPTZ,
    notes TEXT,
    assessment_id BIGINT REFERENCES psychological_assessments(assessment_id) ON DELETE SET NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_high_risk_events_status_created
    ON high_risk_events(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_high_risk_events_user_created
    ON high_risk_events(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_high_risk_events_risk_created
    ON high_risk_events(risk_score DESC, created_at DESC);

CREATE TABLE IF NOT EXISTS admin_interventions (
    intervention_id BIGSERIAL PRIMARY KEY,
    admin_id VARCHAR(64) NOT NULL,
    user_id VARCHAR(64) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    event_id BIGINT NOT NULL REFERENCES high_risk_events(event_id) ON DELETE CASCADE,
    action_type VARCHAR(32) NOT NULL,
    action_details TEXT,
    outcome VARCHAR(32),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_admin_interventions_event
    ON admin_interventions(event_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_admin_interventions_admin
    ON admin_interventions(admin_id, created_at DESC);

DROP VIEW IF EXISTS emotion_trend_analysis;
CREATE VIEW emotion_trend_analysis AS
SELECT
    ueh.user_id,
    DATE(ueh.created_at) AS date,
    COALESCE(AVG(ueh.sentiment_score), 0) AS avg_sentiment,
    COALESCE(MIN(ueh.sentiment_score), 0) AS min_sentiment,
    COALESCE(MAX(ueh.sentiment_score), 0) AS max_sentiment,
    COUNT(*)::INTEGER AS entry_count,
    COUNT(*) FILTER (WHERE COALESCE(ueh.sentiment_score, 0) < -0.3)::INTEGER AS negative_count
FROM user_emotion_history ueh
GROUP BY ueh.user_id, DATE(ueh.created_at);

DROP VIEW IF EXISTS user_behavior_patterns;
CREATE VIEW user_behavior_patterns AS
WITH daily_posts AS (
    SELECT
        s.user_id,
        DATE(s.created_at) AS post_date,
        COUNT(*)::INTEGER AS total_posts,
        COUNT(*) FILTER (
            WHERE COALESCE(s.emotion_score, 0) < -0.3
               OR COALESCE(s.mood_type, 'neutral') IN ('sad', 'anxious', 'angry', 'lonely', 'hopeless')
        )::INTEGER AS negative_posts
    FROM stones s
    WHERE s.deleted_at IS NULL
      AND COALESCE(s.status, 'published') <> 'deleted'
      AND s.created_at >= CURRENT_DATE - INTERVAL '30 days'
    GROUP BY s.user_id, DATE(s.created_at)
),
negative_stats AS (
    SELECT
        dp.user_id,
        COALESCE(SUM(dp.negative_posts), 0)::INTEGER AS negative_posts_30,
        COALESCE(SUM(dp.total_posts), 0)::INTEGER AS total_posts_30,
        COUNT(*) FILTER (
            WHERE dp.post_date >= CURRENT_DATE - INTERVAL '7 days'
              AND dp.negative_posts > 0
        )::INTEGER AS negative_days_7
    FROM daily_posts dp
    GROUP BY dp.user_id
),
recent_week AS (
    SELECT user_id, COALESCE(SUM(total_posts), 0)::INTEGER AS posts
    FROM daily_posts
    WHERE post_date >= CURRENT_DATE - INTERVAL '7 days'
    GROUP BY user_id
),
prior_week AS (
    SELECT user_id, COALESCE(SUM(total_posts), 0)::INTEGER AS posts
    FROM daily_posts
    WHERE post_date >= CURRENT_DATE - INTERVAL '14 days'
      AND post_date < CURRENT_DATE - INTERVAL '7 days'
    GROUP BY user_id
),
friend_counts AS (
    SELECT f.user_id, COUNT(*)::INTEGER AS friend_count
    FROM (
        SELECT user_id FROM friends WHERE status = 'accepted'
        UNION ALL
        SELECT friend_id AS user_id FROM friends WHERE status = 'accepted'
    ) f
    GROUP BY f.user_id
)
SELECT
    u.user_id,
    CURRENT_DATE AS analysis_date,
    COALESCE(
        negative_stats.negative_posts_30::DOUBLE PRECISION
        / NULLIF(negative_stats.total_posts_30, 0),
        0
    ) AS negative_post_frequency,
    COALESCE(
        GREATEST(
            0.0,
            (COALESCE(prior_week.posts, 0) - COALESCE(recent_week.posts, 0))::DOUBLE PRECISION
            / GREATEST(COALESCE(prior_week.posts, 0), 1)
        ),
        0
    ) AS engagement_decline,
    CASE
        WHEN COALESCE(friend_counts.friend_count, 0) = 0 THEN 1.0
        ELSE LEAST(1.0, 1.0 / friend_counts.friend_count::DOUBLE PRECISION)
    END AS social_isolation_score,
    COALESCE(negative_stats.negative_days_7, 0) AS consecutive_negative_days
FROM users u
LEFT JOIN negative_stats ON negative_stats.user_id = u.user_id
LEFT JOIN recent_week ON recent_week.user_id = u.user_id
LEFT JOIN prior_week ON prior_week.user_id = u.user_id
LEFT JOIN friend_counts ON friend_counts.user_id = u.user_id;

DROP VIEW IF EXISTS high_risk_users_monitor;
CREATE VIEW high_risk_users_monitor AS
WITH assessment_summary AS (
    SELECT
        pa.user_id,
        COUNT(*)::INTEGER AS total_assessments,
        COUNT(*) FILTER (WHERE pa.risk_level >= 3)::INTEGER AS high_risk_count,
        COALESCE(MAX(pa.risk_score), 0) AS max_risk_score,
        MAX(pa.created_at) AS last_assessment_at
    FROM psychological_assessments pa
    GROUP BY pa.user_id
),
event_summary AS (
    SELECT
        hre.user_id,
        COUNT(*)::INTEGER AS high_risk_events,
        COUNT(*) FILTER (WHERE hre.status = 'pending')::INTEGER AS pending_events
    FROM high_risk_events hre
    GROUP BY hre.user_id
)
SELECT
    u.user_id,
    COALESCE(u.nickname, '') AS nickname,
    COALESCE(u.email, '') AS email,
    COALESCE(assessment_summary.total_assessments, 0) AS total_assessments,
    COALESCE(assessment_summary.high_risk_count, 0) AS high_risk_count,
    COALESCE(assessment_summary.max_risk_score, 0) AS max_risk_score,
    assessment_summary.last_assessment_at,
    COALESCE(event_summary.high_risk_events, 0) AS high_risk_events,
    COALESCE(event_summary.pending_events, 0) AS pending_events,
    COALESCE(ubp.negative_post_frequency, 0) AS negative_post_frequency,
    COALESCE(ubp.social_isolation_score, 0) AS social_isolation_score,
    COALESCE(ubp.consecutive_negative_days, 0) AS consecutive_negative_days
FROM users u
LEFT JOIN assessment_summary ON assessment_summary.user_id = u.user_id
LEFT JOIN event_summary ON event_summary.user_id = u.user_id
LEFT JOIN user_behavior_patterns ubp ON ubp.user_id = u.user_id;

-- 旧连接兼容视图已退场，统一走 connections 真表
DROP VIEW IF EXISTS temp_connections;

COMMIT;
