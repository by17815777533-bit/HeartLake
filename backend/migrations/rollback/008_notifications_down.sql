-- 008 回滚: 删除通知表
BEGIN;

DROP TABLE IF EXISTS notifications CASCADE;

COMMIT;
