// @file shimmer_loading.dart
// @brief 骨架屏加载组件
// Created by 吴睿璐

library;
import 'package:flutter/material.dart';
import '../../utils/animation_utils.dart';
import '../../utils/app_theme.dart';
import 'dart:math' as math;

class ShimmerLoading extends StatefulWidget {
  final Widget child;
  final Color baseColor;
  final Color highlightColor;
  final Duration duration;
  final bool enabled;

  const ShimmerLoading({
    super.key,
    required this.child,
    this.baseColor = const Color(0xFFE0E0E0),
    this.highlightColor = const Color(0xFFF5F5F5),
    this.duration = const Duration(milliseconds: 1500),
    this.enabled = true,
  });

  @override
  State<ShimmerLoading> createState() => _ShimmerLoadingState();
}

class _ShimmerLoadingState extends State<ShimmerLoading>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _animation;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      duration: widget.duration,
      vsync: this,
    );

    _animation = Tween<double>(
      begin: -1.0,
      end: 2.0,
    ).animate(CurvedAnimation(
      parent: _controller,
      curve: Curves.easeInOut,
    ));

    if (widget.enabled) {
      _controller.repeat();
    }
  }

  @override
  void didUpdateWidget(ShimmerLoading oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.enabled != oldWidget.enabled) {
      if (widget.enabled) {
        _controller.repeat();
      } else {
        _controller.stop();
      }
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (!widget.enabled) {
      return widget.child;
    }

    return AnimatedBuilder(
      animation: _animation,
      builder: (context, child) {
        return ShaderMask(
          shaderCallback: (bounds) {
            return LinearGradient(
              begin: Alignment.centerLeft,
              end: Alignment.centerRight,
              colors: [
                widget.baseColor,
                widget.highlightColor,
                widget.baseColor,
              ],
              stops: [
                math.max(0.0, _animation.value - 0.3),
                _animation.value,
                math.min(1.0, _animation.value + 0.3),
              ],
            ).createShader(bounds);
          },
          blendMode: BlendMode.srcATop,
          child: child,
        );
      },
      child: widget.child,
    );
  }
}

class StoneCardSkeleton extends StatelessWidget {
  final bool showAvatar;
  final bool showContent;
  final bool showActions;

  const StoneCardSkeleton({
    super.key,
    this.showAvatar = true,
    this.showContent = true,
    this.showActions = true,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(20),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            blurRadius: 10,
            offset: const Offset(0, 4),
          ),
        ],
      ),
      child: ShimmerLoading(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // 用户信息行
            if (showAvatar)
              Row(
                children: [
                  // 头像
                  Container(
                    width: 40,
                    height: 40,
                    decoration: const BoxDecoration(
                      color: Colors.grey,
                      shape: BoxShape.circle,
                    ),
                  ),
                  const SizedBox(width: 12),
                  // 用户名和时间
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Container(
                          width: 100,
                          height: 14,
                          decoration: BoxDecoration(
                            color: Colors.grey,
                            borderRadius: BorderRadius.circular(7),
                          ),
                        ),
                        const SizedBox(height: 6),
                        Container(
                          width: 60,
                          height: 12,
                          decoration: BoxDecoration(
                            color: Colors.grey,
                            borderRadius: BorderRadius.circular(6),
                          ),
                        ),
                      ],
                    ),
                  ),
                  // 情绪标签
                  Container(
                    width: 60,
                    height: 24,
                    decoration: BoxDecoration(
                      color: Colors.grey,
                      borderRadius: BorderRadius.circular(12),
                    ),
                  ),
                ],
              ),

            if (showAvatar && showContent) const SizedBox(height: 16),

            // 内容区域
            if (showContent) ...[
              Container(
                width: double.infinity,
                height: 16,
                decoration: BoxDecoration(
                  color: Colors.grey,
                  borderRadius: BorderRadius.circular(8),
                ),
              ),
              const SizedBox(height: 8),
              Container(
                width: MediaQuery.of(context).size.width * 0.7,
                height: 16,
                decoration: BoxDecoration(
                  color: Colors.grey,
                  borderRadius: BorderRadius.circular(8),
                ),
              ),
              const SizedBox(height: 8),
              Container(
                width: MediaQuery.of(context).size.width * 0.5,
                height: 16,
                decoration: BoxDecoration(
                  color: Colors.grey,
                  borderRadius: BorderRadius.circular(8),
                ),
              ),
            ],

            if (showContent && showActions) const SizedBox(height: 16),

            // 操作按钮区域
            if (showActions)
              Row(
                children: [
                  // 涟漪按钮
                  Container(
                    width: 80,
                    height: 32,
                    decoration: BoxDecoration(
                      color: Colors.grey,
                      borderRadius: BorderRadius.circular(16),
                    ),
                  ),
                  const SizedBox(width: 12),
                  // 纸船按钮
                  Container(
                    width: 80,
                    height: 32,
                    decoration: BoxDecoration(
                      color: Colors.grey,
                      borderRadius: BorderRadius.circular(16),
                    ),
                  ),
                  const Spacer(),
                  // 更多按钮
                  Container(
                    width: 32,
                    height: 32,
                    decoration: const BoxDecoration(
                      color: Colors.grey,
                      shape: BoxShape.circle,
                    ),
                  ),
                ],
              ),
          ],
        ),
      ),
    );
  }
}

