// @file stone_card.dart
// @brief 石头卡片组件
// Created by 林子怡

import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../domain/entities/stone.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';
import '../../data/datasources/interaction_service.dart';
import '../screens/stone_detail_screen.dart';
import '../../utils/storage_util.dart';

class StoneCard extends StatefulWidget {
  final Stone stone;
  final VoidCallback? onRippleSuccess;
  final VoidCallback? onDeleted; // 删除回调

  const StoneCard({
    super.key,
    required this.stone,
    this.onRippleSuccess,
    this.onDeleted,
  });

  @override
  State<StoneCard> createState() => _StoneCardState();
}

class _StoneCardState extends State<StoneCard>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _animation;
  final InteractionService _interactionService = InteractionService();
  bool _hasRippled = false;
  int _localRipplesCount = 0;
  int _localBoatsCount = 0;
  String? _currentUserId;

  // 随机初始偏移，使列表中的浮动不同步
  final int _initDelay = math.Random().nextInt(1000);

  // 根据石头内容推断情绪
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
    _localRipplesCount = widget.stone.rippleCount;
    _localBoatsCount = widget.stone.boatCount;
    _checkCurrentUser();

    // 先初始化 controller
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 4),
    );

    _animation = Tween<double>(begin: -5.0, end: 5.0).animate(
      CurvedAnimation(parent: _controller, curve: Curves.easeInOut),
    );

    // 延迟启动动画，制造随机感
    Future.delayed(Duration(milliseconds: _initDelay), () {
      if (mounted) {
        _controller.forward();
      }
    });

    _controller.addStatusListener((status) {
      if (status == AnimationStatus.completed) {
        _controller.reverse();
      } else if (status == AnimationStatus.dismissed) {
        _controller.forward();
      }
    });
  }

  Future<void> _checkCurrentUser() async {
    final userId = await StorageUtil.getUserId();
    if (mounted) {
      setState(() {
        _currentUserId = userId;
      });
    }
  }

  // 外部调用更新计数
  void updateCounts(int ripples, int boats) {
    if (mounted) {
      setState(() {
        _localRipplesCount = ripples;
        _localBoatsCount = boats;
      });
    }
  }

  @override
  void didUpdateWidget(covariant StoneCard oldWidget) {
    super.didUpdateWidget(oldWidget);
    // 当父组件传入新的Stone数据时，同步本地计数
    if (oldWidget.stone.stoneId != widget.stone.stoneId ||
        oldWidget.stone.rippleCount != widget.stone.rippleCount ||
        oldWidget.stone.boatCount != widget.stone.boatCount) {
      setState(() {
        _localRipplesCount = widget.stone.rippleCount;
        _localBoatsCount = widget.stone.boatCount;
      });
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  // 涟漪（点赞）处理
  Future<void> _handleRipple(BuildContext context) async {
    if (_hasRippled) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
            content: Text('你已经在这里泛起过涟漪了 ~'),
            backgroundColor: AppTheme.borderCyan),
      );
      return;
    }

    // 乐观UI：先更新本地状态
    setState(() {
      _hasRippled = true;
      _localRipplesCount++;
    });
    HapticFeedback.lightImpact();

    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
          content: Text('涟漪荡漾开来...'),
          backgroundColor: AppTheme.primaryColor,
          duration: Duration(seconds: 1)),
    );

    // 后台发送请求
    final messenger = ScaffoldMessenger.of(context);
    final result = await _interactionService.createRipple(widget.stone.stoneId);
    if (!result['success'] && mounted) {
      // 失败则回滚
      setState(() {
        _hasRippled = false;
        _localRipplesCount--;
      });
      messenger.showSnackBar(
        SnackBar(
          content: Text(result['message'] ?? '涟漪失败'),
          backgroundColor: AppTheme.errorColor,
        ),
      );
    } else if (result['success'] && widget.onRippleSuccess != null) {
      widget.onRippleSuccess!();
    }
  }

  // 删除石头
  Future<void> _deleteStone(BuildContext context) async {
    final messenger = ScaffoldMessenger.of(context);
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('沉没石头'),
        content: const Text('确定要让这颗石头永远沉入湖底吗？此操作无法撤销。'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context, true),
            style: TextButton.styleFrom(foregroundColor: Colors.red),
            child: const Text('沉没'),
          ),
        ],
      ),
    );

    if (confirmed == true && mounted) {
      try {
        final result =
            await _interactionService.deleteStone(widget.stone.stoneId);

        if (result['success']) {
          if (widget.onDeleted != null) {
            widget.onDeleted!();
          }
          if (mounted) {
            messenger.showSnackBar(
              const SnackBar(
                content: Text('石头已沉入湖底'),
                backgroundColor: AppTheme.skyBlue,
              ),
            );
          }
        } else if (mounted) {
          messenger.showSnackBar(
            SnackBar(
              content: Text(result['message'] ?? '删除失败'),
              backgroundColor: AppTheme.errorColor,
            ),
          );
        }
      } catch (e) {
        if (mounted) {
          messenger.showSnackBar(
            SnackBar(
              content: Text('删除失败: $e'),
              backgroundColor: AppTheme.errorColor,
            ),
          );
        }
      }
    }
  }

  // 更多菜单
  void _showMoreMenu(BuildContext context) {
    bool isAuthor = false;
    if (_currentUserId != null) {
      isAuthor = _currentUserId == widget.stone.userId;
    }

    showModalBottomSheet(
      context: context,
      backgroundColor: Colors.transparent,
      builder: (context) => Container(
        decoration: const BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.vertical(top: Radius.circular(20)),
        ),
        child: SafeArea(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              if (isAuthor)
                ListTile(
                  leading: const Icon(Icons.delete_outline, color: Colors.red),
                  title:
                      const Text('沉没石头', style: TextStyle(color: Colors.red)),
                  onTap: () {
                    Navigator.pop(context);
                    _deleteStone(context);
                  },
                )
              else
                ListTile(
                  leading:
                      const Icon(Icons.flag_outlined, color: Colors.orange),
                  title: const Text('举报内容'),
                  onTap: () {
                    Navigator.pop(context);
                    // 举报逻辑
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(
                        content: Text('举报已提交，我们会尽快处理'),
                        backgroundColor: AppTheme.skyBlue,
                      ),
                    );
                  },
                ),
              ListTile(
                leading: const Icon(Icons.close),
                title: const Text('取消'),
                onTap: () => Navigator.pop(context),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // 纸船（评论）弹窗
  void _showBoatDialog(BuildContext context) {
    final TextEditingController boatController = TextEditingController();

    bool initialized = false;
    bool loading = true;
    String? errorMessage;
    List<Map<String, dynamic>> boats = [];

    Future<void> loadBoats(StateSetter setModalState) async {
      setModalState(() {
        loading = true;
        errorMessage = null;
      });

      final result = await _interactionService.getBoats(
        widget.stone.stoneId,
        page: 1,
        pageSize: 10,
      );

      if (result['success'] == true) {
        setModalState(() {
          boats = List<Map<String, dynamic>>.from(result['boats'] ?? []);
          loading = false;
        });
      } else {
        setModalState(() {
          loading = false;
          errorMessage = result['message'] ?? '加载失败';
        });
      }
    }

    showModalBottomSheet(
      context: context,
      isScrollControlled: true,
      backgroundColor: Colors.transparent,
      builder: (context) => StatefulBuilder(
        builder: (context, setModalState) {
          if (!initialized) {
            initialized = true;
            Future.microtask(() => loadBoats(setModalState));
          }

          return Container(
            padding: EdgeInsets.only(
              bottom: MediaQuery.of(context).viewInsets.bottom,
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
                      Text('放一只纸船',
                          style: Theme.of(context)
                              .textTheme
                              .titleMedium
                              ?.copyWith(fontWeight: FontWeight.bold)),
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
                        itemBuilder: (context, index) {
                          final boat = boats[index];
                          final author = boat['author'] ?? {};
                          final isTemp = boat['_isTemp'] == true;
                          final moodConfig = MoodColors.getConfig(_stoneMood);

                          // 治愈风格的评论卡片
                          return Container(
                            padding: const EdgeInsets.all(12),
                            decoration: BoxDecoration(
                              color: Colors.white,
                              borderRadius: BorderRadius.circular(12),
                              border: Border.all(
                                color: moodConfig.primary.withValues(alpha: 0.3),
                                width: 1.5,
                              ),
                              boxShadow: [
                                BoxShadow(
                                  color: moodConfig.primary.withValues(alpha: 0.1),
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
                                            moodConfig.primary.withValues(alpha: 0.6),
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
                                              : Icons.person,
                                          size: 10,
                                          color: moodConfig.primary,
                                        ),
                                      ),
                                    ),
                                    const SizedBox(width: 6),
                                    Text(
                                      author['nickname']?.toString() ?? '匿名旅人',
                                      style: TextStyle(
                                        fontWeight: FontWeight.bold,
                                        color: moodConfig.textColor,
                                        fontSize: 13,
                                      ),
                                    ),
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
                                            color: moodConfig.primary,
                                          ),
                                        ),
                                      ),
                                    ],
                                  ],
                                ),
                                const SizedBox(height: 8),
                                Container(
                                  padding: const EdgeInsets.all(8),
                                  decoration: BoxDecoration(
                                    color: moodConfig.primary.withValues(alpha: 0.05),
                                    borderRadius: BorderRadius.circular(8),
                                  ),
                                  child: Row(
                                    crossAxisAlignment:
                                        CrossAxisAlignment.start,
                                    children: [
                                      Icon(
                                        Icons.format_quote,
                                        size: 12,
                                        color:
                                            moodConfig.primary.withValues(alpha: 0.4),
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
                      onPressed: () async {
                        final content = boatController.text.trim();
                        final messenger = ScaffoldMessenger.of(context);
                        if (content.isEmpty) {
                          messenger.showSnackBar(
                            const SnackBar(
                              content: Text('纸船上需要写点什么哦'),
                              backgroundColor: AppTheme.warningColor,
                            ),
                          );
                          return;
                        }

                        // 乐观更新：立即在列表中显示新评论
                        final tempBoat = {
                          'boat_id':
                              'temp_${DateTime.now().millisecondsSinceEpoch}',
                          'content': content,
                          'created_at': DateTime.now().toIso8601String(),
                          'author': {'nickname': '我', 'is_anonymous': false},
                          '_isTemp': true,
                        };

                        setModalState(() {
                          boats.insert(0, tempBoat);
                        });

                        setState(() {
                          _localBoatsCount++;
                        });

                        // 清空输入框但不关闭对话框，让用户看到评论已添加
                        boatController.clear();

                        messenger.showSnackBar(
                          const SnackBar(
                              content: Text('纸船正在漂向湖心... 🚣'),
                              backgroundColor: AppTheme.primaryColor,
                              duration: Duration(seconds: 2)),
                        );

                        try {
                          final boatResult =
                              await _interactionService.createBoat(
                            stoneId: widget.stone.stoneId,
                            content: content,
                          );
                          if (boatResult['success'] == true) {
                            // 更新临时评论为正式评论
                            setModalState(() {
                              final index =
                                  boats.indexWhere((b) => b['_isTemp'] == true);
                              if (index >= 0 && boatResult['boat_id'] != null) {
                                boats[index]['boat_id'] = boatResult['boat_id'];
                                boats[index].remove('_isTemp');
                              }
                            });

                            if (mounted) {
                              messenger.showSnackBar(
                                const SnackBar(
                                  content: Text('纸船已成功漂出~ ⛵'),
                                  backgroundColor: AppTheme.successColor,
                                  duration: Duration(seconds: 2),
                                ),
                              );
                            }
                          } else {
                            // 失败时移除临时评论
                            setModalState(() {
                              boats.removeWhere((b) => b['_isTemp'] == true);
                            });
                            if (mounted) {
                              setState(() {
                                _localBoatsCount--;
                              });
                              messenger.showSnackBar(
                                SnackBar(
                                  content:
                                      Text(boatResult['message'] ?? '纸船发送失败'),
                                  backgroundColor: AppTheme.errorColor,
                                  duration: const Duration(seconds: 2),
                                ),
                              );
                            }
                          }
                        } catch (e) {
                          // 失败时移除临时评论
                          setModalState(() {
                            boats.removeWhere((b) => b['_isTemp'] == true);
                          });
                          if (mounted) {
                            setState(() {
                              _localBoatsCount--;
                            });
                            messenger.showSnackBar(
                              const SnackBar(
                                content: Text('网络错误，请检查网络连接'),
                                backgroundColor: AppTheme.errorColor,
                              ),
                            );
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
                      child: const Text('放出纸船', style: TextStyle(fontSize: 16)),
                    ),
                  ),
                ],
              ),
            ),
          );
        },
      ),
    ).then((_) => boatController.dispose());
  }

  Widget _buildTimeStatus() {
    final difference = DateTime.now().difference(widget.stone.createdAt);
    String text;
    Color color = AppTheme.textSecondary;
    FontWeight fontWeight = FontWeight.normal;

    if (difference.inMinutes < 60) {
      text = '刚刚';
    } else if (difference.inHours >= 23) {
      text = '即将沉没';
      color = Colors.red;
      fontWeight = FontWeight.bold;
    } else if (difference.inDays > 0) {
      text = '${difference.inDays}天前';
    } else {
      text = '${difference.inHours}小时前';
    }

    return Padding(
      padding: const EdgeInsets.only(right: 4.0),
      child: Text(
        text,
        style: TextStyle(
          color: color,
          fontSize: 12,
          fontWeight: fontWeight,
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final moodConfig = MoodColors.getConfig(_stoneMood);

    return AnimatedBuilder(
      animation: _controller,
      builder: (context, child) {
        return Transform.translate(
          offset: Offset(0, _animation.value),
          child: child,
        );
      },
      child: Container(
        decoration: BoxDecoration(
          // 使用情绪色彩的卡片背景
          color: moodConfig.cardColor.withValues(alpha: 0.95),
          borderRadius: BorderRadius.circular(24),
          // 使用情绪主色作为边框
          border: Border.all(
              color: moodConfig.primary.withValues(alpha: 0.6), width: 2.5),
          boxShadow: [
            BoxShadow(
              color: moodConfig.primary.withValues(alpha: 0.15),
              blurRadius: 15,
              offset: const Offset(0, 8),
            ),
          ],
        ),
        child: Material(
          color: Colors.transparent,
          child: InkWell(
            onTap: () async {
              // 导航到详情页并等待返回结果
              final result = await Navigator.push<dynamic>(
                context,
                MaterialPageRoute(
                  builder: (context) => StoneDetailScreen(stone: widget.stone),
                ),
              );

              // 如果有互动，同步最新的计数
              if (result is Map && result['updated'] == true && mounted) {
                setState(() {
                  if (result['rippleCount'] is int) {
                    _localRipplesCount = result['rippleCount'];
                  }
                  if (result['boatCount'] is int) {
                    _localBoatsCount = result['boatCount'];
                  }
                });
                // 通知父组件刷新以同步数据
                widget.onRippleSuccess?.call();
              }
            },
            borderRadius: BorderRadius.circular(24),
            child: Padding(
              padding: const EdgeInsets.all(20),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  // 头部信息
                  Row(
                    children: [
                      // 使用情绪色彩的头像
                      Container(
                        width: 36,
                        height: 36,
                        decoration: BoxDecoration(
                          shape: BoxShape.circle,
                          gradient: LinearGradient(
                            begin: Alignment.topLeft,
                            end: Alignment.bottomRight,
                            colors: [
                              moodConfig.primary.withValues(alpha: 0.7),
                              moodConfig.primary,
                            ],
                          ),
                          boxShadow: [
                            BoxShadow(
                              color: moodConfig.primary.withValues(alpha: 0.4),
                              blurRadius: 8,
                              offset: const Offset(2, 2),
                            )
                          ],
                          border: Border.all(color: Colors.white, width: 2),
                        ),
                        child: Icon(moodConfig.icon,
                            color: Colors.white, size: 20),
                      ),
                      const SizedBox(width: 12),
                      Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            widget.stone.authorNickname ?? '匿名旅人',
                            style: Theme.of(context)
                                .textTheme
                                .bodyMedium
                                ?.copyWith(
                                  fontWeight: FontWeight.bold,
                                  color: moodConfig.textColor,
                                  fontSize: 15,
                                ),
                          ),
                          // 情绪标签
                          if (widget.stone.moodType != null)
                            Container(
                              margin: const EdgeInsets.only(top: 2),
                              padding: const EdgeInsets.symmetric(
                                horizontal: 6,
                                vertical: 1,
                              ),
                              decoration: BoxDecoration(
                                color: moodConfig.primary.withValues(alpha: 0.15),
                                borderRadius: BorderRadius.circular(8),
                              ),
                              child: Text(
                                moodConfig.name,
                                style: TextStyle(
                                  fontSize: 10,
                                  color: moodConfig.primary,
                                  fontWeight: FontWeight.w500,
                                ),
                              ),
                            ),
                        ],
                      ),
                      const Spacer(),
                      // 新增：右上角状态倒计时
                      _buildTimeStatus(),
                      // 更多操作按钮
                      IconButton(
                        icon: Icon(Icons.more_horiz_rounded,
                            color: moodConfig.iconColor),
                        onPressed: () {
                          _showMoreMenu(context);
                        },
                      ),
                    ],
                  ),
                  const SizedBox(height: 16),

                  // 内容
                  Container(
                    width: double.infinity,
                    padding: const EdgeInsets.all(16),
                    decoration: BoxDecoration(
                      // 使用情绪色彩的浅色背景
                      color: moodConfig.lakeColor.withValues(alpha: 0.1),
                      borderRadius: BorderRadius.circular(16),
                    ),
                    child: Text(
                      widget.stone.content,
                      style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                            color: moodConfig.textColor,
                            height: 1.6,
                            letterSpacing: 0.3,
                            fontSize: 15,
                          ),
                      maxLines: 6,
                      overflow: TextOverflow.ellipsis,
                    ),
                  ),

                  // 标签
                  if (widget.stone.tags.isNotEmpty) ...[
                    const SizedBox(height: 16),
                    Wrap(
                      spacing: 8,
                      runSpacing: 8,
                      children: widget.stone.tags.map((tag) {
                        return Container(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 10,
                            vertical: 4,
                          ),
                          decoration: BoxDecoration(
                            color: moodConfig.primary.withValues(alpha: 0.1),
                            border: Border.all(
                                color: moodConfig.primary.withValues(alpha: 0.5)),
                            borderRadius: BorderRadius.circular(12),
                          ),
                          child: Text(
                            '# $tag',
                            style: TextStyle(
                              fontSize: 11,
                              color: moodConfig.primary,
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                        );
                      }).toList(),
                    ),
                  ],

                  const SizedBox(height: 16),

                  // 互动操作栏 - 涟漪、纸船、私聊
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceAround,
                    children: [
                      _buildActionButton(context, Icons.water_drop_outlined,
                          _localRipplesCount, '涟漪', () {
                        _handleRipple(context);
                      }, isActive: _hasRippled),
                      _buildActionButton(context, Icons.sailing_outlined,
                          _localBoatsCount, '纸船', () {
                        _showBoatDialog(context);
                      }),
                    ],
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildActionButton(
    BuildContext context,
    IconData icon,
    int count,
    String label,
    VoidCallback onTap, {
    bool isActive = false,
  }) {
    final moodConfig = MoodColors.getConfig(_stoneMood);

    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(20),
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
        decoration: BoxDecoration(
          color: moodConfig.primary.withValues(alpha: isActive ? 0.2 : 0.08),
          borderRadius: BorderRadius.circular(20),
          border: Border.all(
            color: moodConfig.primary.withValues(alpha: isActive ? 0.5 : 0.2),
            width: 1,
          ),
        ),
        child: Row(
          children: [
            Icon(
              icon,
              size: 20,
              color: isActive ? moodConfig.primary : moodConfig.iconColor,
            ),
            const SizedBox(width: 4),
            Text(
              count > 0 ? count.toString() : label,
              style: TextStyle(
                color: isActive ? moodConfig.primary : moodConfig.textColor,
                fontWeight: FontWeight.w600,
                fontSize: 13,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
