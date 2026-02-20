-- Migration 018: 补充缺失索引优化
-- 修复 017 遗漏的索引和查询性能问题
-- UP

-- ==================== 好友消息索引 ====================
-- 会话查询需要 sender+receiver 复合索引，加速聊天记录加载
CREATE INDEX IF NOT EXISTS idx_friend_messages_conversation
    ON friend_messages(LEAST(sender_id, receiver_id), GREATEST(sender_id, receiver_id), created_at DESC);
-- 按时间倒序查询最近消息
CREATE INDEX IF NOT EXISTS idx_friend_messages_created
    ON friend_messages(created_at DESC);

-- ==================== 灯传递索引 ====================
-- lamp_transfers 表在 004 中创建但完全没有索引
CREATE INDEX IF NOT EXISTS idx_lamp_transfers_from ON lamp_transfers(from_user_id);
CREATE INDEX IF NOT EXISTS idx_lamp_transfers_to ON lamp_transfers(to_user_id);
CREATE INDEX IF NOT EXISTS idx_lamp_transfers_created ON lamp_transfers(created_at DESC);

-- ==================== 通知索引优化 ====================
-- 017 的 idx_notifications_user 覆盖了 011 的部分索引，这里恢复未读通知的部分索引
-- 部分索引仅索引未读通知，大幅减少索引体积并加速未读数查询
CREATE INDEX IF NOT EXISTS idx_notifications_user_unread
    ON notifications(user_id, is_read) WHERE is_read = FALSE;

-- ==================== 石头排序索引 ====================
-- 通用时间排序查询（不带 status 过滤时使用）
CREATE INDEX IF NOT EXISTS idx_stones_created_at ON stones(created_at DESC);

-- ==================== 纸船复合索引 ====================
-- 发送者查看自己发出的纸船（按状态筛选）
CREATE INDEX IF NOT EXISTS idx_paper_boats_sender_status
    ON paper_boats(sender_id, status, created_at DESC);
-- 接收者查看收到的纸船
CREATE INDEX IF NOT EXISTS idx_paper_boats_receiver_status
    ON paper_boats(receiver_id, status, created_at DESC);

-- ==================== 情绪追踪索引 ====================
-- 情绪分数范围查询（用于情绪分布统计和预警）
CREATE INDEX IF NOT EXISTS idx_emotion_tracking_score
    ON emotion_tracking(score, created_at DESC);

-- ==================== 用户搜索索引 ====================
-- 管理后台用户搜索需要 nickname 的 trigram 或前缀索引
-- 使用 varchar_pattern_ops 支持 LIKE 'prefix%' 查询
CREATE INDEX IF NOT EXISTS idx_users_nickname_pattern
    ON users(nickname varchar_pattern_ops);
-- 用户名前缀搜索
CREATE INDEX IF NOT EXISTS idx_users_username_pattern
    ON users(username varchar_pattern_ops);

-- ==================== 数据导出任务索引 ====================
-- 用户查看自己的导出任务（按时间倒序）
CREATE INDEX IF NOT EXISTS idx_export_tasks_user_status
    ON data_export_tasks(user_id, status, created_at DESC);

-- DOWN
-- DROP INDEX IF EXISTS idx_friend_messages_conversation;
-- DROP INDEX IF EXISTS idx_friend_messages_created;
-- DROP INDEX IF EXISTS idx_lamp_transfers_from;
-- DROP INDEX IF EXISTS idx_lamp_transfers_to;
-- DROP INDEX IF EXISTS idx_lamp_transfers_created;
-- DROP INDEX IF EXISTS idx_notifications_user_unread;
-- DROP INDEX IF EXISTS idx_stones_created_at;
-- DROP INDEX IF EXISTS idx_paper_boats_sender_status;
-- DROP INDEX IF EXISTS idx_paper_boats_receiver_status;
-- DROP INDEX IF EXISTS idx_emotion_tracking_score;
-- DROP INDEX IF EXISTS idx_users_nickname_pattern;
-- DROP INDEX IF EXISTS idx_users_username_pattern;
-- DROP INDEX IF EXISTS idx_export_tasks_user_status;
