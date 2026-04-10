BEGIN;

-- 1) purge old demo/showcase payload so the database stays presentation-friendly
DELETE FROM notifications
WHERE notification_id LIKE 'demo_%'
   OR notification_id LIKE 'showcase_%'
   OR user_id = 'demo_full_access'
   OR user_id LIKE 'demo_peer_%'
   OR user_id LIKE 'showcase_user_%';

DELETE FROM friends
WHERE user_id = 'demo_full_access'
   OR friend_id = 'demo_full_access'
   OR user_id LIKE 'demo_peer_%'
   OR friend_id LIKE 'demo_peer_%'
   OR user_id LIKE 'showcase_user_%'
   OR friend_id LIKE 'showcase_user_%';

DELETE FROM friend_messages
WHERE sender_id = 'demo_full_access'
   OR receiver_id = 'demo_full_access'
   OR sender_id LIKE 'demo_peer_%'
   OR receiver_id LIKE 'demo_peer_%';

DELETE FROM user_interaction_history
WHERE user_id = 'demo_full_access'
   OR user_id LIKE 'demo_peer_%'
   OR user_id LIKE 'showcase_user_%'
   OR stone_id LIKE 'demo_seed_stone_%'
   OR stone_id LIKE 'demo_showcase_stone_%'
   OR stone_id LIKE 'showcase_stone_%';

DELETE FROM resonance_points
WHERE user_id = 'demo_full_access'
   OR user_id LIKE 'demo_peer_%'
   OR user_id LIKE 'showcase_user_%';

DELETE FROM paper_boats
WHERE boat_id LIKE 'demo_%'
   OR boat_id LIKE 'showcase_%'
   OR sender_id = 'demo_full_access'
   OR receiver_id = 'demo_full_access'
   OR sender_id LIKE 'demo_peer_%'
   OR receiver_id LIKE 'demo_peer_%'
   OR sender_id LIKE 'showcase_user_%'
   OR receiver_id LIKE 'showcase_user_%'
   OR stone_id LIKE 'demo_seed_stone_%'
   OR stone_id LIKE 'demo_showcase_stone_%'
   OR stone_id LIKE 'showcase_stone_%';

DELETE FROM ripples
WHERE ripple_id LIKE 'demo_%'
   OR ripple_id LIKE 'showcase_%'
   OR user_id = 'demo_full_access'
   OR user_id LIKE 'demo_peer_%'
   OR user_id LIKE 'showcase_user_%'
   OR stone_id LIKE 'demo_seed_stone_%'
   OR stone_id LIKE 'demo_showcase_stone_%'
   OR stone_id LIKE 'showcase_stone_%';

DELETE FROM stone_embeddings
WHERE stone_id LIKE 'demo_seed_stone_%'
   OR stone_id LIKE 'demo_showcase_stone_%'
   OR stone_id LIKE 'showcase_stone_%';

DELETE FROM stones
WHERE stone_id LIKE 'demo_seed_stone_%'
   OR stone_id LIKE 'demo_showcase_stone_%'
   OR stone_id LIKE 'showcase_stone_%'
   OR user_id = 'demo_full_access'
   OR user_id LIKE 'demo_peer_%'
   OR user_id LIKE 'showcase_user_%';

DELETE FROM user_preferences
WHERE user_id = 'demo_full_access'
   OR user_id LIKE 'demo_peer_%'
   OR user_id LIKE 'showcase_user_%';

DELETE FROM user_privacy_settings
WHERE user_id = 'demo_full_access'
   OR user_id LIKE 'demo_peer_%'
   OR user_id LIKE 'showcase_user_%';

DELETE FROM vip_upgrade_logs
WHERE user_id = 'demo_full_access'
   OR user_id LIKE 'demo_peer_%'
   OR user_id LIKE 'showcase_user_%';

DELETE FROM users
WHERE user_id LIKE 'demo_peer_%'
   OR user_id LIKE 'showcase_user_%';

