// @file sky_nav_bar.dart
// @brief 光遇风格底部导航栏 - 毛玻璃 + 暖色选中指示 + 柔光图标

import 'dart:ui';
import 'package:flutter/material.dart';
import '../../utils/app_theme.dart';

class SkyNavBar extends StatelessWidget {
  final int selectedIndex;
  final ValueChanged<int> onDestinationSelected;

  const SkyNavBar({
    super.key,
    required this.selectedIndex,
    required this.onDestinationSelected,
  });

  static const _items = [
    _NavItem(icon: Icons.water_outlined, activeIcon: Icons.water, label: '观湖'),
    _NavItem(icon: Icons.explore_outlined, activeIcon: Icons.explore, label: '湖底'),
    _NavItem(icon: Icons.add_circle_outline, activeIcon: Icons.add_circle, label: '投石'),
    _NavItem(icon: Icons.people_outline, activeIcon: Icons.people, label: '好友'),
    _NavItem(icon: Icons.blur_on_outlined, activeIcon: Icons.blur_on, label: '倒影'),
  ];

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final bottomPadding = MediaQuery.of(context).padding.bottom;

    return ClipRRect(
      child: BackdropFilter(
        filter: ImageFilter.blur(sigmaX: 20, sigmaY: 20),
        child: Container(
          decoration: BoxDecoration(
            color: isDark
                ? AppTheme.nightSurface.withValues(alpha: 0.85)
                : Colors.white.withValues(alpha: 0.75),
            border: Border(
              top: BorderSide(
                color: isDark
                    ? Colors.white.withValues(alpha: 0.06)
                    : AppTheme.peachPink.withValues(alpha: 0.15),
              ),
            ),
          ),
          padding: EdgeInsets.only(bottom: bottomPadding, top: 6),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: List.generate(_items.length, (i) {
              final item = _items[i];
              final selected = i == selectedIndex;
              return _buildItem(item, selected, i, isDark);
            }),
          ),
        ),
      ),
    );
  }

  Widget _buildItem(_NavItem item, bool selected, int index, bool isDark) {
    final color = selected
        ? AppTheme.warmOrange
        : (isDark ? Colors.white.withValues(alpha: 0.5) : AppTheme.textTertiary);

    return GestureDetector(
      onTap: () => onDestinationSelected(index),
      behavior: HitTestBehavior.opaque,
      child: SizedBox(
        width: 64,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            AnimatedContainer(
              duration: const Duration(milliseconds: 250),
              curve: Curves.easeOut,
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(14),
                color: selected
                    ? AppTheme.warmOrange.withValues(alpha: isDark ? 0.15 : 0.1)
                    : Colors.transparent,
              ),
              child: Icon(
                selected ? item.activeIcon : item.icon,
                color: color,
                size: 24,
                shadows: selected ? [
                  Shadow(color: AppTheme.warmOrange.withValues(alpha: 0.4), blurRadius: 8),
                ] : null,
              ),
            ),
            const SizedBox(height: 2),
            Text(item.label,
              style: TextStyle(
                fontSize: 10,
                color: color,
                fontWeight: selected ? FontWeight.w500 : FontWeight.w400,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _NavItem {
  final IconData icon;
  final IconData activeIcon;
  final String label;
  const _NavItem({required this.icon, required this.activeIcon, required this.label});
}