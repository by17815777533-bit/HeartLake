--  HeartLake 真实使用场景数据注入脚本
-- 目标：注入多样化、复杂化的用户行为数据，覆盖内容流、社交流、情绪流与系统日志。
-- 用法：
--   set -a; source backend/.env; set +a
--   PGPASSWORD="$DB_PASSWORD" psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -v ON_ERROR_STOP=1 -f scripts/seed_realistic_usage_data.sql

BEGIN;

-- 1) 清理上一轮 sim_* 数据（仅清理本脚本产生的数据）
CREATE TEMP TABLE tmp_cleanup_stones ON COMMIT DROP AS
SELECT stone_id
FROM stones
WHERE user_id LIKE 'sim_user_%';

DELETE FROM connection_messages
WHERE connection_id LIKE 'sim_conn_%';

DELETE FROM connections
WHERE connection_id LIKE 'sim_conn_%';

DELETE FROM consultation_messages
WHERE session_id LIKE 'sim_session_%';

DELETE FROM consultation_sessions
WHERE id LIKE 'sim_session_%';

DELETE FROM friend_messages
WHERE sender_id LIKE 'sim_user_%'
   OR receiver_id LIKE 'sim_user_%';

DELETE FROM temp_friends
WHERE temp_friend_id LIKE 'sim_temp_friend_%'
   OR user1_id LIKE 'sim_user_%'
   OR user2_id LIKE 'sim_user_%';

DELETE FROM friends
WHERE friendship_id LIKE 'sim_friend_%'
   OR user_id LIKE 'sim_user_%'
   OR friend_id LIKE 'sim_user_%';

DELETE FROM lamp_transfers
WHERE from_user_id LIKE 'sim_user_%'
   OR to_user_id LIKE 'sim_user_%';

DELETE FROM notifications
WHERE notification_id LIKE 'sim_notif_%'
   OR user_id LIKE 'sim_user_%';

DELETE FROM user_interaction_history
WHERE user_id LIKE 'sim_user_%'
   OR stone_id IN (SELECT stone_id FROM tmp_cleanup_stones);

DELETE FROM paper_boats
WHERE boat_id LIKE 'sim_boat_%'
   OR sender_id LIKE 'sim_user_%'
   OR receiver_id LIKE 'sim_user_%'
   OR stone_id IN (SELECT stone_id FROM tmp_cleanup_stones);

DELETE FROM ripples
WHERE ripple_id LIKE 'sim_ripple_%'
   OR user_id LIKE 'sim_user_%'
   OR stone_id IN (SELECT stone_id FROM tmp_cleanup_stones);

DELETE FROM stone_embeddings
WHERE stone_id IN (SELECT stone_id FROM tmp_cleanup_stones);

DELETE FROM stones
WHERE stone_id IN (SELECT stone_id FROM tmp_cleanup_stones);

DELETE FROM user_emotion_history
WHERE user_id LIKE 'sim_user_%';

DELETE FROM emotion_tracking
WHERE user_id LIKE 'sim_user_%'
   OR content_hash LIKE 'sim_%';

DELETE FROM resonance_points
WHERE user_id LIKE 'sim_user_%'
   OR reason LIKE 'sim:%';

DELETE FROM vip_upgrade_logs
WHERE user_id LIKE 'sim_user_%'
   OR reason LIKE 'sim:%';

DELETE FROM differential_privacy_budget
WHERE user_id LIKE 'sim_user_%';

DELETE FROM edge_ai_inference_logs
WHERE user_id LIKE 'sim_user_%'
   OR input_hash LIKE 'sim_%';

DELETE FROM lake_god_messages
WHERE user_id LIKE 'sim_user_%';

DELETE FROM security_events
WHERE user_id LIKE 'sim_user_%';

DELETE FROM login_logs
WHERE user_id LIKE 'sim_user_%';

DELETE FROM user_sessions
WHERE user_id LIKE 'sim_user_%';

DELETE FROM user_followups
WHERE user_id LIKE 'sim_user_%';

DELETE FROM user_privacy_settings
WHERE user_id LIKE 'sim_user_%';

DELETE FROM data_export_tasks
WHERE user_id LIKE 'sim_user_%';

DELETE FROM user_similarity
WHERE user1_id LIKE 'sim_user_%'
   OR user2_id LIKE 'sim_user_%';

DELETE FROM intervention_log
WHERE user_id LIKE 'sim_user_%';

DELETE FROM users
WHERE id LIKE 'sim_user_%'
   OR user_id LIKE 'sim_user_%';

-- 2) 构建用户基表（分层活跃度）
CREATE TEMP TABLE tmp_sim_users ON COMMIT DROP AS
SELECT
    gs AS rn,
    'sim_user_' || lpad(gs::text, 4, '0') AS user_id,
    'shadow_sim_' || lpad(gs::text, 4, '0') AS shadow_id,
    'sim_' || lpad(gs::text, 4, '0') AS username,
    (ARRAY['晨光','晚风','远山','湖岸','清溪','微雨','星火','潮声','拾光','云影'])[1 + ((gs - 1) % 10)]
        || (ARRAY['旅人','守望者','拾梦人','摆渡者','听风者','追光者','同行者','归舟客','拾叶人','照夜者'])[1 + ((gs * 3) % 10)]
        || lpad(gs::text, 3, '0') AS nickname,
    CASE
        WHEN gs <= 20 THEN 'power'
        WHEN gs <= 80 THEN 'active'
        WHEN gs <= 160 THEN 'regular'
        ELSE 'quiet'
    END AS segment,
    CASE
        WHEN gs <= 20 THEN 48
        WHEN gs <= 80 THEN 24
        WHEN gs <= 160 THEN 12
        ELSE 5
    END AS planned_stones
FROM generate_series(1, 240) AS gs;

