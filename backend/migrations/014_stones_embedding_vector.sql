-- Migration 014: Add stones.embedding vector column and backfill data
-- Purpose:
-- 1) Ensure pgvector extension exists.
-- 2) Add stones.embedding (vector(256)) for vector similarity queries.
-- 3) Backfill from legacy stone_embeddings.embedding (TEXT) when available.
-- 4) Create ivfflat index for similarity retrieval.

BEGIN;

CREATE EXTENSION IF NOT EXISTS vector;

DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'stones'
          AND column_name = 'embedding'
    ) THEN
        ALTER TABLE stones ADD COLUMN embedding vector(256);
    END IF;
END $$;

DO $$
BEGIN
    IF EXISTS (
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public'
          AND table_name = 'stone_embeddings'
    ) THEN
        UPDATE stones s
        SET embedding = se.embedding::vector
        FROM stone_embeddings se
        WHERE se.stone_id = s.stone_id
          AND s.embedding IS NULL
          AND se.embedding IS NOT NULL
          AND btrim(se.embedding) <> '';
    END IF;
EXCEPTION WHEN OTHERS THEN
    RAISE NOTICE '014 backfill skipped: %', SQLERRM;
END $$;

CREATE INDEX IF NOT EXISTS idx_stones_embedding_ivfflat
ON stones
USING ivfflat (embedding vector_cosine_ops)
WITH (lists = 100);

COMMIT;
