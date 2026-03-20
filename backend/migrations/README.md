# HeartLake 数据库迁移

## 迁移文件列表

| 编号 | 文件 | 说明 |
|------|------|------|
| 001 | `001_users.sql` | 用户表、用户相似度表 |
| 002 | `002_stones.sql` | 石头表、向量嵌入表 |
| 003 | `003_social.sql` | 好友、临时好友、消息 |
| 004 | `004_interactions.sql` | 涟漪、纸船、临时连接 |
| 005 | `005_ai_system.sql` | AI 系统（湖神对话、情绪、推荐、联邦学习） |
| 006 | `006_guardian.sql` | 守望者系统（情绪追踪、共鸣积分、VIP） |
| 007 | `007_admin.sql` | 管理后台（管理员、敏感词、操作日志） |
| 008 | `008_notifications.sql` | 通知系统 |
| 009 | `009_consultation.sql` | 心理咨询 E2EE 加密会话 |
| 010 | `010_data_export.sql` | 数据导出 |
| 011 | `011_performance_schema_hardening.sql` | 性能索引与架构一致性增强 |
| 012 | `012_users_username_compat.sql` | users 表兼容列补齐 |
| 013 | `013_schema_fixes.sql` | 缺失表、索引与时区一致性修复 |
| 014 | `014_stones_embedding_vector.sql` | stone_embeddings 向量列补齐 |
| 015 | `015_runtime_schema_hotfix.sql` | 运行时账号 schema 热修复 |
| 016 | `016_admin_runtime_tables.sql` | 管理后台运行时表补齐 |
| 017 | `017_account_schema_alignment.sql` | 账户字段、拉黑表与消息索引对齐 |
| 018 | `018_user_identity_contract.sql` | 用户身份主键契约与外键统一 |
| 019 | `019_feature_contract_completion.sql` | 推荐/VIP/风险监控缺失契约补齐 |

## 执行方式

迁移按文件名数字顺序执行，所有语句使用 `IF NOT EXISTS` / `IF EXISTS` 保证幂等性。

```bash
for f in $(ls migrations/*.sql | sort); do
  echo "Applying $(basename $f)..."
  psql -h $DB_HOST -U $DB_USER -d $DB_NAME -f "$f"
done
```

## 生产环境注意事项

- 所有 `CREATE INDEX` 语句在生产环境应改为 `CREATE INDEX CONCURRENTLY` 以避免锁表
- `CONCURRENTLY` 不能在事务块内执行，需单独运行
- 大表索引建议在低峰期执行

## 回滚策略

每个迁移的回滚操作如下：

### 001_users — 回滚

```sql
DROP INDEX IF EXISTS idx_user_similarity_score;
DROP TABLE IF EXISTS user_similarity;
DROP INDEX IF EXISTS idx_users_status;
DROP INDEX IF EXISTS idx_users_device;
DROP INDEX IF EXISTS idx_users_shadow;
DROP TABLE IF EXISTS users;
```

### 002_stones — 回滚

```sql
DROP INDEX IF EXISTS idx_stone_embeddings_created;
DROP TABLE IF EXISTS stone_embeddings;
DROP INDEX IF EXISTS idx_stones_deleted_at;
DROP INDEX IF EXISTS idx_stones_user_status_created;
DROP INDEX IF EXISTS idx_stones_status_mood_created;
DROP INDEX IF EXISTS idx_stones_status_created;
DROP INDEX IF EXISTS idx_stones_status;
DROP INDEX IF EXISTS idx_stones_mood;
DROP INDEX IF EXISTS idx_stones_created;
DROP INDEX IF EXISTS idx_stones_user;
ALTER TABLE stones DROP COLUMN IF EXISTS deleted_at;
DROP TABLE IF EXISTS stones;
```

### 003_social — 回滚

```sql
DROP INDEX IF EXISTS idx_friend_messages_receiver;
DROP INDEX IF EXISTS idx_friend_messages_sender;
DROP TABLE IF EXISTS friend_messages;
DROP INDEX IF EXISTS idx_temp_friends_expires;
DROP INDEX IF EXISTS idx_temp_friends_user2_status;
DROP INDEX IF EXISTS idx_temp_friends_user1_status;
DROP TABLE IF EXISTS temp_friends;
DROP INDEX IF EXISTS idx_friends_friend_status;
DROP INDEX IF EXISTS idx_friends_user_status;
DROP TABLE IF EXISTS friends;
```