INSERT INTO users (
    id,
    shadow_id,
    user_id,
    username,
    nickname,
    avatar_url,
    bio,
    device_id,
    recovery_key,
    recovery_key_hash,
    role,
    status,
    is_anonymous,
    vip_level,
    vip_expires_at,
    resonance_total,
    is_guardian,
    guardian_since,
    last_login_at,
    last_active_at,
    created_at,
    updated_at
)
SELECT
    u.user_id,
    u.shadow_id,
    u.user_id,
    u.username,
    u.nickname,
    'https://api.dicebear.com/7.x/glass/svg?seed=' || u.user_id,
    CASE u.segment
        WHEN 'power' THEN '高频记录者，愿意分享情绪与支持他人。'
        WHEN 'active' THEN '稳定活跃用户，持续记录生活与心境。'
        WHEN 'regular' THEN '间歇表达情绪，偶尔参与互动。'
        ELSE '低频用户，倾向浏览与被动参与。'
    END,
    'sim-device-' || lpad(u.rn::text, 4, '0'),
    NULL,
    NULL,
    'user',
    'active',
    (u.rn % 4) <> 0,
    CASE
        WHEN u.rn % 25 = 0 THEN 3
        WHEN u.rn % 9 = 0 THEN 2
        WHEN u.rn % 5 = 0 THEN 1
        ELSE 0
    END,
    CASE
        WHEN u.rn % 25 = 0 THEN NOW() + INTERVAL '365 days'
        WHEN u.rn % 9 = 0 THEN NOW() + INTERVAL '180 days'
        WHEN u.rn % 5 = 0 THEN NOW() + INTERVAL '60 days'
        ELSE NULL
    END,
    0,
    (u.rn % 40 = 0),
    CASE
        WHEN u.rn % 40 = 0 THEN NOW() - (((u.rn * 11) % 160) || ' days')::interval
        ELSE NULL
    END,
    NOW() - (((u.rn * 7) % 35) || ' days')::interval - (((u.rn * 5) % 24) || ' hours')::interval,
    NOW() - (((u.rn * 3) % 10) || ' days')::interval - (((u.rn * 13) % 24) || ' hours')::interval,
    NOW() - (((u.rn * 17) % 320) || ' days')::interval,
    NOW() - (((u.rn * 19) % 7) || ' days')::interval
FROM tmp_sim_users u
ON CONFLICT (id) DO UPDATE
SET
    nickname = EXCLUDED.nickname,
    avatar_url = EXCLUDED.avatar_url,
    bio = EXCLUDED.bio,
    device_id = EXCLUDED.device_id,
    vip_level = EXCLUDED.vip_level,
    vip_expires_at = EXCLUDED.vip_expires_at,
    is_guardian = EXCLUDED.is_guardian,
    guardian_since = EXCLUDED.guardian_since,
    last_login_at = EXCLUDED.last_login_at,
    last_active_at = EXCLUDED.last_active_at,
    updated_at = NOW();

INSERT INTO user_privacy_settings (
    user_id,
    profile_visibility,
    show_online_status,
    allow_friend_request,
    allow_message_from_stranger
)
SELECT
    user_id,
    CASE
        WHEN rn % 11 = 0 THEN 'friends_only'
        WHEN rn % 17 = 0 THEN 'private'
        ELSE 'public'
    END,
    (rn % 7) <> 0,
    (rn % 9) <> 0,
    (rn % 6) = 0
FROM tmp_sim_users
ON CONFLICT (user_id) DO UPDATE
SET
    profile_visibility = EXCLUDED.profile_visibility,
    show_online_status = EXCLUDED.show_online_status,
    allow_friend_request = EXCLUDED.allow_friend_request,
    allow_message_from_stranger = EXCLUDED.allow_message_from_stranger;

-- 3) 登录态与安全事件
WITH login_events AS (
    SELECT
        u.user_id,
        g AS seq,
        CASE
            WHEN u.segment = 'power' THEN 9
            WHEN u.segment = 'active' THEN 6
            WHEN u.segment = 'regular' THEN 4
            ELSE 2
        END AS max_seq
    FROM tmp_sim_users u
    CROSS JOIN generate_series(1, 9) AS g
)
INSERT INTO login_logs (
    user_id,
    login_time,
    ip_address,
    device_type,
    location,
    success
)
SELECT
    user_id,
    NOW() - (((seq * 3 + length(user_id)) % 45) || ' days')::interval
        - (((seq * 5 + length(user_id)) % 24) || ' hours')::interval,
    '10.22.' || ((length(user_id) * 7 + seq) % 255)::text || '.' || ((seq * 17) % 255)::text,
    (ARRAY['android','ios','web','desktop'])[1 + ((seq + length(user_id)) % 4)],
    (ARRAY['上海','北京','深圳','杭州','成都','武汉'])[1 + ((seq * 2 + length(user_id)) % 6)],
    (seq % 8) <> 0
FROM login_events
WHERE seq <= max_seq;

WITH session_events AS (
    SELECT
        u.user_id,
        g AS seq,
        CASE
            WHEN u.segment = 'power' THEN 3
            WHEN u.segment = 'active' THEN 2
            ELSE 1
        END AS max_seq
    FROM tmp_sim_users u
    CROSS JOIN generate_series(1, 3) AS g
)
INSERT INTO user_sessions (
    user_id,
    device_type,
    device_name,
    ip_address,
    is_active,
    last_active_at,
    created_at
)
SELECT
    user_id,
    (ARRAY['android','ios','web'])[1 + ((seq + length(user_id)) % 3)],
    'SIM-' || upper(substr(md5(user_id || ':' || seq::text), 1, 8)),
    '172.18.' || ((seq * 11 + length(user_id)) % 255)::text || '.' || ((seq * 29) % 255)::text,
    seq = 1,
    NOW() - (((seq * 2 + length(user_id)) % 7) || ' days')::interval,
    NOW() - (((seq * 7 + length(user_id)) % 60) || ' days')::interval
FROM session_events
WHERE seq <= max_seq;

INSERT INTO security_events (
    user_id,
    event_type,
    severity,
    description,
    ip_address,
    user_agent,
    metadata,
    created_at
)
SELECT
    u.user_id,
    CASE (u.rn % 5)
        WHEN 0 THEN 'abnormal_login_attempt'
        WHEN 1 THEN 'token_rotation'
        WHEN 2 THEN 'passwordless_challenge'
        WHEN 3 THEN 'device_change'
        ELSE 'privacy_settings_changed'
    END,
    CASE
        WHEN u.rn % 13 = 0 THEN 'high'
        WHEN u.rn % 5 = 0 THEN 'medium'
        ELSE 'low'
    END,
    'sim: security event generated for realism',
    '192.168.' || (u.rn % 255)::text || '.' || ((u.rn * 3) % 255)::text,
    'HeartLake/1.' || ((u.rn % 4) + 2)::text || '.0',
    jsonb_build_object(
        'segment', u.segment,
        'risk_score', ((u.rn * 13) % 100) / 100.0,
        'channel', CASE WHEN u.rn % 2 = 0 THEN 'web' ELSE 'mobile' END
    ),
    NOW() - (((u.rn * 5) % 90) || ' days')::interval
FROM tmp_sim_users u
WHERE u.rn % 3 = 0;

