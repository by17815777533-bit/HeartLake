-- 022_fix_foreign_key_references.sql
-- 修复外键引用：确保所有外键指向 users(id)（实际主键）
-- users.id 是 PK，users.user_id 只是兼容别名列

BEGIN;

-- stones.user_id → users(id)
ALTER TABLE stones DROP CONSTRAINT IF EXISTS stones_user_id_fkey;
ALTER TABLE stones ADD CONSTRAINT stones_user_id_fkey
  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE;

-- consultation_sessions.user_id → users(id)
ALTER TABLE consultation_sessions DROP CONSTRAINT IF EXISTS fk_user;
ALTER TABLE consultation_sessions DROP CONSTRAINT IF EXISTS fk_consultation_user;
ALTER TABLE consultation_sessions ADD CONSTRAINT fk_consultation_user
  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE;

-- friend_messages.sender_id/receiver_id → users(id)
ALTER TABLE friend_messages DROP CONSTRAINT IF EXISTS friend_messages_sender_id_fkey;
ALTER TABLE friend_messages DROP CONSTRAINT IF EXISTS friend_messages_receiver_id_fkey;
ALTER TABLE friend_messages ADD CONSTRAINT friend_messages_sender_id_fkey
  FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE;
ALTER TABLE friend_messages ADD CONSTRAINT friend_messages_receiver_id_fkey
  FOREIGN KEY (receiver_id) REFERENCES users(id) ON DELETE CASCADE;

COMMIT;
