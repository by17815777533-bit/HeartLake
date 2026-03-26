// 情绪热力图页面
//
// 展示用户近期情绪分布的热力图可视化。

import 'dart:async';

import 'package:flutter/material.dart';
import '../../data/datasources/user_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';
import '../../utils/storage_util.dart';
import '../widgets/emotion_heatmap.dart';
import '../widgets/emotion_insights_card.dart';
import '../widgets/shimmer_loading.dart';

/// 情绪热力图页面
///
/// 以 GitHub Contribution 风格的热力图展示用户长期情绪分布，
/// 配合 AI 生成的情绪洞察卡片。
/// 通过 WebSocket 实时监听石头增删事件，自动刷新热力图数据。
class EmotionHeatmapScreen extends StatefulWidget {
  const EmotionHeatmapScreen({super.key});

  @override
  State<EmotionHeatmapScreen> createState() => _EmotionHeatmapScreenState();
}

/// 情绪热力图页面的状态管理
///
/// 使用 [UserService] 获取热力图数据，通过 WebSocket 监听石头增删事件自动刷新。
/// 本地生成情绪洞察文案（近7天趋势、连续积极天数、高能日分析）。
class _EmotionHeatmapScreenState extends State<EmotionHeatmapScreen> {
  final UserService _userService = sl<UserService>();
  final WebSocketManager _wsManager = WebSocketManager();
  Map<String, Map<String, dynamic>> _heatmapData = {};
  List<String> _insights = [];
  bool _isLoading = true;
  String? _loadErrorMessage;
  String? _currentUserId;
  late final void Function(Map<String, dynamic>) _onNewStoneListener;
  late final void Function(Map<String, dynamic>) _onStoneDeletedListener;
  late final void Function(Map<String, dynamic>) _onReconnectedListener;
  Timer? _refreshDebounce;
  int _refreshSeq = 0;

  @override
  void initState() {
    super.initState();
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
    _loadHeatmapData();
  }

