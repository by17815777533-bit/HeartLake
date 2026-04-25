-- HeartLake Curated Showcase Seed
-- 目标：注入一批可用于演示、软著截图和推荐链路验证的高质量数据。
-- 原则：所有对用户可见的昵称、签名、石头正文、纸船回复均逐条策划，
--      不用模板循环拼接，避免展示时出现机械重复感。
-- 用法：
--   set -a; source backend/.env; set +a
--   psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -v ON_ERROR_STOP=1 -f scripts/seed_core_content_only.sql

BEGIN;

-- 0) 清理上一次 showcase 内容，保留真实用户和真实业务数据。
DELETE FROM notifications
WHERE notification_id LIKE 'showcase_%'
   OR user_id LIKE 'showcase_user_%';

DELETE FROM friend_messages
WHERE sender_id LIKE 'showcase_user_%'
   OR receiver_id LIKE 'showcase_user_%';

DELETE FROM temp_friends
WHERE temp_friend_id LIKE 'showcase_%'
   OR user1_id LIKE 'showcase_user_%'
   OR user2_id LIKE 'showcase_user_%';

DELETE FROM friends
WHERE friendship_id LIKE 'showcase_%'
   OR user_id LIKE 'showcase_user_%'
   OR friend_id LIKE 'showcase_user_%';

DELETE FROM user_interaction_history
WHERE user_id LIKE 'showcase_user_%'
   OR stone_id LIKE 'showcase_stone_%'
   OR interaction_type LIKE 'showcase_%';

DELETE FROM user_emotion_history
WHERE user_id LIKE 'showcase_user_%'
   OR content_snippet LIKE 'showcase:%';

DELETE FROM resonance_points
WHERE user_id LIKE 'showcase_user_%'
   OR reason LIKE 'showcase:%';

DELETE FROM paper_boats
WHERE boat_id LIKE 'showcase_boat_%'
   OR sender_id LIKE 'showcase_user_%'
   OR receiver_id LIKE 'showcase_user_%'
   OR stone_id LIKE 'showcase_stone_%';

DELETE FROM ripples
WHERE ripple_id LIKE 'showcase_ripple_%'
   OR user_id LIKE 'showcase_user_%'
   OR stone_id LIKE 'showcase_stone_%';

DELETE FROM stone_embeddings WHERE stone_id LIKE 'showcase_stone_%';
DELETE FROM stones WHERE stone_id LIKE 'showcase_stone_%' OR user_id LIKE 'showcase_user_%';
DELETE FROM user_preferences WHERE user_id LIKE 'showcase_user_%';
DELETE FROM user_privacy_settings WHERE user_id LIKE 'showcase_user_%';
DELETE FROM vip_upgrade_logs WHERE user_id LIKE 'showcase_user_%' OR reason LIKE 'showcase:%';
DELETE FROM users WHERE user_id LIKE 'showcase_user_%';

-- 1) 展示用户：32 个不同身份，覆盖学生、职场、家庭、创作、运动、独处等场景。
CREATE TEMP TABLE tmp_curated_users (
    id TEXT PRIMARY KEY,
    username TEXT NOT NULL,
    nickname TEXT NOT NULL,
    avatar_seed TEXT NOT NULL,
    bio TEXT NOT NULL,
    created_ago TEXT NOT NULL,
    active_ago TEXT NOT NULL,
    resonance_total INTEGER NOT NULL
) ON COMMIT DROP;

INSERT INTO tmp_curated_users VALUES
('showcase_user_001', 'curated_morning_tide', '晨汐', 'morning-tide', '习惯在早起的十分钟里把心事写下来。', '168 days', '35 minutes', 936),
('showcase_user_002', 'curated_harbor_lamp', '岸灯', 'harbor-lamp', '把工作日的疲惫收进一盏小灯里。', '142 days', '1 hour', 884),
('showcase_user_003', 'curated_small_wave', '微澜', 'small-wave', '正在学习把情绪说清楚，而不是藏起来。', '126 days', '2 hours', 812),
('showcase_user_004', 'curated_pine_breeze', '松间风', 'pine-breeze', '跑步、散步和慢慢吃饭都是我的修复方式。', '119 days', '3 hours', 790),
('showcase_user_005', 'curated_star_bridge', '星桥', 'star-bridge', '愿意给陌生人的夜晚留下一句温柔。', '112 days', '48 minutes', 865),
('showcase_user_006', 'curated_cloud_edge', '云边', 'cloud-edge', '白天很忙，晚上给自己留一点安静。', '108 days', '4 hours', 701),
('showcase_user_007', 'curated_homebound', '归航', 'homebound', '在不确定里找回方向感。', '101 days', '5 hours', 756),
('showcase_user_008', 'curated_bookmark', '慢热书签', 'slow-bookmark', '慢热但认真，喜欢把小事记成光。', '96 days', '18 minutes', 731),
('showcase_user_009', 'curated_after_rain', '雨后青苔', 'after-rain', '相信很多事会在安静里重新长出来。', '90 days', '6 hours', 688),
('showcase_user_010', 'curated_south_window', '南窗', 'south-window', '靠近阳光，也允许自己偶尔阴天。', '86 days', '2 hours', 710),
('showcase_user_011', 'curated_north_isle', '北屿', 'north-isle', '独处不是退后，是给心留一点空间。', '80 days', '7 hours', 663),
('showcase_user_012', 'curated_night_reader', '夜读人', 'night-reader', '深夜阅读时最容易和自己和解。', '76 days', '1 hour', 745),
('showcase_user_013', 'curated_xiaoman', '小满', 'xiaoman', '不追求圆满，先让今天刚刚好。', '72 days', '55 minutes', 698),
('showcase_user_014', 'curated_white_tea', '白茶', 'white-tea', '温和一点，慢一点，也是一种力量。', '69 days', '8 hours', 720),
('showcase_user_015', 'curated_orange_lamp', '橘灯', 'orange-lamp', '给低电量的日子补一点暖色。', '64 days', '3 hours', 689),
('showcase_user_016', 'curated_mountain_moon', '山月', 'mountain-moon', '喜欢把复杂的念头放到远处看。', '60 days', '6 hours', 654),
('showcase_user_017', 'curated_shallow_grass', '浅草', 'shallow-grass', '情绪起伏时会去楼下走一圈。', '56 days', '2 hours', 621),
('showcase_user_018', 'curated_kapok', '木棉', 'kapok', '认真生活，也认真休息。', '52 days', '5 hours', 672),
('showcase_user_019', 'curated_far_voice', '远声', 'far-voice', '希望自己的回应能抵达需要的人。', '48 days', '40 minutes', 746),
('showcase_user_020', 'curated_lychee_sea', '荔枝海', 'lychee-sea', '喜欢夏天，也喜欢把心放软。', '45 days', '9 hours', 601),
('showcase_user_021', 'curated_warbler', '知更', 'warbler', '清晨的第一杯水提醒我重新开始。', '41 days', '1 hour', 640),
('showcase_user_022', 'curated_clear_sky', '青空', 'clear-sky', '把压力拆小，再一点点完成。', '38 days', '2 hours', 618),
('showcase_user_023', 'curated_pinellia', '半夏', 'pinellia', '正在练习不把所有事都归咎于自己。', '34 days', '3 hours', 580),
('showcase_user_024', 'curated_warm_river', '暖川', 'warm-river', '愿每一次回应都像缓慢的河流。', '31 days', '50 minutes', 632),
('showcase_user_025', 'curated_ginkgo', '银杏', 'ginkgo', '收藏每个认真撑过来的瞬间。', '28 days', '4 hours', 576),
('showcase_user_026', 'curated_after_clear', '霁色', 'after-clear', '雨停之后，心里也会慢慢放晴。', '24 days', '90 minutes', 590),
('showcase_user_027', 'curated_reed', '芦苇', 'reed', '柔软不等于脆弱，我还在学习。', '21 days', '5 hours', 548),
('showcase_user_028', 'curated_glass_light', '璃光', 'glass-light', '给世界一点透明，也给自己一点边界。', '18 days', '30 minutes', 568),
('showcase_user_029', 'curated_flower_letter', '花信', 'flower-letter', '把迟来的好消息也算作生活的回信。', '15 days', '6 hours', 532),
('showcase_user_030', 'curated_soda', '苏打水', 'soda-water', '偶尔冒泡，偶尔安静，都算正常。', '12 days', '20 minutes', 510),
('showcase_user_031', 'curated_clear_glow', '清辉', 'clear-glow', '夜里也有不刺眼的光。', '9 days', '3 hours', 498),
('showcase_user_032', 'curated_desk_lamp', '伏案人', 'desk-lamp', '写完今天的最后一行，再好好睡觉。', '7 days', '1 hour', 485);

