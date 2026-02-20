-- 数据导出任务表
CREATE TABLE IF NOT EXISTS data_export_tasks (
    task_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(user_id),
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

-- 数据备份记录表
CREATE TABLE IF NOT EXISTS backup_records (
    backup_id VARCHAR(64) PRIMARY KEY,
    backup_type VARCHAR(20) NOT NULL,
    file_path TEXT NOT NULL,
    file_size BIGINT,
    checksum VARCHAR(64) NOT NULL,
    created_at TIMESTAMP DEFAULT NOW(),
    verified_at TIMESTAMP,
    status VARCHAR(20) DEFAULT 'completed'
);
