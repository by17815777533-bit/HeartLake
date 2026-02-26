// @file vip_screen.dart
// @brief 灯权益页面 - 温暖灯火概念
// Created by 林子怡

import 'package:flutter/material.dart';
import '../../data/datasources/vip_service.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';

class VIPScreen extends StatefulWidget {
  const VIPScreen({super.key});

  @override
  State<VIPScreen> createState() => _VIPScreenState();
}

class _VIPScreenState extends State<VIPScreen> with SingleTickerProviderStateMixin {
  bool _isLoading = true;
  bool _hasLight = false; // 是否点亮灯
  int _daysLeft = 0;
  bool _privilegesLoading = true;
  List<_PrivilegeItem> _privileges = [];
  late AnimationController _animController;

  /// 硬编码的默认权益列表，作为 API 失败或返回空数据时的 fallback
  static final List<_PrivilegeItem> _defaultPrivileges = [
    _PrivilegeItem(Icons.water_drop, '湖神回应', '更频繁的温暖', const Color(0xFFFFB74D)),
    _PrivilegeItem(Icons.psychology, '心理咨询', '每月1次免费', const Color(0xFF81C784)),
    _PrivilegeItem(Icons.local_fire_department, '专属灯火', '温暖身份标识', const Color(0xFFFF8A65)),
    _PrivilegeItem(Icons.people, '优先匹配', '更快找到伙伴', const Color(0xFF64B5F6)),
    _PrivilegeItem(Icons.auto_awesome, '专属特效', '独特视觉体验', const Color(0xFFBA68C8)),
  ];

  /// 图标名称到 IconData 的映射
  static final Map<String, IconData> _iconMap = {
    'water_drop': Icons.water_drop,
    'psychology': Icons.psychology,
    'local_fire_department': Icons.local_fire_department,
    'people': Icons.people,
    'auto_awesome': Icons.auto_awesome,
    'favorite': Icons.favorite,
    'star': Icons.star,
    'shield': Icons.shield,
    'spa': Icons.spa,
    'lightbulb': Icons.lightbulb,
    'emoji_emotions': Icons.emoji_emotions,
    'health_and_safety': Icons.health_and_safety,
    'volunteer_activism': Icons.volunteer_activism,
    'diversity_3': Icons.diversity_3,
    'self_improvement': Icons.self_improvement,
  };

  @override
  void initState() {
    super.initState();
    _animController = AnimationController(vsync: this, duration: const Duration(seconds: 2))..repeat(reverse: true);
    _loadLightData();
    _loadPrivileges();
  }

  @override
  void dispose() {
    _animController.dispose();
    super.dispose();
  }

  final _vipService = sl<VIPService>();

  Future<void> _loadLightData() async {
    setState(() => _isLoading = true);
    try {
      final status = await _vipService.getVIPStatus();
      if (mounted) {
        setState(() {
          _hasLight = status['is_vip'] ?? false;
          _daysLeft = status['days_left'] ?? 0;
          _isLoading = false;
        });
      }
    } catch (e) {
      if (mounted) setState(() => _isLoading = false);
    }
  }

  Future<void> _loadPrivileges() async {
    setState(() => _privilegesLoading = true);
    try {
      final result = await _vipService.getPrivileges();
      if (mounted) {
        final List<dynamic> items = result['privileges'] ?? [];
        if (items.isNotEmpty) {
          setState(() {
            _privileges = items.map((item) {
              final iconName = item['icon'] as String? ?? 'star';
              final colorValue = item['color'] as int? ?? 0xFFFFB74D;
              return _PrivilegeItem(
                _iconMap[iconName] ?? Icons.star,
                item['title'] as String? ?? '',
                item['desc'] as String? ?? '',
                Color(colorValue),
              );
            }).toList();
            _privilegesLoading = false;
          });
        } else {
          // API 返回空列表，使用 fallback
          setState(() {
            _privileges = _defaultPrivileges;
            _privilegesLoading = false;
          });
        }
      }
    } catch (e) {
      // API 失败，使用 fallback
      if (mounted) {
        setState(() {
          _privileges = _defaultPrivileges;
          _privilegesLoading = false;
        });
      }
    }
  }