-- 2) upsert demo master account
INSERT INTO users (
    id, shadow_id, user_id, username, nickname, avatar_url, bio, device_id,
    role, status, is_anonymous, vip_level, vip_expires_at, resonance_total,
    is_guardian, guardian_since, last_login_at, last_active_at, created_at,
    updated_at, recovery_key_hash
) VALUES (
    'demo_full_access',
    'shadow_demo_full_access',
    'demo_full_access',
    'demo_showcase',
    '全平台演示号',
    'https://api.dicebear.com/7.x/glass/svg?seed=demo_full_access',
    '演示账号：资料页、推荐、情绪图、好友、纸船、守护和通知都可直接展示。',
    'device_demo_full_access',
    'user',
    'active',
    true,
    1,
    NULL,
    9999,
    true,
    NOW() - INTERVAL '120 days',
    NOW(),
    NOW(),
    NOW() - INTERVAL '180 days',
    NOW(),
    '3f5b4a9d8c2e1f60718293a4b5c6d7e8:d614f3c6c64deb2e2f55b9593b56d8fb2e3b16c1757b1dd4463766d6d4c10da5'
)
ON CONFLICT (id) DO UPDATE SET
    shadow_id = EXCLUDED.shadow_id,
    user_id = EXCLUDED.user_id,
    username = EXCLUDED.username,
    nickname = EXCLUDED.nickname,
    avatar_url = EXCLUDED.avatar_url,
    bio = EXCLUDED.bio,
    device_id = EXCLUDED.device_id,
    role = EXCLUDED.role,
    status = 'active',
    is_anonymous = EXCLUDED.is_anonymous,
    vip_level = EXCLUDED.vip_level,
    vip_expires_at = EXCLUDED.vip_expires_at,
    resonance_total = GREATEST(users.resonance_total, EXCLUDED.resonance_total),
    is_guardian = EXCLUDED.is_guardian,
    guardian_since = COALESCE(users.guardian_since, EXCLUDED.guardian_since),
    last_login_at = NOW(),
    last_active_at = NOW(),
    recovery_key_hash = EXCLUDED.recovery_key_hash,
    updated_at = NOW();

-- 3) seed a small set of peer accounts instead of bulk duplicates
INSERT INTO users (
    id, shadow_id, user_id, username, nickname, avatar_url, bio, device_id,
    role, status, is_anonymous, vip_level, resonance_total, is_guardian,
    last_login_at, last_active_at, created_at, updated_at
)
SELECT
    peer_id,
    'shadow_' || peer_id,
    peer_id,
    username,
    nickname,
    avatar_url,
    bio,
    'device_' || peer_id,
    'user',
    'active',
    true,
    0,
    120 + rn * 17,
    false,
    NOW() - make_interval(days => rn::int),
    NOW() - make_interval(hours => (rn * 3)::int),
    NOW() - make_interval(days => (60 + rn)::int),
    NOW()
FROM (
    VALUES
        (1, 'demo_peer_01', 'peer_morning', '晨光旅人', 'https://api.dicebear.com/7.x/glass/svg?seed=demo_peer_01', '擅长把慌张写成温柔。'),
        (2, 'demo_peer_02', 'peer_harbor', '归港旅人', 'https://api.dicebear.com/7.x/glass/svg?seed=demo_peer_02', '会给别人留一只纸船，也会给自己留灯。'),
        (3, 'demo_peer_03', 'peer_forest', '松风旅人', 'https://api.dicebear.com/7.x/glass/svg?seed=demo_peer_03', '习惯在情绪起伏里慢慢找回节奏。'),
        (4, 'demo_peer_04', 'peer_starlake', '星湖旅人', 'https://api.dicebear.com/7.x/glass/svg?seed=demo_peer_04', '愿意认真听别人说完一整段心事。')
) AS peers(rn, peer_id, username, nickname, avatar_url, bio)
ON CONFLICT (id) DO UPDATE SET
    shadow_id = EXCLUDED.shadow_id,
    user_id = EXCLUDED.user_id,
    username = EXCLUDED.username,
    nickname = EXCLUDED.nickname,
    avatar_url = EXCLUDED.avatar_url,
    bio = EXCLUDED.bio,
    device_id = EXCLUDED.device_id,
    role = EXCLUDED.role,
    status = 'active',
    is_anonymous = EXCLUDED.is_anonymous,
    vip_level = EXCLUDED.vip_level,
    is_guardian = EXCLUDED.is_guardian,
    last_login_at = EXCLUDED.last_login_at,
    last_active_at = EXCLUDED.last_active_at,
    updated_at = NOW();

