-- 清理重复涟漪数据（保留最早的）
DELETE FROM ripples a USING ripples b
WHERE a.created_at > b.created_at AND a.stone_id = b.stone_id AND a.user_id = b.user_id;
-- 添加唯一约束防止重复涟漪
ALTER TABLE ripples ADD CONSTRAINT uq_ripples_stone_user UNIQUE(stone_id, user_id);