INSERT INTO users (
    id, shadow_id, user_id, username, nickname, avatar_url, bio, device_id,
    role, status, is_anonymous, vip_level, resonance_total, is_guardian,
    last_login_at, last_active_at, created_at, updated_at
)
SELECT
    id,
    'shadow_' || id,
    id,
    username,
    nickname,
    'https://api.dicebear.com/7.x/glass/svg?seed=' || avatar_seed,
    bio,
    'device_' || id,
    'user',
    'active',
    TRUE,
    0,
    resonance_total,
    FALSE,
    NOW() - active_ago::interval,
    NOW() - active_ago::interval,
    NOW() - created_ago::interval,
    NOW() - active_ago::interval
FROM tmp_curated_users
ON CONFLICT (id) DO UPDATE SET
    shadow_id = EXCLUDED.shadow_id,
    user_id = EXCLUDED.user_id,
    username = EXCLUDED.username,
    nickname = EXCLUDED.nickname,
    avatar_url = EXCLUDED.avatar_url,
    bio = EXCLUDED.bio,
    device_id = EXCLUDED.device_id,
    role = EXCLUDED.role,
    status = EXCLUDED.status,
    is_anonymous = EXCLUDED.is_anonymous,
    vip_level = EXCLUDED.vip_level,
    resonance_total = EXCLUDED.resonance_total,
    last_login_at = EXCLUDED.last_login_at,
    last_active_at = EXCLUDED.last_active_at,
    updated_at = EXCLUDED.updated_at;

INSERT INTO user_privacy_settings (
    user_id, profile_visibility, show_online_status, allow_friend_request, allow_message_from_stranger
)
SELECT id, 'public', TRUE, TRUE, TRUE
FROM tmp_curated_users
ON CONFLICT (user_id) DO UPDATE SET
    profile_visibility = EXCLUDED.profile_visibility,
    show_online_status = EXCLUDED.show_online_status,
    allow_friend_request = EXCLUDED.allow_friend_request,
    allow_message_from_stranger = EXCLUDED.allow_message_from_stranger;

-- 2) 展示石头：96 条人工策划正文，按真实使用场景分布在近 30 天。
CREATE TEMP TABLE tmp_curated_stones (
    stone_id TEXT PRIMARY KEY,
    user_id TEXT NOT NULL,
    mood_type TEXT NOT NULL,
    stone_type TEXT NOT NULL,
    stone_color TEXT NOT NULL,
    emotion_score DOUBLE PRECISION NOT NULL,
    content TEXT NOT NULL,
    tags TEXT[] NOT NULL,
    created_ago TEXT NOT NULL,
    view_count INTEGER NOT NULL
) ON COMMIT DROP;

