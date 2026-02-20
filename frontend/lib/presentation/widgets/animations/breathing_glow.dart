// @file breathing_glow.dart
// @brief 呼吸光晕效果组件
// Created by 吴睿璐

library;

import 'package:flutter/material.dart';

/// 呼吸光晕包装器
class BreathingGlow extends StatefulWidget {
  final Widget child;
  final Color glowColor;
  final double maxBlur;
  final Duration duration;

  const BreathingGlow({
    super.key,
    required this.child,
    this.glowColor = Colors.blue,
    this.maxBlur = 10.0,
    this.duration = const Duration(seconds: 4), // 更慢更温馨
  });

  @override
  State<BreathingGlow> createState() => _BreathingGlowState();
}

class _BreathingGlowState extends State<BreathingGlow>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(vsync: this, duration: widget.duration)
      ..repeat(reverse: true);
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _controller,
      builder: (context, child) {
        // 使用更柔和的呼吸曲线
        final t = Curves.easeInOutSine.transform(_controller.value);
        final blur = 4.0 + widget.maxBlur * t;
        final spread = 1.0 + t * 2;
        return Container(
          decoration: BoxDecoration(
            borderRadius: BorderRadius.circular(24),
            boxShadow: [
              BoxShadow(
                color: widget.glowColor.withOpacity(0.12 + 0.12 * t),
                blurRadius: blur,
                spreadRadius: spread,
              ),
              // 外层柔光
              BoxShadow(
                color: widget.glowColor.withOpacity(0.05 * t),
                blurRadius: blur * 1.5,
                spreadRadius: spread * 2,
              ),
            ],
          ),
          child: child,
        );
      },
      child: widget.child,
    );
  }
}
