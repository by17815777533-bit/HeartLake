-- 024_create_recommendation_tables.sql
-- 创建推荐系统所需的 user_interaction_history 和 emotion_compatibility 表

BEGIN;

CREATE TABLE IF NOT EXISTS user_interaction_history (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL,
    stone_id VARCHAR(64) NOT NULL,
    interaction_type VARCHAR(32) NOT NULL DEFAULT 'view',
    interaction_weight DOUBLE PRECISION DEFAULT 1.0,
    dwell_time_seconds INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(user_id, stone_id, interaction_type)
);
CREATE INDEX IF NOT EXISTS idx_uih_user ON user_interaction_history(user_id);
CREATE INDEX IF NOT EXISTS idx_uih_stone ON user_interaction_history(stone_id);
CREATE INDEX IF NOT EXISTS idx_uih_user_stone ON user_interaction_history(user_id, stone_id);

CREATE TABLE IF NOT EXISTS emotion_compatibility (
    id SERIAL PRIMARY KEY,
    mood_type_1 VARCHAR(32) NOT NULL,
    mood_type_2 VARCHAR(32) NOT NULL,
    compatibility_score DOUBLE PRECISION NOT NULL DEFAULT 0.5,
    relationship_type VARCHAR(32) DEFAULT 'neutral',
    UNIQUE(mood_type_1, mood_type_2)
);

INSERT INTO emotion_compatibility (mood_type_1, mood_type_2, compatibility_score, relationship_type) VALUES
('happy', 'happy', 0.95, 'resonance'),
('happy', 'calm', 0.80, 'harmony'),
('happy', 'hopeful', 0.90, 'uplift'),
('happy', 'grateful', 0.85, 'warmth'),
('happy', 'sad', 0.65, 'comfort'),
('happy', 'anxious', 0.60, 'soothe'),
('happy', 'angry', 0.50, 'contrast'),
('happy', 'lonely', 0.70, 'companion'),
('calm', 'calm', 0.90, 'resonance'),
('calm', 'sad', 0.75, 'gentle'),
('calm', 'anxious', 0.70, 'soothe'),
('calm', 'hopeful', 0.85, 'harmony'),
('calm', 'grateful', 0.80, 'warmth'),
('calm', 'angry', 0.55, 'balance'),
('calm', 'lonely', 0.75, 'companion'),
('sad', 'sad', 0.85, 'empathy'),
('sad', 'lonely', 0.80, 'empathy'),
('sad', 'hopeful', 0.75, 'uplift'),
('sad', 'grateful', 0.65, 'warmth'),
('sad', 'anxious', 0.70, 'empathy'),
('sad', 'angry', 0.55, 'contrast'),
('anxious', 'anxious', 0.80, 'empathy'),
('anxious', 'hopeful', 0.75, 'uplift'),
('anxious', 'lonely', 0.70, 'empathy'),
('anxious', 'grateful', 0.65, 'soothe'),
('anxious', 'angry', 0.60, 'tension'),
('angry', 'angry', 0.70, 'empathy'),
('angry', 'lonely', 0.60, 'contrast'),
('angry', 'hopeful', 0.65, 'uplift'),
('angry', 'grateful', 0.55, 'balance'),
('lonely', 'lonely', 0.85, 'empathy'),
('lonely', 'hopeful', 0.80, 'uplift'),
('lonely', 'grateful', 0.70, 'warmth'),
('hopeful', 'hopeful', 0.90, 'resonance'),
('hopeful', 'grateful', 0.85, 'warmth'),
('grateful', 'grateful', 0.90, 'resonance')
ON CONFLICT DO NOTHING;

COMMIT;
