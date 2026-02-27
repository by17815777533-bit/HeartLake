-- HeartLake Core Content Seed (Only Stones + Boats + Comments/Ripples)
-- 目标：仅注入内容层数据，其他功能性表交给系统自动推导，便于定位代码问题
-- 用法：
--   set -a; source backend/.env; set +a
--   psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -v ON_ERROR_STOP=1 -f scripts/seed_core_content_only.sql
--   ./scripts/recompute_showcase_sentiment.py

BEGIN;

-- 0) 清理此前的 showcase 非核心数据（避免干扰“系统自动推导”验证）
DELETE FROM vip_upgrade_logs WHERE reason LIKE 'showcase:%';
DELETE FROM differential_privacy_budget WHERE query_type LIKE 'showcase_%';
DELETE FROM edge_ai_inference_logs WHERE input_hash LIKE 'showcase_%';
DELETE FROM lake_god_messages WHERE content LIKE 'showcase:%' OR user_id LIKE 'showcase_user_%';
DELETE FROM community_emotion_snapshots WHERE dominant_emotion LIKE 'showcase_%';
DELETE FROM user_interaction_history WHERE interaction_type LIKE 'showcase_%';
DELETE FROM user_emotion_history WHERE content_snippet LIKE 'showcase:%' OR user_id LIKE 'showcase_user_%';
DELETE FROM emotion_tracking WHERE content_hash LIKE 'showcase_%';
DELETE FROM notifications WHERE notification_id LIKE 'showcase_notif_%';
DELETE FROM temp_friends WHERE temp_friend_id LIKE 'showcase_temp_%';
DELETE FROM friends WHERE friendship_id LIKE 'showcase_friend_%';
DELETE FROM resonance_points WHERE reason LIKE 'showcase:%';
DELETE FROM edge_nodes WHERE node_id LIKE 'showcase_edge_node_%';
DELETE FROM federated_learning_rounds WHERE global_model_hash LIKE 'showcase_model_%';

-- 上一轮相似度是无前缀灌入，这里清空让系统自行重建
DELETE FROM user_similarity;

-- 1) 清理并重建 showcase 内容层
DELETE FROM paper_boats WHERE boat_id LIKE 'showcase_boat_%';
DELETE FROM ripples WHERE ripple_id LIKE 'showcase_ripple_%';
DELETE FROM stone_embeddings WHERE stone_id LIKE 'showcase_stone_%';
DELETE FROM stones WHERE stone_id LIKE 'showcase_stone_%';

-- 2) 补充展示用户（仅作为作者/互动者）
WITH s AS (
    SELECT generate_series(1, 120) AS idx
)
INSERT INTO users (
    id, shadow_id, user_id, username, nickname, avatar_url, bio, device_id,
    role, status, is_anonymous, last_login_at, last_active_at, created_at, updated_at
)
SELECT
    'showcase_user_' || lpad(idx::text, 3, '0') AS id,
    'shadow_showcase_' || lpad(idx::text, 3, '0') AS shadow_id,
    'showcase_user_' || lpad(idx::text, 3, '0') AS user_id,
    'showcase_' || lpad(idx::text, 3, '0') AS username,
    '旅人' || lpad(idx::text, 3, '0') AS nickname,
    'https://api.dicebear.com/7.x/glass/svg?seed=showcase' || idx::text AS avatar_url,
    '内容层演示用户：仅用于石头与纸船链路验证。' AS bio,
    'device-showcase-' || lpad(idx::text, 3, '0') AS device_id,
    'user' AS role,
    'active' AS status,
    TRUE AS is_anonymous,
    NOW() - ((idx % 5) || ' days')::interval AS last_login_at,
    NOW() - ((idx % 36) || ' hours')::interval AS last_active_at,
    NOW() - ((idx % 45) || ' days')::interval AS created_at,
    NOW() - ((idx % 2) || ' days')::interval AS updated_at
FROM s
ON CONFLICT (id) DO NOTHING;

-- 3) 目标用户集合：全量 active，确保当前前端账号也被覆盖
CREATE TEMP TABLE tmp_seed_users ON COMMIT DROP AS
SELECT
    id AS user_id,
    COALESCE(NULLIF(nickname, ''), '匿名旅人') AS nickname,
    row_number() OVER (ORDER BY id) AS rn
FROM users
WHERE status = 'active';

