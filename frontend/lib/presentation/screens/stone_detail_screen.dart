/// 石头详情页
///
/// 展示单颗石头的完整内容、互动数据和纸船列表。

library;

import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../domain/entities/stone.dart';
import '../../data/datasources/interaction_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/mood_colors.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';
import '../widgets/water_background.dart';
import '../widgets/report_dialog.dart';
import '../widgets/similar_stones_section.dart';

/// 石头详情页面
///
/// 展示单颗石头的完整内容，支持：
/// - 情绪色彩背景（根据石头情绪类型动态渲染）
/// - 涟漪（点赞）和纸船（匿名回应）交互
/// - WebSocket 实时同步涟漪/纸船计数变化
/// - 相似石头推荐（HNSW 向量搜索）
/// - 举报功能
class StoneDetailScreen extends StatefulWidget {
  final Stone stone;
  final InteractionDataSource? interactionService;
  final RoomSubscriptionClient? wsClient;

  const StoneDetailScreen({
    super.key,
    required this.stone,
    this.interactionService,
    this.wsClient,
  });

  @override
  State<StoneDetailScreen> createState() => _StoneDetailScreenState();
}

/// 石头详情页面状态管理
///
/// 核心交互：
/// - 涟漪（点赞）：乐观更新计数 + 心形缩放动画 + 触觉反馈
/// - 纸船（评论）：乐观插入临时评论，失败时回滚并恢复输入内容
/// - WebSocket 实时同步：加入 stone:{id} 房间，监听计数变化和删除事件
/// - 返回时携带更新后的计数数据，供上级页面局部刷新
class _StoneDetailScreenState extends State<StoneDetailScreen>
    with TickerProviderStateMixin {
  final List<Map<String, dynamic>> _boats = [];
  final Map<String, int> _boatIndexById = {};
  final Set<String> _tempBoatIds = <String>{};
  bool _isLoading = false;
  bool _isSendingRipple = false;
  int _localRipplesCount = 0;
  int _localBoatsCount = 0;
  bool _hasInteraction = false; // 追踪是否有互动发生
  bool _hasRippled = false; // 当前用户是否已涟漪
  late final InteractionDataSource _interactionService;
  final TextEditingController _commentController = TextEditingController();
  final FocusNode _commentFocusNode = FocusNode();
  late final RoomSubscriptionClient _wsManager;

  // 监听器引用，用于正确移除
  late void Function(Map<String, dynamic>) _rippleListener;
  late void Function(Map<String, dynamic>) _boatListener;
  late void Function(Map<String, dynamic>) _rippleDeletedListener;
  late void Function(Map<String, dynamic>) _boatDeletedListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;
  late void Function(Map<String, dynamic>) _reconnectedListener;

  // 涟漪动画
  late AnimationController _heartAnimationController;
  late Animation<double> _heartScaleAnimation;

  /// 从石头的 moodType 或 sentimentScore 推断情绪类型
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
    _hasRippled = widget.stone.hasRippled;
    _interactionService = widget.interactionService ?? sl<InteractionService>();
    _wsManager = widget.wsClient ?? WebSocketManager();
    _loadBoats();
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

  /// 注册 WebSocket 监听器，加入石头专属房间接收实时事件
  void _setupWebSocketListener() {
    // 详情页可能从非湖面入口进入，确保实时连接被主动拉起
    unawaited(_wsManager.connect());
    // 加入该石头的 WS 房间，只接收与此石头相关的实时消息
    _wsManager.joinRoom('stone:${widget.stone.stoneId}');

    // 定义监听器函数 - 使用服务器返回的实际总数
    _rippleListener = (data) {
      if (extractStoneEntityId(data) == widget.stone.stoneId && mounted) {
        final serverCount = extractRippleCount(data);
        if (serverCount is int) {
          setState(() {
            _localRipplesCount = serverCount;
          });
        }
      }
    };

    _rippleDeletedListener = (data) {
      if (extractStoneEntityId(data) == widget.stone.stoneId && mounted) {
        final serverCount = extractRippleCount(data);
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
      if (extractStoneEntityId(data) == widget.stone.stoneId && mounted) {
        final serverCount = extractBoatCount(data);
        if (serverCount is int) {
          setState(() {
            _localBoatsCount = serverCount;
          });
        }
        // 重新加载评论列表
        _loadBoats();
      }
    };

    _boatDeletedListener = (data) {
      if (extractStoneEntityId(data) == widget.stone.stoneId && mounted) {
        final deletedBoatId = extractBoatEntityId(data);
        final serverCount = extractBoatCount(data);
        setState(() {
          if (serverCount is int) {
            _localBoatsCount = serverCount;
          } else {
            _localBoatsCount = (_localBoatsCount - 1).clamp(0, 99999);
          }
          if (deletedBoatId != null) {
            _removeBoatById(deletedBoatId);
          }
        });
      }
    };

    // 监听石头删除 - 当石头被删除时自动返回
    _stoneDeletedListener = (data) {
      if (extractStoneEntityId(data) == widget.stone.stoneId && mounted) {
        // 使用 addPostFrameCallback 确保在安全的时机执行导航
        WidgetsBinding.instance.addPostFrameCallback((_) {
          if (mounted && Navigator.canPop(context)) {
            Navigator.of(context)
                .pop({'deleted': true, 'stone_id': widget.stone.stoneId});
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

    // 监听重连成功，重新加入房间并刷新数据
    _reconnectedListener = (data) {
      if (mounted) {
        _wsManager.joinRoom('stone:${widget.stone.stoneId}');
        unawaited(_loadBoats());
      }
    };
    _wsManager.on('reconnected', _reconnectedListener);
  }

  void _rebuildBoatIndex() {
    _boatIndexById.clear();
    _tempBoatIds.clear();
    for (var i = 0; i < _boats.length; i++) {
      final normalized = normalizePayloadContract(_boats[i]);
      _boats[i] = normalized;
      final boatId = extractBoatEntityId(normalized);
      if (boatId == null) continue;
      _boatIndexById[boatId] = i;
      if (normalized['_isTemp'] == true) {
        _tempBoatIds.add(boatId);
      }
    }
  }

  bool _removeBoatById(String boatId) {
    final index = _boatIndexById[boatId];
    if (index == null) return false;
    _boats.removeAt(index);
    _rebuildBoatIndex();
    return true;
  }

  int _removeBoatIds(Set<String> boatIds) {
    if (boatIds.isEmpty) return 0;
    final retained = <Map<String, dynamic>>[];
    var removedCount = 0;

    for (final boat in _boats) {
      final boatId = extractBoatEntityId(boat);
      if (boatId != null && boatIds.contains(boatId)) {
        removedCount++;
        continue;
      }
      retained.add(boat);
    }

    if (removedCount == 0) return 0;
    _boats
      ..clear()
      ..addAll(retained);
    _rebuildBoatIndex();
    return removedCount;
  }

  int _removeTempBoats() {
    return _removeBoatIds(Set<String>.from(_tempBoatIds));
  }

  bool _finalizeFirstTempBoat(String boatId) {
    if (_tempBoatIds.isEmpty) return false;
    final tempBoatId = _tempBoatIds.first;
    final index = _boatIndexById[tempBoatId];
    if (index == null) return false;

    final updatedBoat = Map<String, dynamic>.from(_boats[index]);
    updatedBoat['boat_id'] = boatId;
    updatedBoat.remove('_isTemp');
    _boats[index] = updatedBoat;
    _rebuildBoatIndex();
    return true;
  }

  /// 离开石头房间并移除所有 WebSocket 监听器
  void _removeWebSocketListener() {
    // 离开该石头的 WS 房间
    _wsManager.leaveRoom('stone:${widget.stone.stoneId}');
    // 使用具体的监听器引用移除
    _wsManager.off('ripple_update', _rippleListener);
    _wsManager.off('ripple_deleted', _rippleDeletedListener);
    _wsManager.off('boat_update', _boatListener);
    _wsManager.off('boat_deleted', _boatDeletedListener);
    _wsManager.off('stone_deleted', _stoneDeletedListener);
    _wsManager.off('reconnected', _reconnectedListener);
  }

  @override
  void dispose() {
    _removeWebSocketListener();
    _commentController.dispose();
    _commentFocusNode.dispose();
    _heartAnimationController.dispose();
    super.dispose();
  }

  /// 加载纸船列表，同步本地纸船计数（优先使用 pagination.total）
  Future<void> _loadBoats() async {
    setState(() => _isLoading = true);

    try {
      final result = await _interactionService.getBoats(
        widget.stone.stoneId,
        page: 1,
        pageSize: 100,
      );

      if (result['success'] == true && mounted) {
        final rawBoats = result['boats'] ?? result['items'] ?? result['list'];
        final normalizedBoats = (rawBoats as List? ?? const [])
            .whereType<Map>()
            .map((boat) =>
                normalizePayloadContract(Map<String, dynamic>.from(boat)))
            .toList();
        setState(() {
          _boats.clear();
          _boats.addAll(normalizedBoats);
          _rebuildBoatIndex();

          // 同步本地纸船计数：优先用后端 pagination.total，否则用实际加载的数量
          if (result['pagination'] != null &&
              result['pagination']['total'] != null &&
              result['pagination']['total'] is int) {
            _localBoatsCount = result['pagination']['total'];
          } else if (_boats.isNotEmpty) {
            _localBoatsCount = _boats.length;
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

  /// 发送涟漪（点赞）
  Future<void> _sendRipple() async {
    if (_isSendingRipple || _hasRippled) return;

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
        final payload = result['data'] is Map<String, dynamic>
            ? result['data'] as Map<String, dynamic>
            : null;
        final alreadyRippled = result['already_rippled'] == true ||
            payload?['already_rippled'] == true;
        final resolvedRippleCount = extractRippleCount(result) ??
            (payload != null ? extractRippleCount(payload) : null);
        setState(() {
          _hasInteraction = true; // 标记有互动
          _hasRippled = true; // 标记已涟漪
          if (resolvedRippleCount != null) {
            _localRipplesCount = resolvedRippleCount;
          } else if (!alreadyRippled) {
            _localRipplesCount++;
          } else {
            _localRipplesCount = _localRipplesCount.clamp(0, 99999);
          }
        });

        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Row(
              children: [
                const Icon(Icons.waves, color: Colors.white, size: 20),
                const SizedBox(width: 8),
                Text(alreadyRippled ? '你已经点过这道涟漪了' : '你的共鸣已传递'),
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

  /// 发送纸船（评论），采用乐观更新策略
  ///
  /// 先插入临时评论到列表顶部提供即时反馈，后端确认后替换为正式数据。
  /// 失败时回滚临时评论并恢复输入框内容。
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
      _boats.insert(0, normalizePayloadContract(tempBoat)); // 插入到列表顶部
      _rebuildBoatIndex();
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
              _finalizeFirstTempBoat(result['data']['boat_id'].toString());
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
            final removedCount = _removeTempBoats();
            if (removedCount > 0) {
              _localBoatsCount =
                  (_localBoatsCount - removedCount).clamp(0, 99999);
            }
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
          final removedCount = _removeTempBoats();
          if (removedCount > 0) {
            _localBoatsCount =
                (_localBoatsCount - removedCount).clamp(0, 99999);
          }
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
    final isDark = Theme.of(context).brightness == Brightness.dark;
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
        body: Stack(
          children: [
            // 使用统一的水面背景
            const Positioned.fill(child: WaterBackground()),

            SafeArea(
              child: Column(
                children: [
                  // 石头内容卡片
                  Padding(
                    padding: const EdgeInsets.all(16),
                    child: Container(
                      decoration: BoxDecoration(
                        color: isDark
                            ? AppTheme.nightSurface.withValues(alpha: 0.95)
                            : Colors.white.withValues(alpha: 0.95),
                        borderRadius: BorderRadius.circular(24),
                        border: Border.all(
                          color: moodConfig.primary.withValues(alpha: 0.3),
                          width: 2,
                        ),
                        boxShadow: [
                          BoxShadow(
                            color: moodConfig.primary.withValues(alpha: 0.15),
                            blurRadius: 15,
                            offset: const Offset(0, 8),
                          ),
                        ],
                      ),
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
                                color:
                                    moodConfig.primary.withValues(alpha: 0.2),
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
                                  color: moodConfig.textColor
                                      .withValues(alpha: 0.6),
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
                                    color: moodConfig.primary
                                        .withValues(alpha: 0.1),
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
                                onTap: _hasRippled ? null : _sendRipple,
                                isLoading: _isSendingRipple,
                                color: _hasRippled
                                    ? moodConfig.rippleColor
                                    : moodConfig.rippleColor
                                        .withValues(alpha: 0.5),
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
                                      color:
                                          Colors.white.withValues(alpha: 0.3),
                                    ),
                                    const SizedBox(height: 16),
                                    Text(
                                      '还没有纸船漂来',
                                      style: TextStyle(
                                        fontSize: 14,
                                        color:
                                            Colors.white.withValues(alpha: 0.6),
                                      ),
                                    ),
                                    const SizedBox(height: 8),
                                    Text(
                                      '写下你的感受，让它随波漂流吧',
                                      style: TextStyle(
                                        fontSize: 12,
                                        color:
                                            Colors.white.withValues(alpha: 0.4),
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
                  // 相似石头推荐（HNSW向量搜索）
                  SimilarStonesSection(
                    stoneId: widget.stone.stoneId,
                    onStoneTap: (stone) {
                      Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (_) => StoneDetailScreen(stone: stone),
                        ),
                      );
                    },
                  ),
                  // 评论输入框
                  _buildCommentInput(moodConfig),
                ],
              ),
            ),
          ],
        ),
      ), // 添加闭合WillPopScope的child
    ); // 添加闭合WillPopScope
  }

  /// 构建涟漪/纸船交互按钮，支持加载态和缩放动画
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

  /// 构建单条纸船评论卡片，临时评论显示"发送中"标记
  Widget _buildBoatCard(Map<String, dynamic> boat) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
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
                  backgroundColor:
                      isDark ? const Color(0xFF16213E) : Colors.white,
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
                  color: isDark ? Colors.white38 : Colors.grey[500],
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
                      color:
                          isDark ? const Color(0xFFE8EAED) : Colors.grey[800],
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

  /// 底部纸船输入框，带情绪主题色边框
  Widget _buildCommentInput(MoodColorConfig moodConfig) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: isDark ? const Color(0xFF1B2838) : Colors.white,
        boxShadow: [
          BoxShadow(
            color: isDark
                ? Colors.transparent
                : Colors.black.withValues(alpha: 0.05),
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
                  hintStyle: TextStyle(
                      color: isDark ? Colors.white30 : Colors.grey[400]),
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(24),
                    borderSide: BorderSide(
                        color: moodConfig.primary.withValues(alpha: 0.3)),
                  ),
                  enabledBorder: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(24),
                    borderSide: BorderSide(
                        color: moodConfig.primary.withValues(alpha: 0.3)),
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
                  fillColor: isDark ? const Color(0xFF16213E) : Colors.grey[50],
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

  /// 将时间格式化为相对时间（刚刚、N分钟前、N小时前等）
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

  /// 弹出举报对话框
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