-- 4) 石头内容流（UUID 格式 stone_id，避免前端校验报错）
CREATE TEMP TABLE tmp_sim_stones ON COMMIT DROP AS
WITH expanded AS (
    SELECT
        u.*,
        g AS seq
    FROM tmp_sim_users u
    CROSS JOIN generate_series(1, 48) AS g
    WHERE g <= u.planned_stones
),
hashed AS (
    SELECT
        e.*,
        md5('sim-stone:' || e.user_id || ':' || e.seq::text) AS h,
        get_byte(decode(md5('sim-stone:' || e.user_id || ':' || e.seq::text), 'hex'), 0) AS b0,
        get_byte(decode(md5('sim-stone:' || e.user_id || ':' || e.seq::text), 'hex'), 1) AS b1,
        get_byte(decode(md5('sim-stone:' || e.user_id || ':' || e.seq::text), 'hex'), 2) AS b2,
        get_byte(decode(md5('sim-stone:' || e.user_id || ':' || e.seq::text), 'hex'), 3) AS b3
    FROM expanded e
)
SELECT
    lower(
        substr(h, 1, 8) || '-' ||
        substr(h, 9, 4) || '-' ||
        substr(h, 13, 4) || '-' ||
        substr(h, 17, 4) || '-' ||
        substr(h, 21, 12)
    ) AS stone_id,
    user_id,
    rn AS owner_rn,
    seq AS local_seq,
    segment,
    (ARRAY[
        '今天在通勤路上突然想哭，后来抬头看见天光，心慢慢稳下来。',
        '和朋友吃了一顿简单的饭，突然觉得被理解是一种很大的礼物。',
        '事情没有一次做对，但我愿意给自己第二次机会。',
        '昨晚失眠到三点，今天依然完成了清单上的三件事。',
        '看到陌生人给老人让座，心里像被轻轻照亮。',
        '有点焦虑，但我把呼吸调慢之后，身体告诉我可以再坚持一下。',
        '今天被误解了，难受是真的，但我也在练习表达边界。',
        '我把一段旧回忆放下了，像把石头轻轻沉入湖底。',
        '没什么特别的大事，只是平静地过完了一天，这也很好。',
        '今天收到一句真诚的谢谢，原来温柔会回流。',
        '偶尔也会怀疑自己，不过我没有停下脚步。',
        '我在学着把“应该”换成“我想要”，感觉轻松了一点。',
        '做完体检报告后松了一口气，健康就是最大的底气。',
        '给自己做了一顿热饭，原来照顾自己也会有成就感。',
        '有人在评论区鼓励我，陌生人的善意真的很有力量。',
        '今天把坏情绪写下来，发现它没有想象中那么可怕。'
    ])[1 + (b0 % 16)] AS content,
    (ARRAY['happy','grateful','anxious','calm','sad','hopeful','neutral','confused','lonely','relieved'])[1 + (b1 % 10)] AS mood_type,
    (ARRAY[0.82,0.67,-0.43,0.35,-0.58,0.51,0.04,-0.18,-0.49,0.41])[1 + (b1 % 10)] AS emotion_score,
    (ARRAY['#F7C873','#8BC7A8','#7EA8F8','#D9A7C7','#B8D6F2','#F2B5A7','#C8E6A0','#F6D285','#9CC7D8','#E4C1AE'])[1 + (b2 % 10)] AS stone_color,
    (ARRAY['small','medium','large'])[1 + (b3 % 3)] AS stone_type,
    CASE
        WHEN seq <= 3 THEN NOW() - (((rn + seq * 3) % 12) || ' hours')::interval
        WHEN seq <= 10 THEN NOW() - (((rn * 2 + seq * 5) % 14) || ' days')::interval
                              - (((rn + seq) % 24) || ' hours')::interval
        ELSE NOW() - ((15 + ((rn * 5 + seq * 7) % 320)) || ' days')::interval
                    - (((rn * 3 + seq) % 24) || ' hours')::interval
    END AS created_at
FROM hashed;

INSERT INTO stones (
    stone_id,
    user_id,
    content,
    stone_type,
    stone_color,
    mood_type,
    is_anonymous,
    nickname,
    emotion_score,
    sentiment_score,
    ripple_count,
    boat_count,
    view_count,
    status,
    tags,
    ai_tags,
    created_at,
    updated_at
)
SELECT
    s.stone_id,
    s.user_id,
    s.content,
    s.stone_type,
    s.stone_color,
    s.mood_type,
    (s.owner_rn % 3) = 0,
    u.nickname,
    s.emotion_score,
    LEAST(1.0, GREATEST(-1.0, s.emotion_score + (((s.owner_rn + s.local_seq) % 7) - 3) * 0.03)),
    0,
    0,
    ((s.owner_rn * 19 + s.local_seq * 17) % 900),
    'published',
    ARRAY['心湖', s.segment, s.mood_type]::text[],
    ARRAY[
        CASE
            WHEN s.mood_type IN ('sad','lonely','anxious') THEN '需要陪伴'
            WHEN s.mood_type IN ('happy','grateful','relieved') THEN '积极能量'
            ELSE '日常记录'
        END,
        'simulated'
    ]::text[],
    s.created_at,
    s.created_at + (((s.owner_rn + s.local_seq) % 180) || ' minutes')::interval
FROM tmp_sim_stones s
JOIN tmp_sim_users u ON u.user_id = s.user_id;

-- 5) 涟漪与纸船
CREATE TEMP TABLE tmp_sim_ripples ON COMMIT DROP AS
WITH meta AS (
    SELECT COUNT(*)::int AS n FROM tmp_sim_users
),
slots AS (
    SELECT
        s.stone_id,
        s.user_id AS stone_owner_id,
        s.owner_rn,
        s.created_at AS stone_created_at,
        g AS slot,
        1 + (get_byte(decode(md5(s.stone_id), 'hex'), 1) % 6) AS slot_limit
    FROM tmp_sim_stones s
    CROSS JOIN generate_series(1, 6) AS g
),
picked AS (
    SELECT
        sl.*,
        (ARRAY[3,11,19,37,59,83])[sl.slot] AS offset_step
    FROM slots sl
    WHERE sl.slot <= sl.slot_limit
)
SELECT
    'sim_ripple_' || substr(md5(p.stone_id || ':' || ru.user_id), 1, 24) AS ripple_id,
    p.stone_id,
    ru.user_id,
    p.stone_owner_id,
    p.slot,
    p.stone_created_at + ((p.slot * 7 + (p.owner_rn % 9) + 5) || ' minutes')::interval AS created_at
FROM picked p
JOIN meta m ON TRUE
JOIN tmp_sim_users ru
  ON ru.rn = (((p.owner_rn + p.offset_step - 1) % m.n) + 1)
WHERE ru.user_id <> p.stone_owner_id;

