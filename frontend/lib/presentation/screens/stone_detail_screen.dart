/// 石头详情页
///
/// 展示单颗石头的完整内容、互动数据和纸船列表。

library;

import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../domain/entities/stone.dart';
import '../../data/datasources/interaction_service.dart';
import '../../data/datasources/social_payload_normalizer.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/mood_colors.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';
import '../widgets/water_background.dart';
import '../widgets/report_dialog.dart';

/// 石头详情页面
///
/// 展示单颗石头的完整内容，支持：
/// - 情绪色彩背景（根据石头情绪类型动态渲染）
/// - 涟漪（点赞）和纸船（匿名回应）交互
/// - WebSocket 实时同步涟漪/纸船计数变化
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
/// - 涟漪（点赞）：只在服务端确认后更新状态
/// - 纸船（评论）：发送成功后重新拉取服务端列表
/// - WebSocket 实时同步：加入 stone:{id} 房间，监听计数变化和删除事件
/// - 返回时携带更新后的计数数据，供上级页面局部刷新
class _StoneDetailScreenState extends State<StoneDetailScreen>
    with TickerProviderStateMixin {
  final List<Map<String, dynamic>> _boats = [];
  final Map<String, int> _boatIndexById = {};
  bool _isLoading = false;
  bool _isSendingRipple = false;
  bool _isSendingComment = false;
  int _localRipplesCount = 0;
  int _localBoatsCount = 0;
  String? _boatsErrorMessage;
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

  void _reportUiError(
    Object error,
    StackTrace stackTrace,
    String context,
  ) {
    FlutterError.reportError(
      FlutterErrorDetails(
        exception: error,
        stack: stackTrace,
        library: 'heartlake',
        context: ErrorDescription(context),
      ),
    );
  }

  String _resolveMessage(
    Object error, {
    required String fallback,
  }) {
    final message = error.toString().trim();
    if (message.isEmpty) {
      return fallback;
    }
    return message;
  }

  int? _extractExplicitPaginationTotal(Map<String, dynamic> payload) {
    int? parseTotal(dynamic candidate) {
      if (candidate is! Map) return null;
      final pagination = candidate['pagination'] ?? candidate['meta'];
      if (pagination is! Map) return null;
      final total = pagination['total'] ??
          pagination['count'] ??
          pagination['total_count'];
      if (total is int) return total;
      if (total is num) return total.toInt();
      if (total is String) return int.tryParse(total.trim());
      return null;
    }

    return parseTotal(payload) ?? parseTotal(payload['data']);
  }

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

  bool _isAiBoat(Map<String, dynamic> boat) => boat['is_ai_reply'] == true;

  String _boatDisplayName(Map<String, dynamic> boat) {
    if (_isAiBoat(boat)) {
      return '湖神';
    }
    return boat['author']?['nickname']?.toString() ??
        boat['sender_name']?.toString() ??
        '匿名旅人';
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
        if (serverCount is int) {
          setState(() {
            _localRipplesCount = serverCount;
          });
        }
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
        unawaited(_loadBoats(showLoading: false));
      }
    };

    _boatDeletedListener = (data) {
      if (extractStoneEntityId(data) == widget.stone.stoneId && mounted) {
        final deletedBoatId = extractBoatEntityId(data);
        final serverCount = extractBoatCount(data);
        setState(() {
          if (serverCount is int) {
            _localBoatsCount = serverCount;
          }
          if (deletedBoatId != null) {
            _removeBoatById(deletedBoatId);
          }
        });
        if (serverCount is! int) {
          unawaited(_loadBoats(showLoading: false));
        }
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
        unawaited(_loadBoats(showLoading: false));
      }
    };
    _wsManager.on('reconnected', _reconnectedListener);
  }

  void _rebuildBoatIndex() {
    _boatIndexById.clear();
    for (var i = 0; i < _boats.length; i++) {
      final normalized = normalizePayloadContract(_boats[i]);
      _boats[i] = normalized;
      final boatId = extractBoatEntityId(normalized);
      if (boatId == null) continue;
      _boatIndexById[boatId] = i;
    }
  }

  bool _removeBoatById(String boatId) {
    final index = _boatIndexById[boatId];
    if (index == null) return false;
    _boats.removeAt(index);
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

  /// 加载纸船列表，只接受服务端显式返回的总数。
  Future<void> _loadBoats({bool showLoading = true}) async {
    if (mounted) {
      setState(() {
        if (showLoading) {
          _isLoading = true;
        }
        _boatsErrorMessage = null;
      });
    }

    try {
      final result = await _interactionService.getBoats(
        widget.stone.stoneId,
        page: 1,
        pageSize: 100,
      );

      if (result['success'] != true) {
        throw StateError(result['message']?.toString() ?? '加载纸船失败');
      }

      final normalizedBoats = extractNormalizedList(
        result,
        itemNormalizer: normalizeBoatPayload,
        listKeys: const ['boats'],
      );
      final resolvedBoatCount =
          extractBoatCount(result) ?? _extractExplicitPaginationTotal(result);
      if (!mounted) return;
      setState(() {
        _boats
          ..clear()
          ..addAll(normalizedBoats);
        _rebuildBoatIndex();
        if (resolvedBoatCount != null) {
          _localBoatsCount = resolvedBoatCount;
        }
        _boatsErrorMessage = null;
        _isLoading = false;
      });
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'loading boats for stone detail');
      if (!mounted) return;
      setState(() {
        _boatsErrorMessage = _resolveMessage(
          error,
          fallback: '加载纸船失败，请重试',
        );
        _isLoading = false;
      });
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
      } else if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(result['message']?.toString() ?? '发送涟漪失败'),
            behavior: SnackBarBehavior.floating,
          ),
        );
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'sending ripple from stone detail');
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(
              '发送涟漪失败: ${_resolveMessage(error, fallback: '请稍后重试')}',
            ),
          ),
        );
      }
    } finally {
      if (mounted) {
        setState(() => _isSendingRipple = false);
      }
    }
  }

  /// 发送纸船（评论），仅以服务端确认结果为准。
  Future<void> _sendComment() async {
    final content = _commentController.text.trim();
    if (content.isEmpty || _isSendingComment) return;

    final contentToSend = content;
    setState(() {
      _isSendingComment = true;
    });
    HapticFeedback.mediumImpact();

    try {
      final result = await _interactionService.createBoat(
        stoneId: widget.stone.stoneId,
        content: contentToSend,
      );

      if (result['success'] == true) {
        final payload = result['data'] is Map<String, dynamic>
            ? result['data'] as Map<String, dynamic>
            : null;
        final resolvedBoatCount =
            payload == null ? null : extractBoatCount(payload);

        if (!mounted) return;
        setState(() {
          _hasInteraction = true;
          if (resolvedBoatCount != null) {
            _localBoatsCount = resolvedBoatCount;
          }
        });
        _commentController.clear();
        FocusScope.of(context).unfocus();
        await _loadBoats(showLoading: false);
        if (!mounted) return;
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: const Row(
              children: [
                Icon(Icons.sailing, color: Colors.white, size: 20),
                SizedBox(width: 8),
                Text('纸船已成功漂出'),
              ],
            ),
            backgroundColor: MoodColors.getConfig(_stoneMood).primary,
            behavior: SnackBarBehavior.floating,
            duration: const Duration(seconds: 2),
          ),
        );
        return;
      }

      if (mounted) {
        ScaffoldMessenger.of(context).clearSnackBars();
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(result['message']?.toString() ?? '评论发送失败，请重试'),
            backgroundColor: Colors.red[400],
            behavior: SnackBarBehavior.floating,
          ),
        );
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'sending boat from stone detail');
      if (mounted) {
        ScaffoldMessenger.of(context).clearSnackBars();
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(
              _resolveMessage(error, fallback: '评论发送失败，请稍后重试'),
            ),
            backgroundColor: Colors.red[400],
            behavior: SnackBarBehavior.floating,
          ),
        );
      }
    } finally {
      if (mounted) {
        setState(() {
          _isSendingComment = false;
        });
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
                  if (_boatsErrorMessage != null)
                    Padding(
                      padding: const EdgeInsets.fromLTRB(16, 0, 16, 8),
                      child: _buildBoatErrorBanner(),
                    ),
                  // 评论列表
                  Expanded(
                    child: _isLoading
                        ? const Center(
                            child:
                                CircularProgressIndicator(color: Colors.white),
                          )
                        : _boats.isEmpty
                            ? _boatsErrorMessage != null
                                ? _buildBoatLoadFailureState()
                                : Center(
                                    child: Column(
                                      mainAxisAlignment:
                                          MainAxisAlignment.center,
                                      children: [
                                        Icon(
                                          Icons.sailing_outlined,
                                          size: 64,
                                          color: Colors.white
                                              .withValues(alpha: 0.3),
                                        ),
                                        const SizedBox(height: 16),
                                        Text(
                                          '还没有纸船漂来',
                                          style: TextStyle(
                                            fontSize: 14,
                                            color: Colors.white
                                                .withValues(alpha: 0.6),
                                          ),
                                        ),
                                        const SizedBox(height: 8),
                                        Text(
                                          '写下你的感受，让它随波漂流吧',
                                          style: TextStyle(
                                            fontSize: 12,
                                            color: Colors.white
                                                .withValues(alpha: 0.4),
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

  Widget _buildBoatErrorBanner() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      decoration: BoxDecoration(
        color: Colors.orange.withValues(alpha: 0.16),
        borderRadius: BorderRadius.circular(14),
        border: Border.all(color: Colors.orange.withValues(alpha: 0.45)),
      ),
      child: Row(
        children: [
          const Icon(Icons.warning_amber_rounded, color: Colors.orangeAccent),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              _boatsErrorMessage!,
              style: const TextStyle(color: Colors.white, fontSize: 12),
            ),
          ),
          TextButton(
            onPressed: _isLoading ? null : () => _loadBoats(showLoading: false),
            child: const Text('重试'),
          ),
        ],
      ),
    );
  }

  Widget _buildBoatLoadFailureState() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 24),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(
              Icons.error_outline,
              size: 56,
              color: Colors.orangeAccent,
            ),
            const SizedBox(height: 12),
            Text(
              _boatsErrorMessage ?? '加载纸船失败',
              textAlign: TextAlign.center,
              style: const TextStyle(
                fontSize: 14,
                color: Colors.white,
                fontWeight: FontWeight.w600,
              ),
            ),
            const SizedBox(height: 12),
            OutlinedButton(
              onPressed: _isLoading ? null : () => _loadBoats(),
              style: OutlinedButton.styleFrom(
                foregroundColor: Colors.white,
                side: const BorderSide(color: Colors.white70),
              ),
              child: const Text('重新加载'),
            ),
          ],
        ),
      ),
    );
  }

  /// 构建单条纸船评论卡片
  Widget _buildBoatCard(Map<String, dynamic> boat) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final moodConfig = MoodColors.getConfig(_stoneMood);

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
            Colors.white.withValues(alpha: 0.95),
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
                    _isAiBoat(boat) ? Icons.auto_awesome : Icons.person,
                    size: 14,
                    color: moodConfig.primary,
                  ),
                ),
              ),
              const SizedBox(width: 8),
              Text(
                _boatDisplayName(boat),
                style: TextStyle(
                  fontWeight: FontWeight.w600,
                  fontSize: 14,
                  color: moodConfig.textColor,
                ),
              ),
              if (_isAiBoat(boat)) ...[
                const SizedBox(width: 8),
                Container(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
                  decoration: BoxDecoration(
                    color: moodConfig.primary.withValues(alpha: 0.16),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Text(
                    'AI回复',
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
                enabled: !_isSendingComment,
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
                onPressed: _isSendingComment ? null : _sendComment,
                icon: _isSendingComment
                    ? const SizedBox(
                        width: 20,
                        height: 20,
                        child: CircularProgressIndicator(
                          strokeWidth: 2,
                          color: Colors.white,
                        ),
                      )
                    : const Icon(Icons.sailing),
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
