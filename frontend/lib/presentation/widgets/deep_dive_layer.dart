// 3D深潜空间 - 垂直分层渲染

import 'dart:ui' as ui;
import 'package:flutter/material.dart';
import '../../domain/entities/stone.dart';

/// 深度层级
enum DepthLayer { shallow, middle, deep }

/// 根据石头情感分数获取深度层级
DepthLayer getDepthLayer(Stone stone) {
  final score = stone.sentimentScore ?? 0.0;
  if (score > 0.2) return DepthLayer.shallow;
  if (score > -0.2) return DepthLayer.middle;
  return DepthLayer.deep;
}

/// 3D深潜空间容器
class DeepDiveSpace extends StatefulWidget {
  final List<Stone> stones;
  final Widget Function(Stone stone, DepthLayer layer) itemBuilder;

  const DeepDiveSpace({
    super.key,
    required this.stones,
    required this.itemBuilder,
  });

  @override
  State<DeepDiveSpace> createState() => _DeepDiveSpaceState();
}

class _DeepDiveSpaceState extends State<DeepDiveSpace> {
  final ScrollController _controller = ScrollController();
  double _scrollProgress = 0.0;

  @override
  void initState() {
    super.initState();
    _controller.addListener(_onScroll);
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  void _onScroll() {
    if (!_controller.hasClients) return;
    final max = _controller.position.maxScrollExtent;
    if (max > 0) {
      setState(() => _scrollProgress = (_controller.offset / max).clamp(0.0, 1.0));
    }
  }

  @override
  Widget build(BuildContext context) {
    // 按深度分组
    final shallow = <Stone>[];
    final middle = <Stone>[];
    final deep = <Stone>[];

    for (final s in widget.stones) {
      switch (getDepthLayer(s)) {
        case DepthLayer.shallow:
          shallow.add(s);
        case DepthLayer.middle:
          middle.add(s);
        case DepthLayer.deep:
          deep.add(s);
      }
    }

    final all = [...shallow, ...middle, ...deep];

    return Stack(
      children: [
        // 深海雾化背景
        Positioned.fill(
          child: CustomPaint(
            painter: _DeepSeaFogPainter(progress: _scrollProgress),
          ),
        ),
        // 石头列表
        ListView.builder(
          controller: _controller,
          padding: const EdgeInsets.all(16),
          itemCount: all.length,
          itemBuilder: (ctx, i) {
            final stone = all[i];
            final layer = getDepthLayer(stone);
            return _DepthLayerWrapper(
              layer: layer,
              child: widget.itemBuilder(stone, layer),
            );
          },
        ),
      ],
    );
  }
}

/// 深度层包装器 - 添加视觉效果
class _DepthLayerWrapper extends StatelessWidget {
  final DepthLayer layer;
  final Widget child;

  const _DepthLayerWrapper({required this.layer, required this.child});

  @override
  Widget build(BuildContext context) {
    // 深层添加模糊和暗化效果
    final blur = layer == DepthLayer.deep ? 1.0 : 0.0;
    final opacity = layer == DepthLayer.deep ? 0.85 : 1.0;

    Widget result = Opacity(opacity: opacity, child: child);

    if (blur > 0) {
      result = ImageFiltered(
        imageFilter: ui.ImageFilter.blur(sigmaX: blur, sigmaY: blur),
        child: result,
      );
    }

    // 添加深度指示器
    return Padding(
      padding: const EdgeInsets.only(bottom: 12),
      child: Row(
        children: [
          _DepthIndicator(layer: layer),
          const SizedBox(width: 8),
          Expanded(child: result),
        ],
      ),
    );
  }
}

/// 深度指示器
class _DepthIndicator extends StatelessWidget {
  final DepthLayer layer;

  const _DepthIndicator({required this.layer});

  @override
  Widget build(BuildContext context) {
    final (color, icon) = switch (layer) {
      DepthLayer.shallow => (const Color(0xFFFFEB3B), Icons.wb_sunny),
      DepthLayer.middle => (const Color(0xFF64B5F6), Icons.water),
      DepthLayer.deep => (const Color(0xFF37474F), Icons.waves),
    };

    return Container(
      width: 24,
      height: 24,
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.3),
        shape: BoxShape.circle,
      ),
      child: Icon(icon, size: 14, color: color),
    );
  }
}

/// 深海雾化绘制器 - 温馨渐变
class _DeepSeaFogPainter extends CustomPainter {
  final double progress;

  _DeepSeaFogPainter({required this.progress});

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;

    // 温馨的湖水渐变：从浅蓝到柔和的深蓝绿
    final gradient = ui.Gradient.linear(
      const Offset(0, 0),
      Offset(0, size.height),
      [
        Color.lerp(const Color(0xFFE0F7FA), const Color(0xFFB2EBF2), progress)!, // 浅层：清澈
        Color.lerp(const Color(0xFF80DEEA), const Color(0xFF4DD0E1), progress)!, // 中层：温柔青
        Color.lerp(const Color(0xFF26C6DA), const Color(0xFF00ACC1), progress)!, // 深层：宁静蓝绿
      ],
      [0.0, 0.5, 1.0],
    );
    canvas.drawRect(rect, Paint()..shader = gradient);

    // 光斑效果 - 模拟阳光透过水面
    _drawLightSpots(canvas, size);

    // 气泡装饰
    _drawBubbles(canvas, size);
  }

  void _drawLightSpots(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = Colors.white.withValues(alpha: 0.15 - progress * 0.1)
      ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 30);

    // 几个柔和的光斑
    canvas.drawCircle(Offset(size.width * 0.2, size.height * 0.15), 60, paint);
    canvas.drawCircle(Offset(size.width * 0.7, size.height * 0.25), 45, paint);
    canvas.drawCircle(Offset(size.width * 0.5, size.height * 0.4), 35, paint);
  }

  void _drawBubbles(Canvas canvas, Size size) {
    final bubblePaint = Paint()
      ..color = Colors.white.withValues(alpha: 0.2)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.5;

    // 散落的小气泡
    final bubbles = [
      Offset(size.width * 0.15, size.height * 0.6),
      Offset(size.width * 0.3, size.height * 0.75),
      Offset(size.width * 0.8, size.height * 0.5),
      Offset(size.width * 0.65, size.height * 0.85),
      Offset(size.width * 0.9, size.height * 0.7),
    ];

    for (var i = 0; i < bubbles.length; i++) {
      final radius = 4.0 + (i % 3) * 3;
      canvas.drawCircle(bubbles[i], radius, bubblePaint);
    }
  }

  @override
  bool shouldRepaint(_DeepSeaFogPainter old) => old.progress != progress;
}