INSERT INTO ripples (
    ripple_id,
    stone_id,
    user_id,
    created_at
)
SELECT
    ripple_id,
    stone_id,
    user_id,
    created_at
FROM tmp_sim_ripples
ON CONFLICT (stone_id, user_id) DO NOTHING;

INSERT INTO paper_boats (
    boat_id,
    stone_id,
    sender_id,
    receiver_id,
    content,
    mood,
    drift_mode,
    boat_style,
    is_anonymous,
    status,
    drift_delay_seconds,
    created_at,
    updated_at
)
SELECT
    'sim_boat_' || substr(md5(r.ripple_id || ':boat'), 1, 24),
    r.stone_id,
    r.user_id,
    r.stone_owner_id,
    (ARRAY[
        '看见你的文字了，你已经做得很好。',
        '给你留一只纸船，愿今晚能好好休息。',
        '你的表达很真诚，谢谢你把感受说出来。',
        '先照顾好自己，慢一点也没关系。',
        '把这一点光留给你，也留给明天的自己。',
        '你不是一个人，我们都在湖面彼此照看。'
    ])[1 + (get_byte(decode(md5(r.ripple_id), 'hex'), 2) % 6)],
    (ARRAY['hopeful','calm','grateful','neutral'])[1 + (r.slot % 4)],
    CASE WHEN r.slot % 2 = 0 THEN 'direct' ELSE 'random' END,
    CASE WHEN r.slot % 3 = 0 THEN 'glow' ELSE 'paper' END,
    (r.slot % 2) = 1,
    'active',
    15 + (r.slot * 20),
    r.created_at + ((r.slot * 3 + 2) || ' minutes')::interval,
    r.created_at + ((r.slot * 3 + 2) || ' minutes')::interval
FROM tmp_sim_ripples r
WHERE r.slot <= (1 + (get_byte(decode(md5(r.ripple_id), 'hex'), 0) % 3))
ON CONFLICT (boat_id) DO NOTHING;

-- 6) 好友关系、临时好友与私信
CREATE TEMP TABLE tmp_sim_friends ON COMMIT DROP AS
WITH meta AS (
    SELECT COUNT(*)::int AS n FROM tmp_sim_users
),
raw AS (
    SELECT
        u.user_id,
        u.rn,
        (ARRAY[1,5,13,29])[g] AS offset_step,
        g AS slot
    FROM tmp_sim_users u
    CROSS JOIN generate_series(1, 4) AS g
),
picked AS (
    SELECT
        r.user_id,
        r.rn,
        r.slot,
        ru.user_id AS friend_id
    FROM raw r
    JOIN meta m ON TRUE
    JOIN tmp_sim_users ru
      ON ru.rn = (((r.rn + r.offset_step - 1) % m.n) + 1)
    WHERE ru.user_id <> r.user_id
)
SELECT
    'sim_friend_' || substr(md5(user_id || ':' || friend_id), 1, 24) AS friendship_id,
    user_id,
    friend_id,
    CASE
        WHEN ((rn + slot * 2) % 7) = 0 THEN 'pending'
        ELSE 'accepted'
    END AS status,
    NOW() - (((length(user_id) * 3 + slot * 9) % 120) || ' days')::interval AS created_at
FROM picked;

INSERT INTO friends (
    friendship_id,
    user_id,
    friend_id,
    status,
    created_at
)
SELECT
    friendship_id,
    user_id,
    friend_id,
    status,
    created_at
FROM tmp_sim_friends
ON CONFLICT (user_id, friend_id) DO UPDATE
SET
    status = EXCLUDED.status,
    created_at = EXCLUDED.created_at;

WITH meta AS (
    SELECT COUNT(*)::int AS n FROM tmp_sim_users
),
raw AS (
    SELECT
        u.user_id AS requester_id,
        u.rn,
        (ARRAY[47,71])[g] AS offset_step,
        g AS slot
    FROM tmp_sim_users u
    CROSS JOIN generate_series(1, 2) AS g
    WHERE u.rn <= 120
),
pairs AS (
    SELECT
        least(r.requester_id, ru.user_id) AS user1_id,
        greatest(r.requester_id, ru.user_id) AS user2_id,
        r.requester_id,
        r.slot
    FROM raw r
    JOIN meta m ON TRUE
    JOIN tmp_sim_users ru
      ON ru.rn = (((r.rn + r.offset_step - 1) % m.n) + 1)
    WHERE ru.user_id <> r.requester_id
)
INSERT INTO temp_friends (
    temp_friend_id,
    user1_id,
    user2_id,
    requester_id,
    source,
    source_id,
    status,
    upgraded_to_friend,
    expires_at,
    created_at
)
SELECT DISTINCT
    'sim_temp_friend_' || substr(md5(user1_id || ':' || user2_id), 1, 24),
    user1_id,
    user2_id,
    requester_id,
    CASE WHEN slot = 1 THEN 'chat' ELSE 'stone' END,
    'sim_source_' || substr(md5(user1_id || user2_id), 1, 12),
    'active',
    FALSE,
    NOW() + (((slot * 6) + 18) || ' hours')::interval,
    NOW() - (((slot * 3) + 1) || ' hours')::interval
FROM pairs
ON CONFLICT (user1_id, user2_id) DO UPDATE
SET
    requester_id = EXCLUDED.requester_id,
    source = EXCLUDED.source,
    source_id = EXCLUDED.source_id,
    status = EXCLUDED.status,
    upgraded_to_friend = EXCLUDED.upgraded_to_friend,
    expires_at = EXCLUDED.expires_at,
    created_at = EXCLUDED.created_at;

WITH accepted_pairs AS (
    SELECT
        least(user_id, friend_id) AS user_a,
        greatest(user_id, friend_id) AS user_b,
        MIN(created_at) AS base_time
    FROM tmp_sim_friends
    WHERE status = 'accepted'
    GROUP BY 1, 2
),
msgs AS (
    SELECT
        p.user_a,
        p.user_b,
        p.base_time,
        g AS seq
    FROM accepted_pairs p
    CROSS JOIN generate_series(1, 4) AS g
)
INSERT INTO friend_messages (
    sender_id,
    receiver_id,
    content,
    created_at
)
SELECT
    CASE WHEN seq % 2 = 1 THEN user_a ELSE user_b END AS sender_id,
    CASE WHEN seq % 2 = 1 THEN user_b ELSE user_a END AS receiver_id,
    (ARRAY[
        '最近怎么样？我在湖边看到你的新石头了。',
        '谢谢你的回复，那句话帮我很多。',
        '今晚有点累，但和你聊聊就好多了。',
        '周末要不要一起参加心湖任务？',
        '我把纸船收到了，真的很暖心。',
        '晚安，愿你今晚有一个好梦。'
    ])[1 + ((seq + length(user_a)) % 6)],
    base_time + ((seq * 4 + 2) || ' hours')::interval
