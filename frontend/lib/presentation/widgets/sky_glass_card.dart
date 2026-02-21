import 'dart:ui';

import 'package:flutter/material.dart';

import '../../utils/app_theme.dart';

class SkyGlassCard extends StatelessWidget {
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
    this.borderRadius = 18,
    this.blur = 12,
    this.enableGlow = true,
    this.padding = const EdgeInsets.all(16),
    this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final activeGlow = glowColor ?? (isDark ? AppTheme.primaryLightColor : AppTheme.primaryColor);
    final radius = BorderRadius.circular(borderRadius);

    final card = ClipRRect(
      borderRadius: radius,
      child: BackdropFilter(
        filter: ImageFilter.blur(sigmaX: blur, sigmaY: blur),
        child: Container(
          decoration: BoxDecoration(
            gradient: LinearGradient(
              begin: Alignment.topLeft,
              end: Alignment.bottomRight,
              colors: isDark
                  ? [
                      Colors.white.withValues(alpha: 0.11),
                      Colors.white.withValues(alpha: 0.05),
                    ]
                  : [
                      Colors.white.withValues(alpha: 0.82),
                      Colors.white.withValues(alpha: 0.58),
                    ],
            ),
            borderRadius: radius,
            border: Border.all(
              color: activeGlow.withValues(alpha: isDark ? 0.24 : 0.16),
              width: 0.9,
            ),
            boxShadow: [
              if (enableGlow)
                BoxShadow(
                  color: activeGlow.withValues(alpha: isDark ? 0.14 : 0.12),
                  blurRadius: 20,
                  spreadRadius: 0.4,
                ),
            ],
          ),
          child: Material(
            color: Colors.transparent,
            child: InkWell(
              onTap: onTap,
              borderRadius: radius,
              splashColor: activeGlow.withValues(alpha: 0.12),
              highlightColor: Colors.transparent,
              child: Padding(
                padding: padding,
                child: child,
              ),
            ),
          ),
        ),
      ),
    );

    return card;
  }
}
