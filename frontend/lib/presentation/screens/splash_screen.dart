// 启动页面

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import '../widgets/water_background.dart';
import '../../utils/app_theme.dart';
import '../../data/datasources/auth_service.dart';
import '../../di/service_locator.dart';
import '../../utils/storage_util.dart';
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

  /// P2-4 修复：区分网络错误和 token 过期
  Future<void> _checkLoginAndNavigate() async {
    await Future.delayed(const Duration(seconds: 2));

    bool? isNewUser;
    String? loginUserId;

    try {
      var isLoggedIn = await _authService.isLoggedIn();

      if (isLoggedIn) {
        final result = await _authService.refreshToken();
        if (!result['success']) {
          final code = result['code'];
          if (code == 401) {
            // token 过期，降级为匿名
            await _authService.logout();
            final loginResult = await _authService.anonymousLogin();
            if (loginResult['success'] == true) {
              isNewUser = loginResult['is_new_user'] == true;
              loginUserId = loginResult['user_id']?.toString();
            }
          } else {
            // 网络错误(code==null)或服务器错误，保留 token 继续进入
            if (kDebugMode) { debugPrint('Token 刷新失败(code=$code)，保留登录状态'); }
          }
        }
      } else {
        final loginResult = await _authService.anonymousLogin();
        if (!loginResult['success']) {
          if (kDebugMode) { debugPrint('匿名登录失败: ${loginResult['message']}'); }
        } else {
          isNewUser = loginResult['is_new_user'] == true;
          loginUserId = loginResult['user_id']?.toString();
        }
      }
    } catch (e) {
      // 网络不可达等未预期异常，保留现有状态继续进入
      if (kDebugMode) { debugPrint('登录检查异常: $e'); }
    }
    if (!mounted) return;

    final prefs = await SharedPreferences.getInstance();
    final legacyOnboardingDone =
        prefs.getString('onboarding_done') == 'true' ||
        (prefs.getBool('onboarding_done') ?? false);
    final userId = loginUserId ?? await StorageUtil.getUserId();

    bool userOnboardingDone = false;
    if (userId != null && userId.isNotEmpty) {
      final userFlagKey = 'onboarding_done_user_$userId';
      userOnboardingDone =
          prefs.getString(userFlagKey) == 'true' ||
          (prefs.getBool(userFlagKey) ?? false);

      // 老用户不再重复显示新手引导
      if (isNewUser == false && !userOnboardingDone) {
        await prefs.setString(userFlagKey, 'true');
        userOnboardingDone = true;
      }
    }

    final showOnboarding = isNewUser == true
        ? !userOnboardingDone
        : !legacyOnboardingDone && isNewUser == null;

    if (!mounted) return;

    final targetPath = showOnboarding ? '/onboarding' : '/home';

    context.go(targetPath);
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
                            color: Colors.white.withValues(alpha: 0.5), width: 2),
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
