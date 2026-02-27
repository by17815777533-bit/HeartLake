// 发布石头界面

import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import '../widgets/water_background.dart';
import '../widgets/psych_support_dialog.dart';
import '../widgets/ai_content_preview.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';
import '../../data/datasources/stone_service.dart';
import '../../data/datasources/cache_service.dart';
import '../../di/service_locator.dart';
import '../../providers/edge_ai_provider.dart';

class PublishScreen extends StatefulWidget {
  const PublishScreen({super.key, this.onPublished});

  final VoidCallback? onPublished;

  @override
  State<PublishScreen> createState() => _PublishScreenState();
}

class _PublishScreenState extends State<PublishScreen> {
  final TextEditingController _contentController = TextEditingController();
  final StoneService _stoneService = sl<StoneService>();
  final EdgeAIProvider _provider = EdgeAIProvider();
  String _selectedType = 'medium';
  String _selectedColor = '#ADA59E';
  MoodType _selectedMood = MoodType.neutral; // 新增：选中的心情
  bool _isSubmitting = false;
  Map<String, double>? _emotionResult;
  String? _topEmotion;
  Timer? _debounceTimer;
  Timer? _aiPulseTimer;
  AIPreviewResult _previewResult = const AIPreviewResult();
  MoodType? _aiSuggestedMood;
  MoodType? _aiPulseMood;
  double _aiSuggestionConfidence = 0.0;
  bool _manualMoodLocked = false;
  int _analysisSeq = 0;

  void _handlePreviewResultChanged(AIPreviewResult preview) {
    final unchanged =
        _previewResult.status == preview.status &&
        _previewResult.message == preview.message;
    if (unchanged) return;

    void applyUpdate() {
      if (!mounted) return;
      setState(() => _previewResult = preview);
    }

    final phase = SchedulerBinding.instance.schedulerPhase;
    if (phase == SchedulerPhase.idle ||
        phase == SchedulerPhase.postFrameCallbacks) {
      applyUpdate();
    } else {
      WidgetsBinding.instance.addPostFrameCallback((_) => applyUpdate());
    }
  }

  final Map<String, String> _stoneTypes = {
    'light': '轻石',
    'medium': '中石',
    'heavy': '重石',
  };

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return GestureDetector(
      onTap: () => FocusScope.of(context).unfocus(),
      child: Scaffold(
        extendBodyBehindAppBar: true,
        backgroundColor: isDark ? const Color(0xFF0D1B2A) : const Color(0xFFF5F5F5),
        appBar: AppBar(
          title: const Text('投石',
              style:
                  TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
          centerTitle: true,
          backgroundColor: Colors.transparent,
          elevation: 0,
          scrolledUnderElevation: 0,
          foregroundColor: Colors.white,
        ),
        body: Stack(
          fit: StackFit.expand,
          children: [
            const Positioned.fill(child: WaterBackground()),
            SingleChildScrollView(
              padding: EdgeInsets.only(
                top: MediaQuery.of(context).padding.top + kToolbarHeight + 20,
                left: 24,
                right: 24,
                bottom: 24,
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    '向湖中投入你的石头',
                    style: TextStyle(
                        fontSize: 28,
                        fontWeight: FontWeight.bold,
                        color: Colors.white,
                        shadows: [
                          Shadow(
                              color: AppTheme.heavyStone.withValues(alpha: 0.4),
                              offset: const Offset(0, 2),
                              blurRadius: 4)
                        ]),
                  ),
                  const SizedBox(height: 8),
                  const Text(
                    '这里是你的安全空间',
                    style: TextStyle(
                      fontSize: 16,
                      color: Colors.white70,
                    ),
                  ),
                  const SizedBox(height: 32),

                  // 输入区域容器
                  Container(
                    padding: const EdgeInsets.all(24),
                    decoration: BoxDecoration(
                      color: Colors.white.withValues(alpha: 0.9),
                      borderRadius: BorderRadius.circular(24),
                      border: Border.all(
                        color: MoodColors.getConfig(_selectedMood).primary,
                        width: 2,
                      ),
                      boxShadow: [
                        BoxShadow(
                          color: AppTheme.skyBlue.withValues(alpha: 0.15),
                          blurRadius: 20,
                          offset: const Offset(0, 10),
                        )
                      ],
                    ),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        TextField(
                          controller: _contentController,
                          maxLines: 6,
                          maxLength: 500,
                          onChanged: _onContentChanged,
                          decoration: const InputDecoration(
                            hintText: '说说你的心情吧...',
                            counterText: '',
                            border: InputBorder.none,
                          ),
                          style: const TextStyle(
                            fontSize: 16,
                            height: 1.5,
                          ),
                        ),

                        if (_emotionResult != null) ...[
                          const SizedBox(height: 8),
                          Container(
                            padding: const EdgeInsets.symmetric(
                                horizontal: 12, vertical: 6),
                            decoration: BoxDecoration(
                              color:
                                  AppTheme.primaryColor.withValues(alpha: 0.1),
                              borderRadius: BorderRadius.circular(20),
                            ),
                            child: Row(
                              mainAxisSize: MainAxisSize.min,
                              children: [
                                const Icon(Icons.psychology,
                                    size: 16, color: AppTheme.primaryColor),
                                const SizedBox(width: 4),
                                Text(
                                  _buildAIHintText(),
                                  style: const TextStyle(
                                      fontSize: 12,
                                      color: AppTheme.primaryColor),
                                ),
                                const SizedBox(width: 8),
                                const Icon(Icons.shield,
                                    size: 14, color: AppTheme.successColor),
                                const Text(' 差分隐私保护',
                                    style: TextStyle(
                                        fontSize: 10,
                                        color: AppTheme.textTertiary)),
                              ],
                            ),
                          ),
                        ],
                        if (_aiSuggestedMood != null) ...[
                          const SizedBox(height: 10),
                          _buildAIMoodSuggestionBanner(),
                        ],
                        if (_contentController.text.trim().length >= 8) ...[
                          const SizedBox(height: 10),
                          AIContentPreview(
                            text: _contentController.text,
                            onResultChanged: _handlePreviewResultChanged,
                          ),
                        ],

                        const SizedBox(height: 16),

                        // 心情选择器（新增）
                        Text(
                          '选择心情',
                          style:
                              Theme.of(context).textTheme.bodyLarge?.copyWith(
                                    fontWeight: FontWeight.w600,
                                  ),
                        ),
                        const SizedBox(height: 12),
                        _buildMoodSelector(),

                        const SizedBox(height: 24),

                        // 石头类型选择
                        Text(
                          '选择石头类型',
                          style:
                              Theme.of(context).textTheme.bodyLarge?.copyWith(
                                    fontWeight: FontWeight.w600,
                                  ),
                        ),
                        const SizedBox(height: 12),
                        Row(
                          children: _stoneTypes.entries.map((entry) {
                            final isSelected = _selectedType == entry.key;
                            // 根据心情和石头类型动态计算颜色
                            final moodConfig =
                                MoodColors.getConfig(_selectedMood);
                            Color activeColor;
                            if (entry.key == 'light') {
                              activeColor =
                                  moodConfig.primary.withValues(alpha: 0.3);
                            } else if (entry.key == 'medium') {
                              activeColor =
                                  moodConfig.primary.withValues(alpha: 0.6);
                            } else {
                              activeColor =
                                  moodConfig.primary.withValues(alpha: 0.9);
                            }

                            return Expanded(
                              child: GestureDetector(
                                onTap: () {
                                  setState(() {
                                    _selectedType = entry.key;
                                    // 根据心情更新颜色
                                    _updateStoneColor();
                                  });
                                },
                                child: AnimatedContainer(
                                  duration: const Duration(milliseconds: 200),
                                  margin:
                                      const EdgeInsets.symmetric(horizontal: 4),
                                  padding:
                                      const EdgeInsets.symmetric(vertical: 12),
                                  decoration: BoxDecoration(
                                    color: isSelected
                                        ? activeColor
                                        : Colors.transparent,
                                    borderRadius: BorderRadius.circular(12),
                                    border: Border.all(
                                      color: isSelected
                                          ? moodConfig.primary
                                          : Colors.grey.shade300,
                                      width: isSelected ? 2 : 1,
                                    ),
                                  ),
                                  child: Text(
                                    entry.value,
                                    textAlign: TextAlign.center,
                                    style: TextStyle(
                                      color: isSelected
                                          ? Colors.white
                                          : Colors.grey.shade600,
                                      fontWeight: FontWeight.w500,
                                    ),
                                  ),
                                ),
                              ),
                            );
                          }).toList(),
                        ),

                        const SizedBox(height: 32),

                        // 发布按钮
                        _PublishButton(
                          isSubmitting: _isSubmitting,
                          onPressed: _submitStone,
                          moodConfig: MoodColors.getConfig(_selectedMood),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  @override
  void dispose() {
    _debounceTimer?.cancel();
    _aiPulseTimer?.cancel();
    _contentController.dispose();
    super.dispose();
  }

  void _onContentChanged(String text) {
    _debounceTimer?.cancel();
    final trimmed = text.trim();
    if (trimmed.length < 4) {
      setState(() {
        _emotionResult = null;
        _topEmotion = null;
        _aiSuggestedMood = null;
        _aiSuggestionConfidence = 0.0;
        _manualMoodLocked = false;
      });
      return;
    }
    final currentSeq = ++_analysisSeq;
    _debounceTimer = Timer(const Duration(milliseconds: 500), () async {
      final snapshot = _contentController.text.trim();
      if (snapshot.isEmpty) return;
      final result = await _provider.classifyText(snapshot);
      if (!mounted || currentSeq != _analysisSeq) return;
      if (_contentController.text.trim() != snapshot) return;

      final topEmotion = _provider.lastEmotion;
      final topConfidence =
          topEmotion == null ? 0.0 : (result[topEmotion] ?? 0.0);
      final suggestedMood =
          topEmotion == null ? null : MoodColors.fromString(topEmotion);
      var autoSelected = false;
      setState(() {
        _emotionResult = result;
        _topEmotion = topEmotion;
        autoSelected = _applyAIMoodSuggestion(suggestedMood, topConfidence);
      });
      if (autoSelected && suggestedMood != null) {
        _triggerAIMoodPulse(suggestedMood);
      }
    });
  }

  String _buildAIHintText() {
    if (_topEmotion == null || _emotionResult == null) return '湖神参考：分析中';
    final confidence = _emotionResult![_topEmotion!] ?? 0.0;
    if (confidence < 0.45) {
      return '湖神参考：情绪不确定，请以你的真实感受为准';
    }
    final linked = _aiSuggestedMood != null &&
        _selectedMood == _aiSuggestedMood &&
        !_manualMoodLocked;
    final suffix = linked ? '，已联动心情' : '';
    return '湖神参考: ${_provider.getEmotionLabel(_topEmotion!)} ${(confidence * 100).toStringAsFixed(0)}%$suffix（仅供参考）';
  }

  bool _applyAIMoodSuggestion(MoodType? suggestedMood, double confidence) {
    _aiSuggestedMood = suggestedMood;
    _aiSuggestionConfidence = confidence.clamp(0.0, 1.0).toDouble();
    if (suggestedMood == null) return false;
    if (_manualMoodLocked && _selectedMood != suggestedMood) return false;
    if (_aiSuggestionConfidence < 0.58) return false;
    if (_selectedMood == suggestedMood) return false;
    _selectedMood = suggestedMood;
    _updateStoneColor();
    return true;
  }

  void _triggerAIMoodPulse(MoodType mood) {
    _aiPulseTimer?.cancel();
    if (!mounted) return;
    setState(() => _aiPulseMood = mood);
    _aiPulseTimer = Timer(const Duration(milliseconds: 1400), () {
      if (!mounted) return;
      setState(() => _aiPulseMood = null);
    });
  }

  Widget _buildAIMoodSuggestionBanner() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final mood = _aiSuggestedMood;
    if (mood == null) return const SizedBox.shrink();
    final config = MoodColors.getConfig(mood);
    final linked = _selectedMood == mood && !_manualMoodLocked;
    final confidence = (_aiSuggestionConfidence * 100).toStringAsFixed(0);
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      decoration: BoxDecoration(
        color: isDark
            ? config.primary.withValues(alpha: 0.20)
            : config.primary.withValues(alpha: 0.10),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: config.primary.withValues(alpha: isDark ? 0.40 : 0.28)),
      ),
      child: Row(
        children: [
          Icon(Icons.auto_awesome, size: 16, color: config.primary),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              linked
                  ? '湖神已联动心情到「${config.name}」$confidence%'
                  : '湖神建议「${config.name}」$confidence%',
              style: TextStyle(
                color: config.primary,
                fontSize: 12,
                fontWeight: FontWeight.w600,
              ),
            ),
          ),
          TextButton(
            onPressed: () {
              setState(() => _manualMoodLocked = true);
            },
            style: TextButton.styleFrom(
              minimumSize: const Size(0, 30),
              padding: const EdgeInsets.symmetric(horizontal: 8),
            ),
            child: Text(
              linked ? '手动接管' : '保持手动',
              style: const TextStyle(fontSize: 11),
            ),
          ),
        ],
      ),
    );
  }

  // 提交石头
  Future<void> _submitStone() async {
    if (_contentController.text.trim().isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('请输入内容'),
          backgroundColor: AppTheme.warningColor,
        ),
      );
      return;
    }
    if (!_previewResult.canSubmit) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('内容状态异常，请先调整文本后再提交'),
          backgroundColor: AppTheme.warningColor,
        ),
      );
      return;
    }

    setState(() => _isSubmitting = true);

    try {
      // 内容审核由后端 StoneController.createStone 内置的
      // ContentFilter::checkContentSafety() 完成，高危内容返回403
      final result = await _stoneService.createStone(
        content: _contentController.text.trim(),
        stoneType: _selectedType,
        stoneColor: _selectedColor,
        moodType: _selectedMood.name,
        isAnonymous: true,
      );

      if (!mounted) return;

      if (result['success'] == true) {
        if (result['stone_id'] == null) {
          debugPrint('Warning: stone_id is null in success response');
        }
        // 清除石头列表缓存，确保刷新能拿到最新数据
        CacheService().removeByPrefix('GET:/lake/stones');
        CacheService().removeByPrefix('GET:/stones');

        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('你的石头已投入湖中，等待涟漪...'),
            backgroundColor: AppTheme.skyBlue,
          ),
        );
        _contentController.clear();
        setState(() {
          _selectedType = 'medium';
          _selectedColor = '#ADA59E';
          _selectedMood = MoodType.neutral;
          _emotionResult = null;
          _topEmotion = null;
          _aiSuggestedMood = null;
          _aiSuggestionConfidence = 0.0;
          _manualMoodLocked = false;
          _aiPulseMood = null;
          _previewResult = const AIPreviewResult();
        });
        widget.onPublished?.call();
        if (mounted && Navigator.of(context).canPop()) {
          Navigator.pop(context, true);
        }
      } else if (result['high_risk'] == true) {
        final tip = result['help_tip']?.toString();
        if (mounted) {
          PsychSupportDialog.show(context, helpTip: tip);
        }
      } else {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(result['message'] ?? '投石失败'),
            backgroundColor: AppTheme.errorColor,
          ),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('网络异常，请稍后再试'),
            backgroundColor: AppTheme.errorColor,
          ),
        );
      }
    } finally {
      if (mounted) {
        setState(() => _isSubmitting = false);
      }
    }
  }

  // 心情选择器UI
  Widget _buildMoodSelector() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final moods = [
      MoodType.happy,
      MoodType.calm,
      MoodType.sad,
      MoodType.anxious,
      MoodType.angry,
      MoodType.surprised,
      MoodType.confused,
      MoodType.neutral,
    ];

    return Wrap(
      spacing: 8,
      runSpacing: 8,
      children: moods.map((mood) {
        final config = MoodColors.getConfig(mood);
        final isSelected = _selectedMood == mood;
        final isSuggested = _aiSuggestedMood == mood;
        final isPulsing = _aiPulseMood == mood;

        return AnimatedScale(
          duration: const Duration(milliseconds: 220),
          curve: Curves.easeOutBack,
          scale: isPulsing ? 1.08 : 1.0,
          child: GestureDetector(
            onTap: () {
              setState(() {
                _manualMoodLocked = true;
                _selectedMood = mood;
                _updateStoneColor();
              });
            },
            child: AnimatedContainer(
              duration: const Duration(milliseconds: 220),
              padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
              decoration: BoxDecoration(
                color: isSelected ? config.primary : Colors.transparent,
                borderRadius: BorderRadius.circular(20),
                border: Border.all(
                  color: isSelected ? config.primary : (isDark ? Colors.white24 : Colors.grey.shade300),
                  width: isSelected ? 2 : 1,
                ),
                boxShadow: isSuggested
                    ? [
                        BoxShadow(
                          color: config.primary.withValues(
                            alpha: isPulsing ? 0.42 : 0.26,
                          ),
                          blurRadius: isPulsing ? 16 : 10,
                          spreadRadius: isPulsing ? 2 : 0,
                        ),
                      ]
                    : null,
              ),
              child: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(
                    config.icon,
                    size: 18,
                    color: isSelected ? Colors.white : config.primary,
                  ),
                  const SizedBox(width: 6),
                  Text(
                    config.name,
                    style: TextStyle(
                      color: isSelected ? Colors.white : (isDark ? const Color(0xFF9AA0A6) : Colors.grey.shade700),
                      fontWeight:
                          isSelected ? FontWeight.w600 : FontWeight.w400,
                      fontSize: 14,
                    ),
                  ),
                  if (isSuggested) ...[
                    const SizedBox(width: 4),
                    Icon(
                      Icons.auto_awesome,
                      size: 14,
                      color: isSelected
                          ? Colors.white.withValues(alpha: 0.95)
                          : config.primary,
                    ),
                  ],
                ],
              ),
            ),
          ),
        );
      }).toList(),
    );
  }

  // 根据心情和类型更新石头颜色
  void _updateStoneColor() {
    final moodConfig = MoodColors.getConfig(_selectedMood);

    // 将颜色转换为十六进制字符串（确保格式正确）
    String colorToHex(Color color) {
      final r = (color.r * 255.0)
          .round()
          .clamp(0, 255)
          .toRadixString(16)
          .padLeft(2, '0')
          .toUpperCase();
      final g = (color.g * 255.0)
          .round()
          .clamp(0, 255)
          .toRadixString(16)
          .padLeft(2, '0')
          .toUpperCase();
      final b = (color.b * 255.0)
          .round()
          .clamp(0, 255)
          .toRadixString(16)
          .padLeft(2, '0')
          .toUpperCase();
      return '#$r$g$b';
    }

    // 根据石头类型和心情设置颜色 - 使用完整的不透明颜色
    Color targetColor;
    if (_selectedType == 'light') {
      targetColor = Color.lerp(Colors.white, moodConfig.primary, 0.3)!;
    } else if (_selectedType == 'medium') {
      targetColor = Color.lerp(Colors.white, moodConfig.primary, 0.6)!;
    } else {
      targetColor = moodConfig.primary;
    }

    _selectedColor = colorToHex(targetColor);
  }
}

class _PublishButton extends StatefulWidget {
  final bool isSubmitting;
  final VoidCallback onPressed;
  final MoodColorConfig moodConfig;

  const _PublishButton({
    required this.isSubmitting,
    required this.onPressed,
    required this.moodConfig,
  });

  @override
  State<_PublishButton> createState() => _PublishButtonState();
}

class _PublishButtonState extends State<_PublishButton>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _scale;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
        vsync: this, duration: const Duration(milliseconds: 120));
    _scale = Tween<double>(begin: 1.0, end: 0.95)
        .animate(CurvedAnimation(parent: _controller, curve: Curves.easeInOut));
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTapDown: widget.isSubmitting ? null : (_) => _controller.forward(),
      onTapUp: widget.isSubmitting
          ? null
          : (_) {
              _controller.reverse();
              widget.onPressed();
            },
      onTapCancel: () => _controller.reverse(),
      child: AnimatedBuilder(
        // AnimatedBuilder is the correct Flutter widget name
        animation: _scale,
        builder: (context, child) =>
            Transform.scale(scale: _scale.value, child: child),
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 200),
          width: double.infinity,
          padding: const EdgeInsets.symmetric(vertical: 16),
          decoration: BoxDecoration(
            gradient: LinearGradient(
              colors: [
                widget.moodConfig.primary,
                widget.moodConfig.primary.withValues(alpha: 0.8)
              ],
            ),
            borderRadius: BorderRadius.circular(16),
            boxShadow: [
              BoxShadow(
                  color: widget.moodConfig.primary.withValues(alpha: 0.3),
                  blurRadius: 12,
                  offset: const Offset(0, 4)),
            ],
          ),
          child: Center(
            child: widget.isSubmitting
                ? const SizedBox(
                    height: 20,
                    width: 20,
                    child: CircularProgressIndicator(
                        strokeWidth: 2, color: Colors.white))
                : const Text('投入湖中',
                    style: TextStyle(
                        fontSize: 16,
                        fontWeight: FontWeight.bold,
                        color: Colors.white)),
          ),
        ),
      ),
    );
  }
}
