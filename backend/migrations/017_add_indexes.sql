-- Migration 017: 综合索引优化
-- 为高频查询添加索引，提升数据库查询性能
-- UP

-- ==================== 用户相关索引 ====================
CREATE INDEX IF NOT EXISTS idx_users_status ON users(status);
CREATE INDEX IF NOT EXISTS idx_users_created_at ON users(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_device ON users(device_id) WHERE device_id IS NOT NULL;

-- ==================== 石头相关索引 ====================
-- idx_stones_user_id 已在 002_stones.sql 中创建（idx_stones_user）
CREATE INDEX IF NOT EXISTS idx_stones_user_id ON stones(user_id);
-- idx_stones_status_created 已在 012_optimize_indexes.sql 中创建，此处确保存在
CREATE INDEX IF NOT EXISTS idx_stones_status_created ON stones(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_stones_mood ON stones(mood_type);
CREATE INDEX IF NOT EXISTS idx_stones_user_status ON stones(user_id, status);

-- ==================== 纸船相关索引 ====================
CREATE INDEX IF NOT EXISTS idx_paper_boats_sender ON paper_boats(sender_id);
CREATE INDEX IF NOT EXISTS idx_paper_boats_receiver ON paper_boats(receiver_id);
CREATE INDEX IF NOT EXISTS idx_paper_boats_status ON paper_boats(status);
-- 部分索引：仅索引漂流中的纸船，加速捡瓶子查询
CREATE INDEX IF NOT EXISTS idx_paper_boats_drifting ON paper_boats(status) WHERE status = 'drifting';

-- ==================== 好友关系索引 ====================
-- idx_friends_user_status / idx_friends_friend_status 已在 014 中创建，此处确保存在
CREATE INDEX IF NOT EXISTS idx_friends_user_status ON friends(user_id, status);
CREATE INDEX IF NOT EXISTS idx_friends_friend_status ON friends(friend_id, status);
CREATE INDEX IF NOT EXISTS idx_temp_friends_users ON temp_friends(user_id_1, user_id_2, status);

-- ==================== 互动相关索引 ====================
CREATE INDEX IF NOT EXISTS idx_ripples_stone ON ripples(stone_id);
CREATE INDEX IF NOT EXISTS idx_ripples_user ON ripples(user_id);

-- ==================== 其他索引 ====================
CREATE INDEX IF NOT EXISTS idx_reports_status ON reports(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_notifications_user ON notifications(user_id, created_at DESC);
-- 部分索引：仅索引活跃会话
CREATE INDEX IF NOT EXISTS idx_user_sessions_active ON user_sessions(user_id, is_active) WHERE is_active = true;
CREATE INDEX IF NOT EXISTS idx_moderation_logs_created ON moderation_logs(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_sensitive_words_created ON sensitive_words(created_at DESC);

-- DOWN
-- DROP INDEX IF EXISTS idx_users_status;
-- DROP INDEX IF EXISTS idx_users_created_at;
-- DROP INDEX IF EXISTS idx_users_email;
-- DROP INDEX IF EXISTS idx_users_username;
-- DROP INDEX IF EXISTS idx_users_device;
-- DROP INDEX IF EXISTS idx_stones_user_id;
-- DROP INDEX IF EXISTS idx_stones_status_created;
-- DROP INDEX IF EXISTS idx_stones_mood;
-- DROP INDEX IF EXISTS idx_stones_user_status;
-- DROP INDEX IF EXISTS idx_paper_boats_sender;
-- DROP INDEX IF EXISTS idx_paper_boats_receiver;
-- DROP INDEX IF EXISTS idx_paper_boats_status;
-- DROP INDEX IF EXISTS idx_paper_boats_drifting;
-- DROP INDEX IF EXISTS idx_friends_user_status;
-- DROP INDEX IF EXISTS idx_friends_friend_status;
-- DROP INDEX IF EXISTS idx_temp_friends_users;
-- DROP INDEX IF EXISTS idx_ripples_stone;
-- DROP INDEX IF EXISTS idx_ripples_user;
-- DROP INDEX IF EXISTS idx_reports_status;
-- DROP INDEX IF EXISTS idx_notifications_user;
-- DROP INDEX IF EXISTS idx_user_sessions_active;
-- DROP INDEX IF EXISTS idx_moderation_logs_created;
-- DROP INDEX IF EXISTS idx_sensitive_words_created;
