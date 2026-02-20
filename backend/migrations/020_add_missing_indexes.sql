-- 020_add_missing_indexes.sql
-- Performance critical indexes for HeartLake

CREATE INDEX IF NOT EXISTS idx_emotion_tracking_user_id ON emotion_tracking(user_id);
CREATE INDEX IF NOT EXISTS idx_emotion_tracking_user_created ON emotion_tracking(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_intervention_log_user_created ON intervention_log(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_user_emotion_profile_user_date ON user_emotion_profile(user_id, date DESC);
CREATE INDEX IF NOT EXISTS idx_psychological_assessments_created ON psychological_assessments(created_at);
CREATE INDEX IF NOT EXISTS idx_high_risk_events_status ON high_risk_events(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_notifications_user_read ON notifications(user_id, is_read, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_stones_user_status ON stones(user_id, status) WHERE deleted_at IS NULL;
CREATE INDEX IF NOT EXISTS idx_paper_boats_status ON paper_boats(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_ripples_stone_id ON ripples(stone_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_friend_messages_sender_receiver ON friend_messages(sender_id, receiver_id, created_at DESC);
