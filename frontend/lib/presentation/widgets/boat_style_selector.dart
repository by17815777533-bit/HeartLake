// @file boat_style_selector.dart
// @brief 纸船样式选择器 - 温暖治愈风格

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../domain/entities/paper_boat.dart';
import '../../utils/app_theme.dart';

class BoatStyleSelector extends StatelessWidget {
  final BoatStyle selected;
  final ValueChanged<BoatStyle> onChanged;

  const BoatStyleSelector({
    super.key,
    required this.selected,
    required this.onChanged,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            Colors.white,
            AppTheme.peachPink.withOpacity(0.15),
          ],
        ),
        borderRadius: BorderRadius.circular(24),
        border: Border.all(
          color: AppTheme.peachPink.withOpacity(0.5),
          width: 1.5,
        ),
        boxShadow: [
          BoxShadow(
            color: AppTheme.primaryColor.withOpacity(0.08),
            blurRadius: 20,
            offset: const Offset(0, 8),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                padding: const EdgeInsets.all(8),
                decoration: BoxDecoration(
                  color: AppTheme.primaryColor.withOpacity(0.15),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: const Icon(
                  Icons.sailing_rounded,
                  color: AppTheme.primaryColor,
                  size: 20,
                ),
              ),
              const SizedBox(width: 12),
              const Text(
                '选择纸船样式',
                style: TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.w600,
                  color: AppTheme.textPrimary,
                  letterSpacing: 0.5,
                ),
              ),
            ],
          ),
          const SizedBox(height: 20),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: BoatStyle.values.map((style) {
              return _BoatStyleItem(
                style: style,
                isSelected: selected == style,
                onTap: () {
                  HapticFeedback.selectionClick();
                  onChanged(style);
                },
              );
            }).toList(),
          ),
        ],
      ),
    );
  }
}

class _BoatStyleItem extends StatefulWidget {
  final BoatStyle style;
  final bool isSelected;
  final VoidCallback onTap;

  const _BoatStyleItem({
    required this.style,
    required this.isSelected,
    required this.onTap,
  });

  @override
  State<_BoatStyleItem> createState() => _BoatStyleItemState();
}

class _BoatStyleItemState extends State<_BoatStyleItem>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _scaleAnimation;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 150),
    );
    _scaleAnimation = Tween<double>(begin: 1.0, end: 0.92).animate(
      CurvedAnimation(parent: _controller, curve: Curves.easeInOut),
    );
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  Color get _styleColor {
    switch (widget.style) {
      case BoatStyle.paper:
        return AppTheme.skyBlue;
      case BoatStyle.origami:
        return AppTheme.purpleColor;
      case BoatStyle.lotus:
        return AppTheme.warmPink;
    }
  }

  IconData get _styleIcon {
    switch (widget.style) {
      case BoatStyle.paper:
        return Icons.sailing_rounded;
      case BoatStyle.origami:
        return Icons.auto_awesome_rounded;
      case BoatStyle.lotus:
        return Icons.local_florist_rounded;
    }
  }

  String get _styleLabel {
    switch (widget.style) {
      case BoatStyle.paper:
        return '纸船';
      case BoatStyle.origami:
        return '折纸';
      case BoatStyle.lotus:
        return '莲花';
    }
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTapDown: (_) => _controller.forward(),
      onTapUp: (_) {
        _controller.reverse();
        widget.onTap();
      },
      onTapCancel: () => _controller.reverse(),
      child: AnimatedBuilder(
        animation: _scaleAnimation,
        builder: (context, child) {
          return Transform.scale(
            scale: _scaleAnimation.value,
            child: child,
          );
        },
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 250),
          curve: Curves.easeOutCubic,
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
          decoration: BoxDecoration(
            gradient: widget.isSelected
                ? LinearGradient(
                    begin: Alignment.topLeft,
                    end: Alignment.bottomRight,
                    colors: [
                      _styleColor.withOpacity(0.2),
                      _styleColor.withOpacity(0.1),
                    ],
                  )
                : null,
            color: widget.isSelected ? null : Colors.white,
            borderRadius: BorderRadius.circular(20),
            border: Border.all(
              color: widget.isSelected
                  ? _styleColor
                  : Colors.grey.withOpacity(0.25),
              width: widget.isSelected ? 2 : 1,
            ),
            boxShadow: widget.isSelected
                ? [
                    BoxShadow(
                      color: _styleColor.withOpacity(0.25),
                      blurRadius: 12,
                      offset: const Offset(0, 4),
                    ),
                  ]
                : null,
          ),
          child: Column(
            children: [
              Container(
                width: 48,
                height: 48,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  gradient: LinearGradient(
                    begin: Alignment.topLeft,
                    end: Alignment.bottomRight,
                    colors: widget.isSelected
                        ? [_styleColor.withOpacity(0.8), _styleColor]
                        : [Colors.grey[200]!, Colors.grey[300]!],
                  ),
                  boxShadow: widget.isSelected
                      ? [
                          BoxShadow(
                            color: _styleColor.withOpacity(0.4),
                            blurRadius: 8,
                            offset: const Offset(0, 3),
                          ),
                        ]
                      : null,
                ),
                child: Icon(
                  _styleIcon,
                  size: 24,
                  color: widget.isSelected ? Colors.white : Colors.grey[500],
                ),
              ),
              const SizedBox(height: 10),
              Text(
                _styleLabel,
                style: TextStyle(
                  fontSize: 13,
                  fontWeight: widget.isSelected ? FontWeight.w600 : FontWeight.w500,
                  color: widget.isSelected ? _styleColor : Colors.grey[600],
                  letterSpacing: 0.3,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
