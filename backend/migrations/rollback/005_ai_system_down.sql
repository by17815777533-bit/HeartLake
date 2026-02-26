-- 005 回滚: 删除AI系统表和视图
BEGIN;

DROP TABLE IF EXISTS vector_index_metadata CASCADE;
DROP TABLE IF EXISTS edge_nodes CASCADE;
DROP TABLE IF EXISTS community_emotion_snapshots CASCADE;
DROP TABLE IF EXISTS federated_model_versions CASCADE;
DROP TABLE IF EXISTS dp_privacy_budgets CASCADE;
DROP TABLE IF EXISTS emotion_compatibility CASCADE;
DROP TABLE IF EXISTS user_interaction_history CASCADE;
DROP TABLE IF EXISTS user_emotion_history CASCADE;
DROP TABLE IF EXISTS lake_god_messages CASCADE;
DROP VIEW IF EXISTS user_emotion_profile CASCADE;

COMMIT;
