// StoneCard内容组件

import 'package:flutter/material.dart';
import '../../../domain/entities/stone.dart';
import '../../../utils/mood_colors.dart';

/// StoneCard 内容组件 - 显示石头内容和标签
class StoneCardContent extends StatelessWidget {
  final Stone stone;
  final MoodColorConfig moodConfig;

  const StoneCardContent({
    super.key,
    required this.stone,
    required this.moodConfig,
  });

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _buildContent(context),
        if (stone.tags.isNotEmpty) ...[
          const SizedBox(height: 16),
          _buildTags(context),
        ],
      ],
    );
  }

  Widget _buildContent(BuildContext context) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surfaceContainerHighest,
        borderRadius: BorderRadius.circular(12),
      ),
      child: Text(
        stone.content,
        style: Theme.of(context).textTheme.bodyLarge?.copyWith(
              color: moodConfig.textColor,
              height: 1.6,
              letterSpacing: 0.3,
              fontSize: 15,
            ),
        maxLines: 6,
        overflow: TextOverflow.ellipsis,
      ),
    );
  }

  Widget _buildTags(BuildContext context) {
    return Wrap(
      spacing: 8,
      runSpacing: 8,
      children: stone.tags.map((tag) {
        return Container(
          padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
          decoration: BoxDecoration(
            color: Theme.of(context).colorScheme.secondaryContainer,
            borderRadius: BorderRadius.circular(8),
          ),
          child: Text(
            '# $tag',
            style: TextStyle(
              fontSize: 11,
              color: Theme.of(context).colorScheme.onSecondaryContainer,
              fontWeight: FontWeight.w500,
            ),
          ),
        );
      }).toList(),
    );
  }
}
