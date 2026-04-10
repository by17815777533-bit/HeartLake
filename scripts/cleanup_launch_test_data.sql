BEGIN;

CREATE TEMP TABLE launch_test_stones AS
SELECT stone_id
FROM stones
WHERE content LIKE '%[smoke]%'
   OR content LIKE '%[fix-check]%'
   OR content LIKE '%[docker-smoke]%'
   OR content ILIKE '%performance probe%';

CREATE TEMP TABLE launch_test_boats AS
SELECT boat_id
FROM paper_boats
WHERE stone_id IN (SELECT stone_id FROM launch_test_stones)
   OR content LIKE '%[smoke]%'
   OR content LIKE '%[docker-smoke]%'
   OR content ILIKE '%performance probe%';

DELETE FROM notifications
WHERE related_id IN (SELECT stone_id FROM launch_test_stones)
   OR related_id IN (SELECT boat_id FROM launch_test_boats);

DELETE FROM reports
WHERE (target_type = 'stone' AND target_id IN (SELECT stone_id FROM launch_test_stones))
   OR (target_type = 'boat' AND target_id IN (SELECT boat_id FROM launch_test_boats));

DELETE FROM moderation_logs
WHERE (target_type = 'stone' AND target_id IN (SELECT stone_id FROM launch_test_stones))
   OR (target_type = 'boat' AND target_id IN (SELECT boat_id FROM launch_test_boats));

DELETE FROM operation_logs
WHERE (target_type = 'stone' AND target_id IN (SELECT stone_id FROM launch_test_stones))
   OR (target_type = 'boat' AND target_id IN (SELECT boat_id FROM launch_test_boats));

DELETE FROM user_interaction_history
WHERE stone_id IN (SELECT stone_id FROM launch_test_stones);

DELETE FROM paper_boats
WHERE boat_id IN (SELECT boat_id FROM launch_test_boats)
   OR stone_id IN (SELECT stone_id FROM launch_test_stones);

DELETE FROM ripples
WHERE stone_id IN (SELECT stone_id FROM launch_test_stones);

DELETE FROM stone_embeddings
WHERE stone_id IN (SELECT stone_id FROM launch_test_stones);

DELETE FROM stones
WHERE stone_id IN (SELECT stone_id FROM launch_test_stones);

SELECT
    (SELECT COUNT(*) FROM launch_test_stones) AS removed_stones,
    (SELECT COUNT(*) FROM launch_test_boats) AS removed_boats;

COMMIT;
