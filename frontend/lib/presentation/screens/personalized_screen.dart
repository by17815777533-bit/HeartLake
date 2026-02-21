// @file personalized_screen.dart
// @brief 个性化推荐 - 光遇风格星光推荐流
import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../../domain/entities/stone.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../utils/mood_colors.dart';
import '../widgets/water_background.dart';
import 'stone_detail_screen.dart';

class PersonalizedScreen extends StatefulWidget {
  const PersonalizedScreen({super.key});

  @override
  State<PersonalizedScreen> createState() => _PersonalizedScreenState();
}

class _PersonalizedScreenState extends State<PersonalizedScreen>
    with TickerProviderStateMixin {
  final AIRecommendationService _service = AIRecommendationService();
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

  List<Stone> _parseStones(List<Map<String, dynamic>> list) {
    return list.take(10).map((e) => Stone.fromJson(e)).toList();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          const WaterBackground(forceDark: true),
          // 漂浮光点
          AnimatedBuilder(
            animation: _driftController,
            builder: (_, __) => CustomPaint(
              painter: _FloatingLightsPainter(
                progress: _driftController.value,
              ),
              size: Size.infinite,
            ),
          ),
          SafeArea(
            child: Column(
              children: [
                _buildHeader(),
                Expanded(
                  child: _loading
                      ? _buildLoadingState()
                      : FadeTransition(
                          opacity: _fadeController,
                          child: _buildContent(),
                        ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildHeader() {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Row(
        children: [
          IconButton(
            icon: const Icon(Icons.arrow_back_ios, color: Colors.white70, size: 20),
            onPressed: () => Navigator.pop(context),
          ),
          const Expanded(
            child: Text(
              '为你而来',
              textAlign: TextAlign.center,
              style: TextStyle(
                fontSize: 18,
                fontWeight: FontWeight.w300,
                color: Colors.white,
                letterSpacing: 3,
              ),
            ),
          ),
          IconButton(
            icon: const Icon(Icons.refresh, color: Colors.white38, size: 20),
            onPressed: () {
              setState(() => _loading = true);
              _loadRecommendations();
            },
          ),
        ],
      ),
    );
  }

  Widget _buildLoadingState() {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          AnimatedBuilder(
            animation: _driftController,
            builder: (_, __) {
              final scale = 1.0 + math.sin(_driftController.value * math.pi * 2) * 0.1;
              return Transform.scale(
                scale: scale,
                child: Container(
                  width: 60,
                  height: 60,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    gradient: RadialGradient(
                      colors: [
                        const Color(0xFFFFD54F).withValues(alpha: 0.4),
                        const Color(0xFFFFD54F).withValues(alpha: 0.0),
                      ],
                    ),
                  ),
                  child: const Icon(Icons.auto_awesome, color: Color(0xFFFFD54F), size: 24),
                ),
              );
            },
          ),
          const SizedBox(height: 16),
          Text(
            '正在寻找共鸣...',
            style: TextStyle(
              color: Colors.white.withValues(alpha: 0.5),
              fontSize: 13,
              letterSpacing: 2,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildContent() {
    return ListView(
      physics: const BouncingScrollPhysics(),
      padding: const EdgeInsets.symmetric(horizontal: 16),
      children: [
        const SizedBox(height: 12),
        // 个性化推荐区
        if (_personalizedStones.isNotEmpty) ...[
          _buildSectionTitle('心灵共振', Icons.favorite_border, const Color(0xFFF48FB1)),
          const SizedBox(height: 12),
          ..._personalizedStones.asMap().entries.map(
            (e) => _buildDriftingCard(e.value, e.key),
          ),
        ],
        const SizedBox(height: 24),
        // 高级共鸣推荐区
        if (_advancedStones.isNotEmpty) ...[
          _buildSectionTitle('深层共鸣', Icons.auto_awesome, const Color(0xFFFFD54F)),
          const SizedBox(height: 12),
          ..._advancedStones.asMap().entries.map(
            (e) => _buildDriftingCard(e.value, e.key + 100),
          ),
        ],
        // 空状态
        if (_personalizedStones.isEmpty && _advancedStones.isEmpty)
          _buildEmptyState(),
        const SizedBox(height: 40),
      ],
    );
  }

  Widget _buildSectionTitle(String title, IconData icon, Color color) {
    return Row(
      children: [
        Icon(icon, size: 16, color: color.withValues(alpha: 0.8)),
        const SizedBox(width: 8),
        Text(
          title,
          style: TextStyle(
            fontSize: 15,
            fontWeight: FontWeight.w300,
            color: Colors.white.withValues(alpha: 0.8),
            letterSpacing: 2,
          ),
        ),
      ],
    );
  }

  Widget _buildDriftingCard(Stone stone, int index) {
    final mood = stone.moodType != null
        ? MoodColors.fromString(stone.moodType)
        : MoodType.neutral;
    final config = MoodColors.getConfig(mood);

    return AnimatedBuilder(
      animation: _driftController,
      builder: (_, __) {
        final phase = index * 0.3;
        final dx = math.sin(_driftController.value * math.pi * 2 + phase) * 3;
        final dy = math.cos(_driftController.value * math.pi * 2 + phase * 0.7) * 2;

        return Transform.translate(
          offset: Offset(dx, dy),
          child: GestureDetector(
            onTap: () {
              // 记录交互
              _service.trackInteraction(stone.stoneId, 'click');
              Navigator.push(
                context,
                MaterialPageRoute(
                  builder: (_) => StoneDetailScreen(stone: stone),
                ),
              );
            },
            child: Container(
              margin: const EdgeInsets.only(bottom: 12),
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Colors.white.withValues(alpha: 0.06),
                borderRadius: BorderRadius.circular(20),
                border: Border.all(
                  color: config.primary.withValues(alpha: 0.15),
                ),
                boxShadow: [
                  BoxShadow(
                    color: config.primary.withValues(alpha: 0.06),
                    blurRadius: 20,
                  ),
                ],
              ),
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
                      child: Text(config.emoji, style: const TextStyle(fontSize: 18)),
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
                            color: Colors.white.withValues(alpha: 0.85),
                            height: 1.5,
                          ),
                        ),
                        const SizedBox(height: 6),
                        Row(
                          children: [
                            Icon(Icons.water_drop, size: 12,
                                color: Colors.white.withValues(alpha: 0.3)),
                            const SizedBox(width: 4),
                            Text(
                              '${stone.rippleCount}涟漪',
                              style: TextStyle(
                                fontSize: 11,
                                color: Colors.white.withValues(alpha: 0.4),
                              ),
                            ),
                            const SizedBox(width: 12),
                            Icon(Icons.sailing, size: 12,
                                color: Colors.white.withValues(alpha: 0.3)),
                            const SizedBox(width: 4),
                            Text(
                              '${stone.boatCount}纸船',
                              style: TextStyle(
                                fontSize: 11,
                                color: Colors.white.withValues(alpha: 0.4),
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
                    color: Colors.white.withValues(alpha: 0.2),
                  ),
                ],
              ),
            ),
          ),
        );
      },
    );
  }

  Widget _buildEmptyState() {
    return Padding(
      padding: const EdgeInsets.only(top: 80),
      child: Column(
        children: [
          Icon(Icons.explore, size: 48, color: Colors.white.withValues(alpha: 0.2)),
          const SizedBox(height: 16),
          Text(
            '多投几颗石头\n让湖水认识你',
            textAlign: TextAlign.center,
            style: TextStyle(
              fontSize: 14,
              color: Colors.white.withValues(alpha: 0.4),
              height: 1.8,
              letterSpacing: 1,
            ),
          ),
        ],
      ),
    );
  }
}

/// 漂浮光点画笔
class _FloatingLightsPainter extends CustomPainter {
  final double progress;
  _FloatingLightsPainter({required this.progress});

  @override
  void paint(Canvas canvas, Size size) {
    final rng = math.Random(42);
    for (int i = 0; i < 20; i++) {
      final baseX = rng.nextDouble() * size.width;
      final baseY = rng.nextDouble() * size.height;
      final phase = rng.nextDouble() * math.pi * 2;
      final speed = 0.3 + rng.nextDouble() * 0.7;
      final radius = 1.0 + rng.nextDouble() * 2.0;

      final x = baseX + math.sin(progress * math.pi * 2 * speed + phase) * 20;
      final y = baseY + math.cos(progress * math.pi * 2 * speed * 0.7 + phase) * 15;
      final alpha = (0.2 + math.sin(progress * math.pi * 2 + phase) * 0.15).clamp(0.05, 0.4);

      canvas.drawCircle(
        Offset(x, y),
        radius,
        Paint()
          ..color = const Color(0xFFFFD54F).withValues(alpha: alpha)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 3),
      );
    }
  }

  @override
  bool shouldRepaint(_FloatingLightsPainter old) => old.progress != progress;
}
