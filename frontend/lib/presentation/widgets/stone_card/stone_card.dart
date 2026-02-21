// @file stone_card.dart
// @brief 石头卡片组件
// Created by 吴睿璐

import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../../domain/entities/stone.dart';
import '../../../utils/app_theme.dart';
import '../../../utils/mood_colors.dart';
import '../../screens/stone_detail_screen.dart';
import 'stone_card_header.dart';
import 'stone_card_content.dart';
import 'stone_card_actions.dart';
import 'stone_card_controller.dart';

class StoneCard extends StatefulWidget {
  final Stone stone;
  final VoidCallback? onRippleSuccess;
  final VoidCallback? onDeleted;

  const StoneCard({
    super.key,
    required this.stone,
    this.onRippleSuccess,
    this.onDeleted,
  });

  @override
  State<StoneCard> createState() => _StoneCardState();
}

class _StoneCardState extends State<StoneCard> with TickerProviderStateMixin {
  late AnimationController _floatController;
  late AnimationController _pressController;
  late Animation<double> _floatAnimation;
  late Animation<double> _scaleAnimation;
  late StoneCardController _cardController;
  final int _initDelay = math.Random().nextInt(1000);

  MoodType get _stoneMood {
    if (widget.stone.moodType != null) {
      return MoodColors.fromString(widget.stone.moodType);
    }
    if (widget.stone.sentimentScore != null) {
      return MoodColors.fromSentimentScore(widget.stone.sentimentScore!);
    }
    return MoodType.neutral;
  }

  @override
  void initState() {
    super.initState();
    _cardController = StoneCardController(
      stone: widget.stone,
      onStateChanged: () {
        if (mounted) setState(() {});
      },
    );
    _cardController.init();

    _floatController = AnimationController(vsync: this, duration: const Duration(seconds: 5));
    _floatAnimation = Tween<double>(begin: -4.0, end: 4.0).animate(
      CurvedAnimation(parent: _floatController, curve: Curves.easeInOutSine),
    );

    _pressController = AnimationController(vsync: this, duration: const Duration(milliseconds: 150));
    _scaleAnimation = Tween<double>(begin: 1.0, end: 0.97).animate(
      CurvedAnimation(parent: _pressController, curve: Curves.easeOutCubic),
    );

    Future.delayed(Duration(milliseconds: _initDelay), () {
      if (mounted) _floatController.repeat(reverse: true);
    });
  }

  @override
  void didUpdateWidget(covariant StoneCard oldWidget) {
    super.didUpdateWidget(oldWidget);
    _cardController.syncFromStone(widget.stone);
  }

  @override
  void dispose() {
    _floatController.dispose();
    _pressController.dispose();
    _cardController.dispose();
    super.dispose();
  }

