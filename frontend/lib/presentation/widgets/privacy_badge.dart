// @file privacy_badge.dart
// @brief 差分隐私保护徽章组件

import 'package:flutter/material.dart';
import '../../utils/app_theme.dart';

/// 隐私保护徽章 - 展示差分隐私保护状态
class PrivacyBadge extends StatelessWidget {
  const PrivacyBadge({super.key});

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () => _showPrivacyDialog(context),
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
        decoration: BoxDecoration(
          gradient: LinearGradient(
            colors: [
              const Color(0xFF26A69A).withValues(alpha: 0.12),
              const Color(0xFF42A5F5).withValues(alpha: 0.08),
            ],
          ),
          borderRadius: BorderRadius.circular(20),
          border: Border.all(
            color: const Color(0xFF26A69A).withValues(alpha: 0.25),
          ),
        ),
        child: const Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(
              Icons.lock_outline,
              size: 13,
              color: Color(0xFF26A69A),
            ),
            SizedBox(width: 4),
            Text(
              '差分隐私保护中',
              style: TextStyle(
                fontSize: 10,
                fontWeight: FontWeight.w500,
                color: Color(0xFF00897B),
              ),
            ),
          ],
        ),
      ),
    );
  }

  void _showPrivacyDialog(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(20),
        ),
        title: Row(
          children: [
            Container(
              padding: const EdgeInsets.all(8),
              decoration: BoxDecoration(
                color: const Color(0xFF26A69A).withValues(alpha: 0.1),
                borderRadius: BorderRadius.circular(10),
              ),
              child: const Icon(
                Icons.shield_outlined,
                color: Color(0xFF26A69A),
                size: 22,
              ),
            ),
            const SizedBox(width: 12),
            const Text(
              '隐私保护说明',
              style: TextStyle(
                fontSize: 17,
                fontWeight: FontWeight.w600,
              ),
            ),
          ],
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            _buildExplanationItem(
              Icons.analytics_outlined,
              '差分隐私技术',
              '我们在数据分析时添加数学噪声，确保无法从统计结果中反推出你的个人信息。',
            ),
            const SizedBox(height: 12),
            _buildExplanationItem(
              Icons.phone_android_outlined,
              '本地优先处理',
              '情绪分析优先在你的设备上完成，原始数据不会离开手机。',
            ),
            const SizedBox(height: 12),
            _buildExplanationItem(
              Icons.visibility_off_outlined,
              '匿名化保护',
              '即使数据上传，也会经过严格脱敏处理，无法关联到你的身份。',
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            style: TextButton.styleFrom(
              foregroundColor: Theme.of(context).colorScheme.primary,
            ),
            child: const Text('我知道了'),
          ),
        ],
      ),
    );
  }

  static Widget _buildExplanationItem(
    IconData icon,
    String title,
    String description,
  ) {
    return Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Icon(icon, size: 18, color: const Color(0xFF26A69A)),
        const SizedBox(width: 10),
        Expanded(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                title,
                style: const TextStyle(
                  fontSize: 13,
                  fontWeight: FontWeight.w600,
                  color: AppTheme.textPrimary,
                ),
              ),
              const SizedBox(height: 2),
              Text(
                description,
                style: const TextStyle(
                  fontSize: 12,
                  color: AppTheme.textSecondary,
                  height: 1.4,
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }
}
