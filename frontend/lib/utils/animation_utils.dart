// @file animation_utils.dart
// @brief 动画工具类
// Created by 王璐瑶

library;

import 'package:flutter/material.dart';

/// 动画曲线常量
class AnimationCurves {
  static const Curve defaultCurve = Curves.easeInOut;
  static const Curve bounceCurve = Curves.bounceOut;
  static const Curve elasticCurve = Curves.elasticOut;
}

/// 动画时长常量
class AnimationDurations {
  static const Duration fast = Duration(milliseconds: 200);
  static const Duration normal = Duration(milliseconds: 300);
  static const Duration slow = Duration(milliseconds: 500);
}

/// 动画工具方法
class AnimationUtils {
  static Widget fadeSlideIn({
    required Widget child,
    Animation<double>? animation,
    int index = 0,
    Offset beginOffset = const Offset(0, 0.1),
  }) {
    if (animation != null) {
      return FadeTransition(
        opacity: animation,
        child: SlideTransition(
          position: Tween<Offset>(begin: beginOffset, end: Offset.zero).animate(animation),
          child: child,
        ),
      );
    }
    return child;
  }
}

/// 光遇风格页面路由转场 - 淡入 + 轻微上滑
class SkyPageRoute<T> extends PageRouteBuilder<T> {
  final Widget page;
  SkyPageRoute({required this.page})
      : super(
          pageBuilder: (context, animation, secondaryAnimation) => page,
          transitionDuration: const Duration(milliseconds: 400),
          reverseTransitionDuration: const Duration(milliseconds: 300),
          transitionsBuilder: (context, animation, secondaryAnimation, child) {
            final fadeAnimation = CurvedAnimation(parent: animation, curve: Curves.easeOut);
            final slideAnimation = Tween<Offset>(begin: const Offset(0, 0.05), end: Offset.zero)
                .animate(CurvedAnimation(parent: animation, curve: Curves.easeOutCubic));
            return FadeTransition(
              opacity: fadeAnimation,
              child: SlideTransition(position: slideAnimation, child: child),
            );
          },
        );
}