INSERT INTO user_privacy_settings (
    user_id, profile_visibility, show_online_status, allow_friend_request, allow_message_from_stranger
)
VALUES
    ('demo_full_access', 'public', true, true, true),
    ('demo_peer_01', 'public', true, true, true),
    ('demo_peer_02', 'public', true, true, true),
    ('demo_peer_03', 'public', true, true, true),
    ('demo_peer_04', 'public', true, true, true)
ON CONFLICT (user_id) DO UPDATE SET
    profile_visibility = EXCLUDED.profile_visibility,
    show_online_status = EXCLUDED.show_online_status,
    allow_friend_request = EXCLUDED.allow_friend_request,
    allow_message_from_stranger = EXCLUDED.allow_message_from_stranger;

INSERT INTO vip_upgrade_logs (
    user_id, old_vip_level, new_vip_level, upgrade_type, reason, expires_at, created_at
)
VALUES (
    'demo_full_access', 0, 1, 'system_bootstrap', 'demo_seed:full_access', NULL, NOW()
);

INSERT INTO user_preferences (user_id, preferred_moods, preferred_tags, last_updated)
VALUES (
    'demo_full_access',
    ARRAY['hopeful', 'grateful', 'calm', 'lonely']::text[],
    ARRAY['演示', '推荐', '共鸣', '成长']::text[],
    NOW()
)
ON CONFLICT (user_id) DO UPDATE SET
    preferred_moods = EXCLUDED.preferred_moods,
    preferred_tags = EXCLUDED.preferred_tags,
    last_updated = NOW();

