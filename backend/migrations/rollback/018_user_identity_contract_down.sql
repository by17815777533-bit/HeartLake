BEGIN;

DROP TRIGGER IF EXISTS trg_users_sync_identity ON users;
DROP FUNCTION IF EXISTS sync_users_identity_columns();

ALTER TABLE IF EXISTS user_followups DROP CONSTRAINT IF EXISTS fk_user_followups_user;
ALTER TABLE IF EXISTS data_export_tasks DROP CONSTRAINT IF EXISTS fk_data_export_tasks_user;
ALTER TABLE IF EXISTS consultation_sessions DROP CONSTRAINT IF EXISTS fk_consultation_user;
ALTER TABLE IF EXISTS notifications DROP CONSTRAINT IF EXISTS fk_notifications_user;
ALTER TABLE IF EXISTS connections DROP CONSTRAINT IF EXISTS fk_connections_target_user;
ALTER TABLE IF EXISTS connections DROP CONSTRAINT IF EXISTS fk_connections_user;
ALTER TABLE IF EXISTS ripples DROP CONSTRAINT IF EXISTS fk_ripples_user;
ALTER TABLE IF EXISTS friend_messages DROP CONSTRAINT IF EXISTS fk_friend_messages_receiver;
ALTER TABLE IF EXISTS friend_messages DROP CONSTRAINT IF EXISTS fk_friend_messages_sender;
ALTER TABLE IF EXISTS temp_friends DROP CONSTRAINT IF EXISTS fk_temp_friends_user2;
ALTER TABLE IF EXISTS temp_friends DROP CONSTRAINT IF EXISTS fk_temp_friends_user1;
ALTER TABLE IF EXISTS friends DROP CONSTRAINT IF EXISTS fk_friends_friend;
ALTER TABLE IF EXISTS friends DROP CONSTRAINT IF EXISTS fk_friends_user;
ALTER TABLE IF EXISTS stones DROP CONSTRAINT IF EXISTS fk_stones_user;
ALTER TABLE IF EXISTS user_similarity DROP CONSTRAINT IF EXISTS fk_user_similarity_user2;
ALTER TABLE IF EXISTS user_similarity DROP CONSTRAINT IF EXISTS fk_user_similarity_user1;

ALTER TABLE users
    ALTER COLUMN user_id DROP NOT NULL;

COMMIT;