class UserCardSkeleton extends StatelessWidget {
  const UserCardSkeleton({super.key});

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(20),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            blurRadius: 10,
            offset: const Offset(0, 4),
          ),
        ],
      ),
      child: ShimmerLoading(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // 用户信息行
            Row(
              children: [
                // 头像
                Container(
                  width: 60,
                  height: 60,
                  decoration: const BoxDecoration(
                    color: Colors.grey,
                    shape: BoxShape.circle,
                  ),
                ),
                const SizedBox(width: 16),
                // 用户信息
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Container(
                        width: 120,
                        height: 16,
                        decoration: BoxDecoration(
                          color: Colors.grey,
                          borderRadius: BorderRadius.circular(8),
                        ),
                      ),
                      const SizedBox(height: 8),
                      Container(
                        width: 80,
                        height: 14,
                        decoration: BoxDecoration(
                          color: Colors.grey,
                          borderRadius: BorderRadius.circular(7),
                        ),
                      ),
                    ],
                  ),
                ),
                // 关注按钮
                Container(
                  width: 70,
                  height: 32,
                  decoration: BoxDecoration(
                    color: Colors.grey,
                    borderRadius: BorderRadius.circular(16),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),

            // 简介
            Container(
              width: double.infinity,
              height: 14,
              decoration: BoxDecoration(
                color: Colors.grey,
                borderRadius: BorderRadius.circular(7),
              ),
            ),
            const SizedBox(height: 8),
            Container(
              width: MediaQuery.of(context).size.width * 0.6,
              height: 14,
              decoration: BoxDecoration(
                color: Colors.grey,
                borderRadius: BorderRadius.circular(7),
              ),
            ),
            const SizedBox(height: 16),

            // 兼容性分数
            Container(
              width: double.infinity,
              height: 60,
              decoration: BoxDecoration(
                color: Colors.grey,
                borderRadius: BorderRadius.circular(12),
              ),
            ),
            const SizedBox(height: 12),

            // 推荐理由
            Container(
              width: double.infinity,
              height: 40,
              decoration: BoxDecoration(
                color: Colors.grey,
                borderRadius: BorderRadius.circular(12),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class WarmLoadingIndicator extends StatefulWidget {
  final List<String> messages;
  final Duration messageDuration;
  final Color color;
  final double size;

  const WarmLoadingIndicator({
    super.key,
    this.messages = const [
      '正在为你寻找温暖的内容...',
      '用心挑选最适合你的推荐...',
      '让我们一起发现美好...',
      '温柔地为你准备惊喜...',
      '每一个推荐都充满关怀...',
    ],
    this.messageDuration = const Duration(seconds: 2),
    this.color = AppTheme.primaryColor,
    this.size = 40.0,
  });

  @override
  State<WarmLoadingIndicator> createState() => _WarmLoadingIndicatorState();
}

class _WarmLoadingIndicatorState extends State<WarmLoadingIndicator>
    with TickerProviderStateMixin {
  late AnimationController _rotationController;
  late AnimationController _messageController;
  late Animation<double> _rotationAnimation;
  late Animation<double> _fadeAnimation;

  int _currentMessageIndex = 0;

  @override
  void initState() {
    super.initState();

    // 旋转动画控制器
    _rotationController = AnimationController(
      duration: const Duration(seconds: 2),
      vsync: this,
    )..repeat();

    _rotationAnimation = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(
      parent: _rotationController,
      curve: Curves.linear,
    ));

    // 消息切换动画控制器
    _messageController = AnimationController(
      duration: const Duration(milliseconds: 500),
      vsync: this,
    );

    _fadeAnimation = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(
      parent: _messageController,
      curve: Curves.easeInOut,
    ));

    _startMessageRotation();
  }

  void _startMessageRotation() {
    _messageController.forward().then((_) {
      Future.delayed(widget.messageDuration, () {
        if (mounted) {
          _messageController.reverse().then((_) {
            if (mounted) {
              setState(() {
                _currentMessageIndex =
                    (_currentMessageIndex + 1) % widget.messages.length;
              });
              _startMessageRotation();
            }
          });
        }
      });
    });
  }

  @override
  void dispose() {
    _rotationController.dispose();
    _messageController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        // 旋转的加载指示器
        AnimatedBuilder(
          animation: _rotationAnimation,
          builder: (context, child) {
            return Transform.rotate(
              angle: _rotationAnimation.value * 2 * math.pi,
              child: Container(
                width: widget.size,
                height: widget.size,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  gradient: LinearGradient(
                    colors: [
                      widget.color,
                      widget.color.withOpacity(0.3),
                      widget.color,
                    ],
                    stops: const [0.0, 0.5, 1.0],
                  ),
                ),
                child: Center(
                  child: Container(
                    width: widget.size * 0.6,
                    height: widget.size * 0.6,
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      color: Colors.white,
                      boxShadow: [
                        BoxShadow(
                          color: widget.color.withOpacity(0.3),
                          blurRadius: 8,
                          spreadRadius: 2,
                        ),
                      ],
                    ),
                    child: Icon(
                      Icons.favorite,
                      color: widget.color,
                      size: widget.size * 0.3,
                    ),
                  ),
                ),
              ),
            );
          },
        ),
        const SizedBox(height: 24),

        // 轮播消息
        AnimatedBuilder(
          animation: _fadeAnimation,
          builder: (context, child) {
            return Opacity(
              opacity: _fadeAnimation.value,
              child: Transform.translate(
                offset: Offset(0, (1 - _fadeAnimation.value) * 10),
                child: Text(
                  widget.messages[_currentMessageIndex],
                  textAlign: TextAlign.center,
                  style: const TextStyle(
                    fontSize: 16,
                    color: AppTheme.textSecondary,
                    fontWeight: FontWeight.w500,
                    height: 1.4,
                  ),
                ),
              ),
            );
          },
        ),
      ],
    );
  }
}

