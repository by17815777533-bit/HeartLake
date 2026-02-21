// @file recommended_stone_card.dart
// @brief 推荐石头卡片组件
// Created by 吴睿璐

library;
import 'package:flutter/material.dart';
import '../../data/datasources/recommendation_service.dart';
import '../../domain/entities/stone.dart';
import 'stone_card.dart';

class RecommendedStoneCard extends StatelessWidget {
  final RecommendedStone recommendedStone;

  final VoidCallback? onRippleSuccess;

  final VoidCallback? onDeleted;

  final bool showRecommendationReason;

  const RecommendedStoneCard({
    super.key,
    required this.recommendedStone,
    this.onRippleSuccess,
    this.onDeleted,
    this.showRecommendationReason = true,
  });

  Stone _convertToStone() {
    return Stone(
      stoneId: recommendedStone.stoneId,
      userId: recommendedStone.authorId ?? '',
      content: recommendedStone.content,
      stoneType: 'medium', // 默认中等重量
      stoneColor: '#7A92A3', // 默认石头颜色
      moodType: recommendedStone.moodType,
      sentimentScore: recommendedStone.emotionScore,
      rippleCount: recommendedStone.rippleCount,
      boatCount: recommendedStone.boatCount,
      createdAt: recommendedStone.createdAt ?? DateTime.now(),
      status: 'published',
      authorNickname: recommendedStone.authorName,
      tags: recommendedStone.tags ?? [],
      mediaIds: recommendedStone.mediaUrls,
      hasMedia: recommendedStone.mediaUrls != null && recommendedStone.mediaUrls!.isNotEmpty,
    );
  }

  IconData _getRecommendationIcon() {
    switch (recommendedStone.recommendationType) {
      case RecommendationType.similar:
        return Icons.content_copy;
      case RecommendationType.collaborative:
        return Icons.people_outline;
      case RecommendationType.emotionCompatible:
        return Icons.favorite_border;
      case RecommendationType.exploration:
        return Icons.explore_outlined;
      case RecommendationType.trending:
        return Icons.trending_up;
      case RecommendationType.personalized:
        return Icons.auto_awesome;
      case RecommendationType.random:
        return Icons.shuffle;
    }
  }

  @override
  Widget build(BuildContext context) {
    final stone = _convertToStone();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // 推荐理由标签
        if (showRecommendationReason) ...[
          Container(
            margin: const EdgeInsets.only(left: 8, bottom: 8),
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
            decoration: BoxDecoration(
              color: Theme.of(context).colorScheme.secondaryContainer,
              borderRadius: BorderRadius.circular(8),
            ),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(
                  _getRecommendationIcon(),
                  size: 14,
                  color: Theme.of(context).colorScheme.onSecondaryContainer,
                ),
                const SizedBox(width: 6),
                Flexible(
                  child: Text(
                    recommendedStone.recommendationReason,
                    style: TextStyle(
                      fontSize: 12,
                      color: Theme.of(context).colorScheme.onSecondaryContainer,
                      fontWeight: FontWeight.w500,
                    ),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
              ],
            ),
          ),
        ],

        // 石头卡片
        StoneCard(
          stone: stone,
          onRippleSuccess: onRippleSuccess,
          onDeleted: onDeleted,
        ),
      ],
    );
  }
}