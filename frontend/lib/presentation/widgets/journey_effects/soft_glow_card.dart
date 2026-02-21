import 'dart:ui';
import 'package:flutter/material.dart';
import '../../../../utils/app_theme.dart';

class SoftGlowCard extends StatefulWidget {
  final Widget child;
  final Color glowColor;
  final double borderRadius;

  const SoftGlowCard({
    super.key,
    required this.child,
    this.glowColor = AppTheme.warmOrange,
    this.borderRadius = 20,
  });

  @override
  State<SoftGlowCard> createState() => _SoftGlowCardState();
}

class _SoftGlowCardState extends State<SoftGlowCard>
    with SingleTickerProviderStateMixin {
  late final AnimationController _ctrl;
  late final Animation<double> _pulse;

  @override
  void initState() {
    super.initState();
    _ctrl = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 2500),
    )..repeat(reverse: true);
    _pulse = Tween(begin: 0.3, end: 0.7).animate(
      CurvedAnimation(parent: _ctrl, curve: Curves.easeInOut),
    );
  }

  @override
  void dispose() {
    _ctrl.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final radius = BorderRadius.circular(widget.borderRadius);
    return AnimatedBuilder(
      animation: _pulse,
      builder: (_, __) => Container(
        decoration: BoxDecoration(
          borderRadius: radius,
          boxShadow: [
            BoxShadow(
              color: widget.glowColor.withValues(alpha: _pulse.value * 0.3),
              blurRadius: 16,
              spreadRadius: 1,
            ),
          ],
        ),
        child: ClipRRect(
          borderRadius: radius,
          child: BackdropFilter(
            filter: ImageFilter.blur(sigmaX: 12, sigmaY: 12),
            child: Container(
              decoration: BoxDecoration(
                color: Colors.white.withValues(alpha: 0.15),
                borderRadius: radius,
                border: Border.all(
                  color: widget.glowColor.withValues(alpha: _pulse.value * 0.5),
                  width: 1.2,
                ),
              ),
              child: widget.child,
            ),
          ),
        ),
      ),
    );
  }
}