INSERT INTO tmp_curated_stones VALUES
('showcase_stone_001', 'showcase_user_001', 'calm', 'medium', '#7BA7A5', 0.38, '今早六点醒来没有立刻看手机，先把窗帘拉开。外面的光很淡，但足够提醒我今天可以慢慢开始。', ARRAY['晨间','平静','自我照顾']::text[], '2 hours', 238),
('showcase_stone_002', 'showcase_user_001', 'hopeful', 'small', '#B8E0A5', 0.56, '把昨晚没做完的计划拆成三步之后，心里忽然没有那么堵了。原来我需要的不是更用力，而是更清楚。', ARRAY['计划','成长','希望']::text[], '1 day 4 hours', 196),
('showcase_stone_003', 'showcase_user_001', 'grateful', 'medium', '#FFD166', 0.66, '早餐摊阿姨记得我不加辣，那一瞬间被日常接住了。很小的善意，也能把人从匆忙里拉回来。', ARRAY['日常','感恩','温暖']::text[], '5 days 3 hours', 184),
('showcase_stone_004', 'showcase_user_002', 'anxious', 'large', '#CDB4DB', -0.42, '下班路上突然想到明天的汇报，肩膀一下紧起来。我没有继续硬扛，先把担心写成清单。', ARRAY['职场','焦虑','拆解问题']::text[], '3 hours', 286),
('showcase_stone_005', 'showcase_user_002', 'calm', 'medium', '#A8DADC', 0.34, '会议结束后给自己留了十分钟空白，没有马上进入下一件事。那十分钟像一个小缓冲垫。', ARRAY['工作','边界','平静']::text[], '2 days 2 hours', 212),
('showcase_stone_006', 'showcase_user_002', 'happy', 'small', '#FFB9A3', 0.74, '收到同事一句谢谢，才发现自己这周默默做的事真的被看见了。开心不是很大声，却很踏实。', ARRAY['职场','开心','被看见']::text[], '7 days 8 hours', 205),
('showcase_stone_007', 'showcase_user_003', 'confused', 'medium', '#B8C0FF', -0.12, '今天情绪像浅浅的浪，一会儿靠岸，一会儿退开。我还说不清原因，但愿意先承认它存在。', ARRAY['觉察','迷茫','情绪记录']::text[], '5 hours', 251),
('showcase_stone_008', 'showcase_user_003', 'anxious', 'large', '#D0BFFF', -0.46, '我把焦虑写在便签上，旁边又写了能做的一件小事。两列字放在一起，恐惧就没有那么巨大。', ARRAY['焦虑','便签','行动']::text[], '1 day 8 hours', 230),
('showcase_stone_009', 'showcase_user_003', 'calm', 'small', '#9ED2BE', 0.32, '晚上做了一碗汤，水汽把厨房玻璃熏得很白。那一刻我想，照顾自己也可以很具体。', ARRAY['晚餐','自我照顾','平静']::text[], '9 days 1 hour', 173),
('showcase_stone_010', 'showcase_user_004', 'happy', 'medium', '#F4A261', 0.71, '跑完三公里后腿有点酸，但脑子终于安静下来。原来身体动起来，心也会跟着换气。', ARRAY['运动','开心','释放']::text[], '6 hours', 267),
('showcase_stone_011', 'showcase_user_004', 'sad', 'medium', '#90A4AE', -0.52, '被一句无心的话影响了半天，后来才意识到我在意的不是那句话，而是自己又开始否定自己。', ARRAY['难过','自我理解','关系']::text[], '3 days 5 hours', 218),
('showcase_stone_012', 'showcase_user_004', 'grateful', 'small', '#F2CC8F', 0.63, '翻到去年冬天的照片，想起那段日子也有人陪我走过来。谢谢那时没有放弃联系我的朋友。', ARRAY['朋友','感恩','回忆']::text[], '12 days 6 hours', 188),
('showcase_stone_013', 'showcase_user_005', 'confused', 'large', '#BDE0FE', -0.08, '项目卡住一下午，我一度以为自己不适合做这件事。喝完水再回来，问题其实只需要换个入口。', ARRAY['项目','迷茫','复盘']::text[], '7 hours', 301),
('showcase_stone_014', 'showcase_user_005', 'hopeful', 'medium', '#A0C4FF', 0.58, '夜里看见窗外还有几盏灯亮着，忽然觉得并不是只有我在努力。愿我们都能稳稳收工。', ARRAY['夜晚','希望','陪伴']::text[], '2 days 9 hours', 246),
('showcase_stone_015', 'showcase_user_005', 'calm', 'small', '#C3F0CA', 0.36, '今天勇敢拒绝了一个不合适的安排。说出口的时候心跳很快，说完之后世界安静了一点。', ARRAY['边界','平静','勇气']::text[], '10 days 2 hours', 221),
('showcase_stone_016', 'showcase_user_006', 'neutral', 'medium', '#CCD5AE', 0.04, '白天被很多零碎消息切开，晚上把通知关掉二十分钟。不是逃避，是先把注意力还给自己。', ARRAY['专注','夜晚','中性']::text[], '8 hours', 214),
('showcase_stone_017', 'showcase_user_006', 'lonely', 'large', '#A9DEF9', -0.48, '在人很多的地方也会突然觉得孤单。后来我给自己点了一杯热饮，至少让手心先暖起来。', ARRAY['孤独','城市','照顾']::text[], '4 days 7 hours', 239),
('showcase_stone_018', 'showcase_user_006', 'grateful', 'small', '#E9EDC9', 0.61, '妈妈发来一句早点睡，语气和很多年前一样。被惦记这件事，真的会让人重新有力气。', ARRAY['家人','感恩','夜晚']::text[], '14 days 5 hours', 197),
('showcase_stone_019', 'showcase_user_007', 'hopeful', 'medium', '#B5E48C', 0.57, '把这个月最重要的事写在纸上，发现真正急的没有想象中那么多。方向感回来的时候，人会轻一点。', ARRAY['规划','希望','方向']::text[], '9 hours', 242),
('showcase_stone_020', 'showcase_user_007', 'sad', 'small', '#C8D6E5', -0.44, '今天有点想念以前的自己，那个更敢尝试、也更容易开心的人。也许我不是失去她，只是暂时离她远了。', ARRAY['想念','难过','成长']::text[], '2 days 12 hours', 226),
('showcase_stone_021', 'showcase_user_007', 'calm', 'large', '#84A59D', 0.41, '傍晚沿着河边走了很久，什么也没解决，但心从很满变成刚好能呼吸。', ARRAY['散步','平静','呼吸']::text[], '16 days 3 hours', 204),
('showcase_stone_022', 'showcase_user_008', 'happy', 'medium', '#FFAF87', 0.69, '今天终于读完那本拖了很久的书。合上书的一刻，像给自己兑现了一个小约定。', ARRAY['阅读','开心','完成']::text[], '10 hours', 233),
('showcase_stone_023', 'showcase_user_008', 'anxious', 'medium', '#E0BBE4', -0.38, '消息一直没有回复，我开始脑补很多可能。后来提醒自己：没有证据的担心，可以先放到旁边。', ARRAY['关系','焦虑','自我提醒']::text[], '3 days 8 hours', 257),
('showcase_stone_024', 'showcase_user_008', 'grateful', 'small', '#F6BD60', 0.64, '朋友没有急着给建议，只是听我说完。原来被完整听见，比立刻得到答案更重要。', ARRAY['倾听','感恩','朋友']::text[], '18 days 4 hours', 218),
('showcase_stone_025', 'showcase_user_009', 'calm', 'large', '#8ECAE6', 0.35, '雨停以后空气里有青草味，我站在楼下多待了两分钟。世界湿漉漉的，但很干净。', ARRAY['雨后','平静','自然']::text[], '11 hours', 221),
('showcase_stone_026', 'showcase_user_009', 'hopeful', 'medium', '#B8E0A5', 0.54, '之前担心的事情今天有了一个小进展，虽然不算结束，但足够让我相信路在往前。', ARRAY['进展','希望','坚持']::text[], '4 days 6 hours', 246),
('showcase_stone_027', 'showcase_user_009', 'sad', 'small', '#B0BEC5', -0.47, '有些话还是没能说出口。晚上回家时我没有责怪自己，只是承认今天还没准备好。', ARRAY['难过','表达','接纳']::text[], '20 days 2 hours', 193),
('showcase_stone_028', 'showcase_user_010', 'happy', 'medium', '#FFC8A2', 0.76, '阳光落在书桌边缘，连水杯都像在发亮。今天没有特别大的好事，但心情真的不错。', ARRAY['阳光','开心','日常']::text[], '12 hours', 244),
('showcase_stone_029', 'showcase_user_010', 'confused', 'large', '#CDB4DB', -0.16, '听了很多建议，反而更不知道自己想要什么。也许我需要先安静下来，听听自己的声音。', ARRAY['选择','迷茫','自我倾听']::text[], '5 days 5 hours', 271),
('showcase_stone_030', 'showcase_user_010', 'grateful', 'small', '#E9C46A', 0.67, '邻居帮我按住电梯的几秒钟，让匆忙的早晨变得没有那么紧。谢谢这些不被记录的小善意。', ARRAY['善意','感恩','城市']::text[], '21 days 7 hours', 199),
('showcase_stone_031', 'showcase_user_011', 'lonely', 'medium', '#A9DEF9', -0.51, '周末的房间太安静，我开了一盏小灯。它没有解决孤独，但让我感觉自己还在这里。', ARRAY['独处','孤独','夜灯']::text[], '13 hours', 265),
('showcase_stone_032', 'showcase_user_011', 'calm', 'small', '#A8DADC', 0.39, '给植物浇水时发现新叶子冒出来了。生命有自己的节奏，我也可以有。', ARRAY['植物','平静','节奏']::text[], '6 days 3 hours', 223),
('showcase_stone_033', 'showcase_user_011', 'hopeful', 'large', '#C3F0CA', 0.53, '把一件拖了很久的小事完成后，突然有了继续整理生活的勇气。小胜利也很有用。', ARRAY['完成','希望','整理']::text[], '23 days 4 hours', 206),
('showcase_stone_034', 'showcase_user_012', 'calm', 'medium', '#7BA7A5', 0.36, '深夜读到一句话：慢慢来也是来。合上书后，心里像被轻轻拍了一下。', ARRAY['阅读','夜晚','平静']::text[], '14 hours', 252),
('showcase_stone_035', 'showcase_user_012', 'anxious', 'large', '#D0BFFF', -0.43, '明天要面对一场重要谈话，我预演了很多遍。现在先把水喝完，再让自己早点睡。', ARRAY['谈话','焦虑','准备']::text[], '2 days 15 hours', 278),
('showcase_stone_036', 'showcase_user_012', 'grateful', 'small', '#FFD166', 0.62, '书店老板帮我留了想买的那本书。被一个小约定记住，今天因此亮了一点。', ARRAY['书店','感恩','约定']::text[], '19 days 8 hours', 208),
('showcase_stone_037', 'showcase_user_013', 'neutral', 'medium', '#CCD5AE', 0.02, '今天没有特别明显的情绪，只是按时吃饭、按时下楼、按时把自己带回家。平凡也算一种稳定。', ARRAY['稳定','中性','日常']::text[], '15 hours', 217),
('showcase_stone_038', 'showcase_user_013', 'happy', 'small', '#FFB9A3', 0.68, '把抽屉收拾干净后，心也像空出一格。原来整理外面的空间，会顺便整理里面。', ARRAY['整理','开心','空间']::text[], '4 days 9 hours', 235),
('showcase_stone_039', 'showcase_user_013', 'confused', 'large', '#B8C0FF', -0.14, '别人都走得很快，我偶尔会怀疑自己太慢。可是小满不必圆满，我可以先刚刚好。', ARRAY['节奏','迷茫','接纳']::text[], '22 days 6 hours', 202),
('showcase_stone_040', 'showcase_user_014', 'calm', 'medium', '#84A59D', 0.40, '泡茶的时候等水声变轻，心也跟着慢下来。今天决定不催促自己立刻好起来。', ARRAY['茶','平静','慢下来']::text[], '16 hours', 226),
('showcase_stone_041', 'showcase_user_014', 'sad', 'large', '#90A4AE', -0.49, '看到一句熟悉的话，忽然想起很久没联系的人。难过来得很轻，但停留了一会儿。', ARRAY['想念','难过','关系']::text[], '7 days 5 hours', 248),
('showcase_stone_042', 'showcase_user_014', 'grateful', 'small', '#F2CC8F', 0.65, '今天被耐心解释了一遍流程，没有人嫌我问得慢。被善待时，人会重新敢提问。', ARRAY['学习','感恩','耐心']::text[], '26 days 3 hours', 190),
('showcase_stone_043', 'showcase_user_015', 'hopeful', 'medium', '#B5E48C', 0.55, '低电量的一天，还是完成了最重要的那件事。给自己记一盏橘色的小灯。', ARRAY['能量','希望','完成']::text[], '17 hours', 241),
('showcase_stone_044', 'showcase_user_015', 'anxious', 'small', '#E0BBE4', -0.36, '临近截止时间时心跳很快，我先把文件名改对，再做下一步。把事情缩小真的有帮助。', ARRAY['截止','焦虑','步骤']::text[], '3 days 11 hours', 269),
('showcase_stone_045', 'showcase_user_015', 'happy', 'large', '#F4A261', 0.72, '晚上买到喜欢的面包，回家的路都变得轻快。生活有时候会用很小的方式奖励人。', ARRAY['面包','开心','奖励']::text[], '25 days 2 hours', 213),
('showcase_stone_046', 'showcase_user_016', 'confused', 'medium', '#BDE0FE', -0.10, '把复杂的念头放远一点看，发现里面有担心，也有期待。它们不是敌人，只是都想被听见。', ARRAY['念头','迷茫','觉察']::text[], '18 hours', 229),
('showcase_stone_047', 'showcase_user_016', 'calm', 'small', '#9ED2BE', 0.37, '今天没有急着回应所有消息，先把自己的节奏找回来。安静不是冷淡，是需要充电。', ARRAY['边界','平静','充电']::text[], '5 days 10 hours', 237),
('showcase_stone_048', 'showcase_user_016', 'hopeful', 'large', '#C3F0CA', 0.52, '下午的问题晚上突然想通了。原来暂停不是停滞，有时候是在给答案留路。', ARRAY['暂停','希望','答案']::text[], '28 days 4 hours', 184),
('showcase_stone_049', 'showcase_user_017', 'anxious', 'medium', '#CDB4DB', -0.41, '情绪起伏时我去楼下走了一圈。风有点冷，但比待在原地反复想要好。', ARRAY['散步','焦虑','转移']::text[], '19 hours', 255),
('showcase_stone_050', 'showcase_user_017', 'grateful', 'small', '#FFD166', 0.60, '保安叔叔提醒我伞忘拿了，那句提醒很普通，却让我今天多了一点被照看的感觉。', ARRAY['雨伞','感恩','日常']::text[], '8 days 6 hours', 214),
('showcase_stone_051', 'showcase_user_017', 'sad', 'large', '#A9DEF9', -0.46, '翻到一段旧聊天记录，发现自己曾经很用力地解释。现在想抱抱那时候的自己。', ARRAY['旧记录','难过','自我安慰']::text[], '30 days 5 hours', 198),
('showcase_stone_052', 'showcase_user_018', 'calm', 'medium', '#7BA7A5', 0.42, '今天认真午休了二十分钟。醒来以后才发现，疲惫不是懒惰，它只是需要被回应。', ARRAY['休息','平静','身体']::text[], '20 hours', 232),
('showcase_stone_053', 'showcase_user_018', 'happy', 'small', '#FFC8A2', 0.70, '把一盆花搬到阳台后，它看起来精神多了。我也想把自己放到更有光的地方。', ARRAY['阳台','开心','生活']::text[], '6 days 8 hours', 225),
('showcase_stone_054', 'showcase_user_018', 'neutral', 'large', '#CCD5AE', 0.05, '今天只是正常地完成工作、吃饭、洗衣服。没有起伏也不错，平稳本身就值得珍惜。', ARRAY['平稳','中性','生活']::text[], '24 days 6 hours', 189),
('showcase_stone_055', 'showcase_user_019', 'grateful', 'medium', '#F6BD60', 0.69, '给别人回了一段很长的消息，对方说谢谢你认真读完。原来温柔会双向抵达。', ARRAY['回应','感恩','共鸣']::text[], '21 hours', 276),
('showcase_stone_056', 'showcase_user_019', 'hopeful', 'large', '#B8E0A5', 0.59, '如果一句话能让人今晚好过一点，那它就值得被写下。我愿意继续练习这样的表达。', ARRAY['表达','希望','陪伴']::text[], '4 days 12 hours', 264),
('showcase_stone_057', 'showcase_user_019', 'confused', 'small', '#B8C0FF', -0.09, '有时不知道回应是否足够好，但我想真诚比完美更重要。先把心意放稳。', ARRAY['真诚','迷茫','回应']::text[], '27 days 7 hours', 201),
('showcase_stone_058', 'showcase_user_020', 'happy', 'medium', '#FFAF87', 0.73, '今天吃到很甜的荔枝，像夏天提前寄来的明信片。心情也跟着亮了一小格。', ARRAY['夏天','开心','食物']::text[], '22 hours', 236),
('showcase_stone_059', 'showcase_user_020', 'lonely', 'large', '#A9DEF9', -0.50, '热闹散场后心里有点空。我没有急着找人说话，只是把房间灯调暖了一点。', ARRAY['散场','孤独','照顾']::text[], '9 days 9 hours', 243),
('showcase_stone_060', 'showcase_user_020', 'calm', 'small', '#9ED2BE', 0.33, '把冰箱里的水果洗好切好，像提前给明天留一份善意。', ARRAY['明天','平静','照顾']::text[], '29 days 3 hours', 186),
('showcase_stone_061', 'showcase_user_021', 'hopeful', 'medium', '#C3F0CA', 0.56, '清晨喝完第一杯水，感觉今天还有很多机会可以重新开始。愿我不要太早给自己下结论。', ARRAY['清晨','希望','重新开始']::text[], '23 hours', 254),
('showcase_stone_062', 'showcase_user_021', 'sad', 'small', '#90A4AE', -0.45, '今天听到一首旧歌，心里忽然软了一下。不是坏事，只是有些记忆还会回声。', ARRAY['旧歌','难过','记忆']::text[], '7 days 10 hours', 221),
('showcase_stone_063', 'showcase_user_021', 'grateful', 'large', '#E9EDC9', 0.62, '快递小哥把箱子放到门口还发消息提醒，疲惫的一天因此少了一点麻烦。', ARRAY['生活','感恩','便利']::text[], '26 days 8 hours', 175),
('showcase_stone_064', 'showcase_user_022', 'anxious', 'medium', '#D0BFFF', -0.40, '压力堆在一起时我容易把它们想成一堵墙。今天试着只搬走最小的一块。', ARRAY['压力','焦虑','行动']::text[], '1 day 1 hour', 289),
('showcase_stone_065', 'showcase_user_022', 'happy', 'large', '#F4A261', 0.75, '终于把拖了两周的表格交掉，发出邮件那一刻差点笑出声。小小自由回来了。', ARRAY['工作','开心','完成']::text[], '5 days 13 hours', 281),
('showcase_stone_066', 'showcase_user_022', 'calm', 'small', '#A8DADC', 0.37, '睡前没有刷短视频，只听了十分钟白噪音。心里少了一点乱。', ARRAY['睡前','平静','习惯']::text[], '18 days 9 hours', 197),
('showcase_stone_067', 'showcase_user_023', 'confused', 'medium', '#BDE0FE', -0.13, '我总想把所有问题都归咎于自己，今天试着停下来问：事实真的如此吗？', ARRAY['自责','迷茫','事实']::text[], '1 day 2 hours', 266),
('showcase_stone_068', 'showcase_user_023', 'hopeful', 'small', '#B5E48C', 0.51, '给自己写了一句提醒：可以负责，但不必负责所有人的情绪。读完轻松了一点。', ARRAY['边界','希望','提醒']::text[], '6 days 2 hours', 244),
('showcase_stone_069', 'showcase_user_023', 'grateful', 'large', '#F2CC8F', 0.63, '朋友说你不用马上变好，先这样待一会儿也可以。那句话像给我放了一把椅子。', ARRAY['朋友','感恩','接纳']::text[], '23 days 5 hours', 228),
('showcase_stone_070', 'showcase_user_024', 'calm', 'medium', '#84A59D', 0.39, '傍晚河面很慢，我也跟着把脚步放慢。很多答案不在赶路里。', ARRAY['傍晚','平静','河边']::text[], '1 day 3 hours', 219),
('showcase_stone_071', 'showcase_user_024', 'happy', 'small', '#FFB9A3', 0.71, '给陌生人回了一只纸船，对方说今晚舒服多了。原来善意真的会让两个人都暖。', ARRAY['纸船','开心','善意']::text[], '8 days 7 hours', 263),
('showcase_stone_072', 'showcase_user_024', 'lonely', 'large', '#A9DEF9', -0.49, '有时也会羡慕别人很自然地被陪伴。今天先承认这份羡慕，再给自己一点温柔。', ARRAY['陪伴','孤独','温柔']::text[], '27 days 4 hours', 210),
('showcase_stone_073', 'showcase_user_025', 'grateful', 'medium', '#FFD166', 0.64, '银杏叶落在肩上，我才发现自己已经走过了最慌的那段路。谢谢一路撑着的自己。', ARRAY['季节','感恩','自己']::text[], '1 day 4 hours', 247),
('showcase_stone_074', 'showcase_user_025', 'sad', 'small', '#B0BEC5', -0.43, '今天有点失落，因为努力没有立刻被看见。但我知道它没有白费，只是在路上。', ARRAY['失落','难过','努力']::text[], '10 days 8 hours', 235),
('showcase_stone_075', 'showcase_user_025', 'hopeful', 'large', '#C3F0CA', 0.55, '把保存很久的课程终于打开了第一节。开始很小，但它确实开始了。', ARRAY['学习','希望','开始']::text[], '25 days 6 hours', 192),
('showcase_stone_076', 'showcase_user_026', 'happy', 'medium', '#FFC8A2', 0.72, '雨停后云缝里露出一点光，我刚好抬头看见。心里像被盖了一枚小印章：会好的。', ARRAY['雨停','开心','放晴']::text[], '1 day 5 hours', 258),
('showcase_stone_077', 'showcase_user_026', 'calm', 'large', '#7BA7A5', 0.43, '把桌面清空，只留下水杯和笔记本。空间变简单，心也跟着有秩序。', ARRAY['桌面','平静','秩序']::text[], '5 days 14 hours', 237),
('showcase_stone_078', 'showcase_user_026', 'anxious', 'small', '#E0BBE4', -0.37, '等待结果的时间最难熬。我今天给自己安排了三件具体小事，免得一直悬着。', ARRAY['等待','焦虑','安排']::text[], '22 days 8 hours', 216),
('showcase_stone_079', 'showcase_user_027', 'confused', 'medium', '#B8C0FF', -0.11, '我以为柔软会让人被风吹倒，后来发现柔软也能让人不被折断。', ARRAY['柔软','迷茫','理解']::text[], '1 day 6 hours', 224),
('showcase_stone_080', 'showcase_user_027', 'grateful', 'small', '#E9EDC9', 0.60, '今天有人认真接住了我的停顿，没有催我继续说。被允许慢一点，真的很珍贵。', ARRAY['倾听','感恩','慢一点']::text[], '11 days 4 hours', 241),
('showcase_stone_081', 'showcase_user_027', 'calm', 'large', '#A8DADC', 0.34, '夜里风很轻，窗帘动了一下。我突然不想和今天较劲了。', ARRAY['夜风','平静','放下']::text[], '28 days 7 hours', 183),
('showcase_stone_082', 'showcase_user_028', 'neutral', 'medium', '#CCD5AE', 0.03, '今天保持了边界，没有解释太多，也没有冷漠。只是把自己放在合适的位置。', ARRAY['边界','中性','关系']::text[], '1 day 7 hours', 232),
('showcase_stone_083', 'showcase_user_028', 'hopeful', 'small', '#B5E48C', 0.52, '把窗擦干净后，远处的楼灯清楚了很多。也许心里的雾也能这样慢慢擦开。', ARRAY['窗户','希望','清理']::text[], '9 days 3 hours', 226),
('showcase_stone_084', 'showcase_user_028', 'sad', 'large', '#90A4AE', -0.46, '有些关系不能再像以前那样靠近，我有点难过，但也知道距离是在保护彼此。', ARRAY['关系','难过','边界']::text[], '24 days 5 hours', 209),
('showcase_stone_085', 'showcase_user_029', 'happy', 'medium', '#F4A261', 0.74, '迟来的好消息今天到了，虽然等得久，但打开那一刻还是很想笑。', ARRAY['好消息','开心','等待']::text[], '1 day 8 hours', 267),
('showcase_stone_086', 'showcase_user_029', 'grateful', 'large', '#FFD166', 0.66, '感谢那个提醒我提交材料的人，如果不是这句话，我可能又要错过一次机会。', ARRAY['提醒','感恩','机会']::text[], '7 days 12 hours', 238),
('showcase_stone_087', 'showcase_user_029', 'confused', 'small', '#BDE0FE', -0.07, '我还没想清楚下一步，但今天至少知道自己不想继续原地打转。', ARRAY['下一步','迷茫','改变']::text[], '19 days 6 hours', 198),
('showcase_stone_088', 'showcase_user_030', 'happy', 'small', '#FFAF87', 0.70, '下午突然想喝苏打水，气泡在杯子里升起来，像心情也偷偷冒了一下泡。', ARRAY['气泡','开心','下午']::text[], '1 day 9 hours', 219),
('showcase_stone_089', 'showcase_user_030', 'anxious', 'medium', '#D0BFFF', -0.39, '今天有点坐立不安，我没有责怪自己，只是把呼吸数到十，再重新开始。', ARRAY['呼吸','焦虑','重新开始']::text[], '8 days 9 hours', 252),
('showcase_stone_090', 'showcase_user_030', 'calm', 'large', '#9ED2BE', 0.35, '洗完澡后换了干净床单，整个人像被重新放回柔软里。', ARRAY['睡眠','平静','生活']::text[], '21 days 4 hours', 206),
('showcase_stone_091', 'showcase_user_031', 'lonely', 'medium', '#A9DEF9', -0.47, '夜里有一阵很安静，安静到能听见自己的心事。还好这里可以把它放下。', ARRAY['夜晚','孤独','表达']::text[], '1 day 10 hours', 273),
('showcase_stone_092', 'showcase_user_031', 'hopeful', 'small', '#C3F0CA', 0.54, '有人说微弱的光也算光。今天我决定不嫌弃自己这点慢慢亮起来的力气。', ARRAY['光','希望','力气']::text[], '6 days 11 hours', 244),
('showcase_stone_093', 'showcase_user_031', 'grateful', 'large', '#F2CC8F', 0.61, '谢谢这片湖没有要求我表现得很完整。碎一点也能被放在这里。', ARRAY['心湖','感恩','接纳']::text[], '30 days 3 hours', 215),
('showcase_stone_094', 'showcase_user_032', 'anxious', 'medium', '#E0BBE4', -0.35, '写到最后一行时发现还有细节没确认，我有点急。先保存，再检查，事情会一项项落地。', ARRAY['任务','焦虑','检查']::text[], '1 day 11 hours', 261),
('showcase_stone_095', 'showcase_user_032', 'calm', 'small', '#84A59D', 0.38, '关电脑前把明天要做的三件事写好。这样睡觉时，脑子不用继续站岗。', ARRAY['睡前','平静','计划']::text[], '4 days 13 hours', 246),
('showcase_stone_096', 'showcase_user_032', 'happy', 'large', '#FFC8A2', 0.71, '今天的最后一封邮件发出去后，我认真给自己倒了一杯水。收工快乐。', ARRAY['收工','开心','完成']::text[], '18 days 7 hours', 222);

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
    TRUE,
    u.nickname,
    s.emotion_score,
    s.emotion_score,
    0,
    0,
    s.view_count,
    'published',
    s.tags,
    ARRAY['curated_showcase', 'manual_content']::text[],
    NOW() - s.created_ago::interval,
    NOW() - s.created_ago::interval + INTERVAL '8 minutes'
