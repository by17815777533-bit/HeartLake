-- 006 回滚: 删除守望者系统表
BEGIN;

DROP TABLE IF EXISTS vip_upgrade_logs CASCADE;
DROP TABLE IF EXISTS intervention_log CASCADE;
DROP TABLE IF EXISTS lamp_transfers CASCADE;
DROP TABLE IF EXISTS resonance_points CASCADE;
DROP TABLE IF EXISTS emotion_tracking CASCADE;

COMMIT;