-- 4) insert a curated set of stones with visible variation
WITH stone_seed(stone_id, user_id, stone_type, stone_color, mood_type, emotion_score, content, created_at) AS (
    VALUES
        ('demo_seed_stone_01', 'demo_full_access', 'medium', '#7BA7A5', 'hopeful',  0.82, '把这周最难熬的一天记下来，提醒自己：我不是没撑住，我是在慢慢穿过去。', NOW() - INTERVAL '3 hours'),
        ('demo_seed_stone_02', 'demo_full_access', 'small',  '#E6B89C', 'grateful', 0.74, '午后收到一句很轻的安慰，却足够让我从紧绷里松一点下来。', NOW() - INTERVAL '1 day 2 hours'),
        ('demo_seed_stone_03', 'demo_full_access', 'large',  '#A0C4FF', 'calm',     0.42, '今天没有特别大的好消息，但我把呼吸找回来了，这已经很好。', NOW() - INTERVAL '2 days 4 hours'),
        ('demo_seed_stone_04', 'demo_full_access', 'medium', '#CDB4DB', 'lonely',  -0.37, '夜里还是会有空落感，不过我开始学着先站在自己这边。', NOW() - INTERVAL '4 days 1 hour'),
        ('demo_seed_stone_05', 'demo_full_access', 'small',  '#F4A261', 'anxious', -0.34, '焦虑没有一下子消失，但我终于肯把担心一条条写出来。', NOW() - INTERVAL '7 days 5 hours'),
        ('demo_seed_stone_06', 'demo_full_access', 'medium', '#B5E48C', 'hopeful',  0.76, '月底回看这段日子，发现自己其实已经比想象中走得更远。', NOW() - INTERVAL '12 days 2 hours'),
        ('demo_seed_stone_15', 'demo_full_access', 'medium', '#F2C6DE', 'grateful', 0.66, '把被照顾到的片刻记下来，提醒自己我并不是一直独自走。', NOW() - INTERVAL '16 days 4 hours'),
        ('demo_seed_stone_16', 'demo_full_access', 'small',  '#A8DADC', 'calm',     0.37, '今天把节奏放慢下来，才发现原来身体已经替我扛了太久。', NOW() - INTERVAL '19 days 6 hours'),
        ('demo_seed_stone_17', 'demo_full_access', 'medium', '#DDB892', 'hopeful',  0.61, '试着把目标缩小成一步，反而没有那么想逃了。', NOW() - INTERVAL '22 days 3 hours'),
        ('demo_seed_stone_18', 'demo_full_access', 'large',  '#B8C0FF', 'lonely',  -0.22, '想被理解的心情还在，但我已经不会再因为沉默责怪自己。', NOW() - INTERVAL '25 days 8 hours'),
        ('demo_seed_stone_19', 'demo_full_access', 'small',  '#CCD5AE', 'calm',     0.29, '给自己泡一杯热饮，算是把今天好好收尾。', NOW() - INTERVAL '28 days 2 hours'),
        ('demo_seed_stone_20', 'demo_full_access', 'medium', '#FFC8A2', 'anxious', -0.27, '有些担心还是会突然冒出来，但我比以前更知道怎么安顿它们。', NOW() - INTERVAL '30 days 5 hours'),
        ('demo_seed_stone_21', 'demo_full_access', 'medium', '#BDE0FE', 'hopeful',  0.58, '二月那段最混乱的时候过去了，现在回看，自己其实一直在恢复。', NOW() - INTERVAL '42 days 4 hours'),
        ('demo_seed_stone_22', 'demo_full_access', 'small',  '#E9EDC9', 'grateful', 0.52, '今天想谢谢那个没有催我快一点、只是让我先喘口气的人。', NOW() - INTERVAL '59 days 7 hours'),
        ('demo_seed_stone_23', 'demo_full_access', 'medium', '#C3F0CA', 'calm',     0.31, '把注意力从“我做得不够”挪到“我已经做到哪”，心里就安静了一点。', NOW() - INTERVAL '87 days 3 hours'),
        ('demo_seed_stone_24', 'demo_full_access', 'large',  '#E5C1CD', 'sad',     -0.33, '那段时间真的很难，但我现在已经可以平静地提起它。', NOW() - INTERVAL '131 days 6 hours'),
        ('demo_seed_stone_25', 'demo_full_access', 'small',  '#A9DEF9', 'hopeful',  0.49, '夏天写下的愿望，有一半已经悄悄发生了。', NOW() - INTERVAL '214 days 5 hours'),
        ('demo_seed_stone_26', 'demo_full_access', 'medium', '#F1C0E8', 'grateful', 0.44, '去年留下的那块石头，现在看起来像一封写给自己的回信。', NOW() - INTERVAL '302 days 8 hours'),
        ('demo_seed_stone_07', 'demo_peer_01',     'medium', '#F6BD60', 'grateful', 0.69, '有人认真听完我说的话，那一刻忽然觉得世界没有那么锋利。', NOW() - INTERVAL '5 hours'),
        ('demo_seed_stone_08', 'demo_peer_01',     'small',  '#84A59D', 'calm',     0.35, '把手机放远一点，听风声五分钟，心口就没有那么满了。', NOW() - INTERVAL '3 days 6 hours'),
        ('demo_seed_stone_09', 'demo_peer_02',     'large',  '#90BE6D', 'hopeful',  0.71, '今天想把勇气借给还在发抖的人，也顺便借一点给自己。', NOW() - INTERVAL '9 hours'),
        ('demo_seed_stone_10', 'demo_peer_02',     'medium', '#9D8189', 'lonely',  -0.29, '有些时刻还是会想被懂得，但我已经不再用沉默惩罚自己。', NOW() - INTERVAL '6 days 3 hours'),
        ('demo_seed_stone_11', 'demo_peer_03',     'medium', '#A9DEF9', 'calm',     0.33, '今天把节奏放慢一点，发现很多情绪不是对抗，而是需要被看见。', NOW() - INTERVAL '1 day 6 hours'),
        ('demo_seed_stone_12', 'demo_peer_03',     'small',  '#D4A373', 'sad',     -0.41, '难过并不体面，但我想允许自己先难过，再继续往前。', NOW() - INTERVAL '11 days 4 hours'),
        ('demo_seed_stone_13', 'demo_peer_04',     'large',  '#8ECAE6', 'hopeful',  0.79, '如果今天的你已经很累，就把明天缩小成一件可以完成的小事。', NOW() - INTERVAL '2 days 7 hours'),
        ('demo_seed_stone_14', 'demo_peer_04',     'medium', '#F2CC8F', 'grateful', 0.63, '谢谢那些没有急着给答案、只是愿意陪我一起坐一会儿的人。', NOW() - INTERVAL '14 days 2 hours')
)
INSERT INTO stones (
    stone_id, user_id, content, stone_type, stone_color, mood_type, is_anonymous,
    nickname, emotion_score, sentiment_score, ripple_count, boat_count, view_count,
    status, tags, ai_tags, created_at, updated_at
)
SELECT
    s.stone_id,
    s.user_id,
    s.content,
    s.stone_type,
    s.stone_color,
    s.mood_type,
    true,
    u.nickname,
    s.emotion_score,
    s.emotion_score,
    0,
    0,
    CASE WHEN s.user_id = 'demo_full_access' THEN 68 ELSE 44 END,
    'published',
    ARRAY['演示', '情绪', CASE WHEN s.user_id = 'demo_full_access' THEN '我的石头' ELSE '湖面样本' END]::text[],
    ARRAY['demo_seed']::text[],
    s.created_at,
    s.created_at + INTERVAL '5 minutes'
