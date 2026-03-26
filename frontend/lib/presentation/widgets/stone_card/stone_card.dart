// 石头卡片组件

import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../../data/datasources/social_payload_normalizer.dart';
import '../../../domain/entities/stone.dart';
import '../../../data/datasources/websocket_manager.dart';
import '../../../utils/app_theme.dart';
import '../../../utils/mood_colors.dart';
import '../../screens/stone_detail_screen.dart';
import 'stone_card_header.dart';
import 'stone_card_content.dart';
import 'stone_card_actions.dart';
import 'stone_card_controller.dart';

class StoneCard extends StatefulWidget {
  final Stone stone;
  final void Function(int rippleCount, int boatCount)?
      onInteractionCountsChanged;
  final VoidCallback? onDeleted;

  const StoneCard({
    super.key,
    required this.stone,
    this.onInteractionCountsChanged,
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

    _floatController =
        AnimationController(vsync: this, duration: const Duration(seconds: 5));
    _floatAnimation = Tween<double>(begin: -4.0, end: 4.0).animate(
      CurvedAnimation(parent: _floatController, curve: Curves.easeInOutSine),
    );

    _pressController = AnimationController(
        vsync: this, duration: const Duration(milliseconds: 150));
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
        const SnackBar(
            content: Text('你已经在这里泛起过涟漪了 ~'),
            backgroundColor: AppTheme.borderCyan),
      );
      return;
    }

