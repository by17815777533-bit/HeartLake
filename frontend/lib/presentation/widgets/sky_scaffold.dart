import 'dart:ui';

import 'package:flutter/material.dart';

import '../../utils/app_theme.dart';

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
    this.showParticles = false,
    this.showWater = false,
    this.showClouds = false,
    this.particleCount = 18,
    this.appBar,
    this.bottomNavigationBar,
    this.floatingActionButton,
    this.extendBody = false,
    this.extendBodyBehindAppBar = false,
  });

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      extendBody: extendBody,
      extendBodyBehindAppBar: extendBodyBehindAppBar,
      appBar: appBar,
      floatingActionButton: floatingActionButton,
      bottomNavigationBar: bottomNavigationBar,
      body: Stack(
        children: [
          Positioned.fill(child: _SkyBackdrop(showClouds: showClouds)),
          if (showParticles)
            Positioned.fill(
              child: _ParticleLayer(
                count: particleCount > 0 ? particleCount : 18,
              ),
            ),
          if (showWater)
            const Positioned(
              left: 0,
              right: 0,
              bottom: 0,
              child: _WaterLayer(),
            ),
          Positioned.fill(child: body),
        ],
      ),
    );
  }
}

class _SkyBackdrop extends StatelessWidget {
  final bool showClouds;

  const _SkyBackdrop({required this.showClouds});

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;

    return Stack(
      children: [
        Positioned.fill(
          child: DecoratedBox(
            decoration: BoxDecoration(
              gradient: LinearGradient(
                begin: Alignment.topCenter,
                end: Alignment.bottomCenter,
                colors: isDark
                    ? AppTheme.nightGradient
                    : const [
                        Color(0xFFFFFBF5),
                        Color(0xFFFFF3E6),
                        Color(0xFFEAF2FF),
                      ],
              ),
            ),
          ),
        ),
        Positioned(
          top: -120,
          left: -90,
          child: _SoftOrb(
            color: (isDark ? AppTheme.peachPink : AppTheme.primaryColor)
                .withValues(alpha: isDark ? 0.16 : 0.18),
            size: 300,
          ),
        ),
        Positioned(
          top: -80,
          right: -70,
          child: _SoftOrb(
            color: (isDark ? AppTheme.spiritBlue : AppTheme.secondaryColor)
                .withValues(alpha: isDark ? 0.14 : 0.16),
            size: 240,
          ),
        ),
        if (showClouds)
          Positioned(
            top: 48,
            left: 24,
            right: 24,
            child: ClipRRect(
              borderRadius: BorderRadius.circular(30),
              child: BackdropFilter(
                filter: ImageFilter.blur(sigmaX: 10, sigmaY: 10),
                child: Container(
                  height: 110,
                  decoration: BoxDecoration(
                    gradient: LinearGradient(
                      colors: isDark
                          ? [
                              Colors.white.withValues(alpha: 0.08),
                              Colors.white.withValues(alpha: 0.02),
                            ]
                          : [
                              Colors.white.withValues(alpha: 0.42),
                              Colors.white.withValues(alpha: 0.16),
                            ],
                    ),
                  ),
                ),
              ),
            ),
          ),
      ],
    );
  }
}

class _ParticleLayer extends StatelessWidget {
  final int count;

  const _ParticleLayer({required this.count});

  static const List<Alignment> _anchors = [
    Alignment(-0.88, -0.82),
    Alignment(-0.55, -0.65),
    Alignment(-0.22, -0.78),
    Alignment(0.08, -0.7),
    Alignment(0.36, -0.84),
    Alignment(0.74, -0.66),
    Alignment(-0.78, -0.46),
    Alignment(-0.42, -0.34),
    Alignment(-0.04, -0.5),
    Alignment(0.28, -0.4),
    Alignment(0.64, -0.54),
    Alignment(0.84, -0.28),
    Alignment(-0.72, -0.06),
    Alignment(-0.3, -0.14),
    Alignment(0.08, 0.02),
    Alignment(0.42, -0.08),
    Alignment(0.7, 0.08),
    Alignment(-0.6, 0.22),
    Alignment(-0.14, 0.2),
    Alignment(0.2, 0.28),
    Alignment(0.54, 0.22),
    Alignment(0.82, 0.3),
    Alignment(-0.78, 0.42),
    Alignment(-0.4, 0.5),
    Alignment(0.04, 0.46),
    Alignment(0.38, 0.56),
    Alignment(0.72, 0.48),
    Alignment(-0.58, 0.72),
    Alignment(-0.18, 0.76),
    Alignment(0.2, 0.7),
  ];

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final color = isDark ? AppTheme.candleGlow : AppTheme.primaryColor;

    return IgnorePointer(
      child: Stack(
        children: List.generate(count, (index) {
          final alignment = _anchors[index % _anchors.length];
          final size = index % 6 == 0 ? 4.2 : (index % 3 == 0 ? 3.0 : 2.0);
          final alpha = index % 5 == 0 ? 0.34 : 0.2;
          return Align(
            alignment: alignment,
            child: Container(
              width: size,
              height: size,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: color.withValues(alpha: alpha),
                boxShadow: [
                  BoxShadow(
                    color: color.withValues(alpha: alpha * 0.8),
                    blurRadius: 8,
                    spreadRadius: 0.5,
                  ),
                ],
              ),
            ),
          );
        }),
      ),
    );
  }
}

class _WaterLayer extends StatelessWidget {
  const _WaterLayer();

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return IgnorePointer(
      child: Container(
        height: 220,
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: isDark
                ? [
                    AppTheme.lakeMiddle.withValues(alpha: 0.0),
                    AppTheme.lakeMiddle.withValues(alpha: 0.36),
                    AppTheme.lakeDeep.withValues(alpha: 0.8),
                  ]
                : [
                    AppTheme.lakeSurface.withValues(alpha: 0.0),
                    AppTheme.lakeSurface.withValues(alpha: 0.26),
                    AppTheme.lakeMiddle.withValues(alpha: 0.46),
                  ],
          ),
        ),
        child: Align(
          alignment: Alignment.topCenter,
          child: Container(
            height: 2,
            margin: const EdgeInsets.symmetric(horizontal: 32),
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(2),
              color: (isDark ? AppTheme.candleGlow : AppTheme.primaryColor)
                  .withValues(alpha: 0.28),
            ),
          ),
        ),
      ),
    );
  }
}

class _SoftOrb extends StatelessWidget {
  final Color color;
  final double size;

  const _SoftOrb({
    required this.color,
    required this.size,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      width: size,
      height: size,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        gradient: RadialGradient(
          colors: [color, color.withValues(alpha: 0.0)],
        ),
      ),
    );
  }
}
