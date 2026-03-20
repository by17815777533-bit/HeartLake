/**
 * 亲密分服务实现（多信号融合 + 时序衰减 + AI 共鸣）
 *
 * 通过一条大型 CTE SQL 查询完成关系强度的全维度计算，核心信号包括：
 * 1. 互动强度：纸船评论(6.4) > 私聊(4.8) > 涟漪(2.4)，带 18 天半衰期指数衰减
 * 2. 双向互惠：min(out,in)/max(out,in) 防止单向骚扰刷分
 * 3. 共同涟漪：共同关注的石头数量，反映兴趣重叠
 * 4. 情绪兼容：基于 emotion_compatibility 表的加权情绪分布相似度
 * 5. 语义相似：user_similarity 表中的向量余弦相似度
 * 6. 情绪趋势对齐：45 天情绪均值和波动率的差异
 * 7. 对话质量：轮流发言频率 + 响应敏捷度
 * 8. 社交图谱亲和：Jaccard 系数衡量共同邻居比例
 * 9. 情绪同步：日粒度情绪轨迹的逐日差异
 *
 * 最终分数 = 加权融合 * 稳定性系数 + 交叉项奖励 - 反作弊惩罚
 * 分数区间：stranger(0-12) → warm(12-55) → close(55-80) → soulmate(80-100)
 */

#include "infrastructure/services/IntimacyService.h"
#include "utils/RequestHelper.h"
#include <drogon/drogon.h>
#include <algorithm>

using namespace heartlake::utils;

