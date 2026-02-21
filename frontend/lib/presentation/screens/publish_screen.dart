// @file publish_screen.dart
// @brief 发布石头界面
// Created by 林子怡

import 'package:flutter/material.dart';
import '../widgets/water_background.dart';
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
  MoodType _selectedMood = MoodType.neutral; // 新增：选中的心情
  bool _isSubmitting = false;

  final Map<String, String> _stoneTypes = {
    'light': '轻石',
    'medium': '中石',
    'heavy': '重石',
  };

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () => FocusScope.of(context).unfocus(),
      child: Scaffold(
        extendBodyBehindAppBar: true,
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
              color: isSelected ? config.primary : Colors.transparent,
              borderRadius: BorderRadius.circular(20),
              border: Border.all(
                color: isSelected ? config.primary : Colors.grey.shade300,
                width: isSelected ? 2 : 1,
              ),
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
                    color: isSelected ? Colors.white : Colors.grey.shade700,
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
      final r = color.r.toInt().toRadixString(16).padLeft(2, '0').toUpperCase();
      final g = color.g.toInt().toRadixString(16).padLeft(2, '0').toUpperCase();
      final b = color.b.toInt().toRadixString(16).padLeft(2, '0').toUpperCase();
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

class _PublishButtonState extends State<_PublishButton> with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _scale;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(vsync: this, duration: const Duration(milliseconds: 120));
    _scale = Tween<double>(begin: 1.0, end: 0.95).animate(CurvedAnimation(parent: _controller, curve: Curves.easeInOut));
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
      onTapUp: widget.isSubmitting ? null : (_) {
        _controller.reverse();
        widget.onPressed();
      },
      onTapCancel: () => _controller.reverse(),
      child: AnimatedBuilder(
        animation: _scale,
        builder: (context, child) => Transform.scale(scale: _scale.value, child: child),
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 200),
          width: double.infinity,
          padding: const EdgeInsets.symmetric(vertical: 16),
          decoration: BoxDecoration(
            gradient: LinearGradient(
              colors: [widget.moodConfig.primary, widget.moodConfig.primary.withValues(alpha: 0.8)],
            ),
            borderRadius: BorderRadius.circular(16),
            boxShadow: [
              BoxShadow(color: widget.moodConfig.primary.withValues(alpha: 0.3), blurRadius: 12, offset: const Offset(0, 4)),
            ],
          ),
          child: Center(
            child: widget.isSubmitting
                ? const SizedBox(height: 20, width: 20, child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white))
                : const Text('投入湖中', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold, color: Colors.white)),
          ),
        ),
      ),
    );
  }
}
