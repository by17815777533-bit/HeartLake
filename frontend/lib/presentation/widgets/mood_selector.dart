// @file mood_selector.dart
// @brief 情绪选择器组件
// Created by 吴睿璐

library;

import 'package:flutter/material.dart';
import '../../domain/entities/mood.dart';

/// 情绪选择器
class MoodSelector extends StatefulWidget {
  final MoodType? selectedMood;
  final ValueChanged<MoodType> onMoodSelected;
  final bool showLabels;
  final double itemSize;

  const MoodSelector({
    super.key,
    this.selectedMood,
    required this.onMoodSelected,
    this.showLabels = true,
    this.itemSize = 56.0,
  });

  @override
  State<MoodSelector> createState() => _MoodSelectorState();
}

class _MoodSelectorState extends State<MoodSelector> {
  @override
  Widget build(BuildContext context) {
    final moods = MoodConfigs.selectableMoods;

    return Container(
      padding: const EdgeInsets.symmetric(vertical: 12),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisSize: MainAxisSize.min,
        children: [
          if (widget.showLabels)
            Padding(
              padding: const EdgeInsets.only(left: 4, bottom: 12),
              child: Text(
                '此刻的心情',
                style: TextStyle(
                  fontSize: 14,
                  fontWeight: FontWeight.w500,
                  color: Colors.grey[700],
                ),
              ),
            ),
          SingleChildScrollView(
            scrollDirection: Axis.horizontal,
            child: Row(
              children: moods.map((config) {
                final isSelected = widget.selectedMood == config.type;
                return _MoodItem(
                  config: config,
                  isSelected: isSelected,
                  size: widget.itemSize,
                  showLabel: widget.showLabels,
                  onTap: () => widget.onMoodSelected(config.type),
                );
              }).toList(),
            ),
          ),
        ],
      ),
    );
  }
}

class _MoodItem extends StatefulWidget {
  final MoodConfig config;
  final bool isSelected;
  final double size;
  final bool showLabel;
  final VoidCallback onTap;

  const _MoodItem({
    required this.config,
    required this.isSelected,
    required this.size,
    required this.showLabel,
    required this.onTap,
  });

  @override
  State<_MoodItem> createState() => _MoodItemState();
}

class _MoodItemState extends State<_MoodItem>
    with SingleTickerProviderStateMixin {
  late AnimationController _scaleController;
  late Animation<double> _scaleAnimation;

  @override
  void initState() {
    super.initState();
    _scaleController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 150),
    );
    _scaleAnimation = Tween<double>(begin: 1.0, end: 0.9).animate(
      CurvedAnimation(parent: _scaleController, curve: Curves.easeInOut),
    );
  }

  @override
  void dispose() {
    _scaleController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTapDown: (_) => _scaleController.forward(),
      onTapUp: (_) {
        _scaleController.reverse();
        widget.onTap();
      },
      onTapCancel: () => _scaleController.reverse(),
      child: Semantics(
        label: '${widget.config.label}情绪${widget.isSelected ? '，已选中' : ''}',
        button: true,
        selected: widget.isSelected,
        child: AnimatedBuilder(
        animation: _scaleAnimation,
        builder: (context, child) {
          return Transform.scale(
            scale: _scaleAnimation.value,
            child: child,
          );
        },
        child: Container(
          margin: const EdgeInsets.symmetric(horizontal: 8),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              AnimatedContainer(
                duration: const Duration(milliseconds: 200),
                width: widget.size,
                height: widget.size,
                decoration: BoxDecoration(
                  color: widget.isSelected
                      ? widget.config.backgroundColor
                      : Colors.grey[100],
                  shape: BoxShape.circle,
                  border: Border.all(
                    color: widget.isSelected
                        ? widget.config.primaryColor
                        : Colors.grey[300]!,
                    width: widget.isSelected ? 2.5 : 1,
                  ),
                  boxShadow: widget.isSelected
                      ? [
                          BoxShadow(
                            color: widget.config.glowColor.withValues(alpha: 0.4),
                            blurRadius: 10,
                            spreadRadius: 2,
                          ),
                          BoxShadow(
                            color: widget.config.glowColor.withValues(alpha: 0.15),
                            blurRadius: 20,
                            spreadRadius: 4,
                          ),
                        ]
                      : null,
                ),
                child: Center(
                  child: Icon(
                    widget.config.icon,
                    size: widget.size * 0.45,
                    color: widget.isSelected ? Colors.white : widget.config.primaryColor,
                  ),
                ),
              ),
              if (widget.showLabel) ...[
                const SizedBox(height: 6),
                AnimatedDefaultTextStyle(
                  duration: const Duration(milliseconds: 200),
                  style: TextStyle(
                    fontSize: 12,
                    fontWeight:
                        widget.isSelected ? FontWeight.w600 : FontWeight.normal,
                    color: widget.isSelected
                        ? widget.config.primaryColor
                        : Colors.grey[600],
                  ),
                  child: Text(widget.config.label),
                ),
              ],
            ],
          ),
        ),
      ),
      ),
    );
  }
}

