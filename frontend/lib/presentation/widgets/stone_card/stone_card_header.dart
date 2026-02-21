// @file stone_card_header.dart
// @brief StoneCard头部组件
// Created by 吴睿璐

import 'package:flutter/material.dart';
import '../../../domain/entities/stone.dart';
import '../../../utils/app_theme.dart';
import '../../../utils/mood_colors.dart';

/// StoneCard 头部组件 - 显示作者信息和时间状态
class StoneCardHeader extends StatelessWidget {
  final Stone stone;
  final MoodColorConfig moodConfig;
  final VoidCallback onMorePressed;

  const StoneCardHeader({
    super.key,
    required this.stone,
    required this.moodConfig,
    required this.onMorePressed,
  });

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        _buildAvatar(),
        const SizedBox(width: 12),
        _buildAuthorInfo(context),
        const Spacer(),
        _buildTimeStatus(),
        IconButton.filledTonal(
          icon: const Icon(Icons.more_horiz_rounded, size: 20),
          onPressed: onMorePressed,
          style: IconButton.styleFrom(
            minimumSize: const Size(32, 32),
            padding: EdgeInsets.zero,
          ),
        ),
      ],
    );
  }

  Widget _buildAvatar() {
    return Container(
      width: 36,
      height: 36,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            moodConfig.primary.withValues(alpha: 0.7),
            moodConfig.primary,
          ],
        ),
        boxShadow: [
          BoxShadow(
            color: moodConfig.primary.withValues(alpha: 0.4),
            blurRadius: 8,
            offset: const Offset(2, 2),
          )
        ],
        border: Border.all(color: Colors.white, width: 2),
      ),
      child: Icon(moodConfig.icon, color: Colors.white, size: 20),
    );
  }

  Widget _buildAuthorInfo(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          stone.authorNickname ?? '匿名旅人',
          style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                fontWeight: FontWeight.bold,
                color: moodConfig.textColor,
                fontSize: 15,
              ),
        ),
        if (stone.moodType != null)
          Container(
            margin: const EdgeInsets.only(top: 2),
            padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 1),
            decoration: BoxDecoration(
              color: moodConfig.primary.withValues(alpha: 0.15),
              borderRadius: BorderRadius.circular(8),
            ),
            child: Text(
              moodConfig.name,
              style: TextStyle(
                fontSize: 10,
                color: moodConfig.primary,
                fontWeight: FontWeight.w500,
              ),
            ),
          ),
      ],
    );
  }

  Widget _buildTimeStatus() {
    final difference = DateTime.now().difference(stone.createdAt);
    String text;
    Color color = AppTheme.textSecondary;
    FontWeight fontWeight = FontWeight.normal;

    if (difference.inMinutes < 60) {
      text = '刚刚';
    } else if (difference.inHours >= 23) {
      text = '即将沉没';
      color = Colors.red;
      fontWeight = FontWeight.bold;
    } else if (difference.inDays > 0) {
      text = '${difference.inDays}天前';
    } else {
      text = '${difference.inHours}小时前';
    }

    return Padding(
      padding: const EdgeInsets.only(right: 4.0),
      child: Text(text, style: TextStyle(color: color, fontSize: 12, fontWeight: fontWeight)),
    );
  }
}
