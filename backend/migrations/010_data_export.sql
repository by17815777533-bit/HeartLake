-- 数据导出任务表
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

CREATE INDEX idx_export_tasks_user ON data_export_tasks(user_id);
CREATE INDEX idx_export_tasks_status ON data_export_tasks(status);