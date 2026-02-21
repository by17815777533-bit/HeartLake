// @file sky_glass_card.dart
// @brief 光遇风格毛玻璃卡片 - BackdropFilter + 脉动光晕

import 'dart:ui';
import 'package:flutter/material.dart';
import '../../utils/app_theme.dart';

class SkyGlassCard extends StatefulWidget {
  final Widget child;
  final Color? glowColor;
  final double borderRadius;
  final double blur;
  final bool enableGlow;
  final EdgeInsets padding;
  final VoidCallback? onTap;

  const SkyGlassCard({
    super.key,
    required this.child,
    this.glowColor,
    this.borderRadius = 20,
    this.blur = 12,
    this.enableGlow = true,
    this.padding = const EdgeInsets.all(16),
    this.onTap,
  });

  @override
  State<SkyGlassCard> createState() => _SkyGlassCardState();
}

class _SkyGlassCardState extends State<SkyGlassCard>
    with SingleTickerProviderStateMixin {
  AnimationController? _ctrl;
  Animation<double>? _pulse;

  @override
  void initState() {
    super.initState();
    if (widget.enableGlow) {
      _ctrl = AnimationController(
        vsync: this,
        duration: const Duration(milliseconds: 2500),
      )..repeat(reverse: true);
      _pulse = Tween(begin: 0.15, end: 0.45).animate(
        CurvedAnimation(parent: _ctrl!, curve: Curves.easeInOut),
      );
    }
  }

  @override
  void dispose() {
    _ctrl?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final glow = widget.glowColor ?? AppTheme.warmOrange;
    final radius = BorderRadius.circular(widget.borderRadius);
    final bgColor = isDark
        ? Colors.white.withValues(alpha: 0.08)
        : Colors.white.withValues(alpha: 0.65);
    final borderColor = isDark
        ? glow.withValues(alpha: 0.2)
        : glow.withValues(alpha: 0.15);

    Widget card = ClipRRect(
      borderRadius: radius,
      child: BackdropFilter(
        filter: ImageFilter.blur(sigmaX: widget.blur, sigmaY: widget.blur),
        child: Container(
          decoration: BoxDecoration(
            color: bgColor,
            borderRadius: radius,
            border: Border.all(color: borderColor, width: 0.8),
          ),
          padding: widget.padding,
          child: widget.child,
        ),
      ),
    );

    if (widget.enableGlow && _pulse != null) {
      card = AnimatedBuilder(
        animation: _pulse!,
        builder: (_, __) => Container(
          decoration: BoxDecoration(
            borderRadius: radius,
            boxShadow: [
              BoxShadow(
                color: glow.withValues(alpha: _pulse!.value * 0.15),
                blurRadius: 20,
                spreadRadius: 1,
              ),
            ],
          ),
          child: card,
        ),
      );
    }

    if (widget.onTap != null) {
      return GestureDetector(onTap: widget.onTap, child: card);
    }
    return card;
  }
}