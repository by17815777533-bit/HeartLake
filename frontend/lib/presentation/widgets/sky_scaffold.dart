// @file sky_scaffold.dart
// @brief 光遇风格统一页面容器 - 分层背景 + 云层 + 粒子 + 水波纹

import 'package:flutter/material.dart';
import 'dart:math' as math;
import 'journey_effects/glow_particles.dart';
import 'water_background.dart';
import 'atmospheric_background.dart';

class SkyScaffold extends StatelessWidget {
  final Widget body;
  final bool showParticles;
  final bool showWater;
  final bool showClouds;
  final int particleCount;
  final PreferredSizeWidget? appBar;
  final Widget? bottomNavigationBar;
  final Widget? floatingActionButton;
  final bool extendBody;
  final bool extendBodyBehindAppBar;

  const SkyScaffold({
    super.key,
    required this.body,
    this.showParticles = true,
    this.showWater = false,
    this.showClouds = false,
    this.particleCount = 15,
    this.appBar,
    this.bottomNavigationBar,
    this.floatingActionButton,
    this.extendBody = true,
    this.extendBodyBehindAppBar = true,
  });

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBody: extendBody,
      extendBodyBehindAppBar: extendBodyBehindAppBar,
      appBar: appBar,
      floatingActionButton: floatingActionButton,
      bottomNavigationBar: bottomNavigationBar,
      body: Stack(
        children: [
          // Layer 1: 时间感知渐变背景
          Positioned.fill(
            child: AtmosphericBackground(
              enableParticles: false,
              child: const SizedBox.expand(),
            ),
          ),
          // Layer 1.5: 云层漂移（可选）
          if (showClouds)
            const Positioned.fill(
              child: IgnorePointer(child: _CloudLayer()),
            ),
          // Layer 2: 水波纹（可选）
          if (showWater)
            const Positioned.fill(child: WaterBackground()),
          // Layer 3: 柔光粒子（可选）
          if (showParticles)
            Positioned.fill(
              child: IgnorePointer(
                child: GlowParticles(particleCount: particleCount),
              ),
            ),
          // Layer 4: 内容
          Positioned.fill(child: body),
        ],
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// 云层动画组件
// ---------------------------------------------------------------------------

class _CloudLayer extends StatefulWidget {
  const _CloudLayer();

  @override
  State<_CloudLayer> createState() => _CloudLayerState();
}

class _CloudLayerState extends State<_CloudLayer>
    with SingleTickerProviderStateMixin {
  late AnimationController _ctrl;

  @override
  void initState() {
    super.initState();
    _ctrl = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 60),
    )..repeat();
  }

  @override
  void dispose() {
    _ctrl.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _ctrl,
      builder: (_, __) => CustomPaint(
        painter: _CloudPainter(progress: _ctrl.value),
        size: Size.infinite,
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// 云朵数据
// ---------------------------------------------------------------------------

class _CloudData {
  final double cx; // 初始中心 x（0~1 归一化）
  final double cy; // 中心 y（0~1 归一化）
  final double rx; // 椭圆半径 x
  final double ry; // 椭圆半径 y
  final double speed; // 漂移速度倍率
  final double alpha; // 透明度
  final double blur; // 模糊半径

  const _CloudData({
    required this.cx,
    required this.cy,
    required this.rx,
    required this.ry,
    required this.speed,
    required this.alpha,
    required this.blur,
  });
}

// ---------------------------------------------------------------------------
// 云朵绘制器
// ---------------------------------------------------------------------------

class _CloudPainter extends CustomPainter {
  final double progress;

  _CloudPainter({required this.progress});

  // 预生成 7 朵云的参数（前景大+快+透明，背景小+慢+不透明）
  static final List<_CloudData> _clouds = _generateClouds();

  static List<_CloudData> _generateClouds() {
    const int count = 7;
    final rng = math.Random(42);
    final List<_CloudData> list = [];
    for (int i = 0; i < count; i++) {
      // 深度 0=最远背景, 1=最近前景
      final depth = i / (count - 1);
      list.add(_CloudData(
        cx: rng.nextDouble(),
        cy: 0.08 + rng.nextDouble() * 0.35, // 云在上半部分
        rx: 60 + depth * 80 + rng.nextDouble() * 40, // 前景更大
        ry: 18 + depth * 20 + rng.nextDouble() * 10,
        speed: 0.3 + depth * 0.7, // 前景更快
        alpha: 0.03 + (1 - depth) * 0.05, // 背景更不透明
        blur: 30 + depth * 30, // 前景更模糊
      ));
    }
    return list;
  }

  @override
  void paint(Canvas canvas, Size size) {
    for (final c in _clouds) {
      // 水平漂移：progress * speed 控制一个完整周期的位移
      final dx = (c.cx + progress * c.speed) % 1.0;
      final x = dx * (size.width + c.rx * 2) - c.rx; // 允许从左侧滑入
      final y = c.cy * size.height;

      final paint = Paint()
        ..color = Color.fromRGBO(255, 255, 255, c.alpha)
        ..maskFilter = MaskFilter.blur(BlurStyle.normal, c.blur);

      canvas.drawOval(
        Rect.fromCenter(center: Offset(x, y), width: c.rx * 2, height: c.ry * 2),
        paint,
      );
    }
  }

  @override
  bool shouldRepaint(_CloudPainter oldDelegate) {
    return oldDelegate.progress != progress;
  }
}