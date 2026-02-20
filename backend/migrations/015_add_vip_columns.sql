-- Migration: Add VIP columns to users table
-- UP

ALTER TABLE users ADD COLUMN IF NOT EXISTS vip_level INT DEFAULT 0;
ALTER TABLE users ADD COLUMN IF NOT EXISTS vip_expires_at TIMESTAMP;

-- DOWN
-- ALTER TABLE users DROP COLUMN IF EXISTS vip_expires_at;
-- ALTER TABLE users DROP COLUMN IF EXISTS vip_level;
