// 相似石头推荐 - 光遇风格飘浮卡片
import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../../domain/entities/stone.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../di/service_locator.dart';
import '../../utils/mood_colors.dart';

/// 相似石头推荐区域 - 类光遇飘浮星光风格
class SimilarStonesSection extends StatefulWidget {
  final String stoneId;
  final Function(Stone)? onStoneTap;

  const SimilarStonesSection({
    super.key,
    required this.stoneId,
    this.onStoneTap,
  });

  @override
  State<SimilarStonesSection> createState() => _SimilarStonesSectionState();
}

class _SimilarStonesSectionState extends State<SimilarStonesSection>
    with SingleTickerProviderStateMixin {
  final AIRecommendationService _service = sl<AIRecommendationService>();
  List<Stone> _similarStones = [];
  bool _loading = true;
  late AnimationController _floatController;

  @override
  void initState() {
    super.initState();
    _floatController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 6),
    )..repeat();
    _loadSimilarStones();
  }

  @override
  void dispose() {
    _floatController.dispose();
    super.dispose();
  }

  Future<void> _loadSimilarStones() async {
    try {
      final list = await _service.getSimilarStones(widget.stoneId, limit: 6);
      if (mounted) {
        setState(() {
          _similarStones = list.map((e) => Stone.fromJson(e)).toList();
          _loading = false;
        });
      }
    } catch (_) {
      if (mounted) setState(() => _loading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    if (_loading) {
      return _buildShimmer();
    }
    if (_similarStones.isEmpty) return const SizedBox.shrink();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // 标题 - 星光连线风格
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
          child: Row(
            children: [
              AnimatedBuilder(
                animation: _floatController,
                builder: (_, __) => Icon(
                  Icons.auto_awesome,
                  size: 18,
                  color: Color.lerp(
                    const Color(0xFFFFD54F),
                    const Color(0xFFFFAB40),
                    (math.sin(_floatController.value * math.pi * 2) + 1) / 2,
                  ),
                ),
              ),
              const SizedBox(width: 8),
              const Text(
                '共鸣之石',
                style: TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.w600,
                  color: Color(0xFF5F6368),
                  letterSpacing: 1.2,
                ),
              ),
              const Spacer(),
              Text(
                '${_similarStones.length}颗',
                style: TextStyle(
                  fontSize: 12,
                  color: Colors.grey[400],
                ),
              ),
            ],
          ),
        ),
        // 横向滚动的飘浮卡片
        SizedBox(
          height: 160,
          child: ListView.builder(
            scrollDirection: Axis.horizontal,
            padding: const EdgeInsets.symmetric(horizontal: 16),
            itemCount: _similarStones.length,
            itemBuilder: (context, index) {
              return AnimatedBuilder(
                animation: _floatController,
                builder: (_, __) {
                  // 每张卡片有不同的飘浮相位
                  final phase = index * 0.4;
                  final floatY = math.sin(
                    _floatController.value * math.pi * 2 + phase,
                  ) * 4;
                  return Transform.translate(
                    offset: Offset(0, floatY),
                    child: _buildFloatingCard(_similarStones[index], index),
                  );
                },
              );
            },
          ),
        ),
      ],
    );
  }

  Widget _buildFloatingCard(Stone stone, int index) {
    final mood = stone.moodType != null
        ? MoodColors.fromString(stone.moodType)
        : MoodType.neutral;
    final config = MoodColors.getConfig(mood);

    return GestureDetector(
      onTap: () => widget.onStoneTap?.call(stone),
      child: Container(
        width: 130,
        margin: const EdgeInsets.symmetric(horizontal: 6, vertical: 8),
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
            colors: [
              config.primary.withValues(alpha: 0.12),
              config.primary.withValues(alpha: 0.04),
            ],
          ),
          borderRadius: BorderRadius.circular(20),
          border: Border.all(
            color: config.primary.withValues(alpha: 0.15),
            width: 1,
          ),
          boxShadow: [
            BoxShadow(
              color: config.primary.withValues(alpha: 0.08),
              blurRadius: 16,
              offset: const Offset(0, 4),
            ),
          ],
        ),
        child: Padding(
          padding: const EdgeInsets.all(14),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // 情绪光点
              Row(
                children: [
                  Container(
                    width: 8,
                    height: 8,
                    decoration: BoxDecoration(
                      color: config.primary,
                      shape: BoxShape.circle,
                      boxShadow: [
                        BoxShadow(
                          color: config.primary.withValues(alpha: 0.5),
                          blurRadius: 6,
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(width: 6),
                  Text(
                    config.name,
                    style: TextStyle(
                      fontSize: 10,
                      color: config.primary,
                      fontWeight: FontWeight.w500,
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 10),
              // 内容预览
              Expanded(
                child: Text(
                  stone.content,
                  maxLines: 4,
                  overflow: TextOverflow.ellipsis,
                  style: TextStyle(
                    fontSize: 12,
                    height: 1.5,
                    color: Colors.grey[700],
                  ),
                ),
              ),
              // 底部星光指示
              Row(
                mainAxisAlignment: MainAxisAlignment.end,
                children: [
                  Icon(
                    Icons.star_rounded,
                    size: 12,
                    color: Colors.amber.withValues(alpha: 0.6),
                  ),
                  const SizedBox(width: 2),
                  Text(
                    '${stone.rippleCount}',
                    style: TextStyle(
                      fontSize: 10,
                      color: Colors.grey[400],
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildShimmer() {
    return SizedBox(
      height: 160,
      child: ListView.builder(
        scrollDirection: Axis.horizontal,
        padding: const EdgeInsets.symmetric(horizontal: 16),
        itemCount: 3,
        itemBuilder: (_, __) => Container(
          width: 130,
          margin: const EdgeInsets.symmetric(horizontal: 6, vertical: 8),
          decoration: BoxDecoration(
            color: Colors.grey[100],
            borderRadius: BorderRadius.circular(20),
          ),
        ),
      ),
    );
  }
}
