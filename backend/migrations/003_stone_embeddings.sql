-- Migration: Add stone_embeddings table for vector search
-- Created for Task #10: 湖神意志与共鸣搜索

CREATE TABLE IF NOT EXISTS stone_embeddings (
    stone_id VARCHAR(64) PRIMARY KEY REFERENCES stones(stone_id) ON DELETE CASCADE,
    embedding TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_stone_embeddings_created ON stone_embeddings(created_at);

-- DOWN
-- DROP INDEX IF EXISTS idx_stone_embeddings_created;
-- DROP TABLE IF EXISTS stone_embeddings;
