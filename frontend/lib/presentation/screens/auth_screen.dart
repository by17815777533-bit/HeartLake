// 认证界面 - 匿名登录 + 关键词恢复

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:go_router/go_router.dart';
import 'package:heart_lake/data/datasources/auth_service.dart';
import 'package:heart_lake/di/service_locator.dart';
import 'package:heart_lake/presentation/widgets/water_background.dart';
import 'package:heart_lake/utils/app_theme.dart';

class AuthScreen extends StatefulWidget {
  const AuthScreen({super.key});

  @override
  State<AuthScreen> createState() => _AuthScreenState();
}

class _AuthScreenState extends State<AuthScreen> with TickerProviderStateMixin {
  final AuthService _authService = sl<AuthService>();
  bool _isLoading = false;

  // 按钮缩放动画
  late AnimationController _enterBtnController;
  late AnimationController _recoverBtnController;
  late Animation<double> _enterBtnScale;
  late Animation<double> _recoverBtnScale;

  // 淡入动画
  late AnimationController _fadeController;
  late Animation<double> _fadeAnimation;

  @override
  void initState() {
    super.initState();
    _enterBtnController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 120),
      lowerBound: 0.0,
      upperBound: 0.06,
    );
    _recoverBtnController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 120),
      lowerBound: 0.0,
      upperBound: 0.06,
    );
    _enterBtnScale = Tween<double>(begin: 1.0, end: 0.94).animate(
      CurvedAnimation(parent: _enterBtnController, curve: Curves.easeInOut),
    );
    _recoverBtnScale = Tween<double>(begin: 1.0, end: 0.94).animate(
      CurvedAnimation(parent: _recoverBtnController, curve: Curves.easeInOut),
    );

    _fadeController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 800),
    );
    _fadeAnimation = CurvedAnimation(
      parent: _fadeController,
      curve: Curves.easeOut,
    );
    _fadeController.forward();
  }

  @override
  void dispose() {
    _enterBtnController.dispose();
    _recoverBtnController.dispose();
    _fadeController.dispose();
    super.dispose();
  }

  void _showSnackBar(String message, {bool isError = false}) {
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: isError ? AppTheme.errorColor : AppTheme.successColor,
        behavior: SnackBarBehavior.floating,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
        margin: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
      ),
    );
  }

  void _navigateToHome() {
    if (!mounted) return;
    context.go('/home');
  }

  // ==================== 匿名登录 ====================

  Future<void> _handleAnonymousLogin() async {
    setState(() => _isLoading = true);
    try {
      final result = await _authService.anonymousLogin();
      if (!mounted) return;
      setState(() => _isLoading = false);

      if (result['success'] == true) {
        final recoveryKey = result['recovery_key'] as String?;
        if (recoveryKey != null && recoveryKey.isNotEmpty) {
          await _showRecoveryKeySaveDialog(recoveryKey);
        } else {
          _navigateToHome();
        }
      } else {
        _showSnackBar(result['message']?.toString() ?? '登录失败，请稍后重试', isError: true);
      }
    } catch (e) {
      if (!mounted) return;
      setState(() => _isLoading = false);
      _showSnackBar('网络异常，请检查网络连接', isError: true);
    }
  }

  // ==================== 恢复密钥保存对话框 ====================

  Future<void> _showRecoveryKeySaveDialog(String recoveryKey) async {
    bool hasCopied = false;

    await showDialog<void>(
      context: context,
      barrierDismissible: false,
      builder: (dialogContext) {
        return StatefulBuilder(
          builder: (context, setDialogState) {
            final isDark = Theme.of(context).brightness == Brightness.dark;
            return AlertDialog(
              backgroundColor: isDark ? AppTheme.nightSurface : Colors.white,
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(20),
              ),
              title: Row(
                children: [
                  const Icon(Icons.key, color: AppTheme.accentColor, size: 24),
                  const SizedBox(width: 8),
                  Text(
                    '保存你的恢复密钥',
                    style: TextStyle(
                      fontSize: 18,
                      fontWeight: FontWeight.w700,
                      color: isDark ? AppTheme.darkTextPrimary : AppTheme.textPrimary,
                    ),
                  ),
                ],
              ),
              content: Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    '这是你恢复账号的唯一凭证，请妥善保管。\n丢失后将无法找回账号。',
                    style: TextStyle(
                      fontSize: 13,
                      color: isDark ? AppTheme.darkTextSecondary : AppTheme.textSecondary,
                      height: 1.5,
                    ),
                  ),
                  const SizedBox(height: 16),
                  // 密钥展示卡片
                  Container(
                    width: double.infinity,
                    padding: const EdgeInsets.symmetric(vertical: 18, horizontal: 16),
                    decoration: BoxDecoration(
                      color: isDark
                          ? AppTheme.nightDeep.withValues(alpha: 0.6)
                          : AppTheme.lakeBackground.withValues(alpha: 0.06),
                      borderRadius: BorderRadius.circular(14),
                      border: Border.all(
                        color: isDark
                            ? AppTheme.primaryLightColor.withValues(alpha: 0.2)
                            : AppTheme.borderCyan.withValues(alpha: 0.4),
                      ),
                    ),
                    child: SelectableText(
                      recoveryKey,
                      textAlign: TextAlign.center,
                      style: TextStyle(
                        fontSize: 22,
                        fontWeight: FontWeight.w600,
                        color: isDark ? AppTheme.primaryLightColor : AppTheme.lakeBackground,
                        letterSpacing: 2,
                        height: 1.4,
                      ),
                    ),
                  ),
                  const SizedBox(height: 12),
                  // 复制按钮
                  Center(
                    child: TextButton.icon(
                      onPressed: () {
                        Clipboard.setData(ClipboardData(text: recoveryKey));
                        setDialogState(() => hasCopied = true);
                        ScaffoldMessenger.of(dialogContext).showSnackBar(
                          SnackBar(
                            content: const Text('已复制到剪贴板'),
                            backgroundColor: AppTheme.successColor,
                            behavior: SnackBarBehavior.floating,
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(12),
                            ),
                            duration: const Duration(seconds: 1),
                          ),
                        );
                      },
                      icon: Icon(
                        hasCopied ? Icons.check : Icons.copy_rounded,
                        size: 16,
                        color: isDark ? AppTheme.primaryLightColor : AppTheme.primaryColor,
                      ),
                      label: Text(
                        hasCopied ? '已复制' : '复制密钥',
                        style: TextStyle(
                          color: isDark ? AppTheme.primaryLightColor : AppTheme.primaryColor,
                          fontSize: 13,
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(height: 8),
                  // 截图提示
                  Container(
                    padding: const EdgeInsets.all(10),
                    decoration: BoxDecoration(
                      color: AppTheme.warningColor.withValues(alpha: 0.1),
                      borderRadius: BorderRadius.circular(10),
                    ),
                    child: Row(
                      children: [
                        const Icon(Icons.screenshot_monitor, size: 16, color: AppTheme.warningColor),
                        const SizedBox(width: 8),
                        Expanded(
                          child: Text(
                            '建议截图保存，这是找回账号的唯一方式',
                            style: TextStyle(
                              fontSize: 12,
                              color: isDark ? AppTheme.darkTextSecondary : AppTheme.textSecondary,
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
              actions: [
                SizedBox(
                  width: double.infinity,
                  child: ElevatedButton(
                    onPressed: () {
                      Navigator.of(dialogContext).pop();
                      _navigateToHome();
                    },
                    style: ElevatedButton.styleFrom(
                      backgroundColor: isDark ? AppTheme.primaryLightColor : AppTheme.primaryColor,
                      foregroundColor: isDark ? AppTheme.nightDeep : Colors.white,
                      padding: const EdgeInsets.symmetric(vertical: 14),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(12),
                      ),
                    ),
                    child: const Text(
                      '我已保存，进入心湖',
                      style: TextStyle(fontSize: 15, fontWeight: FontWeight.w600),
                    ),
                  ),
                ),
              ],
              actionsPadding: const EdgeInsets.fromLTRB(24, 0, 24, 20),
            );
          },
        );
      },
    );
  }

  // ==================== 关键词恢复对话框 ====================

  Future<void> _showRecoverDialog() async {
    final recoveryKeyController = TextEditingController();

    final confirmed = await showDialog<bool>(
      context: context,
      builder: (dialogContext) {
        final isDark = Theme.of(dialogContext).brightness == Brightness.dark;
        return AlertDialog(
          backgroundColor: isDark ? AppTheme.nightSurface : Colors.white,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(20),
          ),
          title: Row(
            children: [
              const Icon(Icons.vpn_key, color: AppTheme.primaryColor, size: 24),
              const SizedBox(width: 8),
              Text(
                '关键词恢复',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.w700,
                  color: isDark ? AppTheme.darkTextPrimary : AppTheme.textPrimary,
                ),
              ),
            ],
          ),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                '请输入你的恢复密钥来找回账号',
                style: TextStyle(
                  fontSize: 13,
                  color: isDark ? AppTheme.darkTextSecondary : AppTheme.textSecondary,
                ),
              ),
              const SizedBox(height: 16),
              TextField(
                controller: recoveryKeyController,
                decoration: InputDecoration(
                  hintText: '例如：星辰-湖畔-微风-暖阳',
                  hintStyle: TextStyle(
                    color: isDark
                        ? AppTheme.darkTextSecondary.withValues(alpha: 0.5)
                        : AppTheme.textTertiary,
                    fontSize: 14,
                  ),
                  prefixIcon: Icon(
                    Icons.key,
                    color: isDark ? AppTheme.primaryLightColor : AppTheme.primaryColor,
                  ),
                  filled: true,
                  fillColor: isDark
                      ? AppTheme.nightDeep.withValues(alpha: 0.6)
                      : AppTheme.backgroundColor,
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(12),
                    borderSide: BorderSide.none,
                  ),
                  enabledBorder: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(12),
                    borderSide: BorderSide(
                      color: isDark
                          ? AppTheme.primaryLightColor.withValues(alpha: 0.15)
                          : AppTheme.borderCyan.withValues(alpha: 0.3),
                    ),
                  ),
                  focusedBorder: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(12),
                    borderSide: BorderSide(
                      color: isDark ? AppTheme.primaryLightColor : AppTheme.primaryColor,
                      width: 1.4,
                    ),
                  ),
                ),
                style: TextStyle(
                  fontSize: 16,
                  color: isDark ? AppTheme.darkTextPrimary : AppTheme.textPrimary,
                  letterSpacing: 0.5,
                ),
                textAlign: TextAlign.center,
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(dialogContext).pop(false),
              child: Text(
                '取消',
                style: TextStyle(
                  color: isDark ? AppTheme.darkTextSecondary : AppTheme.textSecondary,
                ),
              ),
            ),
            ElevatedButton(
              onPressed: () => Navigator.of(dialogContext).pop(true),
              style: ElevatedButton.styleFrom(
                backgroundColor: isDark ? AppTheme.primaryLightColor : AppTheme.primaryColor,
                foregroundColor: isDark ? AppTheme.nightDeep : Colors.white,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(10),
                ),
                padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 10),
              ),
              child: const Text('恢复账号'),
            ),
          ],
          actionsPadding: const EdgeInsets.fromLTRB(16, 0, 16, 12),
        );
      },
    );

    if (confirmed != true) {
      recoveryKeyController.dispose();
      return;
    }

    final key = recoveryKeyController.text.trim();
    recoveryKeyController.dispose();

    if (key.isEmpty) {
      _showSnackBar('请输入恢复密钥', isError: true);
      return;
    }

    setState(() => _isLoading = true);
    try {
      final result = await _authService.recoverWithKey(key);
      if (!mounted) return;
      setState(() => _isLoading = false);

      if (result['success'] == true) {
        _showSnackBar('账号恢复成功，欢迎回来');
        await Future.delayed(const Duration(milliseconds: 500));
        _navigateToHome();
      } else {
        _showSnackBar(result['message']?.toString() ?? '恢复失败，请检查密钥是否正确', isError: true);
      }
    } catch (e) {
      if (!mounted) return;
      setState(() => _isLoading = false);
      _showSnackBar('网络异常，请检查网络连接', isError: true);
    }
  }

  // ==================== 构建UI ====================

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;

    return Scaffold(
      body: Stack(
        children: [
          // 水面背景
          const Positioned.fill(child: WaterBackground()),

          // 主内容
          SafeArea(
            child: FadeTransition(
              opacity: _fadeAnimation,
              child: Center(
                child: SingleChildScrollView(
                  padding: const EdgeInsets.symmetric(horizontal: 36),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      const SizedBox(height: 60),
                      _buildLogo(isDark),
                      const SizedBox(height: 12),
                      _buildTitle(isDark),
                      const SizedBox(height: 8),
                      _buildSubtitle(isDark),
                      const SizedBox(height: 56),
                      _buildAnonymousLoginButton(isDark),
                      const SizedBox(height: 16),
                      _buildRecoverButton(isDark),
                      const SizedBox(height: 48),
                      _buildPrivacyStatement(isDark),
                      const SizedBox(height: 32),
                    ],
                  ),
                ),
              ),
            ),
          ),

          // 加载遮罩
          if (_isLoading)
            Positioned.fill(
              child: Container(
                color: Colors.black.withValues(alpha: 0.3),
                child: const Center(
                  child: CircularProgressIndicator(
                    color: Colors.white,
                    strokeWidth: 2.5,
                  ),
                ),
              ),
            ),
        ],
      ),
    );
  }

  // ==================== Logo ====================

  Widget _buildLogo(bool isDark) {
    return Container(
      width: 88,
      height: 88,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: isDark
              ? [
                  AppTheme.primaryLightColor.withValues(alpha: 0.3),
                  AppTheme.lakeMiddle.withValues(alpha: 0.2),
                ]
              : [
                  Colors.white.withValues(alpha: 0.9),
                  AppTheme.lakeSurface.withValues(alpha: 0.3),
                ],
        ),
        boxShadow: [
          BoxShadow(
            color: isDark
                ? AppTheme.primaryLightColor.withValues(alpha: 0.15)
                : AppTheme.primaryColor.withValues(alpha: 0.2),
            blurRadius: 24,
            spreadRadius: 4,
          ),
        ],
      ),
      child: Icon(
        Icons.water_drop_rounded,
        size: 44,
        color: isDark ? AppTheme.primaryLightColor : AppTheme.primaryColor,
      ),
    );
  }

  // ==================== 标题 ====================

  Widget _buildTitle(bool isDark) {
    return Text(
      '心湖',
      style: TextStyle(
        fontSize: 36,
        fontWeight: FontWeight.w700,
        color: isDark ? AppTheme.darkTextPrimary : Colors.white,
        letterSpacing: 6,
        shadows: [
          Shadow(
            color: isDark
                ? AppTheme.primaryLightColor.withValues(alpha: 0.3)
                : Colors.black.withValues(alpha: 0.15),
            blurRadius: 12,
          ),
        ],
      ),
    );
  }

  // ==================== 副标题 ====================

  Widget _buildSubtitle(bool isDark) {
    return Text(
      '在这里，倾听内心的声音',
      style: TextStyle(
        fontSize: 15,
        color: isDark
            ? AppTheme.darkTextSecondary.withValues(alpha: 0.8)
            : Colors.white.withValues(alpha: 0.85),
        letterSpacing: 1.5,
      ),
    );
  }

  // ==================== 匿名登录按钮 ====================

  Widget _buildAnonymousLoginButton(bool isDark) {
    return AnimatedBuilder(
      animation: _enterBtnScale,
      builder: (context, child) {
        return Transform.scale(
          scale: _enterBtnScale.value,
          child: child,
        );
      },
      child: GestureDetector(
        onTapDown: (_) => _enterBtnController.forward(),
        onTapUp: (_) {
          _enterBtnController.reverse();
          if (!_isLoading) _handleAnonymousLogin();
        },
        onTapCancel: () => _enterBtnController.reverse(),
        child: Container(
          width: double.infinity,
          padding: const EdgeInsets.symmetric(vertical: 18),
          decoration: BoxDecoration(
            gradient: LinearGradient(
              colors: isDark
                  ? [AppTheme.primaryLightColor, AppTheme.primaryColor]
                  : [AppTheme.primaryColor, AppTheme.lakeBackground],
            ),
            borderRadius: BorderRadius.circular(16),
            boxShadow: [
              BoxShadow(
                color: isDark
                    ? AppTheme.primaryLightColor.withValues(alpha: 0.25)
                    : AppTheme.primaryColor.withValues(alpha: 0.35),
                blurRadius: 16,
                offset: const Offset(0, 6),
              ),
            ],
          ),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(
                Icons.water_drop_outlined,
                color: isDark ? AppTheme.nightDeep : Colors.white,
                size: 20,
              ),
              const SizedBox(width: 10),
              Text(
                '匿名进入心湖',
                style: TextStyle(
                  fontSize: 17,
                  fontWeight: FontWeight.w600,
                  color: isDark ? AppTheme.nightDeep : Colors.white,
                  letterSpacing: 1.5,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // ==================== 恢复账号按钮 ====================

  Widget _buildRecoverButton(bool isDark) {
    return AnimatedBuilder(
      animation: _recoverBtnScale,
      builder: (context, child) {
        return Transform.scale(
          scale: _recoverBtnScale.value,
          child: child,
        );
      },
      child: GestureDetector(
        onTapDown: (_) => _recoverBtnController.forward(),
        onTapUp: (_) {
          _recoverBtnController.reverse();
          if (!_isLoading) _showRecoverDialog();
        },
        onTapCancel: () => _recoverBtnController.reverse(),
        child: Container(
          width: double.infinity,
          padding: const EdgeInsets.symmetric(vertical: 16),
          decoration: BoxDecoration(
            color: isDark
                ? Colors.white.withValues(alpha: 0.06)
                : Colors.white.withValues(alpha: 0.15),
            borderRadius: BorderRadius.circular(16),
            border: Border.all(
              color: isDark
                  ? Colors.white.withValues(alpha: 0.15)
                  : Colors.white.withValues(alpha: 0.4),
              width: 1.2,
            ),
          ),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(
                Icons.vpn_key,
                color: isDark
                    ? Colors.white.withValues(alpha: 0.8)
                    : Colors.white.withValues(alpha: 0.9),
                size: 18,
              ),
              const SizedBox(width: 8),
              Text(
                '恢复我的账号',
                style: TextStyle(
                  fontSize: 15,
                  fontWeight: FontWeight.w500,
                  color: isDark
                      ? Colors.white.withValues(alpha: 0.8)
                      : Colors.white.withValues(alpha: 0.9),
                  letterSpacing: 0.8,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // ==================== 隐私声明 ====================

  Widget _buildPrivacyStatement(bool isDark) {
    return Text(
      '进入即代表你同意以匿名方式使用心湖\n我们不收集任何个人身份信息',
      textAlign: TextAlign.center,
      style: TextStyle(
        fontSize: 11,
        color: isDark
            ? Colors.white.withValues(alpha: 0.35)
            : Colors.white.withValues(alpha: 0.6),
        height: 1.6,
      ),
    );
  }
}
