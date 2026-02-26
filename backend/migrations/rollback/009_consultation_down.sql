-- 009 回滚: 删除心理咨询表
BEGIN;

DROP TABLE IF EXISTS consultation_messages CASCADE;
DROP TABLE IF EXISTS consultation_sessions CASCADE;

COMMIT;
