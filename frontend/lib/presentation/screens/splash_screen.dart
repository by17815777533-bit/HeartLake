// 启动页面

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import '../widgets/water_background.dart';
import '../../utils/app_theme.dart';
import '../../data/datasources/auth_service.dart';
import '../../data/datasources/api_client.dart';
import '../../di/service_locator.dart';
import 'package:shared_preferences/shared_preferences.dart';

class SplashScreen extends StatefulWidget {
  const SplashScreen({super.key});

  @override
  State<SplashScreen> createState() => _SplashScreenState();
}

class _SplashScreenState extends State<SplashScreen>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _fadeAnimation;
  final AuthService _authService = sl<AuthService>();

  bool _readBoolCompat(SharedPreferences prefs, String key) {
    final value = prefs.get(key);
    if (value is bool) return value;
    if (value is String) {
      final normalized = value.trim().toLowerCase();
      return normalized == 'true';
    }
    return false;
  }

  @override
  void initState() {
    super.initState();

    _controller = AnimationController(
      duration: const Duration(milliseconds: 1500),
      vsync: this,
    );

    _fadeAnimation = Tween<double>(begin: 0.0, end: 1.0).animate(
      CurvedAnimation(parent: _controller, curve: Curves.easeIn),
    );

    _controller.forward();

    _checkLoginAndNavigate();
  }

  /// 启动分流：仅允许已登录会话直接进入首页，未登录统一进入认证页。
  Future<void> _checkLoginAndNavigate() async {
    await Future.delayed(const Duration(seconds: 2));

    final prefs = await SharedPreferences.getInstance();
    final manualLogout = _readBoolCompat(prefs, 'manual_logout');
    if (manualLogout) {
      await prefs.remove('manual_logout');
      if (!mounted) return;
      context.go('/auth');
      return;
    }

    try {
      final isLoggedIn = await _authService.isLoggedIn();
      if (!isLoggedIn) {
        if (!mounted) return;
        context.go('/auth');
        return;
      }

      final result = await _authService.refreshToken();
      if (result['success'] == true) {
        if (!mounted) return;
        context.go('/home');
        return;
      }

      final code = result['code'];
      if (code == 401) {
        // 会话失效：清除本地令牌并回到认证页，不再自动匿名登录。
        ApiClient().clearToken();
        await prefs.remove('manual_logout');
        if (!mounted) return;
        context.go('/auth');
        return;
      }

      // 网络错误或服务器异常时，不强制退出当前会话。
      if (kDebugMode) {
        debugPrint('Token 刷新失败(code=$code)，保留登录状态进入首页');
      }
      if (!mounted) return;
      context.go('/home');
    } catch (e) {
      // 网络不可达等异常：若本地已有会话则继续进入首页，避免启动阻塞。
      if (kDebugMode) {
        debugPrint('登录检查异常: $e');
      }
      if (!mounted) return;
      context.go('/home');
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          const Positioned.fill(child: WaterBackground()),
          Center(
            child: FadeTransition(
              opacity: _fadeAnimation,
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Container(
                    width: 120,
                    height: 120,
                    decoration: BoxDecoration(
                        color: Colors.white.withValues(alpha: 0.2),
                        shape: BoxShape.circle,
                        border: Border.all(
                            color: Colors.white.withValues(alpha: 0.5),
                            width: 2),
                        boxShadow: [
                          BoxShadow(
                            color: AppTheme.primaryColor.withValues(alpha: 0.2),
                            blurRadius: 20,
                            offset: const Offset(0, 10),
                          )
                        ]),
                    child: const Icon(
                      Icons.water_drop,
                      size: 60,
                      color: Colors.white,
                    ),
                  ),
                  const SizedBox(height: 32),
                  Text(
                    '心湖',
                    style: TextStyle(
                        fontSize: 48,
                        fontWeight: FontWeight.bold,
                        color: Colors.white,
                        letterSpacing: 4,
                        shadows: [
                          Shadow(
                              color: AppTheme.heavyStone.withValues(alpha: 0.3),
                              blurRadius: 10,
                              offset: const Offset(0, 5))
                        ]),
                  ),
                  const SizedBox(height: 8),
                  const Text(
                    'Heart Lake',
                    style: TextStyle(
                      fontSize: 16,
                      color: Colors.white70,
                      letterSpacing: 6,
                    ),
                  ),
                  const SizedBox(height: 16),
                  Text(
                    '把心事投到湖里',
                    style: TextStyle(
                      fontSize: 14,
                      color: Colors.white.withValues(alpha: 0.7),
                      letterSpacing: 2,
                    ),
                  ),
                  const SizedBox(height: 48),
                  const SizedBox(
                    width: 24,
                    height: 24,
                    child: CircularProgressIndicator(
                      strokeWidth: 2,
                      valueColor: AlwaysStoppedAnimation<Color>(Colors.white),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}