namespace heartlake::infrastructure {

namespace {

constexpr double kDefaultMinFriendScore = 12.0;

inline double clamp01(double v) {
    return std::clamp(v, 0.0, 1.0);
}

// 关系强度多信号模型（全面 + 先进 + 创新）：
// 1) 互动强度（石头评论/涟漪/私聊）+ 时间衰减
// 2) 双向互惠（避免单向骚扰刷分）
// 3) 共兴趣信号（共同涟漪）
// 4) 情绪兼容（emotion_compatibility）
// 5) 语义相似（user_similarity）
// 6) 情绪趋势对齐（user_emotion_history）
static const std::string kIntimacyBaseQuery = R"SQL(
WITH raw_interactions AS (
    -- 石头评论（纸船）是最强的有效互动信号
    SELECT
        pb.sender_id AS actor_id,
        s.user_id AS peer_id,
        pb.created_at AS ts,
        6.4::double precision AS weight,
        'boat_comment'::text AS kind
    FROM paper_boats pb
    JOIN stones s ON s.stone_id = pb.stone_id
    WHERE pb.sender_id IS NOT NULL
      AND s.user_id IS NOT NULL
      AND pb.sender_id <> s.user_id
      AND COALESCE(pb.status, 'active') <> 'deleted'
      AND pb.created_at >= NOW() - INTERVAL '120 days'
      AND (pb.sender_id = $1 OR s.user_id = $1)

    UNION ALL

    -- 涟漪权重低于评论，但反映持续关注
    SELECT
        r.user_id AS actor_id,
        s.user_id AS peer_id,
        r.created_at AS ts,
        2.4::double precision AS weight,
        'ripple'::text AS kind
    FROM ripples r
    JOIN stones s ON s.stone_id = r.stone_id
    WHERE r.user_id <> s.user_id
      AND r.created_at >= NOW() - INTERVAL '120 days'
      AND (r.user_id = $1 OR s.user_id = $1)

    UNION ALL

    -- 既有私聊历史作为附加信号（兼容旧数据）
    SELECT
        fm.sender_id AS actor_id,
        fm.receiver_id AS peer_id,
        fm.created_at AS ts,
        4.8::double precision AS weight,
        'chat'::text AS kind
    FROM friend_messages fm
    WHERE fm.sender_id <> fm.receiver_id
      AND fm.created_at >= NOW() - INTERVAL '120 days'
      AND (fm.sender_id = $1 OR fm.receiver_id = $1)
),
interaction_scored AS (
    SELECT
        CASE WHEN actor_id = $1 THEN peer_id ELSE actor_id END AS peer_id,
        CASE WHEN actor_id = $1 THEN 1 ELSE 0 END AS is_outbound,
        kind,
        ts,
        weight * EXP(-GREATEST(EXTRACT(EPOCH FROM (NOW() - ts)), 0) / 86400.0 / 18.0) AS decayed_weight
    FROM raw_interactions
),
interaction_agg AS (
    SELECT
        peer_id,
        SUM(decayed_weight) AS interaction_weight,
        SUM(CASE WHEN is_outbound = 1 THEN decayed_weight ELSE 0 END) AS out_weight,
        SUM(CASE WHEN is_outbound = 0 THEN decayed_weight ELSE 0 END) AS in_weight,
        COUNT(*)::int AS interaction_count,
        COUNT(*) FILTER (WHERE ts >= NOW() - INTERVAL '1 hour')::int AS recent_1h_count,
        COUNT(*) FILTER (WHERE ts >= NOW() - INTERVAL '24 hours')::int AS recent_24h_count,
        COUNT(DISTINCT DATE(ts))::int AS active_days,
        SUM(CASE WHEN kind = 'boat_comment' THEN 1 ELSE 0 END)::int AS boat_comments,
        SUM(CASE WHEN kind = 'ripple' THEN 1 ELSE 0 END)::int AS ripple_count,
        MIN(ts) AS first_interacted_at,
        MAX(ts) AS last_interacted_at
    FROM interaction_scored
    GROUP BY peer_id
),
candidate_peers AS (
    SELECT ia.peer_id
    FROM interaction_agg ia
    ORDER BY ia.interaction_weight DESC, ia.interaction_count DESC
    LIMIT 300
),
co_ripple AS (
    SELECT
        r2.user_id AS peer_id,
        COUNT(*)::double precision AS co_ripple_count
    FROM ripples r1
    JOIN ripples r2
      ON r1.stone_id = r2.stone_id
     AND r1.user_id <> r2.user_id
    WHERE r1.user_id = $1
      AND r2.user_id IN (SELECT peer_id FROM candidate_peers)
      AND r1.created_at >= NOW() - INTERVAL '120 days'
      AND r2.created_at >= NOW() - INTERVAL '120 days'
    GROUP BY r2.user_id
),
mood_self AS (
    SELECT
        s.mood_type,
        COUNT(*)::double precision AS cnt
    FROM stones s
    WHERE s.user_id = $1
      AND s.status = 'published'
      AND s.deleted_at IS NULL
      AND s.created_at >= NOW() - INTERVAL '90 days'
      AND s.mood_type IS NOT NULL
      AND s.mood_type <> ''
    GROUP BY s.mood_type
),
mood_self_total AS (
    SELECT COALESCE(SUM(cnt), 0.0) AS total
    FROM mood_self
),
mood_self_dist AS (
    SELECT
        ms.mood_type,
        ms.cnt / NULLIF((SELECT total FROM mood_self_total), 0.0) AS p
    FROM mood_self ms
),
mood_peer AS (
    SELECT
        s.user_id AS peer_id,
        s.mood_type,
        COUNT(*)::double precision AS cnt
    FROM stones s
    WHERE s.user_id IN (SELECT peer_id FROM candidate_peers)
      AND s.status = 'published'
      AND s.deleted_at IS NULL
      AND s.created_at >= NOW() - INTERVAL '90 days'
      AND s.mood_type IS NOT NULL
      AND s.mood_type <> ''
    GROUP BY s.user_id, s.mood_type
),
mood_peer_total AS (
    SELECT
        mp.peer_id,
        SUM(mp.cnt) AS total
    FROM mood_peer mp
    GROUP BY mp.peer_id
),
mood_peer_dist AS (
    SELECT
        mp.peer_id,
        mp.mood_type,
        mp.cnt / NULLIF(mpt.total, 0.0) AS p
    FROM mood_peer mp
    JOIN mood_peer_total mpt ON mpt.peer_id = mp.peer_id
),
mood_compat AS (
    SELECT
        mp.peer_id,
        COALESCE(
            SUM(ms.p * mp.p * COALESCE(ec.compatibility_score, 0.5))
            / NULLIF(SUM(ms.p * mp.p), 0.0),
            0.5
        ) AS mood_resonance
    FROM mood_peer_dist mp
    CROSS JOIN mood_self_dist ms
    LEFT JOIN emotion_compatibility ec
      ON ec.mood_type_1 = ms.mood_type
     AND ec.mood_type_2 = mp.mood_type
    GROUP BY mp.peer_id
),
semantic_similarity AS (
    SELECT
        cp.peer_id,
        COALESCE(
            MAX(
                CASE
                    WHEN us.user1_id = $1 AND us.user2_id = cp.peer_id THEN us.similarity_score
                    WHEN us.user2_id = $1 AND us.user1_id = cp.peer_id THEN us.similarity_score
                    ELSE NULL
                END
            ),
            0.5
        ) AS semantic_similarity
    FROM candidate_peers cp
    LEFT JOIN user_similarity us
      ON (us.user1_id = $1 AND us.user2_id = cp.peer_id)
      OR (us.user2_id = $1 AND us.user1_id = cp.peer_id)
    GROUP BY cp.peer_id
),
emotion_self AS (
    SELECT
        COUNT(*)::double precision AS sample_count,
        COALESCE(AVG(h.sentiment_score), 0.0) AS avg_score,
        COALESCE(STDDEV_POP(h.sentiment_score), 0.0) AS volatility
    FROM user_emotion_history h
    WHERE h.user_id = $1
      AND h.created_at >= NOW() - INTERVAL '45 days'
),
emotion_peer AS (
    SELECT
        h.user_id AS peer_id,
        COALESCE(AVG(h.sentiment_score), 0.0) AS avg_score,
        COALESCE(STDDEV_POP(h.sentiment_score), 0.0) AS volatility
    FROM user_emotion_history h
    WHERE h.user_id IN (SELECT peer_id FROM candidate_peers)
      AND h.created_at >= NOW() - INTERVAL '45 days'
    GROUP BY h.user_id
),
emotion_alignment AS (
    SELECT
        cp.peer_id,
        CASE
            WHEN es.sample_count = 0 OR ep.peer_id IS NULL THEN 0.5
            ELSE
                (
                    1.0 - LEAST(1.0, ABS(ep.avg_score - es.avg_score) / 1.5)
                ) * 0.72
                +
                (
                    1.0 - LEAST(1.0, ABS(ep.volatility - es.volatility) / 1.0)
                ) * 0.28
        END AS emotion_alignment
    FROM candidate_peers cp
    CROSS JOIN emotion_self es
    LEFT JOIN emotion_peer ep ON ep.peer_id = cp.peer_id
),
dialogue_events AS (
    SELECT
        iss.peer_id,
        CASE WHEN iss.is_outbound = 1 THEN $1 ELSE iss.peer_id END AS actor_id,
        iss.ts,
        LAG(CASE WHEN iss.is_outbound = 1 THEN $1 ELSE iss.peer_id END)
            OVER (PARTITION BY iss.peer_id ORDER BY iss.ts) AS prev_actor_id,
        EXTRACT(EPOCH FROM (
            iss.ts - LAG(iss.ts) OVER (PARTITION BY iss.peer_id ORDER BY iss.ts)
        )) / 3600.0 AS gap_hours
    FROM interaction_scored iss
),
dialogue_quality AS (
    SELECT
        de.peer_id,
        LEAST(
            1.0,
            COUNT(*) FILTER (
                WHERE de.prev_actor_id IS NOT NULL
                  AND de.prev_actor_id <> de.actor_id
            )::double precision / 12.0
        ) AS turn_taking,
        COALESCE(
            1.0 - LEAST(
                1.0,
                AVG(
                    CASE
                        WHEN de.prev_actor_id IS NOT NULL
                          AND de.prev_actor_id <> de.actor_id
                          AND de.gap_hours >= 0
                        THEN LEAST(de.gap_hours, 72.0)
                        ELSE NULL
                    END
                ) / 36.0
            ),
            0.5
        ) AS response_agility
    FROM dialogue_events de
    GROUP BY de.peer_id
),
graph_interactions AS (
    SELECT
        pb.sender_id AS actor_id,
        s.user_id AS peer_id
    FROM paper_boats pb
    JOIN stones s ON s.stone_id = pb.stone_id
    WHERE pb.sender_id IS NOT NULL
      AND s.user_id IS NOT NULL
      AND pb.sender_id <> s.user_id
      AND COALESCE(pb.status, 'active') <> 'deleted'
      AND pb.created_at >= NOW() - INTERVAL '120 days'
      AND (
          pb.sender_id = $1 OR s.user_id = $1
          OR pb.sender_id IN (SELECT peer_id FROM candidate_peers)
          OR s.user_id IN (SELECT peer_id FROM candidate_peers)
      )

    UNION ALL

    SELECT
        r.user_id AS actor_id,
        s.user_id AS peer_id
    FROM ripples r
    JOIN stones s ON s.stone_id = r.stone_id
    WHERE r.user_id <> s.user_id
      AND r.created_at >= NOW() - INTERVAL '120 days'
      AND (
          r.user_id = $1 OR s.user_id = $1
          OR r.user_id IN (SELECT peer_id FROM candidate_peers)
          OR s.user_id IN (SELECT peer_id FROM candidate_peers)
      )

    UNION ALL

    SELECT
        fm.sender_id AS actor_id,
        fm.receiver_id AS peer_id
    FROM friend_messages fm
    WHERE fm.sender_id <> fm.receiver_id
      AND fm.created_at >= NOW() - INTERVAL '120 days'
      AND (
          fm.sender_id = $1 OR fm.receiver_id = $1
          OR fm.sender_id IN (SELECT peer_id FROM candidate_peers)
          OR fm.receiver_id IN (SELECT peer_id FROM candidate_peers)
      )
),
graph_neighbors AS (
    SELECT
        gi.actor_id AS user_id,
        gi.peer_id AS neighbor_id
    FROM graph_interactions gi
    UNION ALL
    SELECT
        gi.peer_id AS user_id,
        gi.actor_id AS neighbor_id
    FROM graph_interactions gi
),
user_neighbor_set AS (
    SELECT DISTINCT gn.neighbor_id
    FROM graph_neighbors gn
    WHERE gn.user_id = $1
      AND gn.neighbor_id <> $1
),
user_neighbor_count AS (
    SELECT COUNT(*)::double precision AS neighbor_count
    FROM user_neighbor_set
),
peer_neighbor_stats AS (
    SELECT
        cp.peer_id,
        COUNT(DISTINCT gn.neighbor_id) FILTER (
            WHERE gn.neighbor_id <> cp.peer_id
              AND gn.neighbor_id <> $1
              AND uns.neighbor_id IS NOT NULL
        )::double precision AS common_neighbors,
        COUNT(DISTINCT gn.neighbor_id) FILTER (
            WHERE gn.neighbor_id <> cp.peer_id
              AND gn.neighbor_id <> $1
        )::double precision AS peer_neighbor_count
    FROM candidate_peers cp
    LEFT JOIN graph_neighbors gn ON gn.user_id = cp.peer_id
    LEFT JOIN user_neighbor_set uns ON uns.neighbor_id = gn.neighbor_id
    GROUP BY cp.peer_id
),
graph_affinity AS (
    SELECT
        pns.peer_id,
        CASE
            WHEN COALESCE(unc.neighbor_count, 0.0) = 0.0
              AND pns.peer_neighbor_count = 0.0 THEN 0.5
            ELSE COALESCE(
                LEAST(
                    1.0,
                    pns.common_neighbors /
                    NULLIF(unc.neighbor_count + pns.peer_neighbor_count - pns.common_neighbors, 0.0)
                    * 2.6
                ),
                0.5
            )
        END AS triadic_affinity
    FROM peer_neighbor_stats pns
    CROSS JOIN user_neighbor_count unc
),
emotion_daily_self AS (
    SELECT
        DATE(h.created_at) AS day_key,
        AVG(h.sentiment_score) AS avg_score
    FROM user_emotion_history h
    WHERE h.user_id = $1
      AND h.created_at >= NOW() - INTERVAL '45 days'
    GROUP BY DATE(h.created_at)
),
emotion_daily_peer AS (
    SELECT
        h.user_id AS peer_id,
        DATE(h.created_at) AS day_key,
        AVG(h.sentiment_score) AS avg_score
    FROM user_emotion_history h
    WHERE h.user_id IN (SELECT peer_id FROM candidate_peers)
      AND h.created_at >= NOW() - INTERVAL '45 days'
    GROUP BY h.user_id, DATE(h.created_at)
),
emotion_synchrony AS (
    SELECT
        cp.peer_id,
        COALESCE(
            1.0 - LEAST(
                1.0,
                AVG(ABS(edp.avg_score - eds.avg_score)) / 1.4
            ),
            0.5
        ) AS emotion_synchrony
    FROM candidate_peers cp
    LEFT JOIN emotion_daily_peer edp ON edp.peer_id = cp.peer_id
    LEFT JOIN emotion_daily_self eds ON eds.day_key = edp.day_key
    GROUP BY cp.peer_id
),
feature_frame AS (
    SELECT
        ia.peer_id,
        ia.interaction_weight,
        ia.out_weight,
        ia.in_weight,
        ia.interaction_count,
        ia.recent_1h_count,
        ia.recent_24h_count,
        ia.active_days,
        ia.boat_comments,
        ia.ripple_count,
        ia.first_interacted_at,
        ia.last_interacted_at,
        COALESCE(cr.co_ripple_count, 0.0) AS co_ripple_count,
        COALESCE(mc.mood_resonance, 0.5) AS mood_resonance,
        COALESCE(ss.semantic_similarity, 0.5) AS semantic_similarity,
        COALESCE(ea.emotion_alignment, 0.5) AS emotion_alignment,
        COALESCE(
            dq.turn_taking * 0.62 + dq.response_agility * 0.38,
            0.5
        ) AS dialogue_cohesion,
        COALESCE(dq.response_agility, 0.5) AS response_agility,
        COALESCE(ga.triadic_affinity, 0.5) AS graph_affinity,
        COALESCE(esy.emotion_synchrony, 0.5) AS emotion_synchrony,
        EXP(-GREATEST(EXTRACT(EPOCH FROM (NOW() - ia.last_interacted_at)), 0) / 86400.0 / 35.0) AS freshness
    FROM interaction_agg ia
    JOIN candidate_peers cp ON cp.peer_id = ia.peer_id
    LEFT JOIN co_ripple cr ON cr.peer_id = ia.peer_id
    LEFT JOIN mood_compat mc ON mc.peer_id = ia.peer_id
    LEFT JOIN semantic_similarity ss ON ss.peer_id = ia.peer_id
    LEFT JOIN emotion_alignment ea ON ea.peer_id = ia.peer_id
    LEFT JOIN dialogue_quality dq ON dq.peer_id = ia.peer_id
    LEFT JOIN graph_affinity ga ON ga.peer_id = ia.peer_id
    LEFT JOIN emotion_synchrony esy ON esy.peer_id = ia.peer_id
),
scored AS (
    SELECT
        ff.peer_id,
        ff.interaction_count,
        ff.boat_comments,
        ff.ripple_count,
        ff.last_interacted_at,
        (1.0 - EXP(-ff.interaction_weight / 14.0)) AS interaction_strength,
        COALESCE(
            LEAST(ff.out_weight, ff.in_weight) / NULLIF(GREATEST(ff.out_weight, ff.in_weight), 0.0),
            0.0
        ) AS reciprocity_score,
        (1.0 - EXP(-ff.co_ripple_count / 4.0)) AS co_ripple_score,
        LEAST(GREATEST(ff.mood_resonance, 0.0), 1.0) AS mood_resonance,
        LEAST(GREATEST(ff.semantic_similarity, 0.0), 1.0) AS semantic_similarity,
        LEAST(GREATEST(ff.emotion_alignment, 0.0), 1.0) AS emotion_alignment,
        LEAST(GREATEST(ff.dialogue_cohesion, 0.0), 1.0) AS dialogue_cohesion,
        LEAST(GREATEST(ff.response_agility, 0.0), 1.0) AS response_agility,
        LEAST(GREATEST(ff.graph_affinity, 0.0), 1.0) AS graph_affinity,
        LEAST(GREATEST(ff.emotion_synchrony, 0.0), 1.0) AS emotion_synchrony,
        LEAST(GREATEST(ff.freshness, 0.0), 1.0) AS freshness,
        LEAST(1.0, ff.interaction_count / 10.0) AS stability,
        LEAST(
            1.0,
            ff.active_days::double precision / GREATEST(
                1.0,
                EXTRACT(EPOCH FROM (ff.last_interacted_at - ff.first_interacted_at)) / 86400.0 + 1.0
            )
        ) AS temporal_diversity,
        LEAST(
            0.55,
            GREATEST(
                0.0,
                (
                    GREATEST(
                        ff.recent_1h_count::double precision /
                        NULLIF(ff.interaction_count::double precision, 0.0) - 0.55,
                        0.0
                    ) * 0.80
                )
                +
                (
                    GREATEST(
                        ff.recent_24h_count::double precision /
                        NULLIF(ff.interaction_count::double precision, 0.0) - 0.85,
                        0.0
                    ) * 0.60
                )
                +
                (
                    GREATEST(
                        0.25 - LEAST(
                            1.0,
                            ff.active_days::double precision / GREATEST(
                                1.0,
                                EXTRACT(EPOCH FROM (ff.last_interacted_at - ff.first_interacted_at)) / 86400.0 + 1.0
                            )
                        ),
                        0.0
                    ) * 0.90
                )
            )
        ) AS anti_gaming_penalty,
        (
            (
                0.40 * (1.0 - EXP(-ff.interaction_weight / 14.0)) +
                0.13 * COALESCE(
                    LEAST(ff.out_weight, ff.in_weight) / NULLIF(GREATEST(ff.out_weight, ff.in_weight), 0.0),
                    0.0
                ) +
                0.08 * (1.0 - EXP(-COALESCE(ff.co_ripple_count, 0.0) / 4.0)) +
                0.10 * LEAST(GREATEST(ff.mood_resonance, 0.0), 1.0) +
                0.08 * LEAST(GREATEST(ff.semantic_similarity, 0.0), 1.0) +
                0.06 * LEAST(GREATEST(ff.emotion_alignment, 0.0), 1.0) +
                0.05 * LEAST(GREATEST(ff.dialogue_cohesion, 0.0), 1.0) +
                0.04 * LEAST(GREATEST(ff.graph_affinity, 0.0), 1.0) +
                0.04 * LEAST(GREATEST(ff.emotion_synchrony, 0.0), 1.0) +
                0.06 * LEAST(GREATEST(ff.freshness, 0.0), 1.0)
            ) * (0.80 + 0.20 * LEAST(1.0, ff.interaction_count / 10.0))
            + 0.04 * SQRT(
                GREATEST(
                    LEAST(GREATEST(ff.mood_resonance, 0.0), 1.0) *
                    LEAST(GREATEST(ff.semantic_similarity, 0.0), 1.0),
                    0.0
                )
            )
            + 0.02 * SQRT(
                GREATEST(
                    LEAST(GREATEST(ff.dialogue_cohesion, 0.0), 1.0) *
                    LEAST(GREATEST(ff.graph_affinity, 0.0), 1.0),
                    0.0
                )
            )
        ) AS fused_signal
    FROM feature_frame ff
),
final_score AS (
    SELECT
        s.peer_id,
        (
            CASE
                WHEN s.interaction_count <= 1 THEN LEAST(28.0, GREATEST(0.0, 100.0 * s.fused_signal))
                WHEN s.interaction_count <= 3 THEN LEAST(58.0, GREATEST(0.0, 100.0 * s.fused_signal))
                ELSE LEAST(100.0, GREATEST(0.0, 100.0 * s.fused_signal))
            END
        ) * (1.0 - s.anti_gaming_penalty) AS intimacy_score,
        s.interaction_count,
        s.boat_comments,
        s.ripple_count,
        s.last_interacted_at,
        s.interaction_strength,
        s.reciprocity_score,
        s.co_ripple_score,
        s.mood_resonance,
        s.semantic_similarity,
        s.emotion_alignment,
        s.dialogue_cohesion,
        s.response_agility,
        s.graph_affinity,
        s.emotion_synchrony,
        s.freshness,
        s.temporal_diversity,
        s.anti_gaming_penalty,
        (1.0 - s.anti_gaming_penalty) AS behavior_health
    FROM scored s
)
)SQL";

}  // namespace

IntimacyService& IntimacyService::getInstance() {
    static IntimacyService instance;
    return instance;
}

std::vector<IntimacyProfile> IntimacyService::getTopIntimacyPeers(
    const std::string& userId,
    int limit,
    double minScore
) const {
    auto db = drogon::app().getDbClient("default");
    std::vector<IntimacyProfile> peers;

    try {
        const int safeLimit = std::clamp(limit, 1, 200);
        const double safeMinScore = std::max(0.0, minScore);

        // 非协程上下文，使用同步查询
        auto result = db->execSqlSync(
            kIntimacyBaseQuery +
            "SELECT "
            "  fs.peer_id AS user_id, "
            "  COALESCE(NULLIF(u.username, ''), u.user_id) AS username, "
            "  COALESCE(NULLIF(u.nickname, ''), '匿名旅人') AS nickname, "
            "  fs.intimacy_score, "
            "  fs.interaction_count, "
            "  fs.boat_comments, "
            "  fs.ripple_count, "
            "  fs.last_interacted_at, "
            "  fs.interaction_strength, "
            "  fs.reciprocity_score, "
            "  fs.co_ripple_score, "
            "  fs.mood_resonance, "
            "  fs.semantic_similarity, "
            "  fs.emotion_alignment, "
            "  fs.dialogue_cohesion, "
            "  fs.response_agility, "
            "  fs.graph_affinity, "
            "  fs.emotion_synchrony, "
            "  fs.freshness, "
            "  fs.temporal_diversity, "
            "  fs.anti_gaming_penalty, "
            "  fs.behavior_health "
            "FROM final_score fs "
            "JOIN users u ON u.user_id = fs.peer_id "
            "WHERE u.status = 'active' "
            "  AND fs.intimacy_score >= CAST($2 AS DOUBLE PRECISION) "
            "ORDER BY fs.intimacy_score DESC, fs.last_interacted_at DESC "
            "LIMIT CAST($3 AS INTEGER)",
            userId, safeMinScore, safeLimit
        );

        peers.reserve(result.size());
        for (const auto& row : result) {
            IntimacyProfile profile;
            profile.userId = row["user_id"].as<std::string>();
            profile.username = row["username"].as<std::string>();
            profile.nickname = row["nickname"].as<std::string>();
            profile.intimacyScore = row["intimacy_score"].as<double>();
            profile.intimacyLevel = levelFromScore(profile.intimacyScore);
            profile.interactionCount = row["interaction_count"].as<int>();
            profile.boatComments = row["boat_comments"].as<int>();
            profile.rippleCount = row["ripple_count"].as<int>();
            profile.lastInteractedAt = row["last_interacted_at"].as<std::string>();
            profile.interactionStrength = row["interaction_strength"].as<double>();
            profile.reciprocityScore = row["reciprocity_score"].as<double>();
            profile.coRippleScore = row["co_ripple_score"].as<double>();
            profile.moodResonance = row["mood_resonance"].as<double>();
            profile.semanticSimilarity = row["semantic_similarity"].as<double>();
            profile.emotionTrendAlignment = row["emotion_alignment"].as<double>();
            profile.dialogueCohesion = row["dialogue_cohesion"].as<double>();
            profile.responseAgility = row["response_agility"].as<double>();
            profile.graphAffinity = row["graph_affinity"].as<double>();
            profile.emotionSynchrony = row["emotion_synchrony"].as<double>();
            profile.freshnessScore = row["freshness"].as<double>();
            profile.temporalDiversity = row["temporal_diversity"].as<double>();
            profile.antiGamingPenalty = row["anti_gaming_penalty"].as<double>();
            profile.behaviorHealth = row["behavior_health"].as<double>();
            profile.aiCompatibility = clamp01(
                0.27 * profile.moodResonance
                + 0.24 * profile.semanticSimilarity
                + 0.18 * profile.emotionTrendAlignment
                + 0.15 * profile.dialogueCohesion
                + 0.10 * profile.graphAffinity
                + 0.06 * profile.emotionSynchrony
            );
            profile.canChat = profile.intimacyScore >= kDefaultMinFriendScore;
            peers.push_back(std::move(profile));
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "getTopIntimacyPeers failed: " << e.base().what();
    }

    return peers;
}

double IntimacyService::getIntimacyScore(
    const std::string& userId,
    const std::string& peerId
) const {
    auto db = drogon::app().getDbClient("default");

    try {
        // 非协程上下文，使用同步查询
        auto result = db->execSqlSync(
            kIntimacyBaseQuery +
            "SELECT fs.intimacy_score "
            "FROM final_score fs "
            "WHERE fs.peer_id = $2 "
            "LIMIT 1",
            userId, peerId
        );

        if (auto rowOpt = safeRow(result)) {
            return (*rowOpt)["intimacy_score"].as<double>();
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "getIntimacyScore failed: " << e.base().what();
    }

    return 0.0;
}

bool IntimacyService::canChat(
    const std::string& userId,
    const std::string& peerId,
    double threshold
) const {
    return getIntimacyScore(userId, peerId) >= std::max(0.0, threshold);
}

std::string IntimacyService::levelFromScore(double score) {
    if (score >= 80.0) return "soulmate";
    if (score >= 55.0) return "close";
    if (score >= 12.0) return "warm";
    return "stranger";
}

}  // namespace heartlake::infrastructure
