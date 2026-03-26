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
  String? _personalizedErrorMessage;
  String? _advancedErrorMessage;

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

  void _reportUiError(
    Object error,
    StackTrace stackTrace,
    String context,
  ) {
    FlutterError.reportError(
      FlutterErrorDetails(
        exception: error,
        stack: stackTrace,
        library: 'heartlake',
        context: ErrorDescription(context),
      ),
    );
  }

  void _showMessage(String message) {
    final messenger = ScaffoldMessenger.maybeOf(context);
    if (messenger == null) return;
    messenger
      ..hideCurrentSnackBar()
      ..showSnackBar(SnackBar(content: Text(message)));
  }

  String _resolveErrorMessage(Object error, String fallback) {
    final message = error.toString().trim();
    if (message.isEmpty) return fallback;
    if (message.startsWith('Bad state: ')) {
      return message.substring('Bad state: '.length).trim();
    }
    if (message.startsWith('Exception: ')) {
      return message.substring('Exception: '.length).trim();
    }
    return message;
  }

  Future<_RecommendationSectionLoadResult> _loadSection(
    Future<List<Map<String, dynamic>>> Function() request,
    String context,
    String fallbackMessage,
  ) async {
    try {
      final payload = await request();
      return _RecommendationSectionLoadResult.success(_parseStones(payload));
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, context);
      return _RecommendationSectionLoadResult.failure(
        _resolveErrorMessage(error, fallbackMessage),
      );
    }
  }

  /// 并行请求个性化推荐和深层共鸣推荐
  Future<void> _loadRecommendations({bool showFeedback = false}) async {
    final hadVisibleData =
        _personalizedStones.isNotEmpty || _advancedStones.isNotEmpty;
    if (!hadVisibleData && mounted) {
      setState(() => _loading = true);
    }

    final results = await Future.wait<_RecommendationSectionLoadResult>([
      _loadSection(
        () => _service.getPersonalizedRecommendations(limit: 10),
        'PersonalizedScreen._loadRecommendations.personalized',
        '加载心灵共振失败',
      ),
      _loadSection(
        () => _service.getAdvancedRecommendations(limit: 10),
        'PersonalizedScreen._loadRecommendations.advanced',
        '加载深层共鸣失败',
      ),
    ]);

    if (!mounted) return;

    final personalized = results[0];
    final advanced = results[1];
    final hasFreshData = personalized.items != null || advanced.items != null;

    setState(() {
      if (personalized.items != null) {
        _personalizedStones = personalized.items!;
      }
      if (advanced.items != null) {
        _advancedStones = advanced.items!;
      }
      _personalizedErrorMessage = personalized.errorMessage;
      _advancedErrorMessage = advanced.errorMessage;
      _loading = false;
    });

    if (hasFreshData) {
      _fadeController.forward(from: 0);
    }

    if (!showFeedback) return;

    final failureCount =
        results.where((item) => item.errorMessage != null).length;
    if (failureCount == 2) {
      _showMessage('推荐刷新失败，请稍后再试');
      return;
    }
    if (failureCount == 1) {
      _showMessage('部分推荐未更新，已保留上次结果');
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
              _loadRecommendations(showFeedback: true);
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
    final warningMessages = [
      _personalizedErrorMessage,
      _advancedErrorMessage,
    ].whereType<String>().toList();

    return ListView(
      physics: const BouncingScrollPhysics(),
      padding: const EdgeInsets.symmetric(horizontal: 16),
      children: [
        const SizedBox(height: 12),
        if (warningMessages.isNotEmpty) ...[
          _buildWarningCard(
            title: warningMessages.length == 2 ? '推荐暂时未完全更新' : '部分推荐未更新',
            message: warningMessages.join('；'),
          ),
          const SizedBox(height: 16),
        ],
        if (_personalizedStones.isNotEmpty) ...[
          _buildSectionTitle(
              '心灵共振', Icons.favorite_border, AppTheme.primaryLightColor),
          if (_personalizedErrorMessage != null) ...[
            const SizedBox(height: 8),
            _buildWarningCard(
              title: '心灵共振未完成刷新',
              message: _personalizedErrorMessage!,
              compact: true,
            ),
          ],
          const SizedBox(height: 12),
          ..._personalizedStones.asMap().entries.map(
                (e) => _buildDriftingCard(e.value, e.key),
              ),
        ],
        const SizedBox(height: 24),
        if (_advancedStones.isNotEmpty) ...[
          _buildSectionTitle(
              '深层共鸣', Icons.auto_awesome, AppTheme.primaryLightColor),
          if (_advancedErrorMessage != null) ...[
            const SizedBox(height: 8),
            _buildWarningCard(
              title: '深层共鸣未完成刷新',
              message: _advancedErrorMessage!,
              compact: true,
            ),
          ],
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

  Widget _buildWarningCard({
    required String title,
    required String message,
    bool compact = false,
  }) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final background = isDark
        ? const Color(0xFF2B2112).withValues(alpha: 0.92)
        : const Color(0xFFFFF4DE);
    final border = const Color(0xFFD39D2A).withValues(alpha: 0.32);
    final titleColor =
        isDark ? const Color(0xFFFFD27A) : const Color(0xFF8A5A00);
    final textColor =
        isDark ? AppTheme.darkTextSecondary : AppTheme.textSecondary;

    return Container(
      padding: EdgeInsets.symmetric(
        horizontal: compact ? 12 : 14,
        vertical: compact ? 10 : 12,
      ),
      decoration: BoxDecoration(
        color: background,
        borderRadius: BorderRadius.circular(18),
        border: Border.all(color: border),
      ),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Icon(Icons.warning_amber_rounded,
              size: compact ? 18 : 20, color: titleColor),
          const SizedBox(width: 10),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  style: TextStyle(
                    fontSize: compact ? 12 : 13,
                    fontWeight: FontWeight.w600,
                    color: titleColor,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  message,
                  style: TextStyle(
                    fontSize: compact ? 11 : 12,
                    height: 1.45,
                    color: textColor.withValues(alpha: 0.88),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
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

  String _algorithmLabel(Stone stone) {
    switch (stone.recommendationAlgorithm) {
      case 'emotion_temporal_resonance':
        return '四维共鸣';
      case 'emotion_resonance_hybrid':
        return '混合共鸣';
      case 'user_cf':
        return '同频旅人';
      case 'item_cf':
        return '相似心声';
      case 'content_based':
        return '情绪匹配';
      case 'ucb_explore':
        return '探索发现';
      case 'graph_walk':
        return '关系扩散';
      case 'multi_armed_bandit_mmr':
        return '多路融合';
      default:
        return '为你而来';
    }
  }

  String? _scoreBreakdown(Stone stone) {
    final parts = <String>[];
    if (stone.semanticScore != null) {
      parts.add('语义 ${(stone.semanticScore! * 100).round()}');
    }
    if (stone.trajectoryScore != null) {
      parts.add('轨迹 ${(stone.trajectoryScore! * 100).round()}');
    }
    if (stone.temporalScore != null) {
      parts.add('时序 ${(stone.temporalScore! * 100).round()}');
    }
    if (stone.diversityScore != null) {
      parts.add('多样 ${(stone.diversityScore! * 100).round()}');
    }
    if (parts.isEmpty) return null;
    return parts.join(' · ');
  }

  String _formatRecommendationScore(double score) {
    final normalized = score > 1 ? score : score * 100;
    return '${normalized.round()}分';
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
                            Wrap(
                              spacing: 8,
                              runSpacing: 6,
                              children: [
                                _InfoChip(label: _algorithmLabel(stone)),
                                if (stone.recommendationScore != null)
                                  _InfoChip(
                                    label: _formatRecommendationScore(
                                      stone.recommendationScore!,
                                    ),
                                  ),
                              ],
                            ),
                            const SizedBox(height: 8),
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
                            if ((stone.recommendationReason ?? '')
                                .isNotEmpty) ...[
                              const SizedBox(height: 6),
                              Text(
                                stone.recommendationReason!,
                                style: TextStyle(
                                  fontSize: 11,
                                  color: config.primary.withValues(alpha: 0.9),
                                  height: 1.4,
                                ),
                                maxLines: 2,
                                overflow: TextOverflow.ellipsis,
                              ),
                            ],
                            if (_scoreBreakdown(stone) != null) ...[
                              const SizedBox(height: 6),
                              Text(
                                _scoreBreakdown(stone)!,
                                style: TextStyle(
                                  fontSize: 10,
                                  color: (isDark
                                          ? AppTheme.darkTextSecondary
                                          : AppTheme.textSecondary)
                                      .withValues(alpha: 0.68),
                                ),
                              ),
                            ],
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
    final hasError =
        _personalizedErrorMessage != null || _advancedErrorMessage != null;
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
              hasError ? '推荐加载失败' : '投出更多石头，收获更多共鸣',
              style: TextStyle(
                color: (isDark
                        ? AppTheme.darkTextSecondary
                        : AppTheme.textSecondary)
                    .withValues(alpha: 0.7),
                fontSize: 14,
                letterSpacing: 1,
              ),
            ),
            if (hasError) ...[
              const SizedBox(height: 10),
              Text(
                [
                  _personalizedErrorMessage,
                  _advancedErrorMessage,
                ].whereType<String>().join('；'),
                textAlign: TextAlign.center,
                style: TextStyle(
                  color: (isDark
                          ? AppTheme.darkTextSecondary
                          : AppTheme.textSecondary)
                      .withValues(alpha: 0.72),
                  fontSize: 12,
                  height: 1.5,
                ),
              ),
              const SizedBox(height: 20),
              OutlinedButton(
                onPressed: () => _loadRecommendations(showFeedback: true),
                child: const Text('重新加载'),
              ),
            ],
          ],
        ),
      ),
    );
  }
}

class _InfoChip extends StatelessWidget {
  final String label;

  const _InfoChip({required this.label});

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: isDark
            ? Colors.white.withValues(alpha: 0.08)
            : Colors.black.withValues(alpha: 0.05),
        borderRadius: BorderRadius.circular(999),
      ),
      child: Text(
        label,
        style: TextStyle(
          fontSize: 10,
          fontWeight: FontWeight.w600,
          color: isDark ? AppTheme.darkTextSecondary : AppTheme.textSecondary,
          letterSpacing: 0.4,
        ),
      ),
    );
  }
}

class _RecommendationSectionLoadResult {
  final List<Stone>? items;
  final String? errorMessage;

  const _RecommendationSectionLoadResult._({
    this.items,
    this.errorMessage,
  });

  factory _RecommendationSectionLoadResult.success(List<Stone> items) {
    return _RecommendationSectionLoadResult._(items: items);
  }

  factory _RecommendationSectionLoadResult.failure(String message) {
    return _RecommendationSectionLoadResult._(errorMessage: message);
  }
}
