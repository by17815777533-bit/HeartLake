// @file ai_content_preview.dart
// @brief AI内容审核预览组件 - 实时情感分析 + 内容审核
// 类光遇温暖风格：毛玻璃卡片、柔和渐变、脉动动画

import 'dart:async';
import 'dart:ui';
import 'package:flutter/material.dart';
import '../../data/datasources/edge_ai_service.dart';

/// AI审核状态
enum ModerationStatus { idle, loading, passed, warning, rejected }

/// AI内容审核预览结果（供外部读取）
class AIPreviewResult {
  final ModerationStatus status;
  final String? message;

  const AIPreviewResult({this.status = ModerationStatus.idle, this.message});

  bool get canSubmit =>
      status == ModerationStatus.idle ||
      status == ModerationStatus.passed ||
      status == ModerationStatus.warning;
}

class AIContentPreview extends StatefulWidget {
  final String text;
  final ValueChanged<AIPreviewResult>? onResultChanged;

  const AIContentPreview({
    super.key,
    required this.text,
    this.onResultChanged,
  });

  @override
  State<AIContentPreview> createState() => _AIContentPreviewState();
}

class _AIContentPreviewState extends State<AIContentPreview>
    with SingleTickerProviderStateMixin {
  final EdgeAIService _aiService = EdgeAIService();
  Timer? _debounceTimer;

  ModerationStatus _status = ModerationStatus.idle;
  String? _sentimentLabel;
  double? _sentimentScore;
  String? _moderationMessage;
  bool _showCareHint = false;
  String? _careHintText;
  double? _uncertainty;
  String? _reliabilityTier;
  bool _abstained = false;
  String? _recommendedAction;

  late AnimationController _pulseController;
  late Animation<double> _pulseAnimation;

  @override
  void initState() {
    super.initState();
    _pulseController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 1500),
    )..repeat(reverse: true);
    _pulseAnimation = Tween<double>(begin: 0.4, end: 1.0).animate(
      CurvedAnimation(parent: _pulseController, curve: Curves.easeInOut),
    );
  }

  @override
  void didUpdateWidget(covariant AIContentPreview oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.text != widget.text) {
      _scheduleAnalysis();
    }
  }

  @override
  void dispose() {
    _debounceTimer?.cancel();
    _pulseController.dispose();
    super.dispose();
  }

  void _scheduleAnalysis() {
    _debounceTimer?.cancel();

    if (widget.text.trim().length <= 10) {
      setState(() {
        _status = ModerationStatus.idle;
        _sentimentLabel = null;
        _sentimentScore = null;
        _moderationMessage = null;
        _showCareHint = false;
        _uncertainty = null;
        _reliabilityTier = null;
        _abstained = false;
        _recommendedAction = null;
      });
      _notifyResult();
      return;
    }

    setState(() => _status = ModerationStatus.loading);
    _notifyResult();

    _debounceTimer = Timer(const Duration(milliseconds: 800), _runAnalysis);
  }

  Future<void> _runAnalysis() async {
    final text = widget.text.trim();
    if (text.length <= 10) return;

    try {
      // 仅调用情感分析（内容审核为admin端点，后端在发布时已内置审核）
      final sentimentRes = await _aiService.analyzeSentiment(text);

      if (!mounted) return;

      // 解析情感分析结果
      if (sentimentRes['success'] == true && sentimentRes['data'] is Map) {
        final data = Map<String, dynamic>.from(sentimentRes['data'] as Map);
        final rawMood = data['mood']?.toString() ??
            data['sentiment']?.toString() ??
            data['label']?.toString();
        _sentimentLabel = _normalizeMoodLabel(rawMood);

        final score = data['score'];
        final confidence = data['calibrated_confidence'] ?? data['confidence'];
        if (score != null) {
          _sentimentScore = (score is num)
              ? score.toDouble()
              : double.tryParse(score.toString());
        } else if (confidence != null) {
          _sentimentScore = (confidence is num)
              ? confidence.toDouble()
              : double.tryParse(confidence.toString());
        }
        final uncertaintyRaw = data['uncertainty'];
        if (uncertaintyRaw is num) {
          _uncertainty = uncertaintyRaw.toDouble().clamp(0.0, 1.0);
        } else if (uncertaintyRaw != null) {
          _uncertainty =
              double.tryParse(uncertaintyRaw.toString())?.clamp(0.0, 1.0);
        } else {
          _uncertainty = null;
        }

        _reliabilityTier = data['reliability_tier']?.toString();
        final abstainedRaw = data['abstained'];
        if (abstainedRaw is bool) {
          _abstained = abstainedRaw;
        } else {
          final decision = data['decision']?.toString().toLowerCase();
          _abstained = decision == 'abstain';
        }
        _recommendedAction = data['recommended_action']?.toString();

        // 检测心理风险（情感分析结果中可能包含风险标记）
        final highRisk =
            data['high_risk'] == true || data['mental_risk'] == true;
        if (highRisk) {
          _showCareHint = true;
          _careHintText = data['help_tip']?.toString() ??
              '湖神看见你可能正承受很重的情绪。如果你愿意，请先联系可信任的人，也可以拨打心理援助热线：400-161-9995。';
        } else {
          _showCareHint = false;
        }
      }

      // 前端不做硬拦截：不确定时给 warning 提示，由用户决定
      _status = _abstained ? ModerationStatus.warning : ModerationStatus.passed;
      _moderationMessage = _abstained ? '湖神暂时不确定当前语境，建议补充更具体的事件与感受。' : null;

      setState(() {});
      _notifyResult();
    } catch (e) {
      if (!mounted) return;
      // 网络错误时默认通过
      setState(() {
        _status = ModerationStatus.passed;
        _abstained = false;
        _uncertainty = null;
        _reliabilityTier = null;
        _recommendedAction = null;
      });
      _notifyResult();
    }
  }

  void _notifyResult() {
    widget.onResultChanged?.call(AIPreviewResult(
      status: _status,
      message: _moderationMessage,
    ));
  }

  @override
  Widget build(BuildContext context) {
    if (_status == ModerationStatus.idle) {
      return const SizedBox.shrink();
    }

    return AnimatedSize(
      duration: const Duration(milliseconds: 300),
      curve: Curves.easeInOut,
      child: ClipRRect(
        borderRadius: BorderRadius.circular(16),
        child: BackdropFilter(
          filter: ImageFilter.blur(sigmaX: 12, sigmaY: 12),
          child: AnimatedContainer(
            duration: const Duration(milliseconds: 300),
            width: double.infinity,
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(16),
              gradient: LinearGradient(
                begin: Alignment.topLeft,
                end: Alignment.bottomRight,
                colors: [
                  _cardBaseColor.withValues(alpha: 0.15),
                  _cardBaseColor.withValues(alpha: 0.08),
                ],
              ),
              border: Border.all(
                color: _cardBaseColor.withValues(alpha: 0.25),
                width: 1,
              ),
            ),
            child: _status == ModerationStatus.loading
                ? _buildLoadingState()
                : _buildResultState(),
          ),
        ),
      ),
    );
  }

  Color get _cardBaseColor {
    switch (_status) {
      case ModerationStatus.passed:
        return const Color(0xFF4CAF50);
      case ModerationStatus.warning:
        return const Color(0xFFFF9800);
      case ModerationStatus.rejected:
        return const Color(0xFFE57373);
      case ModerationStatus.loading:
        return const Color(0xFF90CAF9);
      case ModerationStatus.idle:
        return Colors.white;
    }
  }

  Widget _buildLoadingState() {
    return AnimatedBuilder(
      animation: _pulseAnimation,
      builder: (context, child) {
        return Opacity(
          opacity: _pulseAnimation.value,
          child: Row(
            children: [
              SizedBox(
                width: 16,
                height: 16,
                child: CircularProgressIndicator(
                  strokeWidth: 2,
                  color: const Color(0xFF90CAF9)
                      .withValues(alpha: _pulseAnimation.value),
                ),
              ),
              const SizedBox(width: 10),
              Text(
                '湖神正在感知你的文字...',
                style: TextStyle(
                  fontSize: 13,
                  color: Colors.white.withValues(alpha: 0.8),
                ),
              ),
            ],
          ),
        );
      },
    );
  }

  Widget _buildResultState() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // 审核状态行
        _buildStatusRow(),

        // 情感分析
        if (_sentimentLabel != null) ...[
          const SizedBox(height: 12),
          _buildSentimentBar(),
        ],

        if (_reliabilityTier != null || _uncertainty != null) ...[
          const SizedBox(height: 10),
          _buildReliabilityHint(),
        ],

        // 心理关怀提示
        if (_showCareHint) ...[
          const SizedBox(height: 12),
          _buildCareHint(),
        ],

        // 审核消息
        if (_moderationMessage != null &&
            _status != ModerationStatus.passed) ...[
          const SizedBox(height: 8),
          Text(
            _moderationMessage!,
            style: TextStyle(
              fontSize: 12,
              color: Colors.white.withValues(alpha: 0.7),
            ),
          ),
        ],
      ],
    );
  }

  Widget _buildStatusRow() {
    IconData icon;
    String label;
    Color glowColor;

    switch (_status) {
      case ModerationStatus.passed:
        icon = Icons.check_circle_outline;
        label = '湖神分析完成，可继续发布';
        glowColor = const Color(0xFF4CAF50);
        break;
      case ModerationStatus.warning:
        icon = Icons.info_outline;
        label = '湖神信心不足，建议补充语境';
        glowColor = const Color(0xFFFF9800);
        break;
      case ModerationStatus.rejected:
        icon = Icons.shield_outlined;
        label = '内容未通过审核';
        glowColor = const Color(0xFFE57373);
        break;
      default:
        return const SizedBox.shrink();
    }

    return Row(
      children: [
        Container(
          padding: const EdgeInsets.all(4),
          decoration: BoxDecoration(
            shape: BoxShape.circle,
            boxShadow: [
              BoxShadow(
                color: glowColor.withValues(alpha: 0.5),
                blurRadius: 8,
                spreadRadius: 1,
              ),
            ],
          ),
          child: Icon(icon, size: 18, color: glowColor),
        ),
        const SizedBox(width: 8),
        Text(
          label,
          style: TextStyle(
            fontSize: 13,
            fontWeight: FontWeight.w500,
            color: Colors.white.withValues(alpha: 0.9),
          ),
        ),
      ],
    );
  }

  String _normalizeMoodLabel(String? mood) {
    if (mood == null || mood.trim().isEmpty) return 'neutral';
    final key = mood.trim().toLowerCase();
    const moodMap = {
      'happy': '开心',
      'calm': '平静',
      'neutral': '中性',
      'anxious': '焦虑',
      'sad': '难过',
      'angry': '愤怒',
      'surprised': '惊讶',
      'confused': '困惑',
      'fear': '焦虑',
      'fearful': '焦虑',
    };
    return moodMap[key] ?? mood;
  }

  double _normalizeSentimentScore(double? rawScore) {
    if (rawScore == null) return 0.5;
    if (rawScore >= -1.0 && rawScore <= 1.0) {
      return ((rawScore + 1.0) / 2.0).clamp(0.0, 1.0);
    }
    return rawScore.clamp(0.0, 1.0);
  }

  Widget _buildSentimentBar() {
    final score = _normalizeSentimentScore(_sentimentScore);

    // 情感渐变色：从冷色到暖色
    final gradientColors = [
      const Color(0xFF64B5F6), // 平静蓝
      const Color(0xFF81C784), // 舒适绿
      const Color(0xFFFFD54F), // 温暖黄
      const Color(0xFFFFB74D), // 活力橙
      const Color(0xFFE57373), // 热烈红
    ];

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text(
              '情绪感知: ${_sentimentLabel ?? "分析中"}',
              style: TextStyle(
                fontSize: 12,
                color: Colors.white.withValues(alpha: 0.7),
              ),
            ),
            Text(
              '${(score * 100).toInt()}%',
              style: TextStyle(
                fontSize: 12,
                color: Colors.white.withValues(alpha: 0.6),
              ),
            ),
          ],
        ),
        const SizedBox(height: 6),
        ClipRRect(
          borderRadius: BorderRadius.circular(4),
          child: SizedBox(
            height: 6,
            child: Stack(
              children: [
                // 背景
                Container(
                  decoration: BoxDecoration(
                    color: Colors.white.withValues(alpha: 0.1),
                    borderRadius: BorderRadius.circular(4),
                  ),
                ),
                // 渐变进度条
                FractionallySizedBox(
                  widthFactor: score,
                  child: Container(
                    decoration: BoxDecoration(
                      gradient: LinearGradient(colors: gradientColors),
                      borderRadius: BorderRadius.circular(4),
                      boxShadow: [
                        BoxShadow(
                          color: gradientColors[
                                  (score * (gradientColors.length - 1)).toInt()]
                              .withValues(alpha: 0.5),
                          blurRadius: 6,
                        ),
                      ],
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }

  Widget _buildReliabilityHint() {
    final tier = (_reliabilityTier ?? 'unknown').toLowerCase();
    Color tierColor;
    String tierText;
    switch (tier) {
      case 'high':
        tierColor = const Color(0xFF66BB6A);
        tierText = '高';
        break;
      case 'medium':
        tierColor = const Color(0xFFFFD54F);
        tierText = '中';
        break;
      default:
        tierColor = const Color(0xFFFF8A65);
        tierText = '低';
    }
    final uncertainty = ((_uncertainty ?? 0.0) * 100).toStringAsFixed(0);
    final action = _recommendedAction == 'ask_for_more_context'
        ? '建议补充具体事件、对象和结果'
        : '可直接使用当前情绪建议';
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(10),
        color: Colors.white.withValues(alpha: 0.08),
        border: Border.all(color: tierColor.withValues(alpha: 0.42)),
      ),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Icon(Icons.analytics_outlined, size: 15, color: tierColor),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              '可信度 $tierText | 不确定性 $uncertainty% | $action',
              style: TextStyle(
                fontSize: 11,
                color: Colors.white.withValues(alpha: 0.86),
                height: 1.35,
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildCareHint() {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(12),
        gradient: LinearGradient(
          colors: [
            const Color(0xFFFFCDD2).withValues(alpha: 0.2),
            const Color(0xFFF8BBD0).withValues(alpha: 0.15),
          ],
        ),
        border: Border.all(
          color: const Color(0xFFFFAB91).withValues(alpha: 0.3),
        ),
      ),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Icon(
            Icons.favorite_outline,
            size: 18,
            color: Color(0xFFFFAB91),
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              _careHintText ?? '',
              style: TextStyle(
                fontSize: 12,
                height: 1.5,
                color: Colors.white.withValues(alpha: 0.85),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
