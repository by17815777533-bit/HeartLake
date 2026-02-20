// @file vip_screen.dart
// @brief 灯权益页面 - 温暖灯火概念
// Created by 林子怡

import 'package:flutter/material.dart';
import '../../data/datasources/vip_service.dart';

class VIPScreen extends StatefulWidget {
  const VIPScreen({super.key});

  @override
  State<VIPScreen> createState() => _VIPScreenState();
}

class _VIPScreenState extends State<VIPScreen> with SingleTickerProviderStateMixin {
  bool _isLoading = true;
  bool _hasLight = false; // 是否点亮灯
  int _daysLeft = 0;
  late AnimationController _animController;

  @override
  void initState() {
    super.initState();
    _animController = AnimationController(vsync: this, duration: const Duration(seconds: 2))..repeat(reverse: true);
    _loadLightData();
  }

  @override
  void dispose() {
    _animController.dispose();
    super.dispose();
  }

  final _vipService = VIPService();

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

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [const Color(0xFF1A237E).withOpacity(0.9), const Color(0xFF0D1B2A)],
          ),
        ),
        child: SafeArea(
          child: _isLoading
              ? const Center(child: CircularProgressIndicator(color: Color(0xFFFFD54F)))
              : RefreshIndicator(
                  onRefresh: _loadLightData,
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
                  ? [const Color(0xFFFF8F00), const Color(0xFFFFD54F)]
                  : [const Color(0xFF37474F), const Color(0xFF546E7A)],
              begin: Alignment.topLeft,
              end: Alignment.bottomRight,
            ),
            borderRadius: BorderRadius.circular(24),
            boxShadow: _hasLight
                ? [BoxShadow(color: const Color(0xFFFFD54F).withOpacity(glowOpacity), blurRadius: 30, spreadRadius: 5)]
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
                    color: Colors.white.withOpacity(0.2),
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
    final privileges = [
      _PrivilegeItem(Icons.water_drop, '湖神回应', '更频繁的温暖', const Color(0xFFFFB74D)),
      _PrivilegeItem(Icons.psychology, '心理咨询', '每月1次免费', const Color(0xFF81C784)),
      _PrivilegeItem(Icons.local_fire_department, '专属灯火', '温暖身份标识', const Color(0xFFFF8A65)),
      _PrivilegeItem(Icons.people, '优先匹配', '更快找到伙伴', const Color(0xFF64B5F6)),
      _PrivilegeItem(Icons.auto_awesome, '专属特效', '独特视觉体验', const Color(0xFFBA68C8)),
    ];

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text('点灯权益', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Colors.white)),
        const SizedBox(height: 16),
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
                color: Colors.white.withOpacity(0.1),
                borderRadius: BorderRadius.circular(16),
                border: Border.all(color: p.color.withOpacity(0.3)),
              ),
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Icon(p.icon, color: p.color, size: 28),
                  const SizedBox(height: 8),
                  Text(p.title, style: const TextStyle(fontWeight: FontWeight.w600, fontSize: 12, color: Colors.white)),
                  const SizedBox(height: 2),
                  Text(p.desc, style: TextStyle(color: Colors.white.withOpacity(0.6), fontSize: 10), textAlign: TextAlign.center),
                  if (_hasLight)
                    const Padding(
                      padding: EdgeInsets.only(top: 4),
                      child: Icon(Icons.check_circle, color: Color(0xFFFFD54F), size: 16),
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
        color: Colors.white.withOpacity(0.1),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: const Color(0xFFFFD54F).withOpacity(0.3)),
      ),
      child: Column(
        children: [
          const Icon(Icons.favorite, color: Color(0xFFFFD54F), size: 32),
          const SizedBox(height: 12),
          const Text('灯关怀计划', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold, color: Colors.white)),
          const SizedBox(height: 8),
          Text(
            '灯完全免费！当系统感知到您需要温暖时，将自动为您点亮灯，照亮前行的路',
            style: TextStyle(color: Colors.white.withOpacity(0.8), fontSize: 13),
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
        color: const Color(0xFFFFD54F).withOpacity(0.15),
        borderRadius: BorderRadius.circular(20),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 14, color: const Color(0xFFFFD54F)),
          const SizedBox(width: 4),
          Text(label, style: const TextStyle(fontSize: 11, color: Color(0xFFFFD54F))),
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