### 004_interactions — 回滚

```sql
DROP INDEX IF EXISTS idx_connection_messages_conn;
DROP TABLE IF EXISTS connection_messages;
DROP INDEX IF EXISTS idx_connections_target;
DROP INDEX IF EXISTS idx_connections_user;
DROP TABLE IF EXISTS connections;
DROP INDEX IF EXISTS idx_paper_boats_status;
DROP INDEX IF EXISTS idx_paper_boats_receiver;
DROP INDEX IF EXISTS idx_paper_boats_sender;
DROP INDEX IF EXISTS idx_paper_boats_stone;
DROP TABLE IF EXISTS paper_boats;
DROP INDEX IF EXISTS idx_ripples_user;
DROP INDEX IF EXISTS idx_ripples_stone;
DROP TABLE IF EXISTS ripples;
```

### 005_ai_system — 回滚

```sql
DROP TABLE IF EXISTS vector_index_metadata;
DROP INDEX IF EXISTS idx_edge_nodes_heartbeat;
DROP INDEX IF EXISTS idx_edge_nodes_status;
DROP TABLE IF EXISTS edge_nodes;
DROP INDEX IF EXISTS idx_emotion_snapshots_time;
DROP TABLE IF EXISTS community_emotion_snapshots;
DROP TABLE IF EXISTS federated_model_updates;
DROP TABLE IF EXISTS emotion_compatibility;
DROP INDEX IF EXISTS idx_uih_stone;
DROP INDEX IF EXISTS idx_uih_user;
DROP TABLE IF EXISTS user_interaction_history;
DROP VIEW IF EXISTS user_emotion_profile;
DROP INDEX IF EXISTS idx_user_emotion_history_created;
DROP INDEX IF EXISTS idx_user_emotion_history_user;
DROP TABLE IF EXISTS user_emotion_history;
DROP INDEX IF EXISTS idx_lake_god_messages_user_created;
DROP INDEX IF EXISTS idx_lake_god_messages_user;
DROP TABLE IF EXISTS lake_god_messages;
```

### 006_guardian — 回滚

```sql
DROP INDEX IF EXISTS idx_vip_upgrade_logs_user;
DROP TABLE IF EXISTS vip_upgrade_logs;
DROP INDEX IF EXISTS idx_intervention_log_user;
DROP TABLE IF EXISTS intervention_log;
DROP TABLE IF EXISTS lamp_transfers;
DROP INDEX IF EXISTS idx_resonance_points_user;
DROP TABLE IF EXISTS resonance_points;
DROP INDEX IF EXISTS idx_emotion_tracking_user;
DROP TABLE IF EXISTS emotion_tracking;
```

### 007_admin — 回滚

```sql
DROP INDEX IF EXISTS idx_operation_logs_admin;
DROP TABLE IF EXISTS operation_logs;
DROP INDEX IF EXISTS idx_sensitive_words_active;
DROP TABLE IF EXISTS sensitive_words;
DROP TABLE IF EXISTS admin_users;
```

### 008_notifications — 回滚

```sql
DROP INDEX IF EXISTS idx_notifications_type;
DROP INDEX IF EXISTS idx_notifications_user;
DROP TABLE IF EXISTS notifications;
```

### 009_consultation — 回滚

```sql
DROP INDEX IF EXISTS idx_consultation_messages_session;
DROP TABLE IF EXISTS consultation_messages;
DROP INDEX IF EXISTS idx_consultation_sessions_user;
DROP TABLE IF EXISTS consultation_sessions;
```

### 010_data_export — 回滚

```sql
DROP INDEX IF EXISTS idx_export_tasks_status;
DROP INDEX IF EXISTS idx_export_tasks_user;
DROP TABLE IF EXISTS data_export_tasks;
```

### 011_performance_schema_hardening — 回滚