FROM tmp_curated_stones s
JOIN users u ON u.user_id = s.user_id;

CREATE TEMP TABLE tmp_showcase_users ON COMMIT DROP AS
SELECT id AS user_id, row_number() OVER (ORDER BY id) AS rn
FROM users
WHERE id LIKE 'showcase_user_%'
ORDER BY id;

CREATE TEMP TABLE tmp_showcase_stones ON COMMIT DROP AS
SELECT stone_id, user_id, row_number() OVER (ORDER BY stone_id) AS rn
FROM tmp_curated_stones
ORDER BY stone_id;

-- 3) 涟漪：为每颗展示石头建立 3 条真实互动记录，计数可被前端和推荐链路使用。
WITH user_total AS (
    SELECT COUNT(*)::int AS n FROM tmp_showcase_users
),
offsets(offset_value) AS (
    VALUES (5), (11), (19)
),
planned AS (
    SELECT
        'showcase_ripple_' || substr(md5(s.stone_id || ':' || u.user_id), 1, 24) AS ripple_id,
        s.stone_id,
        u.user_id,
        NOW() - ((s.rn % 48) + offsets.offset_value) * INTERVAL '17 minutes' AS created_at
    FROM tmp_showcase_stones s
    CROSS JOIN user_total
    JOIN offsets ON TRUE
    JOIN tmp_showcase_users u
      ON u.rn = (((s.rn + offsets.offset_value - 1) % user_total.n) + 1)
    WHERE u.user_id <> s.user_id
)
INSERT INTO ripples (ripple_id, stone_id, user_id, created_at)
SELECT ripple_id, stone_id, user_id, created_at
FROM planned
ON CONFLICT (stone_id, user_id) DO NOTHING;