FROM stone_seed s
JOIN users u ON u.user_id = s.user_id;

CREATE TEMP TABLE demo_own_stones ON COMMIT DROP AS
SELECT stone_id, row_number() OVER (ORDER BY created_at DESC, stone_id) AS rn
FROM stones
WHERE user_id = 'demo_full_access'
ORDER BY created_at DESC, stone_id;

CREATE TEMP TABLE demo_peer_stones ON COMMIT DROP AS
SELECT stone_id, user_id AS owner_id, row_number() OVER (ORDER BY created_at DESC, stone_id) AS rn
FROM stones
WHERE user_id LIKE 'demo_peer_%'
ORDER BY created_at DESC, stone_id;

CREATE TEMP TABLE demo_peers ON COMMIT DROP AS
SELECT user_id, row_number() OVER (ORDER BY user_id) AS rn
FROM users
WHERE user_id LIKE 'demo_peer_%'
ORDER BY user_id;

-- 5) deterministic embeddings for recommendation/similar/resonance pages
UPDATE stones s
SET embedding = vec.embedding
FROM (
    SELECT stone_id,
           (
               '[' ||
               array_to_string(
                   array_agg(
                       (((get_byte(decode(md5(stone_id || ':' || gs::text), 'hex'), 0) - 128)::numeric / 128.0))::text
                       ORDER BY gs
                   ),
                   ','
               ) || ']'
           )::vector AS embedding
    FROM (
        SELECT stone_id, generate_series(1, 256) AS gs
        FROM stones
        WHERE stone_id LIKE 'demo_seed_stone_%'
    ) src
    GROUP BY stone_id
) vec
WHERE s.stone_id = vec.stone_id;

