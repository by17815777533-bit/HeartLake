// 个性化推荐页面
//
// 基于协同过滤和情绪匹配的个性化石头推荐。
import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../../domain/entities/stone.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../di/service_locator.dart';
import '../../utils/mood_colors.dart';
import '../../utils/app_theme.dart';
import '../widgets/water_background.dart';
import 'stone_detail_screen.dart';

/// 个性化推荐页面
///
/// 基于 AI 推荐引擎为用户量身定制的石头推荐列表，
/// 融合协同过滤、内容推荐和情绪轨迹 DTW 匹配算法。
/// 点击石头会上报交互事件，用于推荐模型的在线学习。
class PersonalizedScreen extends StatefulWidget {
  const PersonalizedScreen({super.key});
  @override
  State<PersonalizedScreen> createState() => _PersonalizedScreenState();
}

/// 个性化推荐页面状态管理
///
/// 使用两个 AnimationController：
/// - _driftController: 驱动卡片微浮动效果（循环播放）
/// - _fadeController: 数据加载完成后的淡入过渡
class _PersonalizedScreenState extends State<PersonalizedScreen>
    with TickerProviderStateMixin {
  final AIRecommendationService _service = sl<AIRecommendationService>();
  late AnimationController _driftController;
  late AnimationController _fadeController;
  List<Stone> _personalizedStones = [];
  List<Stone> _advancedStones = [];
  bool _loading = true;

  @override
  void initState() {
    super.initState();
    _driftController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 8),
    )..repeat();
    _fadeController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 600),
    );
    _loadRecommendations();
  }

  @override
  void dispose() {
    _driftController.dispose();
    _fadeController.dispose();
    super.dispose();
  }

  /// 并行请求个性化推荐和深层共鸣推荐
  Future<void> _loadRecommendations() async {
    try {
      final results = await Future.wait([
        _service.getPersonalizedRecommendations(limit: 10),
        _service.getAdvancedRecommendations(limit: 10),
      ]);
      if (mounted) {
        setState(() {
          _personalizedStones = _parseStones(results[0]);
          _advancedStones = _parseStones(results[1]);
          _loading = false;
        });
        _fadeController.forward();
      }
    } catch (_) {
      if (mounted) setState(() => _loading = false);
    }
  }

  /// 将后端 JSON 列表解析为 Stone 实体，最多取 10 条
  List<Stone> _parseStones(List<Map<String, dynamic>> list) {
    return list.take(10).map((e) => Stone.fromJson(e)).toList();
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor:
            isDark ? AppTheme.darkTextPrimary : AppTheme.textPrimary,
        title: const Text(
          '为你而来',
          style: TextStyle(
            fontSize: 18,
            fontWeight: FontWeight.w500,
            letterSpacing: 1,
          ),
        ),
        centerTitle: true,
        actions: [
          IconButton(
            icon: Icon(Icons.refresh,
                color: (isDark
                        ? AppTheme.darkTextSecondary
                        : AppTheme.textSecondary)
                    .withValues(alpha: 0.6),
                size: 20),
            onPressed: () {
              setState(() => _loading = true);
              _loadRecommendations();
            },
          ),
        ],
      ),
      body: Stack(
        children: [
          const WaterBackground(),
          SafeArea(
            child: _loading
                ? _buildLoadingState()
                : FadeTransition(
                    opacity: _fadeController,
                    child: _buildContent(),
                  ),
          ),
        ],
      ),
    );
  }

  /// 加载中的呼吸动画占位
  Widget _buildLoadingState() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          AnimatedBuilder(
            animation: _driftController,
            builder: (_, __) {
              final scale =
                  1.0 + math.sin(_driftController.value * math.pi * 2) * 0.1;
              return Transform.scale(
                scale: scale,
                child: Container(
                  width: 60,
                  height: 60,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    gradient: RadialGradient(
                      colors: [
                        AppTheme.primaryColor.withValues(alpha: 0.4),
                        AppTheme.primaryColor.withValues(alpha: 0.0),
                      ],
                    ),
                  ),
                  child: const Icon(Icons.auto_awesome,
                      color: AppTheme.primaryLightColor, size: 24),
                ),
              );
            },
          ),
          const SizedBox(height: 16),
          Text(
            '正在寻找共鸣...',
            style: TextStyle(
              color:
                  (isDark ? AppTheme.darkTextSecondary : AppTheme.textSecondary)
                      .withValues(alpha: 0.7),
              fontSize: 13,
              letterSpacing: 2,
            ),
          ),
        ],
      ),
    );
  }

  /// 推荐内容主体：心灵共振 + 深层共鸣两个分区
  Widget _buildContent() {
    return ListView(
      physics: const BouncingScrollPhysics(),
      padding: const EdgeInsets.symmetric(horizontal: 16),
      children: [
        const SizedBox(height: 12),
        if (_personalizedStones.isNotEmpty) ...[
          _buildSectionTitle(
              '心灵共振', Icons.favorite_border, AppTheme.primaryLightColor),
          const SizedBox(height: 12),
          ..._personalizedStones.asMap().entries.map(
                (e) => _buildDriftingCard(e.value, e.key),
              ),
        ],
        const SizedBox(height: 24),
        if (_advancedStones.isNotEmpty) ...[
          _buildSectionTitle(
              '深层共鸣', Icons.auto_awesome, AppTheme.primaryLightColor),
          const SizedBox(height: 12),
          ..._advancedStones.asMap().entries.map(
                (e) => _buildDriftingCard(e.value, e.key + 100),
              ),
        ],
        if (_personalizedStones.isEmpty && _advancedStones.isEmpty)
          _buildEmptyState(),
        const SizedBox(height: 40),
      ],
    );
  }

  Widget _buildSectionTitle(String title, IconData icon, Color color) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Row(
      children: [
        Icon(icon, size: 16, color: color.withValues(alpha: 0.8)),
        const SizedBox(width: 8),
        Text(
          title,
          style: TextStyle(
            fontSize: 15,
            fontWeight: FontWeight.w300,
            color: (isDark ? AppTheme.darkTextPrimary : AppTheme.textPrimary)
                .withValues(alpha: 0.8),
            letterSpacing: 2,
          ),
        ),
      ],
    );
  }

  /// 带微浮动动画的石头推荐卡片，点击时上报交互事件
  Widget _buildDriftingCard(Stone stone, int index) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final mood = stone.moodType != null
        ? MoodColors.fromString(stone.moodType)
        : MoodType.neutral;
    final config = MoodColors.getConfig(mood);

    return AnimatedBuilder(
      animation: _driftController,
      builder: (_, __) {
        final phase = index * 0.3;
        final dx = math.sin(_driftController.value * math.pi * 2 + phase) * 3;
        final dy =
            math.cos(_driftController.value * math.pi * 2 + phase * 0.7) * 2;

        return Transform.translate(
          offset: Offset(dx, dy),
          child: GestureDetector(
            onTap: () {
              _service.trackInteraction(
                  stoneId: stone.stoneId, interactionType: 'click');
              Navigator.push(
                context,
                MaterialPageRoute(
                    builder: (_) => StoneDetailScreen(stone: stone)),
              );
            },
            child: Padding(
              padding: const EdgeInsets.only(bottom: 12),
              child: Card(
                shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(20)),
                elevation: 1,
                child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: Row(
                    children: [
                      // 情绪光点
                      Container(
                        width: 40,
                        height: 40,
                        decoration: BoxDecoration(
                          shape: BoxShape.circle,
                          gradient: RadialGradient(
                            colors: [
                              config.primary.withValues(alpha: 0.5),
                              config.primary.withValues(alpha: 0.1),
                            ],
                          ),
                        ),
                        child: Center(
                          child: Icon(config.icon,
                              size: 18, color: config.primary),
                        ),
                      ),
                      const SizedBox(width: 14),
                      // 内容
                      Expanded(
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Text(
                              stone.content.length > 50
                                  ? '${stone.content.substring(0, 50)}...'
                                  : stone.content,
                              style: TextStyle(
                                fontSize: 14,
                                color: (isDark
                                        ? AppTheme.darkTextPrimary
                                        : AppTheme.textPrimary)
                                    .withValues(alpha: 0.85),
                                height: 1.5,
                              ),
                            ),
                            const SizedBox(height: 6),
                            Row(
                              children: [
                                Icon(Icons.water_drop,
                                    size: 12,
                                    color: (isDark
                                            ? AppTheme.darkTextSecondary
                                            : AppTheme.textSecondary)
                                        .withValues(alpha: 0.5)),
                                const SizedBox(width: 4),
                                Text(
                                  '${stone.rippleCount}涟漪',
                                  style: TextStyle(
                                    fontSize: 11,
                                    color: (isDark
                                            ? AppTheme.darkTextSecondary
                                            : AppTheme.textSecondary)
                                        .withValues(alpha: 0.6),
                                  ),
                                ),
                                const SizedBox(width: 12),
                                Icon(Icons.sailing,
                                    size: 12,
                                    color: (isDark
                                            ? AppTheme.darkTextSecondary
                                            : AppTheme.textSecondary)
                                        .withValues(alpha: 0.5)),
                                const SizedBox(width: 4),
                                Text(
                                  '${stone.boatCount}纸船',
                                  style: TextStyle(
                                    fontSize: 11,
                                    color: (isDark
                                            ? AppTheme.darkTextSecondary
                                            : AppTheme.textSecondary)
                                        .withValues(alpha: 0.6),
                                  ),
                                ),
                              ],
                            ),
                          ],
                        ),
                      ),
                      // 共鸣指示
                      Icon(
                        Icons.arrow_forward_ios,
                        size: 14,
                        color: (isDark
                                ? AppTheme.darkTextSecondary
                                : AppTheme.textSecondary)
                            .withValues(alpha: 0.3),
                      ),
                    ],
                  ),
                ),
              ),
            ),
          ),
        );
      },
    );
  }

  /// 推荐列表为空时的引导提示
  Widget _buildEmptyState() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Card(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      elevation: 0,
      child: Padding(
        padding: const EdgeInsets.symmetric(vertical: 60, horizontal: 20),
        child: Column(
          children: [
            Icon(Icons.explore,
                size: 48,
                color:
                    (isDark ? AppTheme.darkTextPrimary : AppTheme.textPrimary)
                        .withValues(alpha: 0.3)),
            const SizedBox(height: 16),
            Text(
              '投出更多石头，收获更多共鸣',
              style: TextStyle(
                color: (isDark
                        ? AppTheme.darkTextSecondary
                        : AppTheme.textSecondary)
                    .withValues(alpha: 0.7),
                fontSize: 14,
                letterSpacing: 1,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
