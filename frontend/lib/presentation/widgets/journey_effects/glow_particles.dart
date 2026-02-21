import 'dart:math';
import 'package:flutter/material.dart';
import '../../../../utils/app_theme.dart';

class GlowParticles extends StatefulWidget {
  final int particleCount;
  const GlowParticles({super.key, this.particleCount = 30});

  @override
  State<GlowParticles> createState() => _GlowParticlesState();
}

class _GlowParticlesState extends State<GlowParticles>
    with SingleTickerProviderStateMixin {
  late final AnimationController _ctrl;
  late final List<_Particle> _particles;
  final _rng = Random();

  static const _colors = [
    AppTheme.warmOrange,
    AppTheme.peachPink,
    AppTheme.warmPink,
    AppTheme.purpleColor,
    AppTheme.skyBlue,
  ];

  @override
  void initState() {
    super.initState();
    _particles = List.generate(widget.particleCount, (_) => _randomParticle());
    _ctrl = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 1),
    )..repeat();
  }

  _Particle _randomParticle({bool fromBottom = false}) => _Particle(
        x: _rng.nextDouble(),
        y: fromBottom ? 1.0 + _rng.nextDouble() * 0.1 : _rng.nextDouble(),
        size: 2 + _rng.nextDouble() * 4,
        speed: 0.15 + _rng.nextDouble() * 0.35,
        wobble: _rng.nextDouble() * 2 * pi,
        wobbleSpeed: 0.5 + _rng.nextDouble(),
        color: _colors[_rng.nextInt(_colors.length)],
      );

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
        painter: _ParticlePainter(_particles, _rng, _randomParticle),
        size: Size.infinite,
      ),
    );
  }
}

class _Particle {
  double x, y, size, speed, wobble, wobbleSpeed;
  Color color;
  _Particle({
    required this.x,
    required this.y,
    required this.size,
    required this.speed,
    required this.wobble,
    required this.wobbleSpeed,
    required this.color,
  });
}

class _ParticlePainter extends CustomPainter {
  final List<_Particle> particles;
  final Random rng;
  final _Particle Function({bool fromBottom}) respawn;

  _ParticlePainter(this.particles, this.rng, this.respawn);

  @override
  void paint(Canvas canvas, Size size) {
    const dt = 0.016;
    for (final p in particles) {
      p.y -= p.speed * dt;
      p.wobble += p.wobbleSpeed * dt;
      final dx = sin(p.wobble) * 0.003;
      p.x += dx;

      final opacity = p.y < 0.1 ? (p.y / 0.1).clamp(0.0, 1.0) : 1.0;
      if (p.y < -0.02) {
        final np = respawn(fromBottom: true);
        p.x = np.x; p.y = np.y; p.size = np.size;
        p.speed = np.speed; p.color = np.color;
      }

      final paint = Paint()
        ..color = p.color.withValues(alpha: 0.6 * opacity)
        ..maskFilter = MaskFilter.blur(BlurStyle.normal, p.size * 0.8);
      canvas.drawCircle(
        Offset(p.x * size.width, p.y * size.height),
        p.size,
        paint,
      );
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}