-- 4) 纸船：48 条人工策划回复，覆盖安慰、共鸣、提醒、鼓励等不同语气。
CREATE TEMP TABLE tmp_curated_boats (
    boat_id TEXT PRIMARY KEY,
    stone_id TEXT NOT NULL,
    sender_id TEXT NOT NULL,
    receiver_id TEXT NOT NULL,
    content TEXT NOT NULL,
    mood TEXT NOT NULL,
    boat_style TEXT NOT NULL,
    created_ago TEXT NOT NULL
) ON COMMIT DROP;

INSERT INTO tmp_curated_boats VALUES
('showcase_boat_001', 'showcase_stone_001', 'showcase_user_005', 'showcase_user_001', '这段早晨很轻，我读完也想把明天的第一分钟留给自己。', 'calm', 'paper', '90 minutes'),
('showcase_boat_002', 'showcase_stone_004', 'showcase_user_014', 'showcase_user_002', '把担心写成清单已经很厉害了，愿你明天汇报时能先稳住呼吸。', 'hopeful', 'glow', '2 hours'),
('showcase_boat_003', 'showcase_stone_007', 'showcase_user_019', 'showcase_user_003', '说不清也没关系，你已经把它看见了，这就是很重要的一步。', 'calm', 'paper', '3 hours'),
('showcase_boat_004', 'showcase_stone_010', 'showcase_user_022', 'showcase_user_004', '身体动起来以后心会换气，这句话我想带走，今晚也试着走一走。', 'grateful', 'leaf', '4 hours'),
('showcase_boat_005', 'showcase_stone_013', 'showcase_user_008', 'showcase_user_005', '卡住不代表不适合，可能只是入口还没被找到。谢谢你写得这么具体。', 'hopeful', 'paper', '5 hours'),
('showcase_boat_006', 'showcase_stone_016', 'showcase_user_027', 'showcase_user_006', '把注意力还给自己这句很有力量，愿你的夜晚少一点被打扰。', 'calm', 'glow', '6 hours'),
('showcase_boat_007', 'showcase_stone_019', 'showcase_user_003', 'showcase_user_007', '方向感回来时人会轻一点，这句话像一根绳子，把我也拉稳了。', 'grateful', 'paper', '7 hours'),
('showcase_boat_008', 'showcase_stone_022', 'showcase_user_030', 'showcase_user_008', '兑现小约定真的会让人重新相信自己，祝贺你读完那本书。', 'happy', 'leaf', '8 hours'),
('showcase_boat_009', 'showcase_stone_025', 'showcase_user_012', 'showcase_user_009', '雨后的青草味被你写得很安静，我也跟着慢下来了。', 'calm', 'paper', '9 hours'),
('showcase_boat_010', 'showcase_stone_028', 'showcase_user_024', 'showcase_user_010', '没有大事也能心情不错，这种日子值得被好好记住。', 'happy', 'glow', '10 hours'),
('showcase_boat_011', 'showcase_stone_031', 'showcase_user_015', 'showcase_user_011', '那盏小灯很重要，它至少告诉你：你没有把自己丢在黑里。', 'hopeful', 'paper', '11 hours'),
('showcase_boat_012', 'showcase_stone_034', 'showcase_user_021', 'showcase_user_012', '慢慢来也是来，我也需要这句话。谢谢你把它放进湖里。', 'grateful', 'leaf', '12 hours'),
('showcase_boat_013', 'showcase_stone_037', 'showcase_user_002', 'showcase_user_013', '平凡地把自己带回家，也是一种很可靠的照顾。', 'calm', 'paper', '13 hours'),
('showcase_boat_014', 'showcase_stone_040', 'showcase_user_026', 'showcase_user_014', '不催促自己立刻好起来，这句话很温柔，也很难得。', 'calm', 'glow', '14 hours'),
('showcase_boat_015', 'showcase_stone_043', 'showcase_user_011', 'showcase_user_015', '低电量还能完成重要的事，真的可以给自己亮一盏灯。', 'hopeful', 'paper', '15 hours'),
('showcase_boat_016', 'showcase_stone_046', 'showcase_user_006', 'showcase_user_016', '担心和期待都想被听见，这个角度让我突然松了一口气。', 'grateful', 'leaf', '16 hours'),
('showcase_boat_017', 'showcase_stone_049', 'showcase_user_020', 'showcase_user_017', '愿那一圈散步替你拦住一点反复想，也把一点风留给你。', 'calm', 'paper', '17 hours'),
('showcase_boat_018', 'showcase_stone_052', 'showcase_user_009', 'showcase_user_018', '疲惫不是懒惰，这句话应该贴在很多人的桌前。谢谢你。', 'grateful', 'glow', '18 hours'),
('showcase_boat_019', 'showcase_stone_055', 'showcase_user_001', 'showcase_user_019', '温柔双向抵达的时候，湖面真的会亮一点。', 'happy', 'paper', '19 hours'),
('showcase_boat_020', 'showcase_stone_058', 'showcase_user_031', 'showcase_user_020', '这颗荔枝味的石头很甜，读完我也想给今天加一点亮色。', 'happy', 'leaf', '20 hours'),
('showcase_boat_021', 'showcase_stone_061', 'showcase_user_018', 'showcase_user_021', '不要太早给自己下结论，这句话像清晨的一杯水。', 'hopeful', 'paper', '21 hours'),
('showcase_boat_022', 'showcase_stone_064', 'showcase_user_004', 'showcase_user_022', '先搬走最小的一块，很实际，也很有力量。愿你的墙慢慢变矮。', 'hopeful', 'glow', '22 hours'),
('showcase_boat_023', 'showcase_stone_067', 'showcase_user_025', 'showcase_user_023', '问事实真的如此吗，是很温柔的自救。愿你少背一点不属于自己的重量。', 'calm', 'paper', '23 hours'),
('showcase_boat_024', 'showcase_stone_070', 'showcase_user_017', 'showcase_user_024', '很多答案不在赶路里，我读到这里也想把步子放慢。', 'grateful', 'leaf', '1 day 1 hour'),
('showcase_boat_025', 'showcase_stone_073', 'showcase_user_013', 'showcase_user_025', '谢谢一路撑着的自己，这句话值得被郑重说出来。', 'grateful', 'paper', '1 day 2 hours'),
('showcase_boat_026', 'showcase_stone_076', 'showcase_user_028', 'showcase_user_026', '会好的三个字很轻，但在雨停的时候看见，分量刚刚好。', 'hopeful', 'glow', '1 day 3 hours'),
('showcase_boat_027', 'showcase_stone_079', 'showcase_user_010', 'showcase_user_027', '柔软不等于会被折断，这句话我会记很久。', 'grateful', 'paper', '1 day 4 hours'),
('showcase_boat_028', 'showcase_stone_082', 'showcase_user_016', 'showcase_user_028', '把自己放在合适的位置，很清醒，也很不容易。', 'calm', 'leaf', '1 day 5 hours'),
('showcase_boat_029', 'showcase_stone_085', 'showcase_user_007', 'showcase_user_029', '迟来的好消息也是好消息，替你把这份笑意接住。', 'happy', 'paper', '1 day 6 hours'),
('showcase_boat_030', 'showcase_stone_088', 'showcase_user_023', 'showcase_user_030', '这颗气泡石头好轻快，愿你的下午继续冒一点小泡。', 'happy', 'glow', '1 day 7 hours'),
('showcase_boat_031', 'showcase_stone_091', 'showcase_user_029', 'showcase_user_031', '这里可以放下心事，也可以慢慢把自己捡回来。愿今晚柔和一点。', 'hopeful', 'paper', '1 day 8 hours'),
('showcase_boat_032', 'showcase_stone_094', 'showcase_user_005', 'showcase_user_032', '先保存，再检查，这个顺序很稳。愿最后一行也顺利落地。', 'calm', 'leaf', '1 day 9 hours'),
('showcase_boat_033', 'showcase_stone_020', 'showcase_user_032', 'showcase_user_007', '也许以前的你没有消失，只是在等现在的你回头牵她一下。', 'hopeful', 'glow', '1 day 10 hours'),
('showcase_boat_034', 'showcase_stone_023', 'showcase_user_026', 'showcase_user_008', '没有证据的担心可以先放旁边，这句话我今天也需要。', 'calm', 'paper', '1 day 11 hours'),
('showcase_boat_035', 'showcase_stone_029', 'showcase_user_019', 'showcase_user_010', '建议很多时，先听自己声音。愿你慢慢分辨出真正想要的东西。', 'hopeful', 'leaf', '1 day 12 hours'),
('showcase_boat_036', 'showcase_stone_035', 'showcase_user_001', 'showcase_user_012', '重要谈话前先喝水、早点睡，这份照顾比预演更稳。', 'calm', 'paper', '1 day 13 hours'),
('showcase_boat_037', 'showcase_stone_041', 'showcase_user_022', 'showcase_user_014', '难过轻轻来过，也可以被轻轻送走。愿你今晚睡得安稳。', 'calm', 'glow', '1 day 14 hours'),
('showcase_boat_038', 'showcase_stone_047', 'showcase_user_008', 'showcase_user_016', '安静不是冷淡，是充电。谢谢你替很多慢热的人说清楚了。', 'grateful', 'paper', '1 day 15 hours'),
('showcase_boat_039', 'showcase_stone_059', 'showcase_user_014', 'showcase_user_020', '把灯调暖一点，是很具体的温柔。愿散场后的房间不再那么空。', 'hopeful', 'leaf', '1 day 16 hours'),
('showcase_boat_040', 'showcase_stone_062', 'showcase_user_027', 'showcase_user_021', '旧歌带来的回声不一定要赶走，陪它坐一会儿也可以。', 'calm', 'paper', '1 day 17 hours'),
('showcase_boat_041', 'showcase_stone_068', 'showcase_user_012', 'showcase_user_023', '不必负责所有人的情绪，这句话像帮人卸下一个背包。', 'grateful', 'glow', '1 day 18 hours'),
('showcase_boat_042', 'showcase_stone_074', 'showcase_user_030', 'showcase_user_025', '努力没有立刻被看见，也仍然在生长。愿你先替自己看见它。', 'hopeful', 'paper', '1 day 19 hours'),
('showcase_boat_043', 'showcase_stone_078', 'showcase_user_003', 'showcase_user_026', '等待最容易让人悬着，三件具体小事是很好的锚。', 'calm', 'leaf', '1 day 20 hours'),
('showcase_boat_044', 'showcase_stone_084', 'showcase_user_018', 'showcase_user_028', '距离有时不是冷掉，而是在保护还值得保护的部分。', 'calm', 'paper', '1 day 21 hours'),
('showcase_boat_045', 'showcase_stone_087', 'showcase_user_024', 'showcase_user_029', '知道不想原地打转，已经是一种方向。愿下一步慢慢出现。', 'hopeful', 'glow', '1 day 22 hours'),
('showcase_boat_046', 'showcase_stone_089', 'showcase_user_011', 'showcase_user_030', '数到十再重新开始，这样的小动作很朴素，也很可靠。', 'calm', 'paper', '1 day 23 hours'),
('showcase_boat_047', 'showcase_stone_092', 'showcase_user_020', 'showcase_user_031', '微弱的光也算光，慢慢亮起来也值得被认真珍惜。', 'hopeful', 'leaf', '2 days'),
('showcase_boat_048', 'showcase_stone_095', 'showcase_user_006', 'showcase_user_032', '把明天写好再睡，像把脑子从站岗的位置接回来。愿你好眠。', 'calm', 'paper', '2 days 1 hour');