FROM msgs;

-- 7) 缘分连接与连接消息
CREATE TEMP TABLE tmp_sim_connections ON COMMIT DROP AS
WITH meta AS (
    SELECT COUNT(*)::int AS n FROM tmp_sim_users
),
candidates AS (
    SELECT
        s.stone_id,
        s.user_id,
        s.owner_rn,
        s.local_seq
    FROM tmp_sim_stones s
    WHERE s.local_seq IN (2, 5, 8)
      AND s.owner_rn <= 150
),
picked AS (
    SELECT
        c.*,
        ru.user_id AS target_user_id
    FROM candidates c
    JOIN meta m ON TRUE
    JOIN tmp_sim_users ru
      ON ru.rn = (((c.owner_rn + 97 + c.local_seq - 1) % m.n) + 1)
    WHERE ru.user_id <> c.user_id
)
SELECT
    'sim_conn_' || substr(md5(stone_id || ':' || user_id || ':' || target_user_id), 1, 24) AS connection_id,
    user_id,
    target_user_id,
    stone_id,
    CASE
        WHEN (local_seq % 5) = 0 THEN 'expired'
        WHEN (local_seq % 3) = 0 THEN 'pending'
        ELSE 'accepted'
    END AS status,
    CASE
        WHEN (local_seq % 5) = 0 THEN NOW() - INTERVAL '1 day'
        ELSE NOW() + INTERVAL '2 days'
    END AS expires_at,
    NOW() - (((owner_rn * 2 + local_seq) % 30) || ' days')::interval AS created_at
FROM picked;

INSERT INTO connections (
    connection_id,
    user_id,
    target_user_id,
    stone_id,
    status,
    expires_at,
    created_at
)
SELECT
    connection_id,
    user_id,
    target_user_id,
    stone_id,
    status,
    expires_at,
    created_at
FROM tmp_sim_connections
ON CONFLICT (connection_id) DO UPDATE
SET
    status = EXCLUDED.status,
    expires_at = EXCLUDED.expires_at,
    created_at = EXCLUDED.created_at;

WITH msg_source AS (
    SELECT
        c.connection_id,
        c.user_id,
        c.target_user_id,
        c.created_at,
        g AS seq
    FROM tmp_sim_connections c
    CROSS JOIN generate_series(1, 3) AS g
    WHERE c.status IN ('accepted', 'pending')
)
INSERT INTO connection_messages (
    connection_id,
    sender_id,
    content,
    created_at
)
SELECT
    connection_id,
    CASE WHEN seq % 2 = 1 THEN user_id ELSE target_user_id END AS sender_id,
    (ARRAY[
        '你好，我在湖面看到了你的石头，想和你打个招呼。',
        '谢谢你愿意连接，希望今天对你是温柔的一天。',
        '我们可以从一句近况开始，不着急。'
    ])[seq],
    created_at + ((seq * 11) || ' minutes')::interval
FROM msg_source;

-- 8) 用户互动历史（浏览/涟漪/收藏/分享）
INSERT INTO user_interaction_history (
    user_id,
    stone_id,
    interaction_type,
    interaction_weight,
    dwell_time_seconds,
    created_at
)
SELECT
    user_id,
    stone_id,
    'ripple',
    1.8,
    35 + (slot * 8),
    created_at
FROM tmp_sim_ripples
ON CONFLICT (user_id, stone_id, interaction_type) DO UPDATE
SET
    interaction_weight = EXCLUDED.interaction_weight,
    dwell_time_seconds = EXCLUDED.dwell_time_seconds,
    created_at = EXCLUDED.created_at;

WITH meta AS (
    SELECT COUNT(*)::int AS n FROM tmp_sim_users
),
view_events AS (
    SELECT
        s.stone_id,
        s.owner_rn,
        s.created_at,
        g AS slot,
        (ARRAY[9,27,45])[g] AS offset_step,
        (ARRAY['view','favorite','share'])[g] AS interaction_type
    FROM tmp_sim_stones s
    CROSS JOIN generate_series(1, 3) AS g
),
picked AS (
    SELECT
        v.stone_id,
        ru.user_id,
        v.interaction_type,
        v.created_at,
        v.slot
    FROM view_events v
    JOIN meta m ON TRUE
    JOIN tmp_sim_users ru
      ON ru.rn = (((v.owner_rn + v.offset_step - 1) % m.n) + 1)
)
INSERT INTO user_interaction_history (
    user_id,
    stone_id,
    interaction_type,
    interaction_weight,
    dwell_time_seconds,
    created_at
)
SELECT
    user_id,
    stone_id,
    interaction_type,
    CASE interaction_type
        WHEN 'view' THEN 1.0
        WHEN 'favorite' THEN 2.5
        ELSE 2.0
    END,
    18 + slot * 14,
    created_at + ((slot * 9 + 3) || ' minutes')::interval
FROM picked
ON CONFLICT (user_id, stone_id, interaction_type) DO UPDATE
SET
    interaction_weight = EXCLUDED.interaction_weight,
    dwell_time_seconds = EXCLUDED.dwell_time_seconds,
    created_at = EXCLUDED.created_at;

-- 9) 情绪历史、情绪追踪、随访
WITH emotion_rows AS (
    SELECT
        u.user_id,
        g AS seq,
        (ARRAY['happy','grateful','anxious','calm','sad','hopeful','neutral','confused','lonely','relieved'])[1 + ((u.rn + g) % 10)] AS mood_type,
        round(((((u.rn * 37 + g * 19) % 200)::numeric / 100.0) - 1.0), 3) AS sentiment_score,
        NOW() - (((g * 2 + (u.rn % 5)) % 120) || ' days')::interval - (((g * 3 + u.rn) % 24) || ' hours')::interval AS created_at
    FROM tmp_sim_users u
    CROSS JOIN generate_series(1, 36) AS g
)
INSERT INTO user_emotion_history (
    user_id,
    sentiment_score,
    mood_type,
    content_snippet,
    created_at
)
SELECT
    user_id,
    sentiment_score,
    mood_type,
    'sim: ' || (ARRAY[
        '今天状态有波动，但我在慢慢调整。',
        '和人交流后心情变得更轻一些。',
        '压力有点大，先让自己缓一缓。',
        '完成小目标，给自己一点肯定。',
        '需要被理解，也在学着表达需求。',
        '情绪回落了，准备重新出发。'
    ])[1 + ((length(user_id) + seq) % 6)],
    created_at
FROM emotion_rows;

