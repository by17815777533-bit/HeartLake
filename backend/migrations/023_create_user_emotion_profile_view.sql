-- 023_create_user_emotion_profile_view.sql
-- 创建 user_emotion_profile 视图，基于 stones 表聚合用户情绪数据
-- 被 RecommendationController, VIPService, RecommendationEngine, EmotionResonanceEngine 引用

CREATE OR REPLACE VIEW user_emotion_profile AS
SELECT
    s.user_id,
    s.mood_type,
    COALESCE(AVG(s.emotion_score), 0.5) AS avg_emotion_score,
    DATE(s.created_at) AS date
FROM stones s
WHERE s.status = 'published' AND s.deleted_at IS NULL
GROUP BY s.user_id, s.mood_type, DATE(s.created_at)
ORDER BY date DESC;
