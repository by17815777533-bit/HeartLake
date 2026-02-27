-- Rollback 015: runtime schema hotfix
-- Intentionally no-op to avoid destructive rollback on production data.
-- Tables may already contain live data and shrinking recovery_key_hash may truncate values.

SELECT 1;
