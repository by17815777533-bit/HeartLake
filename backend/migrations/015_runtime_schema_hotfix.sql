-- Migration 015: Runtime schema hotfix
-- Purpose:
-- 1) Expand users.recovery_key_hash length to prevent hash truncation failures
--
-- Note:
-- Auxiliary account/session tables and temp_connections view were consolidated
-- into 013_schema_fixes.sql. Keeping 015 focused avoids repeated DDL on every
-- startup migration pass.

BEGIN;

-- widen recovery key hash column (idempotent)
DO $$
BEGIN
    IF EXISTS (
        SELECT 1
        FROM information_schema.columns c
        WHERE c.table_schema = 'public'
          AND c.table_name = 'users'
          AND c.column_name = 'recovery_key_hash'
    ) THEN
        BEGIN
            ALTER TABLE users
                ALTER COLUMN recovery_key_hash TYPE VARCHAR(256);
        EXCEPTION WHEN OTHERS THEN
            RAISE NOTICE 'skip alter users.recovery_key_hash: %', SQLERRM;
        END;
    END IF;
END $$;

COMMIT;
