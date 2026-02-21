import 'dart:ui';
import 'package:flutter/material.dart';
import '../../data/datasources/guardian_service.dart';
import '../../utils/app_theme.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';
import '../../utils/animation_utils.dart';
import 'lake_god_chat_screen.dart';

class GuardianScreen extends StatefulWidget {
  const GuardianScreen({super.key});

  @override
  State<GuardianScreen> createState() => _GuardianScreenState();
}

class _GuardianScreenState extends State<GuardianScreen>
    with SingleTickerProviderStateMixin {
  final _service = GuardianService();
  Map<String, dynamic>? _stats;
  bool _loading = true;
  late AnimationController _animController;
  late Animation<double> _fadeAnim;
  late Animation<double> _scaleAnim;

  @override
  void initState() {
    super.initState();
    _animController = AnimationController(
      duration: const Duration(milliseconds: 600),
      vsync: this,
    );
    _fadeAnim = Tween<double>(begin: 0.0, end: 1.0).animate(
      CurvedAnimation(parent: _animController, curve: Curves.easeOut),
    );
    _scaleAnim = Tween<double>(begin: 0.8, end: 1.0).animate(
      CurvedAnimation(parent: _animController, curve: Curves.easeOutBack),
    );
    _loadStats();
  }

  @override
  void dispose() {
    _animController.dispose();
    super.dispose();
  }

  Future<void> _loadStats() async {
    try {
      final stats = await _service.getStats();
      setState(() { _stats = stats; _loading = false; });
      _animController.forward();
    } catch (e) {
      setState(() => _loading = false);
    }
  }

  Future<void> _showTransferDialog() async {
    final controller = TextEditingController();
    final result = await showDialog<String>(
      context: context,
      builder: (ctx) => AlertDialog(
        backgroundColor: AppTheme.nightSurface,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
        title: Text(
          '转赠灯火',
          style: TextStyle(
            color: AppTheme.candleGlow,
            fontWeight: FontWeight.bold,
            shadows: [
              Shadow(color: AppTheme.candleGlow.withValues(alpha: 0.5), blurRadius: 8),
            ],
          ),
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text('将你的温暖传递给需要的人',
                style: TextStyle(color: AppTheme.darkTextSecondary)),
            const SizedBox(height: 16),
            TextField(
              controller: controller,
              style: const TextStyle(color: AppTheme.darkTextPrimary),
              decoration: InputDecoration(
                labelText: '对方ID',
                labelStyle: const TextStyle(color: AppTheme.darkTextSecondary),
                filled: true,
                fillColor: AppTheme.nightDeep,
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                  borderSide: BorderSide(color: AppTheme.candleGlow.withValues(alpha: 0.3)),
                ),
                enabledBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                  borderSide: BorderSide(color: AppTheme.candleGlow.withValues(alpha: 0.2)),
                ),
                focusedBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                  borderSide: const BorderSide(color: AppTheme.candleGlow, width: 1.5),
                ),
              ),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('取消', style: TextStyle(color: AppTheme.darkTextSecondary)),
          ),
          ElevatedButton(
            onPressed: () => Navigator.pop(ctx, controller.text.trim()),
            style: ElevatedButton.styleFrom(
              backgroundColor: AppTheme.warmOrange,
              foregroundColor: AppTheme.nightDeep,
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
            ),
            child: const Text('转赠', style: TextStyle(fontWeight: FontWeight.bold)),
          ),
        ],
      ),
    );
    controller.dispose();
    if (result != null && result.isNotEmpty && mounted) {
      try {
        final response = await _service.transferLamp(result);
        final bool success = response['success'] == true;
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(
                success ? '灯火已传递' : '转赠失败',
                style: const TextStyle(color: AppTheme.darkTextPrimary),
              ),
              backgroundColor: success
                  ? AppTheme.nightSurface
                  : AppTheme.warmPink.withValues(alpha: 0.9),
              behavior: SnackBarBehavior.floating,
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
            ),
          );
          if (success) _loadStats();
        }
      } catch (e) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: const Text('网络异常，请稍后再试',
                  style: TextStyle(color: AppTheme.darkTextPrimary)),
              backgroundColor: AppTheme.nightSurface,
              behavior: SnackBarBehavior.floating,
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
            ),
          );
        }
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showParticles: true,
      showWater: true,
      appBar: AppBar(
        title: Text(
          '点灯人',
          style: TextStyle(
            color: AppTheme.candleGlow,
            fontWeight: FontWeight.bold,
            shadows: [
              Shadow(color: AppTheme.candleGlow.withValues(alpha: 0.6), blurRadius: 12),
            ],
          ),
        ),
        backgroundColor: Colors.transparent,
        elevation: 0,
        scrolledUnderElevation: 0,
        foregroundColor: AppTheme.darkTextPrimary,
      ),
      body: SafeArea(
        child: _loading
            ? const Center(child: CircularProgressIndicator(color: AppTheme.candleGlow))
            : _stats == null
                ? const Center(child: Text('暂无数据', style: TextStyle(color: AppTheme.darkTextPrimary)))
                : FadeTransition(
                    opacity: _fadeAnim,
                    child: ScaleTransition(
                      scale: _scaleAnim,
                      child: ListView(
                        padding: const EdgeInsets.all(16),
                        children: [
                          // 主卡片 - 点灯人状态
                          SkyGlassCard(
                            glowColor: _stats!['is_guardian'] == true
                                ? AppTheme.candleGlow
                                : AppTheme.spiritBlue,
                            padding: const EdgeInsets.all(20),
                            child: Column(
                              children: [
                                Icon(
                                  _stats!['is_guardian'] == true
                                      ? Icons.local_fire_department
                                      : Icons.local_fire_department_outlined,
                                  size: 64,
                                  color: _stats!['is_guardian'] == true
                                      ? AppTheme.candleGlow
                                      : AppTheme.darkTextSecondary,
                                ),
// PLACEHOLDER_BUILD_CONT