WITH tracking_rows AS (
    SELECT
        u.user_id,
        g AS seq,
        round(((((u.rn * 29 + g * 23) % 200)::numeric / 100.0) - 1.0), 3) AS score,
        NOW() - (((g * 3 + (u.rn % 7)) % 150) || ' days')::interval AS created_at
    FROM tmp_sim_users u
    CROSS JOIN generate_series(1, 18) AS g
)
INSERT INTO emotion_tracking (
    user_id,
    score,
    content_hash,
    created_at
)
SELECT
    user_id,
    score,
    'sim_' || substr(md5(user_id || ':' || seq::text), 1, 20),
    created_at
FROM tracking_rows;

INSERT INTO user_followups (
    user_id,
    followup_day,
    created_at
)
SELECT
    u.user_id,
    d.day,
    NOW() - ((35 - d.day) || ' days')::interval
FROM tmp_sim_users u
JOIN LATERAL (
    VALUES (1), (3), (7), (14), (30)
) AS d(day) ON TRUE
WHERE u.rn % 4 <> 0
ON CONFLICT (user_id, followup_day) DO UPDATE
SET created_at = EXCLUDED.created_at;

-- 10) 通知
WITH notif_source AS (
    SELECT
        u.user_id,
        g AS seq,
        (ARRAY['friend_request','stone_replied','boat_arrived','lamp_transfer','system','guardian_update'])[g] AS type
    FROM tmp_sim_users u
    CROSS JOIN generate_series(1, 6) AS g
)
INSERT INTO notifications (
    user_id,
    type,
    title,
    content,
    related_id,
    is_read,
    created_at,
    notification_id,
    related_type
)
SELECT
    n.user_id,
    n.type,
    CASE n.type
        WHEN 'friend_request' THEN '新的同行申请'
        WHEN 'stone_replied' THEN '你的石头有新回应'
        WHEN 'boat_arrived' THEN '纸船靠岸'
        WHEN 'lamp_transfer' THEN '收到一盏灯'
        WHEN 'guardian_update' THEN '守护者动态'
        ELSE '系统提醒'
    END,
    CASE n.type
        WHEN 'friend_request' THEN '有人希望与你建立连接，去看看吧。'
        WHEN 'stone_replied' THEN '有人在你的石头下留下了温柔回应。'
        WHEN 'boat_arrived' THEN '新的纸船已送达，愿你被温柔看见。'
        WHEN 'lamp_transfer' THEN '你收到一盏灯，来自共鸣的传递。'
        WHEN 'guardian_update' THEN '守护者计划有新的阶段进展。'
        ELSE '你的心湖已完成一次数据同步。'
    END,
    'sim_related_' || substr(md5(n.user_id || ':' || n.seq::text), 1, 16),
    (n.seq % 3) = 0,
    NOW() - (((n.seq * 2 + length(n.user_id)) % 40) || ' days')::interval,
    'sim_notif_' || substr(md5(n.user_id || ':' || n.type), 1, 24),
    CASE
        WHEN n.type = 'friend_request' THEN 'friend'
        WHEN n.type IN ('stone_replied', 'boat_arrived') THEN 'stone'
        ELSE 'system'
    END
FROM notif_source n
ON CONFLICT DO NOTHING;

-- 11) 共鸣积分、灯传递、VIP 日志
INSERT INTO resonance_points (
    user_id,
    points,
    reason,
    created_at
)
SELECT
    u.user_id,
    (ARRAY[5,8,12,20])[1 + ((u.rn + g) % 4)],
    'sim:' || (ARRAY['daily_checkin','ripple_help','boat_reply','friend_support'])[1 + ((u.rn + g) % 4)],
    NOW() - (((u.rn + g * 4) % 120) || ' days')::interval
FROM tmp_sim_users u
CROSS JOIN generate_series(1, 4) AS g;

WITH accepted_pairs AS (
    SELECT
        user_id,
        friend_id,
        row_number() OVER (ORDER BY user_id, friend_id) AS rn
    FROM tmp_sim_friends
    WHERE status = 'accepted'
)
INSERT INTO lamp_transfers (
    from_user_id,
    to_user_id,
    created_at
)
SELECT
    user_id,
    friend_id,
    NOW() - (((rn * 3) % 90) || ' days')::interval
FROM accepted_pairs
WHERE rn <= 800;

INSERT INTO vip_upgrade_logs (
    user_id,
    old_vip_level,
    new_vip_level,
    upgrade_type,
    reason,
    expires_at,
    created_at
)
SELECT
    u.user_id,
    GREATEST(0, u.rn % 3 - 1),
    CASE
        WHEN u.rn % 25 = 0 THEN 3
        WHEN u.rn % 9 = 0 THEN 2
        ELSE 1
    END,
    CASE
        WHEN u.rn % 25 = 0 THEN 'manual_grant'
        ELSE 'auto_emotion'
    END,
    'sim: engagement-driven upgrade',
    NOW() + (((u.rn % 12) + 1) || ' months')::interval,
    NOW() - (((u.rn * 2) % 180) || ' days')::interval
FROM tmp_sim_users u
WHERE u.rn % 5 = 0;

-- 12) AI/隐私日志与社区快照
INSERT INTO differential_privacy_budget (
    user_id,
    query_type,
    epsilon_spent,
    delta_spent,
    noise_mechanism,
    created_at
)
SELECT
    u.user_id,
    q.query_type,
    q.epsilon_spent,
    1e-6,
    CASE WHEN q.query_type = 'emotion_trends' THEN 'gaussian' ELSE 'laplace' END,
    NOW() - (((u.rn + q.seq * 5) % 90) || ' days')::interval
FROM tmp_sim_users u
JOIN LATERAL (
    VALUES
        (1, 'emotion_trends', 0.35::double precision),
        (2, 'emotion_heatmap', 0.28::double precision),
        (3, 'friend_recommendation', 0.42::double precision)
) AS q(seq, query_type, epsilon_spent) ON TRUE;

INSERT INTO edge_ai_inference_logs (
    user_id,
    inference_type,
    input_hash,
    result,
    latency_ms,
    model_version,
    is_fallback,
    created_at
)
SELECT
    u.user_id,
    t.inference_type,
    'sim_' || substr(md5(u.user_id || ':' || t.inference_type || ':' || t.seq::text), 1, 28),
    jsonb_build_object(
        'mood', (ARRAY['happy','grateful','anxious','calm','sad','hopeful'])[1 + ((u.rn + t.seq) % 6)],
        'score', round(((((u.rn * 17 + t.seq * 13) % 200)::numeric / 100.0) - 1.0), 3),
        'confidence', round((0.60 + (((u.rn + t.seq) % 35) / 100.0)::numeric), 3)
    ),
    65 + ((u.rn * 3 + t.seq * 11) % 180),
    'onnx-heartlake-v2.3',
    (u.rn + t.seq) % 19 = 0,
    NOW() - (((u.rn * 2 + t.seq * 4) % 60) || ' days')::interval
