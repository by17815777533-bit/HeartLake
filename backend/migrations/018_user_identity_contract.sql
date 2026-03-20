-- 018: 用户身份契约统一
-- 目标：
-- 1) 将 users.user_id 收敛为和 users.id 一致的稳定业务主键
-- 2) 为未来写入提供同步触发器，防止 id / user_id 再次漂移
-- 3) 将核心业务表的外键统一切到 users(user_id)
BEGIN;

-- users.id 是现有主键，也是历史外键的来源；先将 user_id 回填并与 id 保持一致
UPDATE users
SET user_id = id
WHERE user_id IS NULL
   OR user_id = ''
   OR user_id <> id;

ALTER TABLE users
    ALTER COLUMN user_id SET NOT NULL;

CREATE OR REPLACE FUNCTION sync_users_identity_columns()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.id IS NULL OR NEW.id = '' THEN
        NEW.id := NEW.user_id;
    END IF;

    IF NEW.user_id IS NULL OR NEW.user_id = '' THEN
        NEW.user_id := NEW.id;
    END IF;

    IF NEW.id IS NULL OR NEW.id = '' THEN
        RAISE EXCEPTION 'users.id and users.user_id cannot both be empty';
    END IF;

    -- 统一以 id 为最终值，避免外键和值语义继续分叉。
    NEW.user_id := NEW.id;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_users_sync_identity ON users;
CREATE TRIGGER trg_users_sync_identity
BEFORE INSERT OR UPDATE OF id, user_id ON users
FOR EACH ROW
EXECUTE FUNCTION sync_users_identity_columns();

CREATE OR REPLACE FUNCTION __heartlake_ensure_user_fk(
    target_table_name text,
    target_column text,
    constraint_name text,
    on_delete_action text DEFAULT 'CASCADE'
)
RETURNS void AS $$
DECLARE
    target_table regclass;
    existing record;
BEGIN
    target_table := to_regclass(target_table_name);
    IF target_table IS NULL THEN
        RETURN;
    END IF;

    FOR existing IN
        SELECT con.conname
        FROM pg_constraint con
        JOIN unnest(con.conkey) AS cols(attnum) ON TRUE
        JOIN pg_attribute attr
          ON attr.attrelid = con.conrelid
         AND attr.attnum = cols.attnum
        WHERE con.contype = 'f'
          AND con.conrelid = target_table
          AND con.confrelid = 'users'::regclass
          AND attr.attname = target_column
    LOOP
        EXECUTE format(
            'ALTER TABLE %s DROP CONSTRAINT IF EXISTS %I',
            target_table, existing.conname
        );
    END LOOP;

    EXECUTE format(
        'ALTER TABLE %s ADD CONSTRAINT %I FOREIGN KEY (%I) REFERENCES users(user_id) ON DELETE %s',
        target_table, constraint_name, target_column, on_delete_action
    );
END;
$$ LANGUAGE plpgsql;

SELECT __heartlake_ensure_user_fk('user_similarity', 'user1_id', 'fk_user_similarity_user1');
SELECT __heartlake_ensure_user_fk('user_similarity', 'user2_id', 'fk_user_similarity_user2');
SELECT __heartlake_ensure_user_fk('stones', 'user_id', 'fk_stones_user');
SELECT __heartlake_ensure_user_fk('friends', 'user_id', 'fk_friends_user');
SELECT __heartlake_ensure_user_fk('friends', 'friend_id', 'fk_friends_friend');
SELECT __heartlake_ensure_user_fk('temp_friends', 'user1_id', 'fk_temp_friends_user1');
SELECT __heartlake_ensure_user_fk('temp_friends', 'user2_id', 'fk_temp_friends_user2');
SELECT __heartlake_ensure_user_fk('friend_messages', 'sender_id', 'fk_friend_messages_sender');
SELECT __heartlake_ensure_user_fk('friend_messages', 'receiver_id', 'fk_friend_messages_receiver');
SELECT __heartlake_ensure_user_fk('ripples', 'user_id', 'fk_ripples_user');
SELECT __heartlake_ensure_user_fk('connections', 'user_id', 'fk_connections_user');
SELECT __heartlake_ensure_user_fk('connections', 'target_user_id', 'fk_connections_target_user');
SELECT __heartlake_ensure_user_fk('notifications', 'user_id', 'fk_notifications_user');
SELECT __heartlake_ensure_user_fk('consultation_sessions', 'user_id', 'fk_consultation_user');
SELECT __heartlake_ensure_user_fk('data_export_tasks', 'user_id', 'fk_data_export_tasks_user', 'NO ACTION');
SELECT __heartlake_ensure_user_fk('user_followups', 'user_id', 'fk_user_followups_user');

DROP FUNCTION IF EXISTS __heartlake_ensure_user_fk(text, text, text, text);

COMMIT;