INSERT INTO paper_boats (
    boat_id, stone_id, sender_id, receiver_id, content, mood, drift_mode,
    boat_style, is_anonymous, status, created_at, updated_at
)
SELECT
    boat_id,
    stone_id,
    sender_id,
    receiver_id,
    content,
    mood,
    CASE WHEN boat_style = 'glow' THEN 'direct' ELSE 'random' END,
    boat_style,
    TRUE,
    'active',
    NOW() - created_ago::interval,
    NOW() - created_ago::interval
FROM tmp_curated_boats;

-- 5) 推荐和画像信号：由真实涟漪、纸船、浏览行为回填。
INSERT INTO user_interaction_history (
    user_id, stone_id, interaction_type, interaction_weight, dwell_time_seconds, created_at
)
SELECT
    user_id,
    stone_id,
    'ripple',
    2.0,
    18 + ((row_number() OVER (ORDER BY created_at, ripple_id))::int % 40),
    created_at
FROM ripples
WHERE ripple_id LIKE 'showcase_ripple_%'
ON CONFLICT (user_id, stone_id, interaction_type) DO UPDATE SET
    interaction_weight = EXCLUDED.interaction_weight,
    dwell_time_seconds = EXCLUDED.dwell_time_seconds,
    created_at = EXCLUDED.created_at;

