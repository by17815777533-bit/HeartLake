// 动画工具类

library;

import 'package:flutter/material.dart';

/// 应用内常用动画曲线，统一管理避免各处硬编码
class AnimationCurves {
  static const Curve defaultCurve = Curves.easeInOut;
  static const Curve bounceCurve = Curves.bounceOut;
  static const Curve elasticCurve = Curves.elasticOut;
}

/// 应用内常用动画时长，分为快/中/慢三档
class AnimationDurations {
  static const Duration fast = Duration(milliseconds: 200);
  static const Duration normal = Duration(milliseconds: 300);
  static const Duration slow = Duration(milliseconds: 500);
}

/// 动画组合工具，提供常用的复合过渡效果
class AnimationUtils {
  /// 淡入 + 上滑组合动画，常用于列表项入场
  ///
  /// [animation] 为 null 时直接返回原 Widget，不做任何动画包装。
  /// [beginOffset] 控制滑入起始偏移量，默认从下方 10% 处滑入。
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
