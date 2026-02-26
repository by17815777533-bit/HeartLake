-- 011 回滚: 移除性能索引和回访表
BEGIN;

-- 移除慢查询优化索引
DROP INDEX IF EXISTS idx_vip_upgrade_logs_created_user;
DROP INDEX IF EXISTS idx_vip_upgrade_logs_reason_prefix_created;
DROP INDEX IF EXISTS idx_users_inactive_non_guardian;
DROP INDEX IF EXISTS idx_stones_user_negative_recent;
DROP INDEX IF EXISTS idx_stones_status_created_user;
DROP INDEX IF EXISTS idx_paper_boats_pending_lakegod_created;
DROP INDEX IF EXISTS idx_paper_boats_stone_sender;
DROP INDEX IF EXISTS idx_stones_published_zero_interaction_created;

-- 移除回访表
DROP TABLE IF EXISTS user_followups CASCADE;

-- 移除通知表兼容列（保留原始列不动）
DROP INDEX IF EXISTS idx_notifications_user_notification_id;
DROP INDEX IF EXISTS idx_notifications_notification_id;
ALTER TABLE notifications DROP COLUMN IF EXISTS related_type;
ALTER TABLE notifications DROP COLUMN IF EXISTS notification_id;

COMMIT;
