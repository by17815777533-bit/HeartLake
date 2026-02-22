-- 022_fix_foreign_key_references.sql
-- 修复外键引用：stones/consultation_sessions/friend_messages 的外键应指向 users(user_id) 而非 users(id)
-- 代码中所有地方都使用 user_id 列操作，外键必须与之一致

BEGIN;

-- stones.user_id → users(user_id)
ALTER TABLE stones DROP CONSTRAINT IF EXISTS stones_user_id_fkey;
ALTER TABLE stones ADD CONSTRAINT stones_user_id_fkey
  FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE;

-- consultation_sessions.user_id → users(user_id)
ALTER TABLE consultation_sessions DROP CONSTRAINT IF EXISTS fk_user;
ALTER TABLE consultation_sessions ADD CONSTRAINT fk_user
  FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE;

-- friend_messages.sender_id/receiver_id → users(user_id)
ALTER TABLE friend_messages DROP CONSTRAINT IF EXISTS friend_messages_sender_id_fkey;
ALTER TABLE friend_messages DROP CONSTRAINT IF EXISTS friend_messages_receiver_id_fkey;
ALTER TABLE friend_messages ADD CONSTRAINT friend_messages_sender_id_fkey
  FOREIGN KEY (sender_id) REFERENCES users(user_id) ON DELETE CASCADE;
ALTER TABLE friend_messages ADD CONSTRAINT friend_messages_receiver_id_fkey
  FOREIGN KEY (receiver_id) REFERENCES users(user_id) ON DELETE CASCADE;

COMMIT;
