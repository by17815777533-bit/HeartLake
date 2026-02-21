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

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;

    return SafeArea(
      top: false,
      child: Padding(
        padding: const EdgeInsets.fromLTRB(12, 0, 12, 10),
        child: ClipRRect(
          borderRadius: BorderRadius.circular(24),
          child: BackdropFilter(
            filter: ImageFilter.blur(sigmaX: 18, sigmaY: 18),
            child: DecoratedBox(
              decoration: BoxDecoration(
                gradient: LinearGradient(
                  begin: Alignment.topCenter,
                  end: Alignment.bottomCenter,
                  colors: isDark
                      ? [
                          AppTheme.lightStone.withValues(alpha: 0.86),
                          AppTheme.backgroundColor.withValues(alpha: 0.72),
                        ]
                      : [
                          Colors.white.withValues(alpha: 0.9),
                          const Color(0xFFFFF3E9).withValues(alpha: 0.78),
                        ],
                ),
                border: Border.all(
                  color: (isDark ? AppTheme.primaryLightColor : AppTheme.primaryColor)
                      .withValues(alpha: isDark ? 0.2 : 0.16),
                ),
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withValues(alpha: isDark ? 0.16 : 0.06),
                    blurRadius: 20,
                    offset: const Offset(0, 8),
                  ),
                ],
              ),
              child: NavigationBarTheme(
                data: NavigationBarThemeData(
                  backgroundColor: Colors.transparent,
                  elevation: 0,
                  indicatorColor: AppTheme.primaryColor.withValues(alpha: 0.18),
                  iconTheme: WidgetStateProperty.resolveWith((states) {
                    final selected = states.contains(WidgetState.selected);
                    return IconThemeData(
                      color: selected
                          ? (isDark ? AppTheme.primaryLightColor : AppTheme.primaryLightColor)
                          : (isDark ? AppTheme.textSecondary : AppTheme.textTertiary),
                    );
                  }),
                  labelTextStyle: WidgetStateProperty.resolveWith((states) {
                    final selected = states.contains(WidgetState.selected);
                    return TextStyle(
                      fontSize: 11,
                      fontWeight: selected ? FontWeight.w600 : FontWeight.w500,
                      color: selected
                          ? (isDark ? AppTheme.primaryLightColor : AppTheme.textPrimary)
                          : (isDark ? AppTheme.textSecondary : AppTheme.textTertiary),
                    );
                  }),
                ),
                child: NavigationBar(
                  selectedIndex: selectedIndex,
                  onDestinationSelected: onDestinationSelected,
                  labelBehavior: NavigationDestinationLabelBehavior.alwaysShow,
                  height: 72,
                  destinations: const [
                    NavigationDestination(
                      icon: Icon(Icons.water_outlined),
                      selectedIcon: Icon(Icons.water),
                      label: '观湖',
                    ),
                    NavigationDestination(
                      icon: Icon(Icons.explore_outlined),
                      selectedIcon: Icon(Icons.explore),
                      label: '湖底',
                    ),
                    NavigationDestination(
                      icon: Icon(Icons.add_circle_outline),
                      selectedIcon: Icon(Icons.add_circle),
                      label: '投石',
                    ),
                    NavigationDestination(
                      icon: Icon(Icons.people_outline),
                      selectedIcon: Icon(Icons.people),
                      label: '好友',
                    ),
                    NavigationDestination(
                      icon: Icon(Icons.blur_on_outlined),
                      selectedIcon: Icon(Icons.blur_on),
                      label: '倒影',
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