```sql
DROP INDEX IF EXISTS idx_vip_upgrade_logs_created_user;
DROP INDEX IF EXISTS idx_vip_upgrade_logs_reason_prefix_created;
DROP INDEX IF EXISTS idx_users_inactive_non_guardian;
DROP INDEX IF EXISTS idx_stones_user_negative_recent;
DROP INDEX IF EXISTS idx_stones_status_created_user;
DROP INDEX IF EXISTS idx_paper_boats_pending_lakegod_created;
DROP INDEX IF EXISTS idx_paper_boats_stone_sender;
DROP INDEX IF EXISTS idx_stones_published_zero_interaction_created;
DROP INDEX IF EXISTS idx_user_followups_created;
DROP TABLE IF EXISTS user_followups;
DROP INDEX IF EXISTS idx_notifications_user_notification_id;
DROP INDEX IF EXISTS idx_notifications_notification_id;
ALTER TABLE notifications DROP COLUMN IF EXISTS related_type;
ALTER TABLE notifications DROP COLUMN IF EXISTS notification_id;
```

### 012_users_username_compat — 回滚

```sql
DROP INDEX IF EXISTS idx_users_username;
ALTER TABLE users DROP COLUMN IF EXISTS recovery_key_hash;
ALTER TABLE users DROP COLUMN IF EXISTS username;
```

### 017_account_schema_alignment — 回滚

```sql
DROP INDEX IF EXISTS idx_connection_messages_conn_created;
DROP INDEX IF EXISTS idx_user_blocks_blocked_created;
DROP INDEX IF EXISTS idx_user_blocks_user_created;
DROP TABLE IF EXISTS user_blocks;
DROP INDEX IF EXISTS idx_users_email;
ALTER TABLE users DROP COLUMN IF EXISTS location;
ALTER TABLE users DROP COLUMN IF EXISTS birthday;
ALTER TABLE users DROP COLUMN IF EXISTS gender;
ALTER TABLE users DROP COLUMN IF EXISTS email;
```

### 018_user_identity_contract — 回滚

```sql
DROP TRIGGER IF EXISTS trg_users_sync_identity ON users;
DROP FUNCTION IF EXISTS sync_users_identity_columns();
ALTER TABLE IF EXISTS user_followups DROP CONSTRAINT IF EXISTS fk_user_followups_user;
ALTER TABLE IF EXISTS data_export_tasks DROP CONSTRAINT IF EXISTS fk_data_export_tasks_user;
ALTER TABLE IF EXISTS consultation_sessions DROP CONSTRAINT IF EXISTS fk_consultation_user;
ALTER TABLE IF EXISTS notifications DROP CONSTRAINT IF EXISTS fk_notifications_user;
ALTER TABLE IF EXISTS connections DROP CONSTRAINT IF EXISTS fk_connections_target_user;
ALTER TABLE IF EXISTS connections DROP CONSTRAINT IF EXISTS fk_connections_user;
ALTER TABLE IF EXISTS ripples DROP CONSTRAINT IF EXISTS fk_ripples_user;
ALTER TABLE IF EXISTS friend_messages DROP CONSTRAINT IF EXISTS fk_friend_messages_receiver;
ALTER TABLE IF EXISTS friend_messages DROP CONSTRAINT IF EXISTS fk_friend_messages_sender;
ALTER TABLE IF EXISTS temp_friends DROP CONSTRAINT IF EXISTS fk_temp_friends_user2;
ALTER TABLE IF EXISTS temp_friends DROP CONSTRAINT IF EXISTS fk_temp_friends_user1;
ALTER TABLE IF EXISTS friends DROP CONSTRAINT IF EXISTS fk_friends_friend;
ALTER TABLE IF EXISTS friends DROP CONSTRAINT IF EXISTS fk_friends_user;
ALTER TABLE IF EXISTS stones DROP CONSTRAINT IF EXISTS fk_stones_user;
ALTER TABLE IF EXISTS user_similarity DROP CONSTRAINT IF EXISTS fk_user_similarity_user2;
ALTER TABLE IF EXISTS user_similarity DROP CONSTRAINT IF EXISTS fk_user_similarity_user1;
ALTER TABLE users ALTER COLUMN user_id DROP NOT NULL;
```

## 紧急回滚流程

1. 确认需要回滚的迁移编号
2. 按逆序执行对应的回滚 SQL
3. 验证应用功能正常
4. 记录回滚原因和时间

```bash
# 示例：回滚 012
psql -h $DB_HOST -U $DB_USER -d $DB_NAME -c "
DROP INDEX IF EXISTS idx_users_username;
ALTER TABLE users DROP COLUMN IF EXISTS recovery_key_hash;
ALTER TABLE users DROP COLUMN IF EXISTS username;
"
```