  Future<void> _refreshAll() async {
    await Future.wait([_loadLightData(), _loadPrivileges()]);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [AppTheme.lakeDeep.withValues(alpha: 0.9), AppTheme.nightDeep],
          ),
        ),
        child: SafeArea(
          child: _isLoading
              ? const Center(child: CircularProgressIndicator(color: AppTheme.vipGold))
              : RefreshIndicator(
                  onRefresh: _refreshAll,
                  child: ListView(
                    padding: const EdgeInsets.all(20),
                    children: [
                      _buildHeader(),
                      const SizedBox(height: 24),
                      _buildLampCard(),
                      const SizedBox(height: 24),
                      _buildPrivilegesGrid(),
                      const SizedBox(height: 24),
                      _buildInfoSection(),
                    ],
                  ),
                ),
        ),
      ),
    );
  }

  Widget _buildHeader() {
    return Row(
      children: [
        IconButton(
          icon: const Icon(Icons.arrow_back_ios, color: Colors.white70),
          onPressed: () => Navigator.pop(context),
        ),
        const Expanded(
          child: Text('灯', style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold, color: Colors.white), textAlign: TextAlign.center),
        ),
        const SizedBox(width: 48),
      ],
    );
  }

  Widget _buildLampCard() {
    return AnimatedBuilder(
      animation: _animController,
      builder: (context, child) {
        final glowOpacity = 0.4 + (_animController.value * 0.4);
        return Container(
          padding: const EdgeInsets.all(32),
          decoration: BoxDecoration(
            gradient: LinearGradient(
              colors: _hasLight
                  ? [AppTheme.vipGoldDark, AppTheme.vipGold]
                  : [const Color(0xFF37474F), const Color(0xFF546E7A)],
              begin: Alignment.topLeft,
              end: Alignment.bottomRight,
            ),
            borderRadius: BorderRadius.circular(24),
            boxShadow: _hasLight
                ? [BoxShadow(color: AppTheme.vipGold.withValues(alpha: glowOpacity), blurRadius: 30, spreadRadius: 5)]
                : null,
          ),
          child: Column(
            children: [
              Icon(
                _hasLight ? Icons.lightbulb : Icons.lightbulb_outline,
                size: 64,
                color: _hasLight ? Colors.white : Colors.white54,
              ),
              const SizedBox(height: 16),
              Text(
                _hasLight ? '灯已点亮' : '灯未点亮',
                style: const TextStyle(color: Colors.white, fontSize: 24, fontWeight: FontWeight.bold),
              ),
              if (_hasLight) ...[
                const SizedBox(height: 8),
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
                  decoration: BoxDecoration(
                    color: Colors.white.withValues(alpha: 0.2),
                    borderRadius: BorderRadius.circular(20),
                  ),
                  child: Text('灯火将燃 $_daysLeft 天', style: const TextStyle(color: Colors.white, fontSize: 14)),
                ),
              ],
            ],
          ),
        );
      },
    );
  }

  Widget _buildPrivilegesGrid() {
    final privileges = _privileges.isNotEmpty ? _privileges : _defaultPrivileges;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text('点灯权益', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Colors.white)),
        const SizedBox(height: 16),
        if (_privilegesLoading)
          const Center(
            child: Padding(
              padding: EdgeInsets.symmetric(vertical: 32),
              child: CircularProgressIndicator(color: AppTheme.vipGold, strokeWidth: 2),
            ),
          )
        else
          GridView.builder(
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
              crossAxisCount: 3,
              mainAxisSpacing: 12,
              crossAxisSpacing: 12,
              childAspectRatio: 0.9,
            ),
            itemCount: privileges.length,
            itemBuilder: (context, index) {
              final p = privileges[index];
              return Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.white.withValues(alpha: 0.1),
                  borderRadius: BorderRadius.circular(16),
                  border: Border.all(color: p.color.withValues(alpha: 0.3)),
                ),
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Icon(p.icon, color: p.color, size: 28),
                    const SizedBox(height: 8),
                    Text(p.title, style: const TextStyle(fontWeight: FontWeight.w600, fontSize: 12, color: Colors.white)),
                    const SizedBox(height: 2),
                    Text(p.desc, style: TextStyle(color: Colors.white.withValues(alpha: 0.6), fontSize: 10), textAlign: TextAlign.center),
                    if (_hasLight)
                      const Padding(
                        padding: EdgeInsets.only(top: 4),
                        child: Icon(Icons.check_circle, color: AppTheme.vipGold, size: 16),
                      ),
                  ],
                ),
              );
            },
          ),
      ],
    );
  }

  Widget _buildInfoSection() {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white.withValues(alpha: 0.1),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: AppTheme.vipGold.withValues(alpha: 0.3)),
      ),
      child: Column(
        children: [
          const Icon(Icons.favorite, color: AppTheme.vipGold, size: 32),
          const SizedBox(height: 12),
          const Text('灯关怀计划', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold, color: Colors.white)),
          const SizedBox(height: 8),
          Text(
            '灯完全免费！当系统感知到您需要温暖时，将自动为您点亮灯，照亮前行的路',
            style: TextStyle(color: Colors.white.withValues(alpha: 0.8), fontSize: 13),
            textAlign: TextAlign.center,
          ),
          const SizedBox(height: 16),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              _infoChip(Icons.auto_awesome, '智能感知'),
              const SizedBox(width: 12),
              _infoChip(Icons.local_fire_department, '自动点灯'),
              const SizedBox(width: 12),
              _infoChip(Icons.spa, '温暖陪伴'),
            ],
          ),
        ],
      ),
    );
  }

  Widget _infoChip(IconData icon, String label) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
      decoration: BoxDecoration(
        color: AppTheme.vipGold.withValues(alpha: 0.15),
        borderRadius: BorderRadius.circular(20),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 14, color: AppTheme.vipGold),
          const SizedBox(width: 4),
          Text(label, style: const TextStyle(fontSize: 11, color: AppTheme.vipGold)),
        ],
      ),
    );
  }
}

class _PrivilegeItem {
  final IconData icon;
  final String title;
  final String desc;
  final Color color;
  _PrivilegeItem(this.icon, this.title, this.desc, this.color);
}