INSERT INTO stone_embeddings (stone_id, embedding, created_at)
SELECT stone_id, embedding::text, NOW()
FROM stones
WHERE stone_id LIKE 'demo_seed_stone_%';

-- 6) received ripples on demo stones
INSERT INTO ripples (ripple_id, stone_id, user_id, created_at)
SELECT
    'demo_ripple_in_' || lpad(ds.rn::text, 2, '0') || '_' || lpad(dp.rn::text, 2, '0'),
    ds.stone_id,
    dp.user_id,
    NOW() - make_interval(days => (ds.rn - 1)::int, hours => (dp.rn * 3)::int)
FROM demo_own_stones ds
JOIN demo_peers dp ON dp.rn <= 2
WHERE ds.rn <= 4;

-- 7) sent ripples from demo account
INSERT INTO ripples (ripple_id, stone_id, user_id, created_at)
SELECT
    'demo_ripple_out_' || lpad(ps.rn::text, 2, '0'),
    ps.stone_id,
    'demo_full_access',
    NOW() - make_interval(days => ps.rn::int, hours => 2)
FROM demo_peer_stones ps
WHERE ps.rn <= 5;

-- 8) received boats
INSERT INTO paper_boats (
    boat_id, stone_id, sender_id, receiver_id, content, mood, drift_mode,
    boat_style, is_anonymous, status, created_at, updated_at
)
SELECT
    'demo_boat_in_' || lpad(ds.rn::text, 2, '0') || '_' || lpad(dp.rn::text, 2, '0'),
    ds.stone_id,
    dp.user_id,
    'demo_full_access',
    CASE dp.rn
        WHEN 1 THEN '把这只纸船留给你，提醒你已经比自己想象中更稳了。'
        ELSE '你写下来的东西很有力量，愿今晚能比昨天轻一点。'
    END,
    CASE dp.rn WHEN 1 THEN 'grateful' ELSE 'hopeful' END,
    CASE WHEN dp.rn = 1 THEN 'direct' ELSE 'random' END,
    CASE WHEN dp.rn = 1 THEN 'glow' ELSE 'paper' END,
    true,
    'active',
    NOW() - make_interval(days => ds.rn::int, hours => dp.rn::int),
    NOW() - make_interval(days => ds.rn::int, hours => dp.rn::int)
FROM demo_own_stones ds
JOIN demo_peers dp ON dp.rn <= 2
WHERE ds.rn <= 3;

-- 9) sent boats from demo account
INSERT INTO paper_boats (
    boat_id, stone_id, sender_id, receiver_id, content, mood, drift_mode,
    boat_style, is_anonymous, status, created_at, updated_at
)
SELECT
    'demo_boat_out_' || lpad(ps.rn::text, 2, '0'),
    ps.stone_id,
    'demo_full_access',
    ps.owner_id,
    CASE ps.rn
        WHEN 1 THEN '看到你的石头了，愿今晚能有一小段安静属于你。'
        WHEN 2 THEN '给你留一只纸船，祝你把心事慢慢放下。'
        ELSE '有波动也没关系，我们都在学着和情绪并肩走。'
    END,
    CASE WHEN ps.rn = 2 THEN 'hopeful' ELSE 'calm' END,
    'direct',
    CASE WHEN ps.rn = 2 THEN 'glow' ELSE 'paper' END,
    true,
    'active',
    NOW() - make_interval(days => ps.rn::int, hours => 1),
    NOW() - make_interval(days => ps.rn::int, hours => 1)
FROM demo_peer_stones ps
WHERE ps.rn <= 3;

