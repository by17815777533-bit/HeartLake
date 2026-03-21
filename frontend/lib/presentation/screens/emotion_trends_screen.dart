// 情绪趋势可视化页面
//
// 展示用户情绪变化的时序折线图。
import 'dart:async';

import 'package:flutter/material.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../data/datasources/edge_ai_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';
import '../../utils/storage_util.dart';
import '../widgets/water_background.dart';
import '../widgets/emotion_pulse_widget.dart';

/// 情绪趋势可视化页面
///
/// 以湖面风格展示用户近期的情绪变化趋势，数据来源：
/// - 后端 AI 推荐引擎的情绪趋势 API
/// - 端侧 EdgeAI 的本地情绪分析结果
///
/// 通过 WebSocket 实时监听新石头事件，自动追加最新情绪数据点。
/// 包含情绪脉搏动画组件 [EmotionPulseWidget] 实时反映当前情绪状态。
class EmotionTrendsScreen extends StatefulWidget {
  const EmotionTrendsScreen({super.key});
  @override
  State<EmotionTrendsScreen> createState() => _EmotionTrendsScreenState();
}

/// 情绪趋势可视化页面的状态管理
///
/// 使用 [AIRecommendationService] 和 [EdgeAIService] 获取趋势数据和隐私预算，
/// 通过 WebSocket 监听当前用户的石头增删事件，debounce 后自动刷新。
class _EmotionTrendsScreenState extends State<EmotionTrendsScreen>
    with SingleTickerProviderStateMixin {
  final AIRecommendationService _aiService = sl<AIRecommendationService>();
  final EdgeAIService _edgeService = sl<EdgeAIService>();
  final WebSocketManager _wsManager = WebSocketManager();
  late AnimationController _fadeController;
  Map<String, dynamic> _trends = {};
  Map<String, dynamic>? _privacyInfo;
  bool _loading = true;
  String? _currentUserId;
  late final void Function(Map<String, dynamic>) _onNewStoneListener;
  late final void Function(Map<String, dynamic>) _onStoneDeletedListener;
  late final void Function(Map<String, dynamic>) _onReconnectedListener;
  Timer? _refreshDebounce;
  int _refreshSeq = 0;

  @override
  void initState() {
    super.initState();
    _fadeController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 800),
    );
    _onNewStoneListener = (payload) {
      if (!_isCurrentUserEvent(payload)) {
        return;
      }
      _scheduleRealtimeRefresh(withFollowUp: true);
    };
    _onStoneDeletedListener = (payload) {
      if (!_isCurrentUserEvent(payload)) {
        return;
      }
      _scheduleRealtimeRefresh();
    };
    _onReconnectedListener = (_) {
      _scheduleRealtimeRefresh(withFollowUp: true);
    };
    _initRealtimeSync();
    _loadData();
  }

  @override
  void dispose() {
    _wsManager.leaveRoom('lake');
    _wsManager.off('new_stone', _onNewStoneListener);
    _wsManager.off('stone_deleted', _onStoneDeletedListener);
    _wsManager.off('reconnected', _onReconnectedListener);
    _refreshDebounce?.cancel();
    _fadeController.dispose();
    super.dispose();
  }

  /// 初始化 WebSocket 实时同步：获取用户ID、加入 lake 房间、注册事件监听
  Future<void> _initRealtimeSync() async {
    _currentUserId = await StorageUtil.getUserId();
    if (!_wsManager.isConnected) {
      _wsManager.connect();
    }
    _wsManager.joinRoom('lake');
    _wsManager.on('new_stone', _onNewStoneListener);
    _wsManager.on('stone_deleted', _onStoneDeletedListener);
    _wsManager.on('reconnected', _onReconnectedListener);
  }

  /// 判断 WebSocket 事件是否由当前用户触发，用于过滤无关事件
  bool _isCurrentUserEvent(Map<String, dynamic> payload) {
    final currentUserId = _currentUserId;
    if (currentUserId == null || currentUserId.isEmpty) {
      return true;
    }
    return extractPayloadUserId(payload) == currentUserId;
  }

  /// 防抖刷新：300ms 内合并多次事件，[withFollowUp] 为 true 时 2s 后再补刷一次
  void _scheduleRealtimeRefresh({bool withFollowUp = false}) {
    _refreshDebounce?.cancel();
    _refreshDebounce = Timer(const Duration(milliseconds: 300), () {
      _loadData();
      if (!withFollowUp) return;
      final token = ++_refreshSeq;
      Future.delayed(const Duration(seconds: 2), () {
        if (!mounted || token != _refreshSeq) return;
        _loadData();
      });
    });
  }

  /// 并行加载情绪趋势数据和差分隐私预算，加载完成后触发淡入动画
  Future<void> _loadData() async {
    try {
      final results = await Future.wait([
        _aiService.getEmotionTrends(),
        _edgeService.getPrivacyBudget(),
      ]);
      final trendPayload = results[0];
      final privacyPayload = results[1];
      if (mounted) {
        setState(() {
          _trends = _normalizeTrendPayload(trendPayload);
          _privacyInfo = (privacyPayload['success'] == true &&
                  privacyPayload['data'] is Map)
              ? Map<String, dynamic>.from(privacyPayload['data'] as Map)
              : null;
          _loading = false;
        });
        _fadeController.forward();
      }
    } catch (_) {
      if (mounted) setState(() => _loading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: isDark ? Colors.white : AppTheme.textPrimary,
        title: const Text(
          '情绪趋势',
          style: TextStyle(
            fontSize: 18,
            fontWeight: FontWeight.w500,
            letterSpacing: 1,
          ),
        ),
        centerTitle: true,
      ),
      body: Stack(
        children: [
          const WaterBackground(),
          SafeArea(
            child: _loading
                ? const Center(
                    child: CircularProgressIndicator(
                      color: AppTheme.primaryColor,
                      strokeWidth: 2,
                    ),
                  )
                : FadeTransition(
                    opacity: _fadeController,
                    child: _buildContent(),
                  ),
          ),
        ],
      ),
    );
  }

  /// 构建页面主体内容：情绪脉搏 + 隐私徽章 + 趋势卡片
  Widget _buildContent() {
    return SingleChildScrollView(
      physics: const BouncingScrollPhysics(),
      padding: const EdgeInsets.symmetric(horizontal: 20),
      child: Column(
        children: [
          const SizedBox(height: 12),
          // 情绪脉搏
          const EmotionPulseWidget(size: 140),
          const SizedBox(height: 20),
          // 隐私徽章
          _buildPrivacyBadge(),
          const SizedBox(height: 24),
          // 趋势卡片
          _buildTrendCards(),
          const SizedBox(height: 40),
        ],
      ),
    );
  }

  /// 构建差分隐私保护状态徽章，展示隐私预算消耗百分比和剩余量
  Widget _buildPrivacyBadge() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final consumed = (_privacyInfo?['consumed'] as num?)?.toDouble() ?? 0.0;
    final total = (_privacyInfo?['total_budget'] as num?)?.toDouble() ?? 10.0;
    final remaining = (_privacyInfo?['remaining_budget'] as num?)?.toDouble() ??
        (total - consumed);
    final percent =
        total > 0 ? (consumed / total * 100).clamp(0.0, 100.0) : 0.0;

    return Card(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      elevation: 0,
      color: Theme.of(context)
          .colorScheme
          .surfaceContainerHighest
          .withValues(alpha: 0.7),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Icon(Icons.shield_outlined,
                size: 14, color: AppTheme.secondaryColor),
            const SizedBox(width: 6),
            Text(
              '差分隐私保护中',
              style: TextStyle(
                fontSize: 11,
                color: isDark ? Colors.white70 : AppTheme.textSecondary,
                letterSpacing: 0.5,
              ),
            ),
            const SizedBox(width: 8),
            Text(
              '已用 ${percent.toStringAsFixed(0)}% · 剩余${remaining.toStringAsFixed(1)}',
              style: TextStyle(
                fontSize: 11,
                color: AppTheme.secondaryColor.withValues(alpha: 0.8),
              ),
            ),
          ],
        ),
      ),
    );
  }

  /// 构建情绪分布条形图和湖神关怀提示卡片
  Widget _buildTrendCards() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final distribution = _trends['distribution'] is Map
        ? Map<String, dynamic>.from(_trends['distribution'] as Map)
        : <String, dynamic>{};
    final insights = _trends['insights'] as List? ?? [];

    if (distribution.isEmpty && insights.isEmpty) {
      return Card(
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Text(
            '最近还没有足够的情绪样本，继续记录几次心情后这里会自动更新。',
            style: TextStyle(
                fontSize: 13,
                color: isDark ? Colors.white70 : AppTheme.textSecondary,
                height: 1.5),
          ),
        ),
      );
    }

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // 情绪分布
        if (distribution.isNotEmpty) ...[
          Text(
            '情绪分布',
            style: TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.w500,
              color: isDark ? Colors.white : AppTheme.textPrimary,
              letterSpacing: 1,
            ),
          ),
          const SizedBox(height: 12),
          Card(
            shape:
                RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
            elevation: 1,
            child: Padding(
              padding: const EdgeInsets.all(14),
              child: Column(
                children: distribution.entries.map((e) {
                  final value = (e.value as num).toDouble().clamp(0.0, 1.0);
                  final color = _moodColor(e.key);
                  return Padding(
                    padding: const EdgeInsets.only(bottom: 8),
                    child: Row(
                      children: [
                        SizedBox(
                          width: 50,
                          child: Text(
                            _moodLabel(e.key),
                            style: TextStyle(
                              fontSize: 11,
                              color: isDark
                                  ? Colors.white70
                                  : AppTheme.textSecondary,
                            ),
                          ),
                        ),
                        Expanded(
                          child: Container(
                            height: 6,
                            decoration: BoxDecoration(
                              color:
                                  AppTheme.textTertiary.withValues(alpha: 0.15),
                              borderRadius: BorderRadius.circular(3),
                            ),
                            child: FractionallySizedBox(
                              alignment: Alignment.centerLeft,
                              widthFactor: value,
                              child: Container(
                                decoration: BoxDecoration(
                                  gradient: LinearGradient(colors: [
                                    color.withValues(alpha: 0.8),
                                    color.withValues(alpha: 0.4),
                                  ]),
                                  borderRadius: BorderRadius.circular(3),
                                  boxShadow: [
                                    BoxShadow(
                                      color: color.withValues(alpha: 0.3),
                                      blurRadius: 4,
                                    ),
                                  ],
                                ),
                              ),
                            ),
                          ),
                        ),
                        const SizedBox(width: 8),
                        Text(
                          '${(value * 100).toInt()}%',
                          style: TextStyle(
                            fontSize: 10,
                            color:
                                isDark ? Colors.white54 : AppTheme.textTertiary,
                          ),
                        ),
                      ],
                    ),
                  );
                }).toList(),
              ),
            ),
          ),
        ],
        const SizedBox(height: 24),
        // 湖神关怀提示
        if (insights.isNotEmpty) ...[
          Text(
            '湖神关怀提示',
            style: TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.w500,
              color: isDark ? Colors.white : AppTheme.textPrimary,
              letterSpacing: 1,
            ),
          ),
          const SizedBox(height: 12),
          ...insights.map((insight) => Padding(
                padding: const EdgeInsets.only(bottom: 10),
                child: Card(
                  shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(14)),
                  elevation: 1,
                  child: Padding(
                    padding: const EdgeInsets.all(14),
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Icon(
                          Icons.auto_awesome,
                          size: 14,
                          color: AppTheme.primaryColor.withValues(alpha: 0.7),
                        ),
                        const SizedBox(width: 10),
                        Expanded(
                          child: Text(
                            insight.toString(),
                            style: TextStyle(
                              fontSize: 13,
                              height: 1.5,
                              color: isDark
                                  ? Colors.white70
                                  : AppTheme.textSecondary,
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              )),
        ],
      ],
    );
  }

  /// 情绪类型对应的可视化颜色
  Color _moodColor(String mood) {
    const colors = {
      'happy': Color(0xFFFFD54F),
      'joy': Color(0xFFFFD54F),
      'sad': Color(0xFF64B5F6),
      'sadness': Color(0xFF64B5F6),
      'angry': Color(0xFFEF5350),
      'anger': Color(0xFFEF5350),
      'fear': Color(0xFFB39DDB),
      'fearful': Color(0xFFB39DDB),
      'surprise': Color(0xFFFFAB40),
      'surprised': Color(0xFFFFAB40),
      'neutral': Color(0xFF90CAF9),
      'calm': Color(0xFF80CBC4),
    };
    return colors[mood] ?? const Color(0xFF90CAF9);
  }

  /// 情绪类型英文 key 到中文标签的映射
  String _moodLabel(String mood) {
    const labels = {
      'happy': '开心',
      'joy': '开心',
      'sad': '忧伤',
      'sadness': '忧伤',
      'angry': '愤怒',
      'anger': '愤怒',
      'fear': '恐惧',
      'fearful': '恐惧',
      'surprise': '惊喜',
      'surprised': '惊喜',
      'neutral': '平静',
      'calm': '平静',
    };
    return labels[mood] ?? mood;
  }

  /// 将后端返回的原始趋势数据标准化为 {distribution, insights, period_days, trends} 格式
  ///
  /// 如果原始数据已包含 distribution 和 insights 则直接返回，
  /// 否则从 trends 列表中聚合计算情绪分布和生成洞察文案。
  Map<String, dynamic> _normalizeTrendPayload(Map<String, dynamic> raw) {
    if (raw['distribution'] is Map && raw['insights'] is List) {
      return raw;
    }

    final trends = raw['trends'] is List ? (raw['trends'] as List) : const [];
    final counts = <String, int>{};
    double scoreSum = 0;
    int scoreCount = 0;

    for (final item in trends) {
      if (item is! Map) continue;
      final row = Map<String, dynamic>.from(item);
      final mood = (row['mood'] ?? row['mood_type'] ?? 'neutral').toString();
      final weight = (row['stone_count'] as num?)?.toInt() ?? 1;
      counts[mood] = (counts[mood] ?? 0) + weight;

      final score = row['emotion_score'];
      if (score is num) {
        scoreSum += score.toDouble() * weight;
        scoreCount += weight;
      }
    }

    final total = counts.values.fold<int>(0, (a, b) => a + b);
    final distribution = <String, double>{};
    if (total > 0) {
      counts.forEach((mood, count) {
        distribution[mood] = count / total;
      });
    }

    final insights = <String>[];
    if (total == 0) {
      insights.add('最近还没有足够的情绪样本，继续记录会生成更准确的趋势分析。');
    } else {
      final dominant =
          counts.entries.reduce((a, b) => a.value >= b.value ? a : b).key;
      insights.add('最近的主导情绪是 ${_moodLabel(dominant)}。');
      if (scoreCount > 0) {
        final avg = scoreSum / scoreCount;
        if (avg >= 0.35) {
          insights.add('整体情绪偏积极，继续保持当前节奏。');
        } else if (avg <= -0.35) {
          insights.add('最近情绪偏低，建议先照顾好作息并多和可信任的人聊聊。');
        } else {
          insights.add('整体情绪较平稳，适合保持规律记录。');
        }
      }
    }

    return {
      'distribution': distribution,
      'insights': insights,
      'period_days': raw['period_days'] ?? 30,
      'trends': trends,
    };
  }
}
