-- Migration 014: Add stones.embedding vector column and backfill data
-- Purpose:
-- 1) Ensure pgvector extension exists.
-- 2) Add stones.embedding (vector(256)) for vector similarity queries.
-- 3) Backfill from legacy stone_embeddings.embedding (TEXT) when available.
-- 4) Create ivfflat index for similarity retrieval.

BEGIN;

DO $$
DECLARE
    has_vector_extension BOOLEAN := FALSE;
BEGIN
    BEGIN
        CREATE EXTENSION IF NOT EXISTS vector;
    EXCEPTION WHEN OTHERS THEN
        RAISE NOTICE '014 skipped: pgvector extension unavailable: %', SQLERRM;
    END;

    SELECT EXISTS (
        SELECT 1 FROM pg_extension WHERE extname = 'vector'
    ) INTO has_vector_extension;

    IF NOT has_vector_extension THEN
        RETURN;
    END IF;

    IF NOT EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'stones'
          AND column_name = 'embedding'
    ) THEN
        ALTER TABLE stones ADD COLUMN embedding vector(256);
    END IF;
    
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

    BEGIN
        CREATE INDEX IF NOT EXISTS idx_stones_embedding_ivfflat
        ON stones
        USING ivfflat (embedding vector_cosine_ops)
        WITH (lists = 100);
    EXCEPTION WHEN OTHERS THEN
        RAISE NOTICE '014 index creation skipped: %', SQLERRM;
    END;
EXCEPTION WHEN OTHERS THEN
    RAISE NOTICE '014 migration skipped: %', SQLERRM;
END $$;

COMMIT;