FROM tmp_sim_users u
JOIN LATERAL (
    VALUES
        (1, 'emotion_analysis'),
        (2, 'similar_stone_recommendation'),
        (3, 'risk_screening'),
        (4, 'dialogue_summary')
) AS t(seq, inference_type) ON TRUE;

INSERT INTO community_emotion_snapshots (
    snapshot_time,
    total_posts,
    positive_count,
    neutral_count,
    negative_count,
    avg_sentiment,
    dominant_emotion,
    emotion_distribution,
    window_minutes
)
SELECT
    NOW() - ((g * 4) || ' hours')::interval,
    420 + ((g * 13) % 180),
    180 + ((g * 11) % 90),
    120 + ((g * 7) % 80),
    70 + ((g * 5) % 60),
    round((0.05 + ((g % 14) / 100.0)::numeric), 3),
    (ARRAY['calm','grateful','neutral','hopeful'])[1 + (g % 4)],
    jsonb_build_object(
        'happy', round((0.18 + ((g % 6) / 100.0)::numeric), 3),
        'grateful', round((0.20 + ((g % 5) / 100.0)::numeric), 3),
        'calm', round((0.24 + ((g % 4) / 100.0)::numeric), 3),
        'anxious', round((0.14 + ((g % 3) / 100.0)::numeric), 3),
        'sad', round((0.10 + ((g % 2) / 100.0)::numeric), 3)
    ),
    60
FROM generate_series(1, 72) AS g;

INSERT INTO federated_learning_rounds (
    round_number,
    participants_count,
    model_type,
    aggregation_method,
    global_model_hash,
    metrics,
    privacy_budget_spent,
    started_at,
    completed_at
)
SELECT
    r,
    80 + (r * 3),
    'emotion_encoder',
    'fedavg',
    'sim_model_' || substr(md5('round:' || r::text), 1, 20),
    jsonb_build_object(
        'loss', round((0.35 - r * 0.01)::numeric, 4),
        'accuracy', round((0.68 + r * 0.012)::numeric, 4)
    ),
    round((0.8 + r * 0.07)::numeric, 3),
    NOW() - ((r * 5 + 2) || ' days')::interval,
    NOW() - ((r * 5 + 1) || ' days')::interval
FROM generate_series(1, 12) AS r;

INSERT INTO edge_nodes (
    node_id,
    node_name,
    status,
    last_heartbeat,
    capabilities,
    load_score,
    total_inferences,
    avg_latency_ms,
    created_at,
    updated_at
)
SELECT
    'sim_edge_node_' || lpad(g::text, 2, '0'),
    'SIM-Edge-' || lpad(g::text, 2, '0'),
    CASE WHEN g % 4 = 0 THEN 'busy' ELSE 'active' END,
    NOW() - ((g * 3) || ' minutes')::interval,
    jsonb_build_object('emotion', true, 'recommendation', true, 'chat', g % 2 = 0),
    round((0.2 + g * 0.09)::numeric, 3),
    15000 + g * 1800,
    58 + g * 6,
    NOW() - ((g * 18) || ' days')::interval,
    NOW() - ((g * 5) || ' minutes')::interval
FROM generate_series(1, 8) AS g
ON CONFLICT (node_id) DO UPDATE
SET
    status = EXCLUDED.status,
    last_heartbeat = EXCLUDED.last_heartbeat,
    capabilities = EXCLUDED.capabilities,
    load_score = EXCLUDED.load_score,
    total_inferences = EXCLUDED.total_inferences,
    avg_latency_ms = EXCLUDED.avg_latency_ms,
    updated_at = EXCLUDED.updated_at;

-- 13) 湖神对话与咨询会话
WITH chat_users AS (
    SELECT user_id, rn
    FROM tmp_sim_users
    WHERE rn <= 80
),
dialog_rows AS (
    SELECT
        c.user_id,
        g AS seq,
        CASE WHEN g % 2 = 1 THEN 'user' ELSE 'assistant' END AS role,
        NOW() - (((c.rn + g) % 14) || ' days')::interval + ((g * 3) || ' minutes')::interval AS created_at
    FROM chat_users c
    CROSS JOIN generate_series(1, 6) AS g
)
INSERT INTO lake_god_messages (
    user_id,
    role,
    content,
    mood,
    emotion_score,
    created_at
)
SELECT
    user_id,
    role,
    CASE
        WHEN role = 'user' THEN (ARRAY[
            '我今天有点累，不知道该怎么调整。',
            '最近总是想很多，情绪容易波动。',
            '我想更稳定一点，但不知道从哪里开始。'
        ])[1 + (seq % 3)]
        ELSE (ARRAY[
            '你已经在认真面对自己了，这本身就很珍贵。',
            '先从一个最小可执行动作开始，比如深呼吸三次。',
            '允许情绪存在，再决定下一步，你会更稳。'
        ])[1 + (seq % 3)]
    END,
    (ARRAY['calm','hopeful','neutral'])[1 + (seq % 3)],
    round((0.1 + ((seq % 5) / 10.0)::numeric), 3),
    created_at
FROM dialog_rows;

WITH base AS (
    SELECT
        u.user_id,
        u.shadow_id,
        row_number() OVER (ORDER BY u.user_id) AS rn
    FROM tmp_sim_users u
    WHERE u.rn <= 30
)
INSERT INTO consultation_sessions (
    id,
    user_id,
    counselor_id,
    server_key,
    client_key,
    key_salt,
    status,
    created_at,
    ended_at
)
SELECT
    'sim_session_' || lpad(rn::text, 4, '0'),
    user_id,
    'sim_user_0001',
    'srv_' || substr(md5(user_id || ':server'), 1, 48),
    'cli_' || substr(md5(user_id || ':client'), 1, 48),
    'salt_' || substr(md5(user_id || ':salt'), 1, 16),
    CASE WHEN rn % 4 = 0 THEN 'active' ELSE 'completed' END,
    NOW() - (((rn * 3) % 45) || ' days')::interval,
    CASE WHEN rn % 4 = 0 THEN NULL ELSE NOW() - (((rn * 3) % 45 - 1) || ' days')::interval END
FROM base
ON CONFLICT (id) DO UPDATE
SET
    status = EXCLUDED.status,
    ended_at = EXCLUDED.ended_at;

