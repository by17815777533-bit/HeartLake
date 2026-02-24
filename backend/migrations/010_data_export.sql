-- 010: 数据导出
-- 注意: 生产环境建议将 CREATE INDEX 改为 CREATE INDEX CONCURRENTLY 以避免锁表

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