-- 4) 石头（内容多样，含近24小时热度 + 近30天趋势）
-- ✍🏻 保证 showcase 文案与 mood_type / emotion_score 语义一致，避免演示数据“文案积极但标签消极”。
WITH base_raw AS (
    SELECT
        u.user_id,
        u.nickname,
        u.rn,
        g AS seq,
        ((u.rn * 7 + g * 11) % 10) AS template_idx,
        CASE
            WHEN g <= 4 THEN NOW() - ((g - 1) * 4 + (u.rn % 4)) * INTERVAL '1 hour'
            WHEN g <= 6 THEN NOW() - ((g - 3) * 2 + (u.rn % 3)) * INTERVAL '1 day'
            ELSE NOW() - ((g - 5) * 6 + (u.rn % 5)) * INTERVAL '1 day'
        END AS created_at
    FROM tmp_seed_users u
    CROSS JOIN generate_series(1, 8) AS g
),
base AS (
    SELECT
        user_id,
        nickname,
        rn,
        seq,
        CASE template_idx
            WHEN 0 THEN 'happy'
            WHEN 1 THEN 'grateful'
            WHEN 2 THEN 'anxious'
            WHEN 3 THEN 'calm'
            WHEN 4 THEN 'lonely'
            WHEN 5 THEN 'happy'
            WHEN 6 THEN 'confused'
            WHEN 7 THEN 'grateful'
            WHEN 8 THEN 'neutral'
            ELSE 'hopeful'
        END AS mood_type,
        CASE template_idx
            WHEN 0 THEN 0.84
            WHEN 1 THEN 0.72
            WHEN 2 THEN -0.49
            WHEN 3 THEN 0.42
            WHEN 4 THEN -0.57
            WHEN 5 THEN 0.66
            WHEN 6 THEN -0.08
            WHEN 7 THEN 0.71
            WHEN 8 THEN 0.03
            ELSE 0.58
        END AS emotion_score,
        CASE template_idx
            WHEN 0 THEN '今天收到礼物，颜色像湖面一样温柔，我把开心写成石头。'
            WHEN 1 THEN '老师今天夸了我，我把这份被看见的力量留在这里。'
            WHEN 2 THEN '我有点焦虑，但正在用呼吸把自己慢慢拉回来。'
            WHEN 3 THEN '和朋友散步后，心绪安静下来，像风停在湖面。'
            WHEN 4 THEN '今晚有些孤独，想把这份脆弱轻轻放进心湖。'
            WHEN 5 THEN '完成任务后很有成就感，想分享给同频的人。'
            WHEN 6 THEN '我还在迷茫，但愿意继续尝试，继续发光。'
            WHEN 7 THEN '被一句善意打动，原来温暖会沿着人群传递。'
            WHEN 8 THEN '情绪像潮汐，有起伏，但我能感到自己在成长。'
            ELSE '把祝福折成纸船，送给今天需要勇气的人。'
        END AS content,
        created_at
    FROM base_raw
)
INSERT INTO stones (
    stone_id, user_id, content, stone_type, stone_color, mood_type, is_anonymous,
    nickname, emotion_score, sentiment_score, ripple_count, boat_count, view_count,
    status, tags, ai_tags, created_at, updated_at
)
SELECT
    'showcase_stone_' || lpad(rn::text, 4, '0') || '_' || lpad(seq::text, 2, '0') AS stone_id,
    user_id,
    content,
    CASE WHEN seq % 3 = 0 THEN 'small' WHEN seq % 3 = 1 THEN 'medium' ELSE 'large' END AS stone_type,
    (ARRAY['#7BA7A5','#E6B89C','#A0C4FF','#B8E0A5','#CDB4DB','#FFD166'])[1 + ((rn + seq) % 6)] AS stone_color,
    mood_type,
    ((rn + seq) % 3) = 0 AS is_anonymous,
    nickname,
    emotion_score,
    LEAST(1.0, GREATEST(-1.0, emotion_score + ((rn % 7) - 3) * 0.02)) AS sentiment_score,
    0, 0,
    ((rn * 17 + seq * 13) % 320) AS view_count,
    'published' AS status,
    ARRAY['心湖','核心内容','石头']::text[] AS tags,
    ARRAY['showcase']::text[] AS ai_tags,
    created_at,
    created_at + INTERVAL '5 minutes'
FROM base;

-- 5) 涟漪（评论互动信号之一）
WITH meta AS (
    SELECT COUNT(*)::int AS n FROM tmp_seed_users
),
seed_stones AS (
    SELECT
        s.stone_id,
        s.user_id,
        su.rn AS owner_rn,
        s.created_at
    FROM stones s
    JOIN tmp_seed_users su ON su.user_id = s.user_id
    WHERE s.stone_id LIKE 'showcase_stone_%'
),
candidates AS (
    SELECT
        'showcase_ripple_' || substr(md5(ss.stone_id || ':' || ru.user_id), 1, 24) AS ripple_id,
        ss.stone_id,
        ru.user_id,
        ss.created_at + ((ru.rn % 15) + 6) * INTERVAL '1 minute' AS created_at
    FROM seed_stones ss
    JOIN meta m ON TRUE
    JOIN LATERAL (
        VALUES
            (((ss.owner_rn + 3  - 1) % m.n) + 1),
            (((ss.owner_rn + 11 - 1) % m.n) + 1),
            (((ss.owner_rn + 23 - 1) % m.n) + 1)
    ) AS pick(rn) ON TRUE
    JOIN tmp_seed_users ru ON ru.rn = pick.rn
    WHERE ru.user_id <> ss.user_id
)
INSERT INTO ripples (ripple_id, stone_id, user_id, created_at)
SELECT ripple_id, stone_id, user_id, created_at
FROM candidates;