-- 10) sync visible counters
WITH affected AS (
    SELECT stone_id FROM stones WHERE stone_id LIKE 'demo_seed_stone_%'
),
ripple_counts AS (
    SELECT stone_id, COUNT(*)::INTEGER AS cnt
    FROM ripples
    WHERE stone_id IN (SELECT stone_id FROM affected)
    GROUP BY stone_id
),
boat_counts AS (
    SELECT stone_id, COUNT(*)::INTEGER AS cnt
    FROM paper_boats
    WHERE stone_id IN (SELECT stone_id FROM affected)
      AND COALESCE(status, 'active') <> 'deleted'
    GROUP BY stone_id
)
UPDATE stones s
SET ripple_count = COALESCE(rc.cnt, 0),
    boat_count = COALESCE(bc.cnt, 0),
    updated_at = NOW()
FROM affected a
LEFT JOIN ripple_counts rc ON rc.stone_id = a.stone_id
LEFT JOIN boat_counts bc ON bc.stone_id = a.stone_id
WHERE s.stone_id = a.stone_id;

-- 11) friends for demo presentation
INSERT INTO friends (friendship_id, user_id, friend_id, status, created_at)
SELECT
    'demo_friend_a_' || lpad(dp.rn::text, 2, '0'),
    'demo_full_access',
    dp.user_id,
    'accepted',
    NOW() - make_interval(days => (dp.rn + 3)::int)
FROM demo_peers dp
WHERE dp.rn <= 3
ON CONFLICT (user_id, friend_id) DO UPDATE
SET status = EXCLUDED.status,
    created_at = EXCLUDED.created_at;

INSERT INTO friends (friendship_id, user_id, friend_id, status, created_at)
SELECT
    'demo_friend_b_' || lpad(dp.rn::text, 2, '0'),
    dp.user_id,
    'demo_full_access',
    'accepted',
    NOW() - make_interval(days => (dp.rn + 3)::int)
FROM demo_peers dp
WHERE dp.rn <= 3
ON CONFLICT (user_id, friend_id) DO UPDATE
SET status = EXCLUDED.status,
    created_at = EXCLUDED.created_at;

-- 11.5) seed friend chat history for presentation
INSERT INTO friend_messages (sender_id, receiver_id, content, created_at)
VALUES
    ('demo_peer_01', 'demo_full_access', '我看到你今天那块石头了，读完后很想给你回一句：你已经很努力了。', NOW() - INTERVAL '2 days 4 hours'),
    ('demo_full_access', 'demo_peer_01', '谢谢你来回应我，那句“你已经很努力了”真的让我松了一口气。', NOW() - INTERVAL '2 days 3 hours 40 minutes'),
    ('demo_peer_01', 'demo_full_access', '如果今晚还是有点乱，就先把明天缩小成一件小事。', NOW() - INTERVAL '1 day 22 hours'),
    ('demo_full_access', 'demo_peer_01', '好，我准备先只完成一件小事，不再逼自己一下子全做完。', NOW() - INTERVAL '1 day 21 hours 30 minutes'),
    ('demo_peer_01', 'demo_full_access', '这已经很厉害了，慢慢来，我们都在学着和情绪并肩走。', NOW() - INTERVAL '12 hours'),
    ('demo_full_access', 'demo_peer_01', '收到，这次我会先照顾好自己，再去赶路。', NOW() - INTERVAL '11 hours 40 minutes');

-- 12) notifications without bulk duplicates
INSERT INTO notifications (
    user_id, type, title, content, related_id, is_read, created_at, notification_id, related_type
)
VALUES
    ('demo_full_access', 'system', '演示账号已准备好', '资料、推荐、情绪图、好友、纸船和通知都已经填充。', NULL, false, NOW() - INTERVAL '20 minutes', 'demo_notification_01', 'system'),
    ('demo_full_access', 'ripple', '你的石头有了新的回应', '晨光旅人给你留下了一道涟漪。', 'demo_seed_stone_01', false, NOW() - INTERVAL '3 hours', 'demo_notification_02', 'stone'),
    ('demo_full_access', 'boat', '收到新的纸船', '归港旅人送来一只纸船，提醒你慢慢向前。', 'demo_seed_stone_02', true, NOW() - INTERVAL '1 day', 'demo_notification_03', 'stone'),
    ('demo_full_access', 'friend', '你结识了新的同行者', '你已经和三位旅人建立好友关系。', 'demo_peer_01', true, NOW() - INTERVAL '2 days', 'demo_notification_04', 'user');

