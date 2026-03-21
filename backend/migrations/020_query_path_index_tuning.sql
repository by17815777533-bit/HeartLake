-- Migration 020: query path index tuning
-- 目的：
-- 1. 为纸船、临时好友、私聊等时间排序热路径补齐复合索引
-- 2. 降低咨询会话、情绪历史按用户回溯时的扫描成本
-- 3. 为举报去重检查增加更贴近查询谓词的部分索引

BEGIN;

-- =========================
-- paper_boats 热路径
-- =========================
-- 命中：
-- 1) getMySentBoats / getSentBoats: WHERE sender_id = ? ORDER BY created_at DESC
-- 2) getReceivedBoats: WHERE receiver_id = ? ORDER BY created_at DESC
-- 3) getBoats / getMyReceivedBoats: WHERE stone_id = ? ORDER BY created_at DESC
CREATE INDEX IF NOT EXISTS idx_paper_boats_sender_created
    ON paper_boats(sender_id, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_paper_boats_receiver_created
    ON paper_boats(receiver_id, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_paper_boats_stone_created
    ON paper_boats(stone_id, created_at DESC);

-- =========================
-- temp_friends / friend_messages 热路径
-- =========================
-- 命中：
-- 1) getTempFriends: user1/user2 + status='active' + ORDER BY created_at DESC
-- 2) FriendController::getMessages: 双向会话历史按 created_at ASC
CREATE INDEX IF NOT EXISTS idx_temp_friends_user1_active_created
    ON temp_friends(user1_id, created_at DESC)
    WHERE status = 'active';

CREATE INDEX IF NOT EXISTS idx_temp_friends_user2_active_created
    ON temp_friends(user2_id, created_at DESC)
    WHERE status = 'active';

CREATE INDEX IF NOT EXISTS idx_friend_messages_pair_created
    ON friend_messages(sender_id, receiver_id, created_at ASC);

-- =========================
-- 用户情绪 / 咨询链路
-- =========================
-- 命中：
-- 1) user_emotion_history: WHERE user_id = ? AND created_at >= ... ORDER BY created_at
-- 2) consultation_sessions: counselor 侧会话列表
-- 3) consultation_messages: 会话消息按时间正序读取
CREATE INDEX IF NOT EXISTS idx_user_emotion_history_user_created
    ON user_emotion_history(user_id, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_consultation_sessions_counselor_created
    ON consultation_sessions(counselor_id, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_consultation_messages_session_created
    ON consultation_messages(session_id, created_at ASC);

-- =========================
-- 举报去重检查
-- =========================
-- 命中 createReport:
-- WHERE reporter_id = ? AND target_type = ? AND target_id = ? AND status = 'pending'
CREATE INDEX IF NOT EXISTS idx_reports_pending_reporter_target
    ON reports(reporter_id, target_type, target_id)
    WHERE status = 'pending';

COMMIT;
