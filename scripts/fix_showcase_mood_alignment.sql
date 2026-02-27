-- ✍🏻 一次性修复：对已写入库的 showcase 石头，按文案语义回填正确情绪标签与分数。
BEGIN;

UPDATE stones
SET
    mood_type = CASE regexp_replace(content, '^showcase:\\s*', '')
        WHEN '今天收到礼物，颜色像湖面一样温柔，我把开心写成石头。' THEN 'happy'
        WHEN '老师今天夸了我，我把这份被看见的力量留在这里。' THEN 'grateful'
        WHEN '我有点焦虑，但正在用呼吸把自己慢慢拉回来。' THEN 'anxious'
        WHEN '和朋友散步后，心绪安静下来，像风停在湖面。' THEN 'calm'
        WHEN '今晚有些孤独，想把这份脆弱轻轻放进心湖。' THEN 'lonely'
        WHEN '完成任务后很有成就感，想分享给同频的人。' THEN 'happy'
        WHEN '我还在迷茫，但愿意继续尝试，继续发光。' THEN 'confused'
        WHEN '被一句善意打动，原来温暖会沿着人群传递。' THEN 'grateful'
        WHEN '情绪像潮汐，有起伏，但我能感到自己在成长。' THEN 'neutral'
        WHEN '把祝福折成纸船，送给今天需要勇气的人。' THEN 'hopeful'
        ELSE mood_type
    END,
    emotion_score = CASE regexp_replace(content, '^showcase:\\s*', '')
        WHEN '今天收到礼物，颜色像湖面一样温柔，我把开心写成石头。' THEN 0.84
        WHEN '老师今天夸了我，我把这份被看见的力量留在这里。' THEN 0.72
        WHEN '我有点焦虑，但正在用呼吸把自己慢慢拉回来。' THEN -0.49
        WHEN '和朋友散步后，心绪安静下来，像风停在湖面。' THEN 0.42
        WHEN '今晚有些孤独，想把这份脆弱轻轻放进心湖。' THEN -0.57
        WHEN '完成任务后很有成就感，想分享给同频的人。' THEN 0.66
        WHEN '我还在迷茫，但愿意继续尝试，继续发光。' THEN -0.08
        WHEN '被一句善意打动，原来温暖会沿着人群传递。' THEN 0.71
        WHEN '情绪像潮汐，有起伏，但我能感到自己在成长。' THEN 0.03
        WHEN '把祝福折成纸船，送给今天需要勇气的人。' THEN 0.58
        ELSE emotion_score
    END,
    sentiment_score = CASE regexp_replace(content, '^showcase:\\s*', '')
        WHEN '今天收到礼物，颜色像湖面一样温柔，我把开心写成石头。' THEN 0.84
        WHEN '老师今天夸了我，我把这份被看见的力量留在这里。' THEN 0.72
        WHEN '我有点焦虑，但正在用呼吸把自己慢慢拉回来。' THEN -0.49
        WHEN '和朋友散步后，心绪安静下来，像风停在湖面。' THEN 0.42
        WHEN '今晚有些孤独，想把这份脆弱轻轻放进心湖。' THEN -0.57
        WHEN '完成任务后很有成就感，想分享给同频的人。' THEN 0.66
        WHEN '我还在迷茫，但愿意继续尝试，继续发光。' THEN -0.08
        WHEN '被一句善意打动，原来温暖会沿着人群传递。' THEN 0.71
        WHEN '情绪像潮汐，有起伏，但我能感到自己在成长。' THEN 0.03
        WHEN '把祝福折成纸船，送给今天需要勇气的人。' THEN 0.58
        ELSE sentiment_score
    END,
    updated_at = NOW()
WHERE content LIKE 'showcase:%'
  AND status = 'published'
  AND deleted_at IS NULL;

COMMIT;