-- 13) interaction history to drive recommendation
INSERT INTO user_interaction_history (
    user_id, stone_id, interaction_type, interaction_weight, dwell_time_seconds, created_at
)
SELECT
    'demo_full_access',
    ps.stone_id,
    'view',
    1.0,
    50 + ps.rn * 8,
    NOW() - make_interval(days => ps.rn::int, hours => 3)
FROM demo_peer_stones ps
WHERE ps.rn <= 6
ON CONFLICT (user_id, stone_id, interaction_type) DO UPDATE
SET interaction_weight = EXCLUDED.interaction_weight,
    dwell_time_seconds = EXCLUDED.dwell_time_seconds,
    created_at = EXCLUDED.created_at;

INSERT INTO user_interaction_history (
    user_id, stone_id, interaction_type, interaction_weight, dwell_time_seconds, created_at
)
SELECT
    'demo_full_access',
    ps.stone_id,
    'ripple',
    2.0,
    16,
    NOW() - make_interval(days => ps.rn::int, hours => 2)
FROM demo_peer_stones ps
WHERE ps.rn <= 4
ON CONFLICT (user_id, stone_id, interaction_type) DO UPDATE
SET interaction_weight = EXCLUDED.interaction_weight,
    dwell_time_seconds = EXCLUDED.dwell_time_seconds,
    created_at = EXCLUDED.created_at;

INSERT INTO user_interaction_history (
    user_id, stone_id, interaction_type, interaction_weight, dwell_time_seconds, created_at
)
SELECT
    'demo_full_access',
    ps.stone_id,
    'boat',
    3.0,
    22,
    NOW() - make_interval(days => ps.rn::int, hours => 1)
FROM demo_peer_stones ps
WHERE ps.rn <= 3
ON CONFLICT (user_id, stone_id, interaction_type) DO UPDATE
SET interaction_weight = EXCLUDED.interaction_weight,
    dwell_time_seconds = EXCLUDED.dwell_time_seconds,
    created_at = EXCLUDED.created_at;

-- 14) guardian points / profile counters
INSERT INTO resonance_points (user_id, points, reason, created_at)
VALUES
    ('demo_full_access', 2, 'quality_ripple:demo_01', NOW() - INTERVAL '1 day'),
    ('demo_full_access', 2, 'quality_ripple:demo_02', NOW() - INTERVAL '3 days'),
    ('demo_full_access', 2, 'quality_ripple:demo_03', NOW() - INTERVAL '5 days'),
    ('demo_full_access', 5, 'warm_boat:demo_01', NOW() - INTERVAL '2 days'),
    ('demo_full_access', 5, 'warm_boat:demo_02', NOW() - INTERVAL '6 days'),
    ('demo_full_access', 5, 'warm_boat:demo_03', NOW() - INTERVAL '9 days');

COMMIT;

SELECT
    (SELECT COUNT(*) FROM stones WHERE user_id = 'demo_full_access') AS demo_own_stones,
    (SELECT COUNT(*) FROM stones WHERE user_id LIKE 'demo_peer_%') AS demo_peer_stones,
    (SELECT COUNT(*) FROM friends WHERE user_id = 'demo_full_access' AND status = 'accepted') AS demo_friends,
    (SELECT COUNT(*) FROM paper_boats WHERE sender_id = 'demo_full_access' OR receiver_id = 'demo_full_access') AS demo_boats,
    (SELECT COUNT(*) FROM notifications WHERE user_id = 'demo_full_access') AS demo_notifications;
