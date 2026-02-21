// @file stone_card_actions.dart
// @brief StoneCard操作按钮组件
// Created by 吴睿璐

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../../utils/mood_colors.dart';

/// StoneCard 操作按钮组件
class StoneCardActions extends StatelessWidget {
  final MoodColorConfig moodConfig;
  final int rippleCount;
  final int boatCount;
  final bool hasRippled;
  final VoidCallback onRipple;
  final VoidCallback onBoat;

  const StoneCardActions({
    super.key,
    required this.moodConfig,
    required this.rippleCount,
    required this.boatCount,
    required this.hasRippled,
    required this.onRipple,
    required this.onBoat,
  });

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceAround,
      children: [
        _ActionButton(moodConfig: moodConfig, icon: Icons.water_drop_outlined, count: rippleCount, label: '涟漪', onTap: onRipple, isActive: hasRippled),
        _ActionButton(moodConfig: moodConfig, icon: Icons.sailing_outlined, count: boatCount, label: '纸船', onTap: onBoat),
      ],
    );
  }
}

class _ActionButton extends StatefulWidget {
  final MoodColorConfig moodConfig;
  final IconData icon;
  final int count;
  final String label;
  final VoidCallback onTap;
  final bool isActive;

  const _ActionButton({
    required this.moodConfig,
    required this.icon,
    required this.count,
    required this.label,
    required this.onTap,
    this.isActive = false,
  });

  @override
  State<_ActionButton> createState() => _ActionButtonState();
}

class _ActionButtonState extends State<_ActionButton> with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _scale;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(vsync: this, duration: const Duration(milliseconds: 100));
    _scale = Tween<double>(begin: 1.0, end: 0.92).animate(CurvedAnimation(parent: _controller, curve: Curves.easeInOut));
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final colorScheme = Theme.of(context).colorScheme;
    return GestureDetector(
      onTapDown: (_) => _controller.forward(),
      onTapUp: (_) {
        _controller.reverse();
        HapticFeedback.lightImpact();
        widget.onTap();
      },
      onTapCancel: () => _controller.reverse(),
      child: AnimatedBuilder(
        animation: _scale,
        builder: (context, child) => Transform.scale(scale: _scale.value, child: child),
        child: Container(
          padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
          decoration: BoxDecoration(
            color: widget.isActive ? colorScheme.primaryContainer : colorScheme.surfaceContainerHighest,
            borderRadius: BorderRadius.circular(12),
          ),
          child: Row(
            children: [
              Icon(widget.icon, size: 18, color: widget.isActive ? colorScheme.onPrimaryContainer : colorScheme.onSurfaceVariant),
              const SizedBox(width: 4),
              Text(
                widget.count > 0 ? widget.count.toString() : widget.label,
                style: TextStyle(
                  color: widget.isActive ? colorScheme.onPrimaryContainer : colorScheme.onSurfaceVariant,
                  fontWeight: FontWeight.w500,
                  fontSize: 13,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
