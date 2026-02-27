-- Rollback 014: Remove stones.embedding vector index and column

BEGIN;

DROP INDEX IF EXISTS idx_stones_embedding_ivfflat;
ALTER TABLE stones DROP COLUMN IF EXISTS embedding;

COMMIT;
