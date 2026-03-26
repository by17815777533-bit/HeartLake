// 隐私与安全设置页面
//
// 提供隐私开关、数据导出和账号注销等功能。

import 'dart:async';
import 'package:flutter/material.dart';
import '../../data/datasources/account_service.dart';
import '../../data/datasources/auth_service.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';

/// 隐私与安全设置页面
///
/// 用户可在此管理个人隐私偏好：
/// - 隐私开关：资料可见性、在线状态、陌生人纸船等真实生效的设置
/// - 数据导出：申请导出个人数据（GDPR 合规）
/// - 账号停用 / 永久删除（不可逆操作，需二次确认）
class PrivacySettingsScreen extends StatefulWidget {
  const PrivacySettingsScreen({super.key});

  @override
  State<PrivacySettingsScreen> createState() => _PrivacySettingsScreenState();
}

/// 隐私与安全设置页面状态管理
///
/// 通过 debounce 机制合并频繁的开关操作，减少网络请求。
/// 保存期间禁用所有开关，防止并发写入冲突。
class _PrivacySettingsScreenState extends State<PrivacySettingsScreen> {
  final AccountService _accountService = sl<AccountService>();
  bool _isLoading = true;
  bool _isSaving = false;
  bool _isExporting = false;
  bool _pendingSaveRequested = false;
  String? _loadErrorMessage;
  Timer? _saveDebounceTimer;

  // 隐私设置项
  bool _showOnlineStatus = true;
  bool _allowStrangerBoat = true;
  bool _showProfileToStranger = true;
  bool _savedShowOnlineStatus = true;
  bool _savedAllowStrangerBoat = true;
  bool _savedShowProfileToStranger = true;

  /// 将动态类型安全转换为 bool，兼容 String / num / bool 三种后端返回格式
  bool _asBool(dynamic value, {bool fallback = false}) {
    if (value is bool) return value;
    if (value is String) {
      final normalized = value.trim().toLowerCase();
      if (normalized == 'true') return true;
      if (normalized == 'false') return false;
    }
    if (value is num) {
      return value != 0;
    }
    return fallback;
  }

  @override
  void initState() {
    super.initState();
    _loadPrivacySettings();
  }