INSERT INTO user_interaction_history (
    user_id, stone_id, interaction_type, interaction_weight, dwell_time_seconds, created_at
)
SELECT
    sender_id,
    stone_id,
    'boat',
    3.0,
    30 + ((row_number() OVER (ORDER BY created_at, boat_id))::int % 50),
    created_at
FROM paper_boats
WHERE boat_id LIKE 'showcase_boat_%'
ON CONFLICT (user_id, stone_id, interaction_type) DO UPDATE SET
    interaction_weight = EXCLUDED.interaction_weight,
    dwell_time_seconds = EXCLUDED.dwell_time_seconds,
    created_at = EXCLUDED.created_at;

INSERT INTO user_emotion_history (user_id, sentiment_score, mood_type, content_snippet, created_at)
SELECT
    user_id,
    emotion_score,
    mood_type,
    'showcase:' || left(content, 96),
    NOW() - created_ago::interval
FROM tmp_curated_stones;

INSERT INTO user_preferences (user_id, preferred_moods, preferred_tags, last_updated)
SELECT
    u.user_id,
    COALESCE(ARRAY_AGG(DISTINCT s.mood_type ORDER BY s.mood_type), ARRAY[]::text[]),
    COALESCE(ARRAY_AGG(DISTINCT t.tag ORDER BY t.tag), ARRAY[]::text[]),
    NOW()
