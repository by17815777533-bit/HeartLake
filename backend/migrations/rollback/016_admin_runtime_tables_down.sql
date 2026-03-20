BEGIN;

DROP TABLE IF EXISTS broadcast_messages;
DROP INDEX IF EXISTS idx_moderation_logs_target;
DROP INDEX IF EXISTS idx_moderation_logs_created;
DROP TABLE IF EXISTS moderation_logs;
DROP INDEX IF EXISTS idx_reports_target;
DROP INDEX IF EXISTS idx_reports_status_created;
DROP INDEX IF EXISTS idx_reports_reporter_created;
DROP TABLE IF EXISTS reports;

COMMIT;
