-- Migration 016: Admin runtime tables
-- 补齐管理后台运行期依赖但历史迁移缺失的表。

BEGIN;

CREATE TABLE IF NOT EXISTS reports (
    report_id VARCHAR(64) PRIMARY KEY,
    reporter_id VARCHAR(64) NOT NULL,
    target_type VARCHAR(16) NOT NULL,
    target_id VARCHAR(64) NOT NULL,
    reason VARCHAR(64) NOT NULL,
    description TEXT,
    status VARCHAR(16) NOT NULL DEFAULT 'pending',
    handled_by VARCHAR(64),
    handled_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_reports_reporter_created
    ON reports(reporter_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_reports_status_created
    ON reports(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_reports_target
    ON reports(target_type, target_id);

CREATE TABLE IF NOT EXISTS moderation_logs (
    log_id VARCHAR(64) PRIMARY KEY,
    target_type VARCHAR(16) NOT NULL,
    target_id VARCHAR(64) NOT NULL,
    action VARCHAR(32) NOT NULL,
    reason TEXT,
    operator_id VARCHAR(64),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_moderation_logs_created
    ON moderation_logs(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_moderation_logs_target
    ON moderation_logs(target_type, target_id);

CREATE TABLE IF NOT EXISTS broadcast_messages (
    id SERIAL PRIMARY KEY,
    title VARCHAR(255) NOT NULL DEFAULT '',
    content TEXT NOT NULL DEFAULT '',
    level VARCHAR(16) NOT NULL DEFAULT 'info',
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

COMMIT;