FROM tmp_showcase_users u
LEFT JOIN tmp_curated_stones s ON s.user_id = u.user_id
LEFT JOIN LATERAL unnest(s.tags) AS t(tag) ON TRUE
GROUP BY u.user_id
ON CONFLICT (user_id) DO UPDATE SET
    preferred_moods = EXCLUDED.preferred_moods,
    preferred_tags = EXCLUDED.preferred_tags,
    last_updated = EXCLUDED.last_updated;

-- 6) 轻量 embedding：用于兼容旧版 stone_embeddings 页面和相似度调试。
INSERT INTO stone_embeddings (stone_id, embedding, created_at)
SELECT
    s.stone_id,
    '[' ||
    array_to_string(
        array_agg((((get_byte(decode(md5(s.stone_id || ':' || gs::text), 'hex'), 0) - 128)::numeric / 128.0))::text ORDER BY gs),
        ','
    ) || ']',
    NOW()
FROM tmp_curated_stones s
CROSS JOIN generate_series(1, 8) AS gs
GROUP BY s.stone_id
ON CONFLICT (stone_id) DO UPDATE SET
    embedding = EXCLUDED.embedding,
    created_at = EXCLUDED.created_at;

-- 7) 同步可见计数，保证列表页、详情页、管理端统计一致。
UPDATE stones s
SET ripple_count = COALESCE(x.cnt, 0),
    boat_count = COALESCE(y.cnt, 0),
    updated_at = NOW()
FROM tmp_curated_stones cs
LEFT JOIN (
    SELECT stone_id, COUNT(*)::int AS cnt
    FROM ripples
    WHERE stone_id LIKE 'showcase_stone_%'
    GROUP BY stone_id
) x ON x.stone_id = cs.stone_id
LEFT JOIN (
    SELECT stone_id, COUNT(*)::int AS cnt
    FROM paper_boats
    WHERE stone_id LIKE 'showcase_stone_%'
      AND COALESCE(status, 'active') <> 'deleted'
    GROUP BY stone_id
) y ON y.stone_id = cs.stone_id
WHERE s.stone_id = cs.stone_id;

COMMIT;

SELECT
    (SELECT COUNT(*) FROM users WHERE user_id LIKE 'showcase_user_%') AS showcase_users,
    (SELECT COUNT(*) FROM stones WHERE stone_id LIKE 'showcase_stone_%') AS showcase_stones,
    (SELECT COUNT(*) FROM ripples WHERE ripple_id LIKE 'showcase_ripple_%') AS showcase_ripples,
    (SELECT COUNT(*) FROM paper_boats WHERE boat_id LIKE 'showcase_boat_%') AS showcase_boats,
    (SELECT COUNT(*) FROM user_interaction_history WHERE user_id LIKE 'showcase_user_%') AS showcase_interactions,
    (SELECT COUNT(*) FROM user_emotion_history WHERE user_id LIKE 'showcase_user_%') AS showcase_emotion_history;
