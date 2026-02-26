// @file onboarding_screen.dart
// @brief 新用户引导页面 - 介绍心湖核心概念

import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../widgets/water_background.dart';
import '../../utils/app_theme.dart';

class OnboardingScreen extends StatefulWidget {
  const OnboardingScreen({super.key});

  @override
  State<OnboardingScreen> createState() => _OnboardingScreenState();
}

class _OnboardingScreenState extends State<OnboardingScreen> {
  final PageController _controller = PageController();
  int _currentPage = 0;
  bool _isNavigating = false;

  Future<void> _completeOnboarding() async {
    if (_isNavigating) return;
    setState(() => _isNavigating = true);
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('onboarding_done', 'true');
    if (!mounted) return;
    context.go('/home');
  }

  static const _pages = [
    _PageData(
      icon: Icons.lens_blur,
      title: '石头',
      subtitle: '投入心湖的情绪',
      description: '每一颗石头都是你内心的声音\n轻轻投入湖中，让情绪找到归宿',
      color: AppTheme.warmOrange,
    ),
    _PageData(
      icon: Icons.water_drop_outlined,
      title: '涟漪',
      subtitle: '温暖的回应',
      description: '当有人被你的故事触动\n涟漪便会轻轻荡漾开来',
      color: AppTheme.skyBlue,
    ),
    _PageData(
      icon: Icons.sailing_outlined,
      title: '纸船',
      subtitle: '石头下的回应',
      description: '在石头下写一只纸船评论\n让温暖在公开互动中被看见',
      color: AppTheme.gentlePurple,
    ),
    _PageData(
      icon: Icons.lightbulb_outline,
      title: '守护者',
      subtitle: '守护心湖的光',
      description: '在他人需要时点亮一盏灯\n成为黑夜中温暖的陪伴',
      color: AppTheme.accentColor,
    ),
  ];

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      body: Stack(
        children: [
          const Positioned.fill(child: WaterBackground()),
          SafeArea(
            child: Column(
              children: [
                Align(
                  alignment: Alignment.topRight,
                  child: TextButton(
                    onPressed: _completeOnboarding,
                    child: Text('跳过', style: TextStyle(color: isDark ? Colors.white70 : Colors.white)),
                  ),
                ),
                Expanded(
                  child: PageView.builder(
                    controller: _controller,
                    onPageChanged: (i) => setState(() => _currentPage = i),
                    itemCount: _pages.length,
                    itemBuilder: (_, i) => _buildPage(_pages[i], isDark),
                  ),
                ),
                _buildIndicator(),
                Padding(
                  padding: const EdgeInsets.all(24),
                  child: SizedBox(
                    width: double.infinity,
                    child: ElevatedButton(
                      onPressed: _currentPage == _pages.length - 1
                          ? _completeOnboarding
                          : () => _controller.nextPage(duration: const Duration(milliseconds: 300), curve: Curves.easeOut),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: _pages[_currentPage].color,
                        padding: const EdgeInsets.symmetric(vertical: 16),
                      ),
                      child: Text(_currentPage == _pages.length - 1 ? '开始探索' : '继续'),
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

  Widget _buildPage(_PageData data, bool isDark) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 32),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Container(
            width: 120,
            height: 120,
            decoration: BoxDecoration(
              color: data.color.withValues(alpha: 0.2),
              shape: BoxShape.circle,
              border: Border.all(color: data.color.withValues(alpha: 0.5), width: 2),
            ),
            child: Icon(data.icon, size: 56, color: Colors.white),
          ),
          const SizedBox(height: 40),
          Text(data.title, style: const TextStyle(fontSize: 36, fontWeight: FontWeight.bold, color: Colors.white)),
          const SizedBox(height: 8),
          Text(data.subtitle, style: TextStyle(fontSize: 18, color: Colors.white.withValues(alpha: 0.8))),
          const SizedBox(height: 24),
          Text(data.description, textAlign: TextAlign.center, style: TextStyle(fontSize: 16, color: Colors.white.withValues(alpha: 0.7), height: 1.6)),
        ],
      ),
    );
  }

  Widget _buildIndicator() {
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: List.generate(_pages.length, (i) => AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        margin: const EdgeInsets.symmetric(horizontal: 4),
        width: _currentPage == i ? 24 : 8,
        height: 8,
        decoration: BoxDecoration(
          color: _currentPage == i ? _pages[i].color : Colors.white.withValues(alpha: 0.3),
          borderRadius: BorderRadius.circular(4),
        ),
      )),
    );
  }
}

class _PageData {
  final IconData icon;
  final String title;
  final String subtitle;
  final String description;
  final Color color;
  const _PageData({required this.icon, required this.title, required this.subtitle, required this.description, required this.color});
}