class SkeletonList extends StatelessWidget {
  final int itemCount;
  final Widget Function(int index) itemBuilder;
  final EdgeInsetsGeometry? padding;
  final double itemSpacing;

  const SkeletonList({
    super.key,
    this.itemCount = 5,
    required this.itemBuilder,
    this.padding,
    this.itemSpacing = 8.0,
  });

  @override
  Widget build(BuildContext context) {
    return ListView.separated(
      padding: padding ?? const EdgeInsets.all(16),
      itemCount: itemCount,
      separatorBuilder: (context, index) => SizedBox(height: itemSpacing),
      itemBuilder: (context, index) {
        return AnimationUtils.fadeSlideIn(
          index: index,
          child: itemBuilder(index),
        );
      },
    );
  }
}

class StoneSkeletonList extends StatelessWidget {
  final int itemCount;

  const StoneSkeletonList({
    super.key,
    this.itemCount = 5,
  });

  @override
  Widget build(BuildContext context) {
    return SkeletonList(
      itemCount: itemCount,
      itemBuilder: (index) => const StoneCardSkeleton(),
    );
  }
}

class UserSkeletonList extends StatelessWidget {
  final int itemCount;

  const UserSkeletonList({
    super.key,
    this.itemCount = 5,
  });

  @override
  Widget build(BuildContext context) {
    return SkeletonList(
      itemCount: itemCount,
      itemBuilder: (index) => const UserCardSkeleton(),
    );
  }
}

class ChartSkeleton extends StatelessWidget {
  final double height;

  const ChartSkeleton({
    super.key,
    this.height = 300,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      height: height,
      margin: const EdgeInsets.all(16),
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(20),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            blurRadius: 10,
            offset: const Offset(0, 4),
          ),
        ],
      ),
      child: ShimmerLoading(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // 标题
            Container(
              width: 150,
              height: 20,
              decoration: BoxDecoration(
                color: Colors.grey,
                borderRadius: BorderRadius.circular(10),
              ),
            ),
            const SizedBox(height: 8),
            Container(
              width: 200,
              height: 14,
              decoration: BoxDecoration(
                color: Colors.grey,
                borderRadius: BorderRadius.circular(7),
              ),
            ),
            const SizedBox(height: 24),

            // 图表区域
            Expanded(
              child: Container(
                width: double.infinity,
                decoration: BoxDecoration(
                  color: Colors.grey,
                  borderRadius: BorderRadius.circular(12),
                ),
              ),
            ),
            const SizedBox(height: 16),

            // 图例
            Row(
              children: List.generate(4, (index) => Expanded(
                child: Container(
                  margin: EdgeInsets.only(right: index < 3 ? 8 : 0),
                  height: 40,
                  decoration: BoxDecoration(
                    color: Colors.grey,
                    borderRadius: BorderRadius.circular(8),
                  ),
                ),
              )),
            ),
          ],
        ),
      ),
    );
  }
}