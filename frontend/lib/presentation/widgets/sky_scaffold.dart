// @file sky_scaffold.dart
// @brief 光遇风格统一页面容器 - 分层背景 + 粒子 + 水波纹

import 'package:flutter/material.dart';
import 'journey_effects/glow_particles.dart';
import 'water_background.dart';
import 'atmospheric_background.dart';

class SkyScaffold extends StatelessWidget {
  final Widget body;
  final bool showParticles;
  final bool showWater;
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