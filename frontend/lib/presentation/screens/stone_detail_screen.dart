// @file stone_detail_screen.dart
// @brief 石头详情页
// Created by 林子怡

library;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../domain/entities/stone.dart';
import '../../data/datasources/interaction_service.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../utils/mood_colors.dart';
import '../../utils/storage_util.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';
import '../widgets/report_dialog.dart';
import '../widgets/similar_stones_section.dart';
import '../../utils/animation_utils.dart';

class StoneDetailScreen extends StatefulWidget {
  final Stone stone;

  const StoneDetailScreen({super.key, required this.stone});

  @override
  State<StoneDetailScreen> createState() => _StoneDetailScreenState();
}

class _StoneDetailScreenState extends State<StoneDetailScreen>
    with TickerProviderStateMixin {
  final List<Map<String, dynamic>> _boats = [];
  bool _isLoading = false;
  bool _isSendingRipple = false;
  int _localRipplesCount = 0;
  int _localBoatsCount = 0;
  bool _hasInteraction = false; // 追踪是否有互动发生
  final InteractionService _interactionService = InteractionService();
  final AIRecommendationService _aiService = AIRecommendationService();
  final TextEditingController _commentController = TextEditingController();
  final FocusNode _commentFocusNode = FocusNode();
  late WebSocketManager _wsManager;

  // AI 相似推荐
  List<Map<String, dynamic>> _similarStones = [];
  bool _loadingSimilar = false;

  // 监听器引用，用于正确移除
  late void Function(Map<String, dynamic>) _rippleListener;
  late void Function(Map<String, dynamic>) _boatListener;
  late void Function(Map<String, dynamic>) _rippleDeletedListener;
  late void Function(Map<String, dynamic>) _boatDeletedListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;

  // 当前用户ID（用于过滤自己触发的WebSocket更新）
  String? _currentUserId;

  // 涟漪动画
  late AnimationController _heartAnimationController;
  late Animation<double> _heartScaleAnimation;

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
    _wsManager = WebSocketManager();
    _loadCurrentUser();
    _loadBoats();
    _loadSimilarStones();
    _trackView();
    _setupWebSocketListener();

    // 初始化心形动画
    _heartAnimationController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 300),
    );
    _heartScaleAnimation = Tween<double>(begin: 1.0, end: 1.3).animate(
      CurvedAnimation(
        parent: _heartAnimationController,
        curve: Curves.easeOutBack,
      ),
    );
  }

  /// 获取当前用户ID，用于过滤自己触发的WebSocket更新
  Future<void> _loadCurrentUser() async {
    _currentUserId = await StorageUtil.getUserId();
  }

  void _setupWebSocketListener() {
    // 定义监听器函数 - 使用服务器返回的实际总数，跳过自己触发的
    _rippleListener = (data) {
      if (data['stone_id'] == widget.stone.stoneId && mounted) {
        // 跳过自己触发的更新（已通过乐观更新处理）
        final triggeredBy = data['triggered_by']?.toString();
        if (triggeredBy == _currentUserId) return;

        final serverCount = data['ripple_count'];
        if (serverCount is int) {
          setState(() {
            _localRipplesCount = serverCount;
          });
        }
      }
    };

    _rippleDeletedListener = (data) {
      if (data['stone_id'] == widget.stone.stoneId && mounted) {
        // 跳过自己触发的更新
        final triggeredBy = data['triggered_by']?.toString();
        if (triggeredBy == _currentUserId) return;

        final serverCount = data['ripple_count'];
        setState(() {
          if (serverCount is int) {
            _localRipplesCount = serverCount;
          } else {
            _localRipplesCount = (_localRipplesCount - 1).clamp(0, 99999);
          }
        });
      }
    };

    _boatListener = (data) {
      if (data['stone_id'] == widget.stone.stoneId && mounted) {
        // 跳过自己触发的更新（已通过乐观更新处理）
        final triggeredBy = data['triggered_by']?.toString();
        if (triggeredBy == _currentUserId) return;

        final serverCount = data['boat_count'];
        if (serverCount is int) {
          setState(() {
            _localBoatsCount = serverCount;
          });
        }
        // 重新加载评论列表（其他用户的更新）
        _loadBoats();
      }
    };

    _boatDeletedListener = (data) {
      if (data['stone_id'] == widget.stone.stoneId && mounted) {
        // 跳过自己触发的更新
        final triggeredBy = data['triggered_by']?.toString();
        if (triggeredBy == _currentUserId) return;

        final deletedBoatId = data['boat_id'];
        final serverCount = data['boat_count'];
        setState(() {
          if (serverCount is int) {
            _localBoatsCount = serverCount;
          } else {
            _localBoatsCount = (_localBoatsCount - 1).clamp(0, 99999);
          }
          // 从本地列表中移除被删除的纸船
          _boats.removeWhere((b) => b['boat_id'] == deletedBoatId);
        });
      }
    };

    // 监听石头删除 - 当石头被删除时自动返回
    _stoneDeletedListener = (data) {
      if (data['stone_id'] == widget.stone.stoneId && mounted) {
        // 使用 addPostFrameCallback 确保在安全的时机执行导航
        WidgetsBinding.instance.addPostFrameCallback((_) {
          if (mounted && Navigator.canPop(context)) {
            Navigator.of(context).pop({'deleted': true, 'stone_id': widget.stone.stoneId});
            ScaffoldMessenger.of(context).showSnackBar(
              const SnackBar(
                content: Text('这颗石头已被删除'),
                backgroundColor: Colors.orange,
              ),
            );
          }
        });
      }
    };

    // 注册监听器
    _wsManager.on('ripple_update', _rippleListener);
    _wsManager.on('ripple_deleted', _rippleDeletedListener);
    _wsManager.on('boat_update', _boatListener);
    _wsManager.on('boat_deleted', _boatDeletedListener);
    _wsManager.on('stone_deleted', _stoneDeletedListener);
  }

  void _removeWebSocketListener() {
    // 使用具体的监听器引用移除
    _wsManager.off('ripple_update', _rippleListener);
    _wsManager.off('ripple_deleted', _rippleDeletedListener);
    _wsManager.off('boat_update', _boatListener);
    _wsManager.off('boat_deleted', _boatDeletedListener);
    _wsManager.off('stone_deleted', _stoneDeletedListener);
  }

  @override
  void dispose() {
    _removeWebSocketListener();
    _commentController.dispose();
    _commentFocusNode.dispose();
    _heartAnimationController.dispose();
    super.dispose();
  }

  Future<void> _loadBoats() async {
    setState(() => _isLoading = true);

    try {
      final result = await _interactionService.getBoats(
        widget.stone.stoneId,
        page: 1,
        pageSize: 100,
      );

      if (result['success'] == true && mounted) {
        setState(() {
          _boats.clear();
          _boats.addAll(List<Map<String, dynamic>>.from(result['boats'] ?? []));
          
          // 如果后端返回了总数，且不为0（避免刚加载时覆盖了实时更新），则同步更新本地计数
          if (result['pagination'] != null && 
              result['pagination']['total'] != null && 
              result['pagination']['total'] is int) {
             _localBoatsCount = result['pagination']['total'];
          }
          _isLoading = false;
        });
      }
    } catch (e) {
      debugPrint('Error loading boats: $e');
      if (mounted) {
        setState(() => _isLoading = false);
      }
    }
  }

  /// 加载 AI 相似石头推荐
  Future<void> _loadSimilarStones() async {
    setState(() => _loadingSimilar = true);
    try {
      final stones = await _aiService.getSimilarStones(widget.stone.stoneId);
      if (mounted) {
        setState(() {
          _similarStones = stones;
          _loadingSimilar = false;
        });
      }
    } catch (_) {
      if (mounted) setState(() => _loadingSimilar = false);
    }
  }

  /// 记录浏览行为（用于推荐引擎学习）
  Future<void> _trackView() async {
    await _aiService.trackInteraction(
      stoneId: widget.stone.stoneId,
      interactionType: 'view',
      reward: 0.3,
    );
  }

  /// 发送涟漪（点赞）
  Future<void> _sendRipple() async {
    if (_isSendingRipple) return;

    setState(() => _isSendingRipple = true);

    // 触发视觉反馈
    HapticFeedback.lightImpact();
    _heartAnimationController.forward().then((_) {
      _heartAnimationController.reverse();
    });

    try {
      final result =
          await _interactionService.createRipple(widget.stone.stoneId);

      if (result['success'] == true && mounted) {
        setState(() {
          _hasInteraction = true; // 标记有互动
          if (result['data'] != null && result['data']['ripple_count'] != null) {
             _localRipplesCount = result['data']['ripple_count'];
          } else {
             _localRipplesCount++;
          }
        });

        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: const Row(
              children: [
                Icon(Icons.waves, color: Colors.white, size: 20),
                SizedBox(width: 8),
                Text('你的共鸣已传递'),
              ],
            ),
            backgroundColor: MoodColors.getConfig(_stoneMood).primary,
            behavior: SnackBarBehavior.floating,
            duration: const Duration(seconds: 1),
          ),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('发送涟漪失败: $e')),
        );
      }
    } finally {
      if (mounted) {
        setState(() => _isSendingRipple = false);
      }
    }
  }

  Future<void> _sendComment() async {
    final content = _commentController.text.trim();
    if (content.isEmpty) return;

    // 先清空输入框并收起键盘，提供即时反馈
    final contentToSend = content;
    _commentController.clear();
    FocusScope.of(context).unfocus();
    HapticFeedback.mediumImpact();

    // 乐观更新：立即在列表中显示新评论
    final tempBoat = {
      'boat_id': 'temp_${DateTime.now().millisecondsSinceEpoch}',
      'content': contentToSend,
      'created_at': DateTime.now().toIso8601String(),
      'author': {'nickname': '我', 'is_anonymous': false},
      '_isTemp': true, // 标记为临时评论
    };

    setState(() {
      _boats.insert(0, tempBoat); // 插入到列表顶部
      _localBoatsCount++;
      _hasInteraction = true;
    });

    // 显示发送中提示
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: const Row(
          children: [
            SizedBox(
              width: 16,
              height: 16,
              child: CircularProgressIndicator(
                strokeWidth: 2,
                color: Colors.white,
              ),
            ),
            SizedBox(width: 12),
            Text('纸船正在漂向湖心...'),
          ],
        ),
        backgroundColor: MoodColors.getConfig(_stoneMood).primary,
        behavior: SnackBarBehavior.floating,
        duration: const Duration(seconds: 1),
      ),
    );

    try {
      final result = await _interactionService.createBoat(
        stoneId: widget.stone.stoneId,
        content: contentToSend,
      );

      if (result['success'] == true && mounted) {
        ScaffoldMessenger.of(context).clearSnackBars();
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: const Row(
              children: [
                Icon(Icons.sailing, color: Colors.white, size: 20),
                SizedBox(width: 8),
                Text('纸船已成功漂出~ ⛵'),
              ],
            ),
            backgroundColor: MoodColors.getConfig(_stoneMood).primary,
            behavior: SnackBarBehavior.floating,
            duration: const Duration(seconds: 2),
          ),
        );

          // 更新临时评论为正式评论（使用服务器返回的ID）
        if (result['data'] != null) {
           if (result['data']['boat_count'] != null) {
              setState(() {
                 _localBoatsCount = result['data']['boat_count'];
              });
           }
           
           if (result['data']['boat_id'] != null) {
            setState(() {
              final index = _boats.indexWhere((b) => b['_isTemp'] == true);
              if (index >= 0) {
                _boats[index]['boat_id'] = result['data']['boat_id'];
                _boats[index].remove('_isTemp');
              }
            });
           }
        }

        // 延迟刷新列表以获取最新数据
        Future.delayed(const Duration(milliseconds: 500), () {
          if (mounted) _loadBoats();
        });
      } else {
        if (mounted) {
          // 失败时移除临时评论
          setState(() {
            _boats.removeWhere((b) => b['_isTemp'] == true);
            _localBoatsCount--;
          });
          // 恢复输入内容
          _commentController.text = contentToSend;
          ScaffoldMessenger.of(context).clearSnackBars();
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(result['message'] ?? '评论发送失败，请重试'),
              backgroundColor: Colors.red[400],
              behavior: SnackBarBehavior.floating,
            ),
          );
        }
      }
    } catch (e) {
      if (mounted) {
        // 失败时移除临时评论
        setState(() {
          _boats.removeWhere((b) => b['_isTemp'] == true);
          _localBoatsCount--;
        });
        // 恢复输入内容
        _commentController.text = contentToSend;
        ScaffoldMessenger.of(context).clearSnackBars();
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: const Text('网络错误，请检查网络连接后重试'),
            backgroundColor: Colors.red[400],
            behavior: SnackBarBehavior.floating,
          ),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final moodConfig = MoodColors.getConfig(_stoneMood);

    return PopScope(
      canPop: false,
      onPopInvokedWithResult: (didPop, result) {
        if (didPop) return;
        // 返回时传递更新的数据
        if (_hasInteraction) {
          Navigator.of(context).pop({
            'updated': true,
            'rippleCount': _localRipplesCount,
            'boatCount': _localBoatsCount,
            'deleted': false,
          });
        } else {
          Navigator.of(context).pop();
        }
      },
      child: Scaffold(
        extendBodyBehindAppBar: true,
        appBar: AppBar(
          title: const Text('石头详情'),
          centerTitle: true,
          backgroundColor: Colors.transparent,
          elevation: 0,
          foregroundColor: Colors.white,
          leading: IconButton(
            icon: const Icon(Icons.arrow_back),
            onPressed: () {
              // 返回时传递更新的数据
              if (_hasInteraction) {
                Navigator.of(context).pop({
                  'updated': true,
                  'rippleCount': _localRipplesCount,
                  'boatCount': _localBoatsCount,
                  'deleted': false,
                });
              } else {
                Navigator.of(context).pop();
              }
            },
          ),
          actions: [
            IconButton(
              icon: const Icon(Icons.flag_outlined),
              onPressed: _showReportDialog,
              tooltip: '举报',
            ),
          ],
        ),
        body: SkyScaffold(
          showWater: true,
          child: SafeArea(
              child: Column(
                children: [
                  // 石头内容卡片
                  Padding(
                    padding: const EdgeInsets.all(16),
                    child: SkyGlassCard(
                      borderRadius: 20,
                      child: Container(
                        padding: const EdgeInsets.all(20),
                        child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          // 情绪标签
                          if (widget.stone.moodType != null)
                            Container(
                              padding: const EdgeInsets.symmetric(
                                horizontal: 12,
                                vertical: 4,
                              ),
                              decoration: BoxDecoration(
                                color: moodConfig.primary.withValues(alpha: 0.2),
                                borderRadius: BorderRadius.circular(16),
                              ),
                              child: Row(
                                mainAxisSize: MainAxisSize.min,
                                children: [
                                  Icon(
                                    moodConfig.icon,
                                    size: 16,
                                    color: moodConfig.primary,
                                  ),
                                  const SizedBox(width: 4),
                                  Text(
                                    moodConfig.name,
                                    style: TextStyle(
                                      fontSize: 12,
                                      color: moodConfig.primary,
                                      fontWeight: FontWeight.w500,
                                    ),
                                  ),
                                ],
                              ),
                            ),
                          const SizedBox(height: 12),
                          // 作者信息
                          Row(
                            children: [
                              CircleAvatar(
                                radius: 16,
                                backgroundColor:
                                    moodConfig.primary.withValues(alpha: 0.3),
                                child: Icon(
                                  Icons.person,
                                  size: 18,
                                  color: moodConfig.primary,
                                ),
                              ),
                              const SizedBox(width: 8),
                              Text(
                                widget.stone.authorNickname ?? '匿名旅人',
                                style: TextStyle(
                                  fontSize: 14,
                                  color: moodConfig.textColor,
                                  fontWeight: FontWeight.w500,
                                ),
                              ),
                              const Spacer(),
                              Text(
                                _formatTime(widget.stone.createdAt),
                                style: TextStyle(
                                  fontSize: 12,
                                  color: moodConfig.textColor.withValues(alpha: 0.6),
                                ),
                              ),
                            ],
                          ),
                          const SizedBox(height: 16),
                          // 石头内容
                          Text(
                            widget.stone.content,
                            style: TextStyle(
                              fontSize: 17,
                              height: 1.6,
                              color: moodConfig.textColor,
                            ),
                          ),
                          // AI标签
                          if (widget.stone.aiTags != null &&
                              widget.stone.aiTags!.isNotEmpty) ...[
                            const SizedBox(height: 16),
                            Wrap(
                              spacing: 8,
                              runSpacing: 4,
                              children: widget.stone.aiTags!.map((tag) {
                                return Container(
                                  padding: const EdgeInsets.symmetric(
                                    horizontal: 8,
                                    vertical: 2,
                                  ),
                                  decoration: BoxDecoration(
                                    color: moodConfig.primary.withValues(alpha: 0.1),
                                    borderRadius: BorderRadius.circular(8),
                                  ),
                                  child: Text(
                                    '#$tag',
                                    style: TextStyle(
                                      fontSize: 11,
                                      color: moodConfig.primary,
                                    ),
                                  ),
                                );
                              }).toList(),
                            ),
                          ],
                          const SizedBox(height: 16),
                          // 互动按钮
                          Row(
                            children: [
                              // 涟漪按钮
                              _buildInteractionButton(
                                icon: Icons.waves,
                                count: _localRipplesCount,
                                onTap: _sendRipple,
                                isLoading: _isSendingRipple,
                                color: moodConfig.rippleColor,
                                animation: _heartScaleAnimation,
                              ),
                              const SizedBox(width: 24),
                              // 纸船计数（使用实际总数，非本地列表长度）
                              _buildInteractionButton(
                                icon: Icons.sailing,
                                count: _localBoatsCount,
                                onTap: () => _commentFocusNode.requestFocus(),
                                color: moodConfig.primary,
                              ),
                            ],
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
                  // 相似石头推荐（AI共鸣推荐）
                  SimilarStonesSection(
                    stoneId: widget.stone.stoneId,
                    onStoneTap: (stone) {
                      Navigator.push(
                        context,
                        SkyPageRoute(page: StoneDetailScreen(stone: stone)),
                      );
                    },
                  ),
                  // 评论标题
                  Padding(
                    padding: const EdgeInsets.symmetric(horizontal: 16),
                    child: Row(
                      children: [
                        const Icon(
                          Icons.sailing_outlined,
                          size: 20,
                          color: Colors.white70,
                        ),
                        const SizedBox(width: 8),
                        Text(
                          '纸船回复 ($_localBoatsCount)',
                          style: const TextStyle(
                            fontSize: 16,
                            fontWeight: FontWeight.w600,
                            color: Colors.white,
                          ),
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(height: 8),
                  // 评论列表
                  Expanded(
                    child: _isLoading
                        ? const Center(
                            child:
                                CircularProgressIndicator(color: Colors.white),
                          )
                        : _boats.isEmpty
                            ? Center(
                                child: Column(
                                  mainAxisAlignment: MainAxisAlignment.center,
                                  children: [
                                    Icon(
                                      Icons.sailing_outlined,
                                      size: 64,
                                      color: Colors.white.withValues(alpha: 0.3),
                                    ),
                                    const SizedBox(height: 16),
                                    Text(
                                      '还没有纸船漂来',
                                      style: TextStyle(
                                        fontSize: 14,
                                        color: Colors.white.withValues(alpha: 0.6),
                                      ),
                                    ),
                                    const SizedBox(height: 8),
                                    Text(
                                      '写下你的感受，让它随波漂流吧',
                                      style: TextStyle(
                                        fontSize: 12,
                                        color: Colors.white.withValues(alpha: 0.4),
                                      ),
                                    ),
                                  ],
                                ),
                              )
                            : ListView.builder(
                                padding:
                                    const EdgeInsets.symmetric(horizontal: 16),
                                itemCount: _boats.length,
                                itemBuilder: (context, index) {
                                  return _buildBoatCard(_boats[index]);
                                },
                              ),
                  ),
                  // AI 相似推荐
                  if (_similarStones.isNotEmpty || _loadingSimilar)
                    _buildSimilarStonesSection(moodConfig),
                  // 评论输入框
                  _buildCommentInput(moodConfig),
                ],
              ),
            ),
          ),
        ),
      ), // PopScope
  }

  Widget _buildInteractionButton({
    required IconData icon,
    required int count,
    required VoidCallback? onTap,
    required Color color,
    bool isLoading = false,
    Animation<double>? animation,
  }) {
    Widget button = InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(20),
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
        decoration: BoxDecoration(
          color: Colors.white.withValues(alpha: 0.2),
          borderRadius: BorderRadius.circular(20),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            if (isLoading)
              SizedBox(
                width: 18,
                height: 18,
                child: CircularProgressIndicator(
                  strokeWidth: 2,
                  color: color,
                ),
              )
            else
              Icon(icon, size: 18, color: color),
            const SizedBox(width: 6),
            Text(
              '$count',
              style: TextStyle(
                fontSize: 14,
                color: color,
                fontWeight: FontWeight.w500,
              ),
            ),
          ],
        ),
      ),
    );

    if (animation != null) {
      return AnimatedBuilder(
        animation: animation,
        builder: (context, child) {
          return Transform.scale(
            scale: animation.value,
            child: child,
          );
        },
        child: button,
      );
    }

    return button;
  }

  Widget _buildBoatCard(Map<String, dynamic> boat) {
    final moodConfig = MoodColors.getConfig(_stoneMood);
    final isTemp = boat['_isTemp'] == true;

    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(16),
        // 治愈风格的渐变边框
        border: Border.all(
          color: moodConfig.primary.withValues(alpha: 0.4),
          width: 1.5,
        ),
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            Colors.white.withValues(alpha: isTemp ? 0.85 : 0.95),
            moodConfig.primary.withValues(alpha: 0.05),
          ],
        ),
        boxShadow: [
          BoxShadow(
            color: moodConfig.primary.withValues(alpha: 0.15),
            blurRadius: 12,
            offset: const Offset(0, 4),
          ),
          // 内发光效果
          BoxShadow(
            color: Colors.white.withValues(alpha: 0.8),
            blurRadius: 4,
            spreadRadius: -2,
            offset: const Offset(0, -1),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              // 治愈风格的头像
              Container(
                padding: const EdgeInsets.all(2),
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  gradient: LinearGradient(
                    colors: [
                      moodConfig.primary.withValues(alpha: 0.6),
                      moodConfig.rippleColor.withValues(alpha: 0.4),
                    ],
                  ),
                ),
                child: CircleAvatar(
                  radius: 14,
                  backgroundColor: Colors.white,
                  child: Icon(
                    isTemp ? Icons.hourglass_empty : Icons.person,
                    size: 14,
                    color: moodConfig.primary,
                  ),
                ),
              ),
              const SizedBox(width: 8),
              Text(
                boat['author']?['nickname'] ?? '匿名旅人',
                style: TextStyle(
                  fontWeight: FontWeight.w600,
                  fontSize: 14,
                  color: moodConfig.textColor,
                ),
              ),
              if (isTemp) ...[
                const SizedBox(width: 8),
                Container(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
                  decoration: BoxDecoration(
                    color: moodConfig.primary.withValues(alpha: 0.2),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Text(
                    '发送中...',
                    style: TextStyle(
                      fontSize: 10,
                      color: moodConfig.primary,
                    ),
                  ),
                ),
              ],
              const Spacer(),
              Text(
                _formatTime(DateTime.tryParse(boat['created_at'] ?? '') ??
                    DateTime.now()),
                style: TextStyle(
                  fontSize: 11,
                  color: Colors.grey[500],
                ),
              ),
            ],
          ),
          const SizedBox(height: 10),
          // 评论内容带治愈装饰
          Container(
            padding: const EdgeInsets.all(12),
            decoration: BoxDecoration(
              color: moodConfig.primary.withValues(alpha: 0.05),
              borderRadius: BorderRadius.circular(12),
              border: Border.all(
                color: moodConfig.primary.withValues(alpha: 0.1),
                width: 1,
              ),
            ),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Icon(
                  Icons.format_quote,
                  size: 16,
                  color: moodConfig.primary.withValues(alpha: 0.4),
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: Text(
                    boat['content'] ?? '',
                    style: TextStyle(
                      fontSize: 15,
                      height: 1.5,
                      color: Colors.grey[800],
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  /// 相似石头推荐区域 - 类光遇风格：柔光半透明卡片
  Widget _buildSimilarStonesSection(MoodColorConfig moodConfig) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // 标题行 - 柔光风格
          Row(
            children: [
              Container(
                padding: const EdgeInsets.all(4),
                decoration: BoxDecoration(
                  color: Colors.white.withValues(alpha: 0.15),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Icon(Icons.auto_awesome, size: 14,
                    color: Colors.white.withValues(alpha: 0.8)),
              ),
              const SizedBox(width: 8),
              Text('相似的心声',
                style: TextStyle(
                  fontSize: 13, fontWeight: FontWeight.w500,
                  color: Colors.white.withValues(alpha: 0.7),
                  letterSpacing: 1.2,
                )),
              const Spacer(),
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
                decoration: BoxDecoration(
                  color: moodConfig.primary.withValues(alpha: 0.2),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Text('AI · 语义共鸣',
                  style: TextStyle(fontSize: 9,
                    color: Colors.white.withValues(alpha: 0.5))),
              ),
            ],
          ),
          const SizedBox(height: 10),
          // 横向滚动卡片列表
          if (_loadingSimilar)
            Center(child: Padding(
              padding: const EdgeInsets.all(16),
              child: SizedBox(width: 20, height: 20,
                child: CircularProgressIndicator(strokeWidth: 1.5,
                  color: Colors.white.withValues(alpha: 0.4))),
            ))
          else
            SizedBox(
              height: 100,
              child: ListView.separated(
                scrollDirection: Axis.horizontal,
                itemCount: _similarStones.length,
                separatorBuilder: (_, __) => const SizedBox(width: 10),
                itemBuilder: (context, index) {
                  final stone = _similarStones[index];
                  final content = stone['content'] as String? ?? '';
                  final score = (stone['score'] as num?)?.toDouble() ?? 0;
                  final mood = stone['mood_type'] as String? ?? 'neutral';
                  final moodType = MoodColors.fromString(mood);
                  final cardColor = MoodColors.getConfig(moodType).primary;
                  return GestureDetector(
                    onTap: () {
                      // 记录点击交互
                      final sid = stone['stone_id'] as String? ?? '';
                      if (sid.isNotEmpty) {
                        _aiService.trackInteraction(
                          stoneId: sid,
                          interactionType: 'click_similar',
                          reward: 0.8,
                        );
                      }
                      // 导航到详情
                      try {
                        final s = Stone.fromJson(stone);
                        Navigator.push(context, SkyPageRoute(page: StoneDetailScreen(stone: s)));
                      } catch (_) {}
                    },
                    child: Container(
                      width: 160,
                      padding: const EdgeInsets.all(12),
                      decoration: BoxDecoration(
                        gradient: LinearGradient(
                          colors: [
                            cardColor.withValues(alpha: 0.15),
                            Colors.white.withValues(alpha: 0.05),
                          ],
                          begin: Alignment.topLeft,
                          end: Alignment.bottomRight,
                        ),
                        borderRadius: BorderRadius.circular(14),
                        border: Border.all(
                          color: Colors.white.withValues(alpha: 0.1)),
                      ),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          // 相似度指示
                          Row(
                            children: [
                              Container(
                                width: 6, height: 6,
                                decoration: BoxDecoration(
                                  color: cardColor.withValues(alpha: 0.8),
                                  shape: BoxShape.circle,
                                  boxShadow: [BoxShadow(
                                    color: cardColor.withValues(alpha: 0.4),
                                    blurRadius: 4)],
                                ),
                              ),
                              const SizedBox(width: 4),
                              Text('${(score * 100).toInt()}% 共鸣',
                                style: TextStyle(fontSize: 9,
                                  color: Colors.white.withValues(alpha: 0.5))),
                            ],
                          ),
                          const SizedBox(height: 8),
                          // 内容预览
                          Expanded(
                            child: Text(
                              content.length > 50
                                  ? '${content.substring(0, 50)}...' : content,
                              style: TextStyle(
                                fontSize: 12, height: 1.4,
                                color: Colors.white.withValues(alpha: 0.7)),
                              maxLines: 3, overflow: TextOverflow.ellipsis,
                            ),
                          ),
                        ],
                      ),
                    ),
                  );
                },
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildCommentInput(MoodColorConfig moodConfig) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        boxShadow: [
          BoxShadow(
            color: Colors.black.withValues(alpha: 0.05),
            blurRadius: 10,
            offset: const Offset(0, -5),
          ),
        ],
      ),
      child: SafeArea(
        child: Row(
          children: [
            Expanded(
              child: TextField(
                controller: _commentController,
                focusNode: _commentFocusNode,
                maxLines: null,
                maxLength: 200,
                decoration: InputDecoration(
                  hintText: '写一张纸船漂给TA...',
                  hintStyle: TextStyle(color: Colors.grey[400]),
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(24),
                    borderSide:
                        BorderSide(color: moodConfig.primary.withValues(alpha: 0.3)),
                  ),
                  enabledBorder: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(24),
                    borderSide:
                        BorderSide(color: moodConfig.primary.withValues(alpha: 0.3)),
                  ),
                  focusedBorder: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(24),
                    borderSide: BorderSide(color: moodConfig.primary, width: 2),
                  ),
                  contentPadding: const EdgeInsets.symmetric(
                    horizontal: 20,
                    vertical: 12,
                  ),
                  counterText: '',
                  filled: true,
                  fillColor: Colors.grey[50],
                ),
              ),
            ),
            const SizedBox(width: 12),
            Container(
              decoration: BoxDecoration(
                gradient: LinearGradient(
                  colors: [
                    moodConfig.primary,
                    moodConfig.primary.withValues(alpha: 0.8),
                  ],
                ),
                shape: BoxShape.circle,
                boxShadow: [
                  BoxShadow(
                    color: moodConfig.primary.withValues(alpha: 0.3),
                    blurRadius: 8,
                    offset: const Offset(0, 2),
                  ),
                ],
              ),
              child: IconButton(
                onPressed: _sendComment,
                icon: const Icon(Icons.sailing),
                color: Colors.white,
                iconSize: 24,
              ),
            ),
          ],
        ),
      ),
    );
  }

  String _formatTime(DateTime time) {
    final now = DateTime.now();
    final diff = now.difference(time);

    if (diff.inMinutes < 1) {
      return '刚刚';
    } else if (diff.inHours < 1) {
      return '${diff.inMinutes}分钟前';
    } else if (diff.inDays < 1) {
      return '${diff.inHours}小时前';
    } else if (diff.inDays < 7) {
      return '${diff.inDays}天前';
    } else {
      return '${time.month}月${time.day}日';
    }
  }

  void _showReportDialog() async {
    final result = await showDialog<bool>(
      context: context,
      builder: (context) => ReportDialog(
        targetType: 'stone',
        targetId: widget.stone.stoneId,
      ),
    );

    // 如果举报成功，result会是true（在ReportDialog中设置）
    if (result == true && mounted) {
      // 举报成功的提示已经在ReportDialog中显示了
    }
  }
}
