// @file privacy_settings_screen.dart
// @brief 隐私与安全设置页面 - 隐私开关、数据导出、账号注销
// Created by AI Assistant

import 'package:flutter/material.dart';
import '../../data/datasources/account_service.dart';
import '../../utils/app_theme.dart';

class PrivacySettingsScreen extends StatefulWidget {
  const PrivacySettingsScreen({super.key});

  @override
  State<PrivacySettingsScreen> createState() => _PrivacySettingsScreenState();
}

class _PrivacySettingsScreenState extends State<PrivacySettingsScreen> {
  final AccountService _accountService = AccountService();
  bool _isLoading = true;
  bool _isSaving = false;
  bool _isExporting = false;

  // 隐私设置项
  bool _showOnlineStatus = true;
  bool _allowStrangerBoat = true;
  bool _showMoodHistory = false;
  bool _allowResonanceMatch = true;
  bool _showProfileToStranger = true;

  @override
  void initState() {
    super.initState();
    _loadPrivacySettings();
  }

  Future<void> _loadPrivacySettings() async {
    try {
      final result = await _accountService.getPrivacySettings();
      final data = result['data'] as Map<String, dynamic>? ?? {};
      if (mounted) {
        setState(() {
          _showOnlineStatus = data['show_online_status'] ?? true;
          _allowStrangerBoat = data['allow_stranger_boat'] ?? true;
          _showMoodHistory = data['show_mood_history'] ?? false;
          _allowResonanceMatch = data['allow_resonance_match'] ?? true;
          _showProfileToStranger = data['show_profile_to_stranger'] ?? true;
          _isLoading = false;
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() => _isLoading = false);
      }
    }
  }

  Future<void> _saveSettings() async {
    setState(() => _isSaving = true);
    try {
      await _accountService.updatePrivacySettings({
        'show_online_status': _showOnlineStatus,
        'allow_stranger_boat': _allowStrangerBoat,
        'show_mood_history': _showMoodHistory,
        'allow_resonance_match': _allowResonanceMatch,
        'show_profile_to_stranger': _showProfileToStranger,
      });
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('隐私设置已保存'), behavior: SnackBarBehavior.floating),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('保存失败: $e'), backgroundColor: AppTheme.errorColor),
        );
      }
    } finally {
      if (mounted) setState(() => _isSaving = false);
    }
  }

  Future<void> _exportData() async {
    final confirm = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('导出个人数据'),
        content: const Text('我们将打包你的所有数据（石头、纸船、涟漪、个人资料等），准备好后会通知你下载。'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx, false), child: const Text('取消')),
          FilledButton(onPressed: () => Navigator.pop(ctx, true), child: const Text('开始导出')),
        ],
      ),
    );
    if (confirm != true) return;

    setState(() => _isExporting = true);
    try {
      await _accountService.exportData();
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('数据导出已开始，完成后会通知你'), behavior: SnackBarBehavior.floating),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('导出失败: $e'), backgroundColor: AppTheme.errorColor),
        );
      }
    } finally {
      if (mounted) setState(() => _isExporting = false);
    }
  }

  Future<void> _deactivateAccount() async {
    final confirm = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('停用账号'),
        content: const Text('停用后你的资料将不可见，但数据会保留 30 天。30 天内重新登录即可恢复。'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx, false), child: const Text('取消')),
          FilledButton(
            style: FilledButton.styleFrom(backgroundColor: AppTheme.errorColor),
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('确认停用'),
          ),
        ],
      ),
    );
    if (confirm != true) return;

    try {
      await _accountService.deactivateAccount();
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('账号已停用'), behavior: SnackBarBehavior.floating),
        );
        Navigator.of(context).popUntil((route) => route.isFirst);
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('操作失败: $e'), backgroundColor: AppTheme.errorColor),
        );
      }
    }
  }

  Future<void> _deleteAccountPermanently() async {
    final firstConfirm = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('永久删除账号'),
        content: const Text('此操作不可撤销！你的所有数据（石头、纸船、涟漪、个人资料）将被永久删除。'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx, false), child: const Text('取消')),
          FilledButton(
            style: FilledButton.styleFrom(backgroundColor: AppTheme.errorColor),
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('我理解，继续'),
          ),
        ],
      ),
    );
    if (firstConfirm != true) return;
    if (!mounted) return;

    // 二次确认
    final secondConfirm = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('最后确认'),
        content: const Text('请再次确认：你真的要永久删除账号吗？这将清除你在心湖的所有痕迹。'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx, false), child: const Text('我再想想')),
          FilledButton(
            style: FilledButton.styleFrom(backgroundColor: AppTheme.errorColor),
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('永久删除'),
          ),
        ],
      ),
    );
    if (secondConfirm != true) return;

    try {
      await _accountService.deleteAccountPermanently();
      if (mounted) {
        Navigator.of(context).popUntil((route) => route.isFirst);
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('删除失败: $e'), backgroundColor: AppTheme.errorColor),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      appBar: AppBar(title: const Text('隐私与安全')),
      body: _isLoading
          ? const Center(child: CircularProgressIndicator())
          : ListView(
              padding: const EdgeInsets.all(16),
              children: [
                // 隐私设置
                _buildSectionHeader('隐私设置'),
                Card(
                  child: Column(
                    children: [
                      _buildSwitch(
                        icon: Icons.circle, iconColor: isDark ? Colors.greenAccent : Colors.green,
                        title: '显示在线状态', subtitle: '其他用户可以看到你是否在线',
                        value: _showOnlineStatus,
                        onChanged: (v) { setState(() => _showOnlineStatus = v); _saveSettings(); },
                      ),
                      const Divider(height: 1),
                      _buildSwitch(
                        icon: Icons.mail_outline, iconColor: AppTheme.skyBlue,
                        title: '允许陌生人发纸船', subtitle: '关闭后只有好友可以给你发纸船',
                        value: _allowStrangerBoat,
                        onChanged: (v) { setState(() => _allowStrangerBoat = v); _saveSettings(); },
                      ),
                      const Divider(height: 1),
                      _buildSwitch(
                        icon: Icons.show_chart, iconColor: Colors.orange,
                        title: '公开情绪历史', subtitle: '其他用户可以看到你的情绪变化趋势',
                        value: _showMoodHistory,
                        onChanged: (v) { setState(() => _showMoodHistory = v); _saveSettings(); },
                      ),
                      const Divider(height: 1),
                      _buildSwitch(
                        icon: Icons.favorite_outline, iconColor: Colors.pink,
                        title: '参与情绪共鸣匹配', subtitle: '允许系统根据情绪状态为你匹配共鸣伙伴',
                        value: _allowResonanceMatch,
                        onChanged: (v) { setState(() => _allowResonanceMatch = v); _saveSettings(); },
                      ),
                      const Divider(height: 1),
                      _buildSwitch(
                        icon: Icons.person_outline, iconColor: AppTheme.skyBlue,
                        title: '对陌生人可见', subtitle: '关闭后你的资料页仅好友可见',
                        value: _showProfileToStranger,
                        onChanged: (v) { setState(() => _showProfileToStranger = v); _saveSettings(); },
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 24),

                // 数据管理
                _buildSectionHeader('数据管理'),
                Card(
                  child: Column(
                    children: [
                      ListTile(
                        leading: const Icon(Icons.download_outlined, color: AppTheme.skyBlue),
                        title: const Text('导出我的数据'),
                        subtitle: const Text('下载你在心湖的所有数据'),
                        trailing: _isExporting
                            ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2))
                            : const Icon(Icons.chevron_right),
                        onTap: _isExporting ? null : _exportData,
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 24),

                // 账号管理（危险区域）
                _buildSectionHeader('账号管理'),
                Card(
                  child: Column(
                    children: [
                      ListTile(
                        leading: Icon(Icons.pause_circle_outline, color: isDark ? Colors.orangeAccent : Colors.orange),
                        title: const Text('停用账号'),
                        subtitle: const Text('暂时隐藏你的资料，30 天内可恢复'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: _deactivateAccount,
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.delete_forever, color: AppTheme.errorColor),
                        title: const Text('永久删除账号', style: TextStyle(color: AppTheme.errorColor)),
                        subtitle: const Text('不可撤销，所有数据将被永久删除'),
                        trailing: const Icon(Icons.chevron_right, color: AppTheme.errorColor),
                        onTap: _deleteAccountPermanently,
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 32),
              ],
            ),
    );
  }

  Widget _buildSectionHeader(String title) {
    return Padding(
      padding: const EdgeInsets.only(left: 4, bottom: 8),
      child: Text(title, style: Theme.of(context).textTheme.titleSmall?.copyWith(
        color: Theme.of(context).colorScheme.primary, fontWeight: FontWeight.w600,
      )),
    );
  }

  Widget _buildSwitch({
    required IconData icon,
    required Color iconColor,
    required String title,
    required String subtitle,
    required bool value,
    required ValueChanged<bool> onChanged,
  }) {
    return SwitchListTile(
      secondary: Icon(icon, color: iconColor),
      title: Text(title),
      subtitle: Text(subtitle, style: const TextStyle(fontSize: 12)),
      value: value,
      onChanged: _isSaving ? null : onChanged,
    );
  }
}
