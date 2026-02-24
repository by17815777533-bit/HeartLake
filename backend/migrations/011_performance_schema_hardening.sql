-- 011: 性能与架构一致性增强
-- 目标：
-- 1) 修复代码与数据库表结构漂移，避免运行期SQL异常
-- 2) 为高频/慢查询添加复合索引与部分索引
-- 3) 在不破坏现有API的前提下提升低配服务器可用性

-- =========================
-- 通知表兼容性修复
-- =========================
ALTER TABLE notifications
    ADD COLUMN IF NOT EXISTS notification_id VARCHAR(64);

ALTER TABLE notifications
    ADD COLUMN IF NOT EXISTS related_type VARCHAR(32);

UPDATE notifications
SET notification_id = 'notif_' || id::text
WHERE notification_id IS NULL;

CREATE UNIQUE INDEX IF NOT EXISTS idx_notifications_notification_id
    ON notifications(notification_id)
    WHERE notification_id IS NOT NULL;

CREATE INDEX IF NOT EXISTS idx_notifications_user_notification_id
    ON notifications(user_id, notification_id);

-- =========================
-- 回访服务基础表
-- =========================
CREATE TABLE IF NOT EXISTS user_followups (
    id BIGSERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    followup_day INT NOT NULL,
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE (user_id, followup_day)
);

CREATE INDEX IF NOT EXISTS idx_user_followups_created
    ON user_followups(created_at DESC);

-- =========================
-- 慢查询索引优化
-- =========================

-- LakeGod: 零互动石头扫描
CREATE INDEX IF NOT EXISTS idx_stones_published_zero_interaction_created
    ON stones(created_at DESC)
    WHERE status = 'published'
      AND ripple_count = 0
      AND boat_count = 0
      AND deleted_at IS NULL;

-- LakeGod: 纸船去重与清理
CREATE INDEX IF NOT EXISTS idx_paper_boats_stone_sender
    ON paper_boats(stone_id, sender_id);

CREATE INDEX IF NOT EXISTS idx_paper_boats_pending_lakegod_created
    ON paper_boats(created_at)
    WHERE status = 'pending' AND sender_id = 'lake_god';

-- EmotionTracking: 近72小时发布用户扫描
CREATE INDEX IF NOT EXISTS idx_stones_status_created_user
    ON stones(status, created_at DESC, user_id);

-- EmotionTracking: 用户负向情绪聚合
CREATE INDEX IF NOT EXISTS idx_stones_user_negative_recent
    ON stones(user_id, created_at DESC)
    WHERE status = 'published' AND deleted_at IS NULL AND emotion_score < 0;

-- UserFollowUp: 非守护者7天未活跃扫描
CREATE INDEX IF NOT EXISTS idx_users_inactive_non_guardian
    ON users(last_active_at)
    WHERE is_guardian = FALSE;

-- UserFollowUp: VIP 升级日志按原因+时间过滤
CREATE INDEX IF NOT EXISTS idx_vip_upgrade_logs_reason_prefix_created
    ON vip_upgrade_logs(reason text_pattern_ops, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_vip_upgrade_logs_created_user
    ON vip_upgrade_logs(created_at DESC, user_id);
