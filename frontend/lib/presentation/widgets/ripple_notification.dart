// @file ripple_notification.dart
// @brief 涟漪通知组件
// Created by 林子怡

import 'package:flutter/material.dart';

/// 涟漪通知组件 - 像石头投入湖中泛起涟漪的效果
class RippleNotification {
  static OverlayEntry? _currentOverlay;

  /// 显示涟漪通知
  static void show(
    BuildContext context, {
    required String message,
    IconData icon = Icons.notifications,
    Color color = Colors.blue,
    Duration duration = const Duration(seconds: 3),
  }) {
    // 移除现有通知
    _currentOverlay?.remove();
    _currentOverlay = null;

    final overlay = Overlay.of(context);
    final overlayEntry = OverlayEntry(
      builder: (context) => RippleNotificationWidget(
        message: message,
        icon: icon,
        color: color,
        duration: duration,
        onDismiss: () {
          _currentOverlay?.remove();
          _currentOverlay = null;
        },
      ),
    );

    _currentOverlay = overlayEntry;
    overlay.insert(overlayEntry);
  }
}

class RippleNotificationWidget extends StatefulWidget {
  final String message;
  final IconData icon;
  final Color color;
  final Duration duration;
  final VoidCallback onDismiss;

  const RippleNotificationWidget({
    super.key,
    required this.message,
    required this.icon,
    required this.color,
    required this.duration,
    required this.onDismiss,
  });

  @override
  State<RippleNotificationWidget> createState() =>
      _RippleNotificationWidgetState();
}

class _RippleNotificationWidgetState extends State<RippleNotificationWidget>
    with TickerProviderStateMixin {
  late AnimationController _slideController;
  late AnimationController _rippleController;
  late Animation<Offset> _slideAnimation;
  late Animation<double> _fadeAnimation;

  final List<_Ripple> _ripples = [];

  @override
  void initState() {
    super.initState();

    // 滑入动画
    _slideController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 500),
    );

    _slideAnimation = Tween<Offset>(
      begin: const Offset(0, -1),
      end: Offset.zero,
    ).animate(CurvedAnimation(
      parent: _slideController,
      curve: Curves.elasticOut,
    ));

    _fadeAnimation = Tween<double>(begin: 0, end: 1).animate(
      CurvedAnimation(
        parent: _slideController,
        curve: const Interval(0, 0.3),
      ),
    );

    // 涟漪动画
    _rippleController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 2),
    )..repeat();

    _rippleController.addListener(() {
      setState(() {
        // 定期添加新涟漪
        if (_rippleController.value < 0.01 && _ripples.length < 3) {
          _ripples.add(_Ripple(
            startTime: DateTime.now(),
            color: widget.color,
          ));
        }

        // 移除过期涟漪
        _ripples.removeWhere((r) =>
            DateTime.now().difference(r.startTime).inMilliseconds > 2000);
      });
    });

    _slideController.forward();

    // 自动消失
    Future.delayed(widget.duration, () {
      _dismiss();
    });
  }

  void _dismiss() {
    _slideController.reverse().then((_) {
      widget.onDismiss();
    });
  }

  @override
  void dispose() {
    _slideController.dispose();
    _rippleController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Positioned(
      top: MediaQuery.of(context).padding.top + 16,
      left: 16,
      right: 16,
      child: SlideTransition(
        position: _slideAnimation,
        child: FadeTransition(
          opacity: _fadeAnimation,
          child: Material(
            color: Colors.transparent,
            child: GestureDetector(
              onTap: _dismiss,
              child: Stack(
                clipBehavior: Clip.none,
                children: [
                  // 涟漪效果
                  Positioned.fill(
                    child: CustomPaint(
                      painter: _RipplePainter(
                        ripples: _ripples,
                        progress: _rippleController.value,
                      ),
                    ),
                  ),
                  // 通知卡片
                  Container(
                    padding: const EdgeInsets.all(16),
                    decoration: BoxDecoration(
                      color: Colors.white,
                      borderRadius: BorderRadius.circular(16),
                      boxShadow: [
                        BoxShadow(
                          color: widget.color.withOpacity(0.3),
                          blurRadius: 20,
                          spreadRadius: 2,
                          offset: const Offset(0, 4),
                        ),
                      ],
                    ),
                    child: Row(
                      children: [
                        // 图标 + 涟漪效果
                        Container(
                          width: 48,
                          height: 48,
                          decoration: BoxDecoration(
                            shape: BoxShape.circle,
                            gradient: LinearGradient(
                              colors: [
                                widget.color,
                                widget.color.withOpacity(0.7),
                              ],
                            ),
                          ),
                          child: Icon(
                            widget.icon,
                            color: Colors.white,
                            size: 24,
                          ),
                        ),
                        const SizedBox(width: 16),
                        // 消息内容
                        Expanded(
                          child: Text(
                            widget.message,
                            style: const TextStyle(
                              fontSize: 15,
                              fontWeight: FontWeight.w500,
                              color: Colors.black87,
                            ),
                            maxLines: 2,
                            overflow: TextOverflow.ellipsis,
                          ),
                        ),
                        const SizedBox(width: 8),
                        // 关闭按钮
                        IconButton(
                          icon: const Icon(Icons.close, size: 20),
                          onPressed: _dismiss,
                          padding: EdgeInsets.zero,
                          constraints: const BoxConstraints(),
                        ),
                      ],
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

class _Ripple {
  final DateTime startTime;
  final Color color;

  _Ripple({required this.startTime, required this.color});
}

class _RipplePainter extends CustomPainter {
  final List<_Ripple> ripples;
  final double progress;

  _RipplePainter({required this.ripples, required this.progress});

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(24, size.height / 2); // 从图标中心发出涟漪

    for (final ripple in ripples) {
      final elapsed =
          DateTime.now().difference(ripple.startTime).inMilliseconds;
      final rippleProgress = (elapsed / 2000).clamp(0.0, 1.0);

      if (rippleProgress >= 1.0) continue;

      // 计算涟漪半径和透明度
      final radius = 40 + (rippleProgress * 100);
      final opacity = (1.0 - rippleProgress) * 0.6;

      final paint = Paint()
        ..color = ripple.color.withOpacity(opacity)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2.0;

      canvas.drawCircle(center, radius, paint);

      // 绘制内圈涟漪
      if (rippleProgress < 0.7) {
        final innerRadius = radius * 0.6;
        final innerOpacity = opacity * 0.7;
        final innerPaint = Paint()
          ..color = ripple.color.withOpacity(innerOpacity)
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1.5;

        canvas.drawCircle(center, innerRadius, innerPaint);
      }
    }
  }

  @override
  bool shouldRepaint(_RipplePainter oldDelegate) =>
      ripples.length != oldDelegate.ripples.length;
}
