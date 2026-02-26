-- 013 回滚: 撤销 schema fixes
-- 注意: TIMESTAMPTZ → TIMESTAMP 的回退在生产环境中通常不需要，此处仅做完整性保留
BEGIN;

-- 移除 013 新增的索引
DROP INDEX IF EXISTS idx_lamp_transfers_from;
DROP INDEX IF EXISTS idx_lamp_transfers_to;
DROP INDEX IF EXISTS idx_uih_created;

-- 移除 013 新增的外键约束（安全忽略不存在的情况）
DO $$ BEGIN
    ALTER TABLE stones DROP CONSTRAINT IF EXISTS fk_stones_user;
    ALTER TABLE notifications DROP CONSTRAINT IF EXISTS fk_notifications_user;
    ALTER TABLE emotion_tracking DROP CONSTRAINT IF EXISTS fk_emotion_tracking_user;
    ALTER TABLE user_interaction_history DROP CONSTRAINT IF EXISTS fk_uih_user;
    ALTER TABLE user_interaction_history DROP CONSTRAINT IF EXISTS fk_uih_stone;
    ALTER TABLE resonance_points DROP CONSTRAINT IF EXISTS fk_resonance_points_user;
    ALTER TABLE lamp_transfers DROP CONSTRAINT IF EXISTS fk_lamp_from_user;
    ALTER TABLE lamp_transfers DROP CONSTRAINT IF EXISTS fk_lamp_to_user;
    ALTER TABLE intervention_log DROP CONSTRAINT IF EXISTS fk_intervention_user;
END $$;

-- 移除 013 新增的视图
DROP VIEW IF EXISTS temp_connections;

-- 移除 013 新增的表
DROP TABLE IF EXISTS user_privacy_settings CASCADE;
DROP TABLE IF EXISTS security_events CASCADE;
DROP TABLE IF EXISTS login_logs CASCADE;
DROP TABLE IF EXISTS user_sessions CASCADE;

COMMIT;