/// 紧凑型情绪选择器（用于底部弹窗等场景）
class CompactMoodSelector extends StatelessWidget {
  final MoodType? selectedMood;
  final ValueChanged<MoodType> onMoodSelected;

  const CompactMoodSelector({
    super.key,
    this.selectedMood,
    required this.onMoodSelected,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(16),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '选择心情',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.grey[800],
            ),
          ),
          const SizedBox(height: 16),
          Wrap(
            spacing: 12,
            runSpacing: 12,
            children: MoodConfigs.selectableMoods.map((config) {
              final isSelected = selectedMood == config.type;
              return GestureDetector(
                onTap: () {
                  onMoodSelected(config.type);
                  Navigator.pop(context);
                },
                child: Container(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 16,
                    vertical: 10,
                  ),
                  decoration: BoxDecoration(
                    color:
                        isSelected ? config.backgroundColor : Colors.grey[100],
                    borderRadius: BorderRadius.circular(20),
                    border: Border.all(
                      color:
                          isSelected ? config.primaryColor : Colors.grey[300]!,
                      width: isSelected ? 2 : 1,
                    ),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Text(
                        config.label,
                        style: TextStyle(
                          fontSize: 14,
                          fontWeight:
                              isSelected ? FontWeight.w600 : FontWeight.normal,
                          color: isSelected
                              ? config.primaryColor
                              : Colors.grey[700],
                        ),
                      ),
                    ],
                  ),
                ),
              );
            }).toList(),
          ),
          const SizedBox(height: 8),
        ],
      ),
    );
  }
}

/// 情绪指示器组件（带图标和光晕效果）
class MoodIndicator extends StatelessWidget {
  final MoodType mood;
  final double size;
  final bool showLabel;

  const MoodIndicator({
    super.key,
    required this.mood,
    this.size = 40,
    this.showLabel = true,
  });

  @override
  Widget build(BuildContext context) {
    final config = MoodConfigs.getConfig(mood);
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Container(
          width: size,
          height: size,
          decoration: BoxDecoration(
            shape: BoxShape.circle,
            gradient: LinearGradient(
              begin: Alignment.topLeft,
              end: Alignment.bottomRight,
              colors: [config.primaryColor.withValues(alpha: 0.8), config.primaryColor],
            ),
            boxShadow: [
              BoxShadow(color: config.primaryColor.withValues(alpha: 0.4), blurRadius: 8, spreadRadius: 2),
            ],
          ),
          child: Icon(config.icon, color: Colors.white, size: size * 0.45),
        ),
        if (showLabel) ...[
          const SizedBox(height: 4),
          Text(config.label, style: TextStyle(fontSize: 12, fontWeight: FontWeight.w500, color: config.primaryColor)),
        ],
      ],
    );
  }
}

/// 情绪显示标签（用于石头卡片等展示场景）
class MoodBadge extends StatelessWidget {
  final MoodType mood;
  final bool showGlow;
  final double size;

  const MoodBadge({
    super.key,
    required this.mood,
    this.showGlow = false,
    this.size = 24.0,
  });

  @override
  Widget build(BuildContext context) {
    final config = MoodConfigs.getConfig(mood);

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: config.backgroundColor,
        borderRadius: BorderRadius.circular(12),
        boxShadow: showGlow
            ? [
                BoxShadow(
                  color: config.glowColor.withValues(alpha: 0.5),
                  blurRadius: 8,
                  spreadRadius: 1,
                ),
              ]
            : null,
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            config.label,
            style: TextStyle(
              fontSize: size * 0.5,
              color: config.primaryColor,
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }
}
