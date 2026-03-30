# 数据迁移说明

`backend/migrations/` 当前负责数据库结构和基础数据变更。

## 当前执行方式

```bash
psql "$DATABASE_URL" -f backend/migrations/<file>.sql
```

## 当前迁移清单

### 基础域模型

- `001_users.sql`
- `002_stones.sql`
- `003_social.sql`
- `004_interactions.sql`

### AI、关怀与后台

- `005_ai_system.sql`
- `006_guardian.sql`
- `007_admin.sql`
- `008_notifications.sql`
- `009_consultation.sql`
- `010_data_export.sql`

### 契约补齐与性能治理

- `011_performance_schema_hardening.sql`
- `012_users_username_compat.sql`
- `013_schema_fixes.sql`
- `014_stones_embedding_vector.sql`
- `015_runtime_schema_hotfix.sql`
- `016_admin_runtime_tables.sql`
- `017_account_schema_alignment.sql`
- `018_user_identity_contract.sql`
- `019_feature_contract_completion.sql`
- `020_query_path_index_tuning.sql`
- `021_auth_refresh_sessions.sql`

## 当前数据域

### 用户与身份

- `users`
- `user_similarity`
- `user_sessions`
- `login_logs`
- `security_events`
- `user_privacy_settings`
- `user_blocks`
- `data_export_tasks`

### 内容与互动

- `stones`
- `stone_embeddings`
- `ripples`
- `paper_boats`
- `notifications`

### 关系与消息

- `friends`
- `temp_friends`
- `friend_messages`
- `connections`
- `connection_messages`

### AI、情绪与推荐

- `lake_god_messages`
- `user_emotion_history`
- `user_interaction_history`
- `emotion_tracking`
- `resonance_points`
- `edge_ai_inference_logs`
- `federated_learning_rounds`
- `differential_privacy_budget`
- `community_emotion_snapshots`
- `edge_nodes`
- `vector_index_metadata`

### 管理、关怀与增值

- `admin_users`
- `sensitive_words`
- `operation_logs`
- `reports`
- `moderation_logs`
- `broadcast_messages`
- `consultation_sessions`
- `consultation_messages`
- `vip_privileges`
- `vip_privilege_usage`
- `high_risk_events`
- `admin_interventions`

## 当前重点索引

- `stones(status, created_at)`
- `stones(status, mood_type, created_at)`
- `paper_boats(stone_id)`
- `paper_boats(sender_id)`
- `paper_boats(receiver_id)`
- `temp_friends(user1_id)`
- `temp_friends(user2_id)`
- `temp_friends(expires_at)`
- `friend_messages(sender_id, receiver_id, pair_created)`
- `notifications(user_id, is_read, created_at)`
- `consultation_messages(session_id, created_at)`
- `reports(status, created_at)`
- `user_sessions(refresh_token_hash, refresh_expires_at)`

## 当前要求

- 迁移只写结构和基础数据
- 已上线表结构变更前先核对代码引用
- 新字段需要同步更新模型和接口文档
- 删除列前先完成代码清理
- 迁移说明、接口说明和运行时代码必须一起更新