  @override
  void dispose() {
    _wsManager.leaveRoom('lake');
    _wsManager.off('new_stone', _onNewStoneListener);
    _wsManager.off('stone_deleted', _onStoneDeletedListener);
    _wsManager.off('reconnected', _onReconnectedListener);
    _refreshDebounce?.cancel();
    super.dispose();
  }

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
      return false;
    }
    return extractPayloadUserId(payload) == currentUserId;
  }

  /// 防抖刷新：300ms 内合并多次事件，[withFollowUp] 为 true 时 2s 后再补刷一次
  void _scheduleRealtimeRefresh({bool withFollowUp = false}) {
    _refreshDebounce?.cancel();
    _refreshDebounce = Timer(const Duration(milliseconds: 300), () {
      _loadHeatmapData();
      if (!withFollowUp) return;
      final token = ++_refreshSeq;
      Future.delayed(const Duration(seconds: 2), () {
        if (!mounted || token != _refreshSeq) return;
        _loadHeatmapData();
      });
    });
  }

  /// 从后端加载热力图数据，解析 days 字典并生成本地洞察文案
  Future<void> _loadHeatmapData() async {
    if (mounted) setState(() => _isLoading = true);
    try {
      final result = await _userService.getEmotionHeatmap(days: 365);
      if (!mounted) return;
      if (result['success'] == true) {
        final rawData = (result['data'] as Map<String, dynamic>?)?['days'];
        if (rawData is! Map) {
          throw StateError('Emotion heatmap days payload is not a map');
        }
        final parsed = _normalizeHeatmapDays(rawData);
        setState(() {
          _heatmapData = parsed;
          _insights = _generateInsights(parsed);
          _isLoading = false;
          _loadErrorMessage = null;
        });
        return;
      } else {
        final message = result['message']?.toString().trim();
        setState(() {
          _isLoading = false;
          _loadErrorMessage =
              message == null || message.isEmpty ? '情绪热力图加载失败' : message;
        });
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(_loadErrorMessage!),
          ),
        );
      }
    } catch (error, stackTrace) {
      _reportUiError(
          error, stackTrace, 'EmotionHeatmapScreen._loadHeatmapData');
      if (mounted) {
        setState(() {
          _isLoading = false;
          _loadErrorMessage = '情绪热力图加载失败，请稍后重试';
        });
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('情绪热力图加载失败，请稍后重试')),
        );
      }
    }

    if (mounted) {
      setState(() {
        _isLoading = false;
      });
    }
  }

  Map<String, Map<String, dynamic>> _normalizeHeatmapDays(Map rawData) {
    final parsed = <String, Map<String, dynamic>>{};
    for (final entry in rawData.entries) {
      if (entry.value is! Map) {
        continue;
      }
      final dayData = Map<String, dynamic>.from(
          (entry.value as Map).cast<String, dynamic>());
      final mood = _normalizeMood(dayData['mood']);
      final score = _extractNormalizedScore(dayData);
      final rawScore = _extractSentimentScore(dayData);
      if (mood == null && score == null && rawScore == null) {
        continue;
      }
      if (mood != null) {
        dayData['mood'] = mood;
      }
      if (score != null) {
        dayData['score'] = score;
      }
      if (rawScore != null) {
        dayData['raw_score'] = rawScore;
      }
      parsed[entry.key.toString()] = dayData;
    }
    return parsed;
  }

  String? _normalizeMood(dynamic mood) {
    final text = mood?.toString().trim();
    if (text == null || text.isEmpty) {
      return null;
    }
    return text;
  }

  double? _extractNormalizedScore(Map<String, dynamic> dayData) {
    final score = dayData['score'];
    if (score is num) {
      return score.toDouble().clamp(0.0, 1.0);
    }
    final rawScore = dayData['raw_score'];
    if (rawScore is num) {
      return ((rawScore.toDouble().clamp(-1.0, 1.0) + 1) / 2).clamp(0.0, 1.0);
    }
    return null;
  }

  double? _extractSentimentScore(Map<String, dynamic> dayData) {
    final rawScore = dayData['raw_score'];
    if (rawScore is num) {
      return rawScore.toDouble().clamp(-1.0, 1.0);
    }
    final score = dayData['score'];
    if (score is num) {
      return (score.toDouble().clamp(0.0, 1.0) * 2 - 1).clamp(-1.0, 1.0);
    }
    return null;
  }

  /// 根据热力图数据生成情绪洞察文案
  ///
  /// 分析维度：近7天均值趋势、连续积极天数、一周中情绪最佳的星期几
  List<String> _generateInsights(Map<String, Map<String, dynamic>> data) {
    if (data.isEmpty) return [];
    final insights = <String>[];
    final now = DateTime.now();

    double recentTotal = 0.0;
    int recentCount = 0;
    for (int i = 0; i < 7; i++) {
      final date = now.subtract(Duration(days: i));
      final key =
          '${date.year}-${date.month.toString().padLeft(2, '0')}-${date.day.toString().padLeft(2, '0')}';
      final dayData = data[key];
      if (dayData != null) {
        final score = _extractNormalizedScore(dayData);
        if (score == null) continue;
        recentTotal += score;
        recentCount++;
      }
    }
    if (recentCount >= 3) {
      final avg = recentTotal / recentCount;
      if (avg >= 0.65) {
        insights.add('最近 7 天情绪整体向上。');
      } else if (avg <= 0.35) {
        insights.add('最近 7 天有些低落，记得给自己留出恢复时间。');
      } else {
        insights.add('最近 7 天情绪较为平稳。');
      }
    }

    int positiveStreak = 0;
    for (int i = 0; i < 30; i++) {
      final date = now.subtract(Duration(days: i));
      final key =
          '${date.year}-${date.month.toString().padLeft(2, '0')}-${date.day.toString().padLeft(2, '0')}';
      final dayData = data[key];
      final score = dayData != null ? _extractNormalizedScore(dayData) : null;
      if (score != null && score >= 0.6) {
        positiveStreak++;
      } else {
        break;
      }
    }
    if (positiveStreak >= 2) {
      insights.add('你已连续 $positiveStreak 天保持积极状态。');
    }

    final weekdayScores = <int, List<double>>{};
    for (final entry in data.entries) {
      try {
        final date = DateTime.parse(entry.key);
        final score = _extractNormalizedScore(entry.value);
        if (score == null) {
          continue;
        }
        weekdayScores.putIfAbsent(date.weekday, () => []).add(score);
      } catch (error, stackTrace) {
        _reportUiError(
          error,
          stackTrace,
          'EmotionHeatmapScreen._generateInsights(${entry.key})',
        );
      }
    }
    if (weekdayScores.isNotEmpty) {
      double bestAvg = -1.0;
      int bestDay = 1;
      for (final entry in weekdayScores.entries) {
        final avg = entry.value.reduce((a, b) => a + b) / entry.value.length;
        if (avg > bestAvg) {
          bestAvg = avg;
          bestDay = entry.key;
        }
      }
      const dayNames = ['', '周一', '周二', '周三', '周四', '周五', '周六', '周日'];
      insights.add('你的高能日通常出现在 ${dayNames[bestDay]}。');
    }

    return insights;
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      backgroundColor: isDark ? const Color(0xFF1A1A2E) : Colors.white,
      appBar: AppBar(
        title: const Text('情绪热力图'),
        backgroundColor: isDark ? const Color(0xFF16213E) : Colors.white,
        foregroundColor: isDark ? Colors.white : AppTheme.textPrimary,
      ),
      body: RefreshIndicator(
        onRefresh: _loadHeatmapData,
        child: ListView(
          physics: const AlwaysScrollableScrollPhysics(),
          padding: const EdgeInsets.all(16),
          children: [
            if (_loadErrorMessage != null) _buildLoadWarningCard(),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
              decoration: BoxDecoration(
                color: AppTheme.skyBlue.withValues(alpha: 0.08),
                borderRadius: BorderRadius.circular(14),
                border:
                    Border.all(color: AppTheme.skyBlue.withValues(alpha: 0.2)),
              ),
              child: Text(
                '热力图聚焦长期波动，日历聚焦单日记录。已拆分展示，便于你分别查看。',
                style: TextStyle(
                    fontSize: 13,
                    color: isDark ? Colors.white70 : AppTheme.textSecondary,
                    height: 1.5),
              ),
            ),
            const SizedBox(height: 16),
            if (_isLoading)
              const SizedBox(
                height: 220,
                child: Center(
                  child: WarmLoadingIndicator(
                    messages: ['正在汇总情绪热力图...', '计算最近情绪密度...', '生成情绪关怀提示...'],
                  ),
                ),
              )
            else if (_heatmapData.isEmpty && _loadErrorMessage != null)
              _buildLoadErrorState()
            else
              EmotionHeatmap(data: _heatmapData),
            const SizedBox(height: 16),
            if (_insights.isNotEmpty) EmotionInsightsCard(insights: _insights),
          ],
        ),
      ),
    );
  }

  Widget _buildLoadWarningCard() {
    return Container(
      margin: const EdgeInsets.only(bottom: 16),
      padding: const EdgeInsets.all(14),
      decoration: BoxDecoration(
        color: const Color(0xFFFFF6EB),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: const Color(0xFFE7C79A)),
      ),
      child: Row(
        children: [
          const Icon(Icons.warning_amber_rounded, color: Color(0xFFB7791F)),
          const SizedBox(width: 10),
          Expanded(
            child: Text(
              _loadErrorMessage!,
              style: const TextStyle(
                fontSize: 13,
                color: Color(0xFF7C5A1D),
                height: 1.5,
              ),
            ),
          ),
          TextButton(
            onPressed: _loadHeatmapData,
            child: const Text('重试'),
          ),
        ],
      ),
    );
  }

  Widget _buildLoadErrorState() {
    return Container(
      padding: const EdgeInsets.symmetric(vertical: 36, horizontal: 20),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(16),
      ),
      child: Column(
        children: [
          const Icon(
            Icons.insights_outlined,
            color: AppTheme.errorColor,
            size: 40,
          ),
          const SizedBox(height: 12),
          Text(
            _loadErrorMessage ?? '情绪热力图加载失败',
            style: const TextStyle(fontSize: 14, color: AppTheme.textSecondary),
            textAlign: TextAlign.center,
          ),
          const SizedBox(height: 12),
          FilledButton(
            onPressed: _loadHeatmapData,
            child: const Text('重新加载'),
          ),
        ],
      ),
    );
  }
}
