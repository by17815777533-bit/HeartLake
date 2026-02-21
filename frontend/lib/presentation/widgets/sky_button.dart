// @file sky_button.dart
// @brief 光遇风格暖色渐变按钮 - 橙→粉→紫 + 柔光

import 'package:flutter/material.dart';
import '../../utils/app_theme.dart';

class SkyButton extends StatelessWidget {
  final String label;
  final VoidCallback? onPressed;
  final bool isLoading;
  final List<Color>? gradient;
  final double borderRadius;
  final double? width;
  final IconData? icon;

  const SkyButton({
    super.key,
    required this.label,
    this.onPressed,
    this.isLoading = false,
    this.gradient,
    this.borderRadius = 16,
    this.width,
    this.icon,
  });

  @override
  Widget build(BuildContext context) {
    final colors = gradient ?? AppTheme.warmGradient;
    final disabled = onPressed == null || isLoading;

    return Container(
      width: width,
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(borderRadius),
        gradient: LinearGradient(
          colors: disabled
              ? colors.map((c) => c.withValues(alpha: 0.4)).toList()
              : colors,
        ),
        boxShadow: disabled ? null : [
          BoxShadow(
            color: colors.first.withValues(alpha: 0.3),
            blurRadius: 12,
            offset: const Offset(0, 4),
          ),
        ],
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          onTap: disabled ? null : onPressed,
          borderRadius: BorderRadius.circular(borderRadius),
          splashColor: Colors.white.withValues(alpha: 0.2),
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 28, vertical: 14),
            child: Center(
              child: isLoading
                  ? const SizedBox(
                      width: 20, height: 20,
                      child: CircularProgressIndicator(
                        strokeWidth: 2, color: Colors.white,
                      ),
                    )
                  : Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        if (icon != null) ...[
                          Icon(icon, color: Colors.white, size: 18),
                          const SizedBox(width: 8),
                        ],
                        Text(label,
                          style: const TextStyle(
                            color: Colors.white,
                            fontSize: 15,
                            fontWeight: FontWeight.w500,
                            letterSpacing: 1,
                          ),
                        ),
                      ],
                    ),
            ),
          ),
        ),
      ),
    );
  }
}