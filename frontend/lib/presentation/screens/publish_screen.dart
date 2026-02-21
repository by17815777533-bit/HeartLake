// @file publish_screen.dart
// @brief 发布石头界面 - 光遇风格重构
// Created by 林子怡

import 'package:flutter/material.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';
import '../widgets/sky_button.dart';
import '../widgets/sky_input.dart';
import '../widgets/ai_content_preview.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';
import '../../data/datasources/stone_service.dart';

class PublishScreen extends StatefulWidget {
  const PublishScreen({super.key});

  @override
  State<PublishScreen> createState() => _PublishScreenState();
}

class _PublishScreenState extends State<PublishScreen> {
  final TextEditingController _contentController = TextEditingController();
  final StoneService _stoneService = StoneService();
  String _selectedType = 'medium';
  String _selectedColor = '#ADA59E';
  MoodType _selectedMood = MoodType.neutral;
  bool _isSubmitting = false;
  String _contentText = '';
  AIPreviewResult _aiPreviewResult = const AIPreviewResult();

  final Map<String, String> _stoneTypes = {
    'light': '轻石',
    'medium': '中石',
    'heavy': '重石',
  };

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () => FocusScope.of(context).unfocus(),
      child: SkyScaffold(
        showWater: true,
        showParticles: true,
        appBar: AppBar(
          title: Text('投石',
              style: TextStyle(
                color: AppTheme.candleGlow,
                fontWeight: FontWeight.bold,
                shadows: [
                  Shadow(
                    color: AppTheme.candleGlow.withValues(alpha: 0.6),
                    blurRadius: 12,
                  ),
                ],
              )),
          centerTitle: true,
          backgroundColor: Colors.transparent,
          elevation: 0,
          scrolledUnderElevation: 0,
          foregroundColor: AppTheme.darkTextPrimary,
        ),
        body: SafeArea(
          child: SingleChildScrollView(
            padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 20),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // 标题
                Text(
                  '向湖中投入你的石头',
                  style: TextStyle(
                      fontSize: 28,
                      fontWeight: FontWeight.bold,
                      color: AppTheme.candleGlow,
                      shadows: [
                        Shadow(
                            color: AppTheme.candleGlow.withValues(alpha: 0.5),
                            offset: const Offset(0, 2),
                            blurRadius: 8)
                      ]),
                ),
                const SizedBox(height: 8),
                Text(
                  '这里是你的安全空间',
                  style: TextStyle(
                    fontSize: 16,
                    color: AppTheme.darkTextSecondary,
                  ),
                ),
                const SizedBox(height: 32),

                // 内容输入区域 - SkyGlassCard 包裹
                SkyGlassCard(
                  borderRadius: 24,
                  enableGlow: true,
                  glowColor: MoodColors.getConfig(_selectedMood).primary,
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      // 输入框 - SkyInput
                      SkyInput(
                        controller: _contentController,
                        hintText: '说说你的心情吧...',
                        maxLines: 6,
                        maxLength: 500,
                        onChanged: (text) {
                          setState(() => _contentText = text);
                        },
                      ),

                      const SizedBox(height: 12),

                      // AI内容审核预览
                      AIContentPreview(
                        text: _contentText,
                        onResultChanged: (result) {
                          setState(() => _aiPreviewResult = result);
                        },
                      ),
                    ],
                  ),
                ),

                const SizedBox(height: 20),

                // 心情选择器 - SkyGlassCard 包裹
                SkyGlassCard(
                  borderRadius: 20,
                  enableGlow: false,
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Text(
                        '选择心情',
                        style: TextStyle(
                          color: AppTheme.darkTextPrimary,
                          fontWeight: FontWeight.w600,
                          fontSize: 16,
                        ),
                      ),
                      const SizedBox(height: 12),
                      _buildMoodSelector(),
                    ],
                  ),
                ),

                const SizedBox(height: 20),

                // 石头类型选择 - SkyGlassCard 包裹
                SkyGlassCard(
                  borderRadius: 20,
                  enableGlow: false,
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Text(
                        '选择石头类型',
                        style: TextStyle(
                          color: AppTheme.darkTextPrimary,
                          fontWeight: FontWeight.w600,
                          fontSize: 16,
                        ),
                      ),
                      const SizedBox(height: 12),
                      Row(
                        children: _stoneTypes.entries.map((entry) {
                          final isSelected = _selectedType == entry.key;
                          final moodConfig =
                              MoodColors.getConfig(_selectedMood);
                          Color activeColor;
                          if (entry.key == 'light') {
                            activeColor = moodConfig.primary.withValues(alpha: 0.3);
                          } else if (entry.key == 'medium') {
                            activeColor = moodConfig.primary.withValues(alpha: 0.6);
                          } else {
                            activeColor = moodConfig.primary.withValues(alpha: 0.9);
                          }

                          return Expanded(
                            child: GestureDetector(
                              onTap: () {
                                setState(() {
                                  _selectedType = entry.key;
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
                                      : Colors.white.withValues(alpha: 0.08),
                                  borderRadius: BorderRadius.circular(12),
                                  border: Border.all(
                                    color: isSelected
                                        ? moodConfig.primary
                                        : Colors.white.withValues(alpha: 0.2),
                                    width: isSelected ? 2 : 1,
                                  ),
                                ),
                                child: Text(
                                  entry.value,
                                  textAlign: TextAlign.center,
                                  style: TextStyle(
                                    color: isSelected
                                        ? Colors.white
                                        : Colors.white.withValues(alpha: 0.6),
                                    fontWeight: FontWeight.w500,
                                  ),
                                ),
                              ),
                            ),
                          );
                        }).toList(),
                      ),
                    ],
                  ),
                ),

                const SizedBox(height: 32),

                // 发布按钮 - SkyButton
                SkyButton(
                  label: '投入湖中',
                  onPressed: (_contentController.text.trim().isEmpty || !_aiPreviewResult.canSubmit || _isSubmitting)
                      ? null
                      : _submitStone,
                  isLoading: _isSubmitting,
                  width: double.infinity,
                  icon: Icons.water_drop,
                ),

                const SizedBox(height: 24),
              ],
            ),
          ),
        ),
      ),
    );
  }

  @override
  void dispose() {
    _contentController.dispose();
    super.dispose();
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

    if (!_aiPreviewResult.canSubmit) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(_aiPreviewResult.message ?? '内容未通过审核，请修改后再试'),
          backgroundColor: AppTheme.warningColor,
        ),
      );
      return;
    }

    setState(() => _isSubmitting = true);

    try {
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
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('你的石头已投入湖中，等待涟漪...'),
            backgroundColor: AppTheme.skyBlue,
          ),
        );
        _contentController.clear();
      } else if (result['high_risk'] == true) {
        final tip = result['help_tip']?.toString();
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(tip ?? result['message'] ?? '内容不安全'),
            backgroundColor: AppTheme.warningColor,
          ),
        );
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

        return GestureDetector(
          onTap: () {
            setState(() {
              _selectedMood = mood;
              _updateStoneColor();
            });
          },
          child: AnimatedContainer(
            duration: const Duration(milliseconds: 200),
            padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
            decoration: BoxDecoration(
              color: isSelected
                  ? config.primary
                  : Colors.white.withValues(alpha: 0.08),
              borderRadius: BorderRadius.circular(20),
              border: Border.all(
                color: isSelected
                    ? config.primary
                    : Colors.white.withValues(alpha: 0.2),
                width: isSelected ? 2 : 1,
              ),
              boxShadow: isSelected
                  ? [
                      BoxShadow(
                        color: config.primary.withValues(alpha: 0.4),
                        blurRadius: 8,
                        spreadRadius: 1,
                      )
                    ]
                  : null,
            ),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(
                  config.icon,
                  size: 18,
                  color: isSelected
                      ? Colors.white
                      : Colors.white.withValues(alpha: 0.7),
                ),
                const SizedBox(width: 6),
                Text(
                  config.name,
                  style: TextStyle(
                    color: isSelected
                        ? Colors.white
                        : Colors.white.withValues(alpha: 0.7),
                    fontWeight: isSelected ? FontWeight.w600 : FontWeight.w400,
                    fontSize: 14,
                  ),
                ),
              ],
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
      final r = (color.r * 255).round().toRadixString(16).padLeft(2, '0').toUpperCase();
      final g = (color.g * 255).round().toRadixString(16).padLeft(2, '0').toUpperCase();
      final b = (color.b * 255).round().toRadixString(16).padLeft(2, '0').toUpperCase();
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