  /// 从后端加载当前隐私设置，映射到各开关状态
  Future<void> _loadPrivacySettings() async {
    try {
      final result = await _accountService.getPrivacySettings();
      if (result['success'] != true) {
        throw Exception(_resolveErrorMessage(result, '加载隐私设置失败'));
      }
      final data = result['data'] as Map<String, dynamic>? ?? {};
      if (mounted) {
        setState(() {
          final visibility = data['profile_visibility']?.toString() ?? 'public';
          _showOnlineStatus =
              _asBool(data['show_online_status'], fallback: true);
          _allowStrangerBoat = _asBool(
            data['allow_message_from_stranger'],
            fallback: true,
          );
          _showProfileToStranger = visibility != 'private';
          _savedShowOnlineStatus = _showOnlineStatus;
          _savedAllowStrangerBoat = _allowStrangerBoat;
          _savedShowProfileToStranger = _showProfileToStranger;
          _isLoading = false;
          _loadErrorMessage = null;
        });
      }
    } catch (e, stackTrace) {
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: e,
          stack: stackTrace,
          library: 'privacy_settings_screen',
          context: ErrorDescription('while loading privacy settings'),
        ),
      );
      if (mounted) {
        setState(() {
          _isLoading = false;
          _loadErrorMessage = _extractErrorMessage(e, '加载隐私设置失败，请重试');
        });
      }
    }
  }

  /// 组装隐私设置请求体，只发送当前后端真实支持的字段。
  Map<String, dynamic> _buildPrivacyPayload() {
    final visibility = _showProfileToStranger ? 'public' : 'private';
    return {
      'profile_visibility': visibility,
      'show_online_status': _showOnlineStatus,
      'allow_message_from_stranger': _allowStrangerBoat,
    };
  }

  String _resolveErrorMessage(Map<String, dynamic> result, String fallback) {
    final message = result['message']?.toString().trim();
    return message == null || message.isEmpty ? fallback : message;
  }

  String _extractErrorMessage(Object error, String fallback) {
    final raw = error.toString().replaceFirst(RegExp(r'^Exception:\s*'), '');
    final message = raw.trim();
    return message.isEmpty ? fallback : message;
  }

  void _restoreSavedSettings() {
    _showOnlineStatus = _savedShowOnlineStatus;
    _allowStrangerBoat = _savedAllowStrangerBoat;
    _showProfileToStranger = _savedShowProfileToStranger;
  }

  void _commitSavedSettings() {
    _savedShowOnlineStatus = _showOnlineStatus;
    _savedAllowStrangerBoat = _allowStrangerBoat;
    _savedShowProfileToStranger = _showProfileToStranger;
  }

  void _showErrorSnackBar(String message) {
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: AppTheme.errorColor,
      ),
    );
  }

  /// 延迟 350ms 后触发保存，合并短时间内的多次开关操作
  void _scheduleSave() {
    _saveDebounceTimer?.cancel();
    _saveDebounceTimer =
        Timer(const Duration(milliseconds: 350), _saveSettings);
  }

  /// 提交隐私设置到后端，若保存期间有新的变更则自动重新调度
  Future<void> _saveSettings() async {
    if (_isSaving) {
      _pendingSaveRequested = true;
      return;
    }
    setState(() => _isSaving = true);
    try {
      final result =
          await _accountService.updatePrivacySettings(_buildPrivacyPayload());
      if (result['success'] != true) {
        throw Exception(_resolveErrorMessage(result, '保存失败，请检查网络后重试'));
      }
      _commitSavedSettings();
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
              content: Text('隐私设置已保存'), behavior: SnackBarBehavior.floating),
        );
      }
    } catch (e, stackTrace) {
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: e,
          stack: stackTrace,
          library: 'privacy_settings_screen',
          context: ErrorDescription('while saving privacy settings'),
        ),
      );
      if (mounted) {
        setState(_restoreSavedSettings);
      }
      _showErrorSnackBar(_extractErrorMessage(e, '保存失败，请检查网络后重试'));
    } finally {
      if (mounted) setState(() => _isSaving = false);
      if (_pendingSaveRequested) {
        _pendingSaveRequested = false;
        _scheduleSave();
      }
    }
  }

  @override
  void dispose() {
    _saveDebounceTimer?.cancel();
    super.dispose();
  }

  /// 申请导出个人数据，后端异步打包完成后通知用户下载
  Future<void> _exportData() async {
    final confirm = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('导出个人数据'),
        content: const Text('我们将打包你的所有数据（石头、纸船、涟漪、个人资料等），准备好后会通知你下载。'),
        actions: [
          TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: const Text('取消')),
          FilledButton(
              onPressed: () => Navigator.pop(ctx, true),
              child: const Text('开始导出')),
        ],
      ),
    );
    if (confirm != true) return;

    setState(() => _isExporting = true);
    try {
      final result = await _accountService.exportData();
      if (result['success'] != true) {
        throw Exception(_resolveErrorMessage(result, '导出失败，请稍后重试'));
      }
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
              content: Text('数据导出已开始，完成后会通知你'),
              behavior: SnackBarBehavior.floating),
        );
      }
    } catch (e, stackTrace) {
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: e,
          stack: stackTrace,
          library: 'privacy_settings_screen',
          context: ErrorDescription('while exporting account data'),
        ),
      );
      _showErrorSnackBar(_extractErrorMessage(e, '导出失败，请稍后重试'));
    } finally {
      if (mounted) setState(() => _isExporting = false);
    }
  }

  /// 停用账号，资料隐藏但数据保留 30 天，期间可恢复
  Future<void> _deactivateAccount() async {
    final confirm = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('停用账号'),
        content: const Text('停用后你的资料将不可见，但数据会保留 30 天。30 天内重新登录即可恢复。'),
        actions: [
          TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: const Text('取消')),
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
      final result = await _accountService.deactivateAccount();
      if (result['success'] != true) {
        throw Exception(_resolveErrorMessage(result, '操作失败，请稍后重试'));
      }
      await sl<AuthService>().logout();
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
              content: Text('账号已停用'), behavior: SnackBarBehavior.floating),
        );
        Navigator.of(context).popUntil((route) => route.isFirst);
      }
    } catch (e, stackTrace) {
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: e,
          stack: stackTrace,
          library: 'privacy_settings_screen',
          context: ErrorDescription('while deactivating account'),
        ),
      );
      _showErrorSnackBar(_extractErrorMessage(e, '操作失败，请稍后重试'));
    }
  }

  /// 永久删除账号，需两次确认，操作不可逆
  Future<void> _deleteAccountPermanently() async {
    final firstConfirm = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('永久删除账号'),
        content: const Text('此操作不可撤销！你的所有数据（石头、纸船、涟漪、个人资料）将被永久删除。'),
        actions: [
          TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: const Text('取消')),
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
          TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: const Text('我再想想')),
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
      final result = await _accountService.deleteAccountPermanently();
      if (result['success'] != true) {
        throw Exception(_resolveErrorMessage(result, '删除失败，请稍后重试'));
      }
      await sl<AuthService>().logout();
      if (mounted) {
        Navigator.of(context).popUntil((route) => route.isFirst);
      }
    } catch (e, stackTrace) {
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: e,
          stack: stackTrace,
          library: 'privacy_settings_screen',
          context: ErrorDescription('while deleting account permanently'),
        ),
      );
      _showErrorSnackBar(_extractErrorMessage(e, '删除失败，请稍后重试'));
    }
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      appBar: AppBar(title: const Text('隐私与安全')),
      body: _isLoading
          ? const Center(child: CircularProgressIndicator())
          : _loadErrorMessage != null
              ? Center(
                  child: Padding(
                    padding: const EdgeInsets.all(24),
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        const Icon(
                          Icons.error_outline,
                          color: AppTheme.errorColor,
                          size: 40,
                        ),
                        const SizedBox(height: 12),
                        Text(
                          _loadErrorMessage!,
                          textAlign: TextAlign.center,
                          style: Theme.of(context).textTheme.bodyMedium,
                        ),
                        const SizedBox(height: 16),
                        FilledButton(
                          onPressed: _loadPrivacySettings,
                          child: const Text('重新加载'),
                        ),
                      ],
                    ),
                  ),
                )
              : ListView(
                  padding: const EdgeInsets.all(16),
                  children: [
                    // 隐私设置
                    _buildSectionHeader('隐私设置'),
                    Card(
                      child: Column(
                        children: [
                          _buildSwitch(
                            icon: Icons.circle,
                            iconColor:
                                isDark ? Colors.greenAccent : Colors.green,
                            title: '显示在线状态',
                            subtitle: '其他用户可以看到你是否在线',
                            value: _showOnlineStatus,
                            onChanged: (v) {
                              setState(() => _showOnlineStatus = v);
                              _scheduleSave();
                            },
                          ),
                          const Divider(height: 1),
                          _buildSwitch(
                            icon: Icons.mail_outline,
                            iconColor: AppTheme.skyBlue,
                            title: '允许陌生人发纸船',
                            subtitle: '关闭后只有好友可以给你发纸船',
                            value: _allowStrangerBoat,
                            onChanged: (v) {
                              setState(() => _allowStrangerBoat = v);
                              _scheduleSave();
                            },
                          ),
                          const Divider(height: 1),
                          _buildSwitch(
                            icon: Icons.person_outline,
                            iconColor: AppTheme.skyBlue,
                            title: '对陌生人可见',
                            subtitle: '关闭后你的资料页仅好友可见',
                            value: _showProfileToStranger,
                            onChanged: (v) {
                              setState(() => _showProfileToStranger = v);
                              _scheduleSave();
                            },
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
                            leading: const Icon(Icons.download_outlined,
                                color: AppTheme.skyBlue),
                            title: const Text('导出我的数据'),
                            subtitle: const Text('下载你在心湖的所有数据'),
                            trailing: _isExporting
                                ? const SizedBox(
                                    width: 20,
                                    height: 20,
                                    child: CircularProgressIndicator(
                                        strokeWidth: 2))
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
                            leading: Icon(Icons.pause_circle_outline,
                                color: isDark
                                    ? Colors.orangeAccent
                                    : Colors.orange),
                            title: const Text('停用账号'),
                            subtitle: const Text('暂时隐藏你的资料，30 天内可恢复'),
                            trailing: const Icon(Icons.chevron_right),
                            onTap: _deactivateAccount,
                          ),
                          const Divider(height: 1),
                          ListTile(
                            leading: const Icon(Icons.delete_forever,
                                color: AppTheme.errorColor),
                            title: const Text('永久删除账号',
                                style: TextStyle(color: AppTheme.errorColor)),
                            subtitle: const Text('不可撤销，所有数据将被永久删除'),
                            trailing: const Icon(Icons.chevron_right,
                                color: AppTheme.errorColor),
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

  /// 构建分区标题（如"隐私设置"、"数据管理"）
  Widget _buildSectionHeader(String title) {
    return Padding(
      padding: const EdgeInsets.only(left: 4, bottom: 8),
      child: Text(title,
          style: Theme.of(context).textTheme.titleSmall?.copyWith(
                color: Theme.of(context).colorScheme.primary,
                fontWeight: FontWeight.w600,
              )),
    );
  }

  /// 通用开关行组件，保存中自动禁用交互
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
