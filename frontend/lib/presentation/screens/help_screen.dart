// @file help_screen.dart
// @brief 帮助页面
// Created by 林子怡

import 'package:flutter/material.dart';
import '../../utils/app_theme.dart';

class HelpScreen extends StatelessWidget {
  const HelpScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('帮助与反馈'), centerTitle: true),
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [AppTheme.skyBlue.withValues(alpha: 0.05), Colors.white],
          ),
        ),
        child: ListView(
          padding: const EdgeInsets.all(16),
          children: [
            _buildSection('常见问题', [
              _buildFAQ('如何发布石头？', '点击首页底部的"投石"按钮，选择心情后输入内容即可发布。'),
              _buildFAQ('什么是涟漪？', '涟漪是对石头的共鸣反馈，表示你对这条内容产生了共鸣。'),
              _buildFAQ('什么是纸船？', '纸船是匿名私信功能，可以向石头作者发送温暖的私密消息。'),
              _buildFAQ('如何添加好友？', '通过纸船互动建立临时好友关系，24小时内保持互动可升级为永久好友。'),
              _buildFAQ('灯是什么？', '灯是温暖的陪伴，当系统感知到您需要关怀时会自动点亮，完全免费。'),
              _buildFAQ('如何举报不当内容？', '长按石头或纸船内容，选择"举报"，选择举报原因并提交。我们会在24小时内处理。'),
              _buildFAQ('什么是守护者？', '守护者是心湖的温暖守护系统，当检测到你可能需要关怀时，会为你点亮一盏灯，提供情感支持和建议。'),
              _buildFAQ('VIP有什么特权？', 'VIP用户享有更多灯火关怀、专属心理咨询预约、AI深度情感分析等特权。'),
              _buildFAQ('如何保护我的隐私？', '心湖采用端到端加密和差分隐私技术保护你的数据。你可以在设置中管理隐私选项，包括屏蔽用户和数据导出。'),
              _buildFAQ('情感日历是什么？', '情感日历记录你每天的心情变化，以热力图形式展示，帮助你了解自己的情感趋势。'),
              _buildFAQ('如何注销账号？', '在设置 > 账号管理中可以选择停用或永久删除账号。停用后30天内可恢复，永久删除不可逆。'),
            ]),
            const SizedBox(height: 24),
            _buildSection('安全须知', [
              _buildSafetyTip(Icons.visibility_off, '心湖是匿名社区，请不要在石头中透露个人真实信息。'),
              _buildSafetyTip(Icons.flag, '如遇到骚扰或不当内容，请及时举报。'),
              _buildSafetyTip(Icons.phone, '如果你正在经历心理困扰，请拨打心理援助热线：400-161-9995'),
            ]),
            const SizedBox(height: 24),
            _buildSection('联系我们', [
              Container(
                decoration: BoxDecoration(
                  gradient: LinearGradient(
                    colors: [Colors.blue.shade50, Colors.cyan.shade50],
                  ),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: ListTile(
                  leading: const Icon(Icons.email, color: AppTheme.skyBlue),
                  title: const Text('邮箱反馈'),
                  subtitle: const Text('support@heartlake.app'),
                  onTap: () => _showContactInfo(context, '请发送邮件至 support@heartlake.app'),
                ),
              ),
            ]),
          ],
        ),
      ),
    );
  }

  Widget _buildSection(String title, List<Widget> children) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(title, style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: AppTheme.textPrimary)),
        const SizedBox(height: 12),
        ...children,
      ],
    );
  }

  Widget _buildFAQ(String question, String answer) {
    return Container(
      margin: const EdgeInsets.only(bottom: 8),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        boxShadow: [BoxShadow(color: Colors.black.withValues(alpha: 0.03), blurRadius: 8)],
      ),
      child: ExpansionTile(
        title: Text(question, style: const TextStyle(fontWeight: FontWeight.w500, fontSize: 15)),
        childrenPadding: const EdgeInsets.fromLTRB(16, 0, 16, 16),
        children: [
          Text(answer, style: TextStyle(color: Colors.grey[600], height: 1.5)),
        ],
      ),
    );
  }

  Widget _buildSafetyTip(IconData icon, String text) {
    return Container(
      margin: const EdgeInsets.only(bottom: 8),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        boxShadow: [BoxShadow(color: Colors.black.withValues(alpha: 0.03), blurRadius: 8)],
      ),
      child: ListTile(
        leading: Icon(icon, color: AppTheme.skyBlue, size: 22),
        title: Text(text, style: TextStyle(color: Colors.grey[700], fontSize: 14, height: 1.5)),
      ),
    );
  }

  void _showContactInfo(BuildContext context, String message) {
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(message), backgroundColor: AppTheme.skyBlue));
  }
}
