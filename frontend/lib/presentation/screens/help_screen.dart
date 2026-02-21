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
            colors: [AppTheme.skyBlue.withOpacity(0.05), Colors.white],
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
        boxShadow: [BoxShadow(color: Colors.black.withOpacity(0.03), blurRadius: 8)],
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

  void _showContactInfo(BuildContext context, String message) {
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(message), backgroundColor: AppTheme.skyBlue));
  }
}