WITH msg_source AS (
    SELECT
        s.id AS session_id,
        b.shadow_id AS user_shadow_id,
        row_number() OVER (PARTITION BY s.id ORDER BY g) AS seq,
        g
    FROM consultation_sessions s
    JOIN tmp_sim_users b ON b.user_id = s.user_id
    JOIN generate_series(1, 4) AS g ON TRUE
    WHERE s.id LIKE 'sim_session_%'
)
INSERT INTO consultation_messages (
    session_id,
    sender_shadow_id,
    ciphertext,
    iv,
    tag,
    created_at
)
SELECT
    session_id,
    CASE WHEN g % 2 = 1 THEN user_shadow_id ELSE 'shadow_sim_counselor_0001' END,
    'enc_' || substr(md5(session_id || ':' || g::text || ':cipher'), 1, 64),
    'iv_' || substr(md5(session_id || ':' || g::text || ':iv'), 1, 24),
    'tag_' || substr(md5(session_id || ':' || g::text || ':tag'), 1, 24),
    NOW() - (((seq * 4) % 20) || ' days')::interval + ((seq * 7) || ' minutes')::interval
FROM msg_source;

-- 14) 用户相似度与石头向量
WITH accepted AS (
    SELECT DISTINCT
        user_id,
        friend_id
    FROM tmp_sim_friends
    WHERE status = 'accepted'
),
sim_rows AS (
    SELECT
        user_id AS user1_id,
        friend_id AS user2_id,
        round((0.72 + ((length(user_id) + length(friend_id)) % 20) / 100.0)::numeric, 3) AS similarity_score
    FROM accepted
    UNION ALL
    SELECT
        friend_id AS user1_id,
        user_id AS user2_id,
        round((0.72 + ((length(user_id) + length(friend_id)) % 20) / 100.0)::numeric, 3) AS similarity_score
    FROM accepted
)
INSERT INTO user_similarity (
    user1_id,
    user2_id,
    similarity_score,
    updated_at
)
SELECT
    user1_id,
    user2_id,
    similarity_score,
    NOW()
FROM sim_rows
ON CONFLICT (user1_id, user2_id) DO UPDATE
SET
    similarity_score = EXCLUDED.similarity_score,
    updated_at = EXCLUDED.updated_at;

INSERT INTO stone_embeddings (
    stone_id,
    embedding,
    created_at
)
SELECT
    s.stone_id,
    '[' ||
    array_to_string(ARRAY[
        (((get_byte(decode(md5(s.stone_id), 'hex'), 0) - 128)::numeric / 128.0)::text),
        (((get_byte(decode(md5(s.stone_id), 'hex'), 1) - 128)::numeric / 128.0)::text),
        (((get_byte(decode(md5(s.stone_id), 'hex'), 2) - 128)::numeric / 128.0)::text),
        (((get_byte(decode(md5(s.stone_id), 'hex'), 3) - 128)::numeric / 128.0)::text),
        (((get_byte(decode(md5(s.stone_id), 'hex'), 4) - 128)::numeric / 128.0)::text),
        (((get_byte(decode(md5(s.stone_id), 'hex'), 5) - 128)::numeric / 128.0)::text),
        (((get_byte(decode(md5(s.stone_id), 'hex'), 6) - 128)::numeric / 128.0)::text),
        (((get_byte(decode(md5(s.stone_id), 'hex'), 7) - 128)::numeric / 128.0)::text)
    ], ',') || ']',
    NOW()
FROM tmp_sim_stones s
ON CONFLICT (stone_id) DO UPDATE
SET
    embedding = EXCLUDED.embedding,
    created_at = EXCLUDED.created_at;

-- 15) 反写统计字段
UPDATE stones s
SET
    ripple_count = 0,
    boat_count = 0
WHERE s.user_id LIKE 'sim_user_%';

UPDATE stones s
SET ripple_count = r.cnt
FROM (
    SELECT stone_id, COUNT(*)::int AS cnt
    FROM ripples
    WHERE stone_id IN (SELECT stone_id FROM tmp_sim_stones)
    GROUP BY stone_id
) r
WHERE s.stone_id = r.stone_id;

UPDATE stones s
SET boat_count = b.cnt
FROM (
    SELECT stone_id, COUNT(*)::int AS cnt
    FROM paper_boats
    WHERE stone_id IN (SELECT stone_id FROM tmp_sim_stones)
    GROUP BY stone_id
) b
WHERE s.stone_id = b.stone_id;

UPDATE users u
SET resonance_total = agg.total_points
FROM (
    SELECT user_id, COALESCE(SUM(points), 0)::int AS total_points
    FROM resonance_points
    WHERE user_id LIKE 'sim_user_%'
    GROUP BY user_id
) agg
WHERE u.id = agg.user_id;

COMMIT;

-- 16) 注入摘要
SELECT
    (SELECT COUNT(*) FROM users WHERE id LIKE 'sim_user_%') AS sim_users,
    (SELECT COUNT(*) FROM stones WHERE user_id LIKE 'sim_user_%') AS sim_stones,
    (SELECT COUNT(*) FROM ripples WHERE ripple_id LIKE 'sim_ripple_%') AS sim_ripples,
    (SELECT COUNT(*) FROM paper_boats WHERE boat_id LIKE 'sim_boat_%') AS sim_boats,
    (SELECT COUNT(*) FROM friends WHERE friendship_id LIKE 'sim_friend_%') AS sim_friends,
    (SELECT COUNT(*) FROM temp_friends WHERE temp_friend_id LIKE 'sim_temp_friend_%') AS sim_temp_friends,
    (SELECT COUNT(*) FROM friend_messages WHERE sender_id LIKE 'sim_user_%' OR receiver_id LIKE 'sim_user_%') AS sim_friend_messages,
    (SELECT COUNT(*) FROM connections WHERE connection_id LIKE 'sim_conn_%') AS sim_connections,
    (SELECT COUNT(*) FROM connection_messages WHERE connection_id LIKE 'sim_conn_%') AS sim_connection_messages,
    (SELECT COUNT(*) FROM notifications WHERE notification_id LIKE 'sim_notif_%') AS sim_notifications,
    (SELECT COUNT(*) FROM user_emotion_history WHERE user_id LIKE 'sim_user_%') AS sim_emotion_history,
    (SELECT COUNT(*) FROM user_interaction_history WHERE user_id LIKE 'sim_user_%') AS sim_interactions,
    (SELECT COUNT(*) FROM edge_ai_inference_logs WHERE user_id LIKE 'sim_user_%') AS sim_ai_logs,
    (SELECT COUNT(*) FROM differential_privacy_budget WHERE user_id LIKE 'sim_user_%') AS sim_dp_logs;
