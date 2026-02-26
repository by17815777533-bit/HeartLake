-- 010 回滚: 删除数据导出表
BEGIN;

DROP TABLE IF EXISTS data_export_tasks CASCADE;

COMMIT;