-- 6) 纸船（评论正文）
WITH meta AS (
    SELECT COUNT(*)::int AS n FROM tmp_seed_users
),
seed_stones AS (
    SELECT
        s.stone_id,
        s.user_id,
        su.rn AS owner_rn,
        s.created_at
    FROM stones s
    JOIN tmp_seed_users su ON su.user_id = s.user_id
    WHERE s.stone_id LIKE 'showcase_stone_%'
),
slots AS (
    SELECT ss.stone_id, ss.user_id AS receiver_id, ss.owner_rn, ss.created_at, 1 AS slot_idx
    FROM seed_stones ss
    UNION ALL
    SELECT ss.stone_id, ss.user_id AS receiver_id, ss.owner_rn, ss.created_at, 2 AS slot_idx
    FROM seed_stones ss
),
candidates AS (
    SELECT
        'showcase_boat_' || substr(md5(sl.stone_id || ':' || sl.slot_idx::text || ':' || su.user_id), 1, 24) AS boat_id,
        sl.stone_id,
        su.user_id AS sender_id,
        sl.receiver_id,
        CASE ((su.rn + sl.slot_idx) % 8)
            WHEN 0 THEN '你的文字像一束灯，我看见了你的努力。'
            WHEN 1 THEN '给你一个轻轻的拥抱，愿今晚可以睡个好觉。'
            WHEN 2 THEN '你并不孤单，我愿意在这片湖里陪你走一段。'
            WHEN 3 THEN '谢谢你分享真实感受，你的勇气很珍贵。'
            WHEN 4 THEN '把这份温暖继续传下去，我们一起点亮彼此。'
            WHEN 5 THEN '先休息一下，等风停了再出发也很好。'
            WHEN 6 THEN '你已经做得很好了，别忘了肯定今天的自己。'
            ELSE '送你一只小纸船，愿它带着好运靠岸。'
        END AS content,
        (ARRAY['hopeful','calm','grateful','neutral'])[1 + ((su.rn + sl.slot_idx) % 4)] AS mood,
        CASE WHEN sl.slot_idx = 1 THEN 'random' ELSE 'direct' END AS drift_mode,
        CASE WHEN (su.rn + sl.slot_idx) % 2 = 0 THEN 'paper' ELSE 'glow' END AS boat_style,
        TRUE AS is_anonymous,
        'active' AS status,
        sl.created_at + ((sl.slot_idx * 25) + (su.rn % 17)) * INTERVAL '1 minute' AS created_at
    FROM slots sl
    JOIN meta m ON TRUE
    JOIN tmp_seed_users su
      ON su.rn = CASE
          WHEN sl.slot_idx = 1 THEN (((sl.owner_rn + 5  - 1) % m.n) + 1)
          ELSE (((sl.owner_rn + 17 - 1) % m.n) + 1)
      END
    WHERE su.user_id <> sl.receiver_id
)
INSERT INTO paper_boats (
    boat_id, stone_id, sender_id, receiver_id, content, mood, drift_mode,
    boat_style, is_anonymous, status, created_at, updated_at
)
SELECT
    boat_id, stone_id, sender_id, receiver_id, content, mood, drift_mode,
    boat_style, is_anonymous, status, created_at, created_at
FROM candidates;

-- 7) 回填石头计数
UPDATE stones s
SET ripple_count = 0, boat_count = 0
WHERE s.stone_id LIKE 'showcase_stone_%';

UPDATE stones s
SET ripple_count = x.cnt
FROM (
    SELECT stone_id, COUNT(*)::int AS cnt
    FROM ripples
    WHERE stone_id LIKE 'showcase_stone_%'
    GROUP BY stone_id
) x
WHERE s.stone_id = x.stone_id;

UPDATE stones s
SET boat_count = x.cnt
FROM (
    SELECT stone_id, COUNT(*)::int AS cnt
    FROM paper_boats
    WHERE stone_id LIKE 'showcase_stone_%'
    GROUP BY stone_id
) x
WHERE s.stone_id = x.stone_id;

-- 8) 为石头补轻量 embedding（仅为向量页面不报空）
INSERT INTO stone_embeddings (stone_id, embedding, created_at)
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
    ], ',') || ']' AS embedding,
    NOW()
FROM stones s
WHERE s.stone_id LIKE 'showcase_stone_%';

COMMIT;

-- 9) 摘要输出
SELECT
    (SELECT COUNT(*) FROM stones WHERE stone_id LIKE 'showcase_stone_%') AS showcase_stones,
    (SELECT COUNT(*) FROM paper_boats WHERE boat_id LIKE 'showcase_boat_%') AS showcase_boats,
    (SELECT COUNT(*) FROM ripples WHERE ripple_id LIKE 'showcase_ripple_%') AS showcase_ripples,
    (SELECT COUNT(*) FROM friends WHERE friendship_id LIKE 'showcase_friend_%') AS showcase_friends_should_be_zero,
    (SELECT COUNT(*) FROM notifications WHERE notification_id LIKE 'showcase_notif_%') AS showcase_notifications_should_be_zero,
    (SELECT COUNT(*) FROM user_interaction_history WHERE interaction_type LIKE 'showcase_%') AS showcase_interactions_should_be_zero;