  Future<void> _handleRipple() async {
    if (_cardController.hasRippled) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('你已经在这里泛起过涟漪了 ~'), backgroundColor: AppTheme.borderCyan),
      );
      return;
    }

    HapticFeedback.lightImpact();
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('你的共鸣已传递'), backgroundColor: AppTheme.primaryColor, duration: Duration(seconds: 1)),
    );

    final result = await _cardController.createRipple();
    if (!result['success'] && mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(result['message'] ?? '涟漪失败'), backgroundColor: AppTheme.errorColor),
      );
    } else if (result['success'] && widget.onRippleSuccess != null) {
      widget.onRippleSuccess!();
    }
  }

  void _showMoreMenu() {
    showModalBottomSheet(
      context: context,
      backgroundColor: Colors.transparent,
      builder: (ctx) => Container(
        decoration: const BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.vertical(top: Radius.circular(20)),
        ),
        child: SafeArea(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              if (_cardController.isAuthor)
                ListTile(
                  leading: const Icon(Icons.delete_outline, color: Colors.red),
                  title: const Text('沉没石头', style: TextStyle(color: Colors.red)),
                  onTap: () {
                    Navigator.pop(ctx);
                    _deleteStone();
                  },
                )
              else
                ListTile(
                  leading: const Icon(Icons.flag_outlined, color: Colors.orange),
                  title: const Text('举报内容'),
                  onTap: () {
                    Navigator.pop(ctx);
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(content: Text('举报已提交，我们会尽快处理'), backgroundColor: AppTheme.skyBlue),
                    );
                  },
                ),
              ListTile(
                leading: const Icon(Icons.close),
                title: const Text('取消'),
                onTap: () => Navigator.pop(ctx),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Future<void> _deleteStone() async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('沉没石头'),
        content: const Text('确定要让这颗石头永远沉入湖底吗？此操作无法撤销。'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx, false), child: const Text('取消')),
          TextButton(
            onPressed: () => Navigator.pop(ctx, true),
            style: TextButton.styleFrom(foregroundColor: Colors.red),
            child: const Text('沉没'),
          ),
        ],
      ),
    );

    if (confirmed == true && mounted) {
      final result = await _cardController.deleteStone();
      if (result['success']) {
        widget.onDeleted?.call();
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('已轻轻放下'), backgroundColor: AppTheme.skyBlue),
          );
        }
      } else if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text(result['message'] ?? '删除失败'), backgroundColor: AppTheme.errorColor),
        );
      }
    }
  }

  Future<void> _showBoatDialog() async {
    final controller = TextEditingController();
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('放一只纸船'),
        content: TextField(
          controller: controller,
          maxLines: 3,
          maxLength: 200,
          decoration: const InputDecoration(
            hintText: '写下你想说的话...',
            border: OutlineInputBorder(),
          ),
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx, false), child: const Text('取消')),
          TextButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('放入湖中'),
          ),
        ],
      ),
    );

    if (confirmed == true && controller.text.trim().isNotEmpty && mounted) {
      // 乐观更新：立即+1
      _cardController.localBoatsCount++;
      _cardController.onStateChanged?.call();

      final result = await _cardController.createBoat(controller.text.trim());
      if (result['success'] == true) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('纸船已启航，期待温暖的回响'), backgroundColor: AppTheme.primaryColor),
          );
        }
      } else {
        // 失败回滚
        _cardController.localBoatsCount--;
        _cardController.onStateChanged?.call();
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text(result['message'] ?? '发送失败'), backgroundColor: AppTheme.errorColor),
          );
        }
      }
    }
    controller.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final moodConfig = MoodColors.getConfig(_stoneMood);

    return AnimatedBuilder(
      animation: Listenable.merge([_floatController, _pressController]),
      builder: (context, child) => Transform.translate(
        offset: Offset(0, _floatAnimation.value),
        child: Transform.scale(scale: _scaleAnimation.value, child: child),
      ),
      child: GestureDetector(
        onTapDown: (_) => _pressController.forward(),
        onTapUp: (_) => _pressController.reverse(),
        onTapCancel: () => _pressController.reverse(),
        child: Card(
          elevation: 1,
          color: Theme.of(context).colorScheme.surfaceContainerLow,
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
          child: InkWell(
              onTap: () async {
                final result = await Navigator.push<dynamic>(
                  context,
                  PageRouteBuilder(
                    pageBuilder: (_, __, ___) => StoneDetailScreen(stone: widget.stone),
                    transitionsBuilder: (_, anim, __, child) => FadeTransition(
                      opacity: CurvedAnimation(parent: anim, curve: Curves.easeOut),
                      child: child,
                    ),
                    transitionDuration: const Duration(milliseconds: 300),
                  ),
                );
                if (result is Map && result['updated'] == true && mounted) {
                  _cardController.updateCounts(
                    result['rippleCount'] is int ? result['rippleCount'] : _cardController.localRipplesCount,
                    result['boatCount'] is int ? result['boatCount'] : _cardController.localBoatsCount,
                  );
                  widget.onRippleSuccess?.call();
                }
              },
              borderRadius: BorderRadius.circular(16),
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    StoneCardHeader(stone: widget.stone, moodConfig: moodConfig, onMorePressed: _showMoreMenu),
                    const SizedBox(height: 16),
                    StoneCardContent(stone: widget.stone, moodConfig: moodConfig),
                    const SizedBox(height: 16),
                    StoneCardActions(
                      moodConfig: moodConfig,
                      rippleCount: _cardController.localRipplesCount,
                      boatCount: _cardController.localBoatsCount,
                      hasRippled: _cardController.hasRippled,
                      onRipple: _handleRipple,
                      onBoat: _showBoatDialog,
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