    HapticFeedback.lightImpact();
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
          content: Text('你的共鸣已传递'),
          backgroundColor: AppTheme.primaryColor,
          duration: Duration(seconds: 1)),
    );

    final result = await _cardController.createRipple();
    if (!result['success'] && mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
            content: Text(result['message'] ?? '涟漪失败'),
            backgroundColor: AppTheme.errorColor),
      );
    } else if (result['success'] && widget.onInteractionCountsChanged != null) {
      widget.onInteractionCountsChanged!(
        _cardController.localRipplesCount,
        _cardController.localBoatsCount,
      );
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
                  title:
                      const Text('沉没石头', style: TextStyle(color: Colors.red)),
                  onTap: () {
                    Navigator.pop(ctx);
                    _deleteStone();
                  },
                )
              else
                ListTile(
                  leading:
                      const Icon(Icons.flag_outlined, color: Colors.orange),
                  title: const Text('举报内容'),
                  onTap: () {
                    Navigator.pop(ctx);
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(
                          content: Text('举报已提交，我们会尽快处理'),
                          backgroundColor: AppTheme.skyBlue),
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
          TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: const Text('取消')),
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
            const SnackBar(
                content: Text('已轻轻放下'), backgroundColor: AppTheme.skyBlue),
          );
        }
      } else if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
              content: Text(result['message'] ?? '删除失败'),
              backgroundColor: AppTheme.errorColor),
        );
      }
    }
  }

  Future<void> _showBoatDialog() async {
    final TextEditingController boatController = TextEditingController();
    final wsManager = WebSocketManager();

    bool initialized = false;
    bool dialogActive = true;
    bool loading = true;
    bool sendingBoat = false;
    String? errorMessage;
    List<Map<String, dynamic>> boats = [];
    StateSetter? modalStateSetter;

    Future<void> loadBoats(StateSetter setModalState) async {
      if (!dialogActive) return;
      setModalState(() {
        loading = true;
        errorMessage = null;
      });

      final result = await _cardController.getBoats(page: 1, pageSize: 10);
      if (!mounted || !dialogActive) return;

      if (result['success'] == true) {
        setModalState(() {
          boats = extractNormalizedList(
            result,
            itemNormalizer: normalizeBoatPayload,
            listKeys: const ['boats'],
          );
          loading = false;
        });
      } else {
        setModalState(() {
          loading = false;
          errorMessage = result['message'] ?? '加载失败';
        });
      }
    }

    void onBoatRealtime(Map<String, dynamic> data) {
      if (!dialogActive) return;
      final stoneId = data['stone_id'] ?? data['boat']?['stone_id'];
      if (stoneId != widget.stone.stoneId) return;
      final setter = modalStateSetter;
      if (setter != null) {
        loadBoats(setter);
      }
    }

    wsManager.on('boat_update', onBoatRealtime);
    wsManager.on('boat_deleted', onBoatRealtime);

    await showModalBottomSheet(
      context: context,
      isScrollControlled: true,
      backgroundColor: Colors.transparent,
      builder: (ctx) => StatefulBuilder(
        builder: (ctx, setModalState) {
          modalStateSetter = setModalState;
          if (!initialized) {
            initialized = true;
            Future.microtask(() => loadBoats(setModalState));
          }

          return Container(
            padding: EdgeInsets.only(
              bottom: MediaQuery.of(ctx).viewInsets.bottom,
            ),
            decoration: const BoxDecoration(
              color: Colors.white,
              borderRadius: BorderRadius.vertical(top: Radius.circular(20)),
            ),
            child: Padding(
              padding: const EdgeInsets.all(20),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      const Icon(Icons.sailing_outlined,
                          color: AppTheme.borderCyan),
                      const SizedBox(width: 8),
                      Text(
                        '放一只纸船',
                        style: Theme.of(ctx)
                            .textTheme
                            .titleMedium
                            ?.copyWith(fontWeight: FontWeight.bold),
                      ),
                    ],
                  ),
                  const SizedBox(height: 12),
                  if (loading)
                    const Padding(
                      padding: EdgeInsets.symmetric(vertical: 12),
                      child: Center(child: CircularProgressIndicator()),
                    )
                  else if (errorMessage != null)
                    Padding(
                      padding: const EdgeInsets.symmetric(vertical: 12),
                      child: Text(errorMessage!,
                          style: const TextStyle(color: AppTheme.errorColor)),
                    )
                  else if (boats.isEmpty)
                    const Padding(
                      padding: EdgeInsets.symmetric(vertical: 12),
                      child: Text('还没有纸船，做第一个回应的人吧～'),
                    )
                  else
                    ConstrainedBox(
                      constraints: const BoxConstraints(maxHeight: 220),
                      child: ListView.separated(
                        shrinkWrap: true,
                        itemCount: boats.length,
                        separatorBuilder: (_, __) => const SizedBox(height: 12),
                        itemBuilder: (ctx, index) {
                          final boat = boats[index];
                          final author = boat['author'] ?? {};
                          final isTemp = boat['_isTemp'] == true;
                          final isAiReply = boat['is_ai_reply'] == true;
                          final moodConfig = MoodColors.getConfig(_stoneMood);

                          return Container(
                            padding: const EdgeInsets.all(12),
                            decoration: BoxDecoration(
                              color: Colors.white,
                              borderRadius: BorderRadius.circular(12),
                              border: Border.all(
                                color:
                                    moodConfig.primary.withValues(alpha: 0.3),
                                width: 1.5,
                              ),
                              boxShadow: [
                                BoxShadow(
                                  color:
                                      moodConfig.primary.withValues(alpha: 0.1),
                                  blurRadius: 8,
                                  offset: const Offset(0, 2),
                                ),
                              ],
                            ),
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Row(
                                  children: [
                                    Container(
                                      padding: const EdgeInsets.all(1),
                                      decoration: BoxDecoration(
                                        shape: BoxShape.circle,
                                        gradient: LinearGradient(
                                          colors: [
                                            moodConfig.primary
                                                .withValues(alpha: 0.6),
                                            moodConfig.rippleColor
                                                .withValues(alpha: 0.4),
                                          ],
                                        ),
                                      ),
                                      child: CircleAvatar(
                                        radius: 10,
                                        backgroundColor: Colors.white,
                                        child: Icon(
                                          isTemp
                                              ? Icons.hourglass_empty
                                              : isAiReply
                                                  ? Icons.auto_awesome
                                                  : Icons.person,
                                          size: 10,
                                          color: moodConfig.primary,
                                        ),
                                      ),
                                    ),
                                    const SizedBox(width: 6),
                                    Text(
                                      isAiReply
                                          ? '湖神'
                                          : author['nickname']?.toString() ??
                                              boat['sender_name']?.toString() ??
                                              '匿名旅人',
                                      style: TextStyle(
                                        fontWeight: FontWeight.bold,
                                        color: moodConfig.textColor,
                                        fontSize: 13,
                                      ),
                                    ),
                                    if (isAiReply && !isTemp) ...[
                                      const SizedBox(width: 6),
                                      Container(
                                        padding: const EdgeInsets.symmetric(
                                            horizontal: 4, vertical: 1),
                                        decoration: BoxDecoration(
                                          color: moodConfig.primary
                                              .withValues(alpha: 0.18),
                                          borderRadius:
                                              BorderRadius.circular(6),
                                        ),
                                        child: Text(
                                          'AI回复',
                                          style: TextStyle(
                                              fontSize: 9,
                                              color: moodConfig.primary),
                                        ),
                                      ),
                                    ],
                                    if (isTemp) ...[
                                      const SizedBox(width: 6),
                                      Container(
                                        padding: const EdgeInsets.symmetric(
                                            horizontal: 4, vertical: 1),
                                        decoration: BoxDecoration(
                                          color: moodConfig.primary
                                              .withValues(alpha: 0.2),
                                          borderRadius:
                                              BorderRadius.circular(6),
                                        ),
                                        child: Text(
                                          '发送中...',
                                          style: TextStyle(
                                              fontSize: 9,
                                              color: moodConfig.primary),
                                        ),
                                      ),
                                    ],
                                  ],
                                ),
                                const SizedBox(height: 8),
                                Container(
                                  padding: const EdgeInsets.all(8),
                                  decoration: BoxDecoration(
                                    color: moodConfig.primary
                                        .withValues(alpha: 0.05),
                                    borderRadius: BorderRadius.circular(8),
                                  ),
                                  child: Row(
                                    crossAxisAlignment:
                                        CrossAxisAlignment.start,
                                    children: [
                                      Icon(
                                        Icons.format_quote,
                                        size: 12,
                                        color: moodConfig.primary
                                            .withValues(alpha: 0.4),
                                      ),
                                      const SizedBox(width: 4),
                                      Expanded(
                                        child: Text(
                                          boat['content']?.toString() ?? '',
                                          style: const TextStyle(
                                            color: AppTheme.textSecondary,
                                            height: 1.4,
                                            fontSize: 13,
                                          ),
                                        ),
                                      ),
                                    ],
                                  ),
                                ),
                              ],
                            ),
                          );
                        },
                      ),
                    ),
                  const SizedBox(height: 16),
                  TextField(
                    controller: boatController,
                    maxLines: 3,
                    maxLength: 200,
                    decoration: InputDecoration(
                      hintText: '写下你想对TA说的话...',
                      border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(12)),
                      focusedBorder: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(12),
                        borderSide: const BorderSide(
                            color: AppTheme.borderCyan, width: 2),
                      ),
                    ),
                  ),
                  const SizedBox(height: 16),
                  SizedBox(
                    width: double.infinity,
                    child: ElevatedButton(
                      onPressed: sendingBoat
                          ? null
                          : () async {
                              final content = boatController.text.trim();
                              if (content.isEmpty) {
                                ScaffoldMessenger.of(context).showSnackBar(
                                  const SnackBar(
                                      content: Text('纸船上需要写点什么哦'),
                                      backgroundColor: AppTheme.warningColor),
                                );
                                return;
                              }

                              setModalState(() => sendingBoat = true);
                              ScaffoldMessenger.of(context).showSnackBar(
                                const SnackBar(
                                  content: Text('纸船正在漂向湖心... 🚣'),
                                  backgroundColor: AppTheme.primaryColor,
                                  duration: Duration(seconds: 2),
                                ),
                              );

                              try {
                                final result =
                                    await _cardController.createBoat(content);
                                if (result['success'] == true) {
                                  boatController.clear();
                                  if (mounted) {
                                    ScaffoldMessenger.of(context).showSnackBar(
                                      const SnackBar(
                                        content: Text('纸船已成功漂出~ ⛵'),
                                        backgroundColor: AppTheme.successColor,
                                        duration: Duration(seconds: 2),
                                      ),
                                    );
                                  }
                                  widget.onInteractionCountsChanged?.call(
                                    _cardController.localRipplesCount,
                                    _cardController.localBoatsCount,
                                  );
                                  await loadBoats(setModalState);
                                } else {
                                  if (mounted) {
                                    ScaffoldMessenger.of(context).showSnackBar(
                                      SnackBar(
                                        content:
                                            Text(result['message'] ?? '纸船发送失败'),
                                        backgroundColor: AppTheme.errorColor,
                                        duration: const Duration(seconds: 2),
                                      ),
                                    );
                                  }
                                }
                              } catch (error, stackTrace) {
                                FlutterError.reportError(
                                  FlutterErrorDetails(
                                    exception: error,
                                    stack: stackTrace,
                                    library: 'stone_card',
                                    context: ErrorDescription(
                                      'while sending paper boat from stone card',
                                    ),
                                  ),
                                );
                                if (mounted) {
                                  ScaffoldMessenger.of(context).showSnackBar(
                                    SnackBar(
                                      content: Text(
                                        error
                                                .toString()
                                                .replaceFirst(
                                                  RegExp(r'^Exception:\\s*'),
                                                  '',
                                                )
                                                .trim()
                                                .isEmpty
                                            ? '纸船发送失败，请稍后重试'
                                            : error
                                                .toString()
                                                .replaceFirst(
                                                  RegExp(r'^Exception:\\s*'),
                                                  '',
                                                )
                                                .trim(),
                                      ),
                                      backgroundColor: AppTheme.errorColor,
                                    ),
                                  );
                                }
                              } finally {
                                if (dialogActive) {
                                  setModalState(() => sendingBoat = false);
                                }
                              }
                            },
                      style: ElevatedButton.styleFrom(
                        backgroundColor: AppTheme.borderCyan,
                        foregroundColor: Colors.white,
                        padding: const EdgeInsets.symmetric(vertical: 12),
                        shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(12)),
                      ),
                      child: sendingBoat
                          ? const SizedBox(
                              width: 18,
                              height: 18,
                              child: CircularProgressIndicator(
                                strokeWidth: 2,
                                valueColor:
                                    AlwaysStoppedAnimation<Color>(Colors.white),
                              ),
                            )
                          : const Text('放出纸船', style: TextStyle(fontSize: 16)),
                    ),
                  ),
                ],
              ),
            ),
          );
        },
      ),
    );

    dialogActive = false;
    wsManager.off('boat_update', onBoatRealtime);
    wsManager.off('boat_deleted', onBoatRealtime);
    boatController.dispose();
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
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(16),
            side: BorderSide(
              color: moodConfig.primary.withValues(alpha: 0.35),
              width: 3.0,
            ),
          ),
          child: InkWell(
            onTap: () async {
              final result = await Navigator.push<dynamic>(
                context,
                PageRouteBuilder(
                  pageBuilder: (_, __, ___) =>
                      StoneDetailScreen(stone: widget.stone),
                  transitionsBuilder: (_, anim, __, child) => FadeTransition(
                    opacity:
                        CurvedAnimation(parent: anim, curve: Curves.easeOut),
                    child: child,
                  ),
                  transitionDuration: const Duration(milliseconds: 300),
                ),
              );
              if (result is Map && result['updated'] == true && mounted) {
                _cardController.updateCounts(
                  result['rippleCount'] is int
                      ? result['rippleCount']
                      : _cardController.localRipplesCount,
                  result['boatCount'] is int
                      ? result['boatCount']
                      : _cardController.localBoatsCount,
                );
                widget.onInteractionCountsChanged?.call(
                  _cardController.localRipplesCount,
                  _cardController.localBoatsCount,
                );
              }
            },
            borderRadius: BorderRadius.circular(16),
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  StoneCardHeader(
                      stone: widget.stone,
                      moodConfig: moodConfig,
                      onMorePressed: _showMoreMenu),
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
