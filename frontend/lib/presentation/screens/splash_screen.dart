// @file splash_screen.dart
// @brief 启动页面
// Created by 林子怡

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'dart:async';
import 'dart:io';
import 'package:dio/dio.dart';
import 'home_screen.dart';
import '../widgets/water_background.dart';
import '../../utils/app_theme.dart';
import '../../data/datasources/auth_service.dart';

class SplashScreen extends StatefulWidget {
  const SplashScreen({super.key});

  @override
  State<SplashScreen> createState() => _SplashScreenState();
}

class _SplashScreenState extends State<SplashScreen>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _fadeAnimation;
  final AuthService _authService = AuthService();

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

    try {
      var isLoggedIn = await _authService.isLoggedIn();

      if (isLoggedIn) {
        final result = await _authService.refreshToken();
        if (!result['success']) {
          final statusCode = result['statusCode'];
          if (statusCode == 401) {
            // token 过期，降级为匿名
            await _authService.logout();
            await _authService.anonymousLogin();
          } else {
            // 网络错误等其他情况，保留 token 继续进入
            if (kDebugMode) { debugPrint('Token 刷新失败但非401，保留登录状态'); }
          }
        }
      } else {
        await _authService.anonymousLogin();
      }
    } on SocketException catch (_) {
      // 网络不可达，保留现有 token，不降级
      if (kDebugMode) { debugPrint('网络不可达，保留现有登录状态'); }
    } on DioException catch (e) {
      if (e.response?.statusCode == 401) {
        // 明确的 401，token 已失效
        await _authService.logout();
        await _authService.anonymousLogin();
      } else {
        // 其他 Dio 错误（超时、连接失败等），保留 token
        if (kDebugMode) { debugPrint('网络错误，保留登录状态: $e'); }
      }
    } catch (e) {
      if (kDebugMode) { debugPrint('登录检查异常: $e'); }
    }
    if (!mounted) return;

    Navigator.of(context).pushReplacement(
      PageRouteBuilder(
        pageBuilder: (context, animation, secondaryAnimation) => const HomeScreen(),
        transitionsBuilder: (context, animation, secondaryAnimation, child) {
          return FadeTransition(opacity: animation, child: child);
        },
        transitionDuration: const Duration(milliseconds: 500),
      ),
    );
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
                        color: Colors.white.withOpacity(0.2),
                        shape: BoxShape.circle,
                        border: Border.all(
                            color: Colors.white.withOpacity(0.5), width: 2),
                        boxShadow: [
                          BoxShadow(
                            color: AppTheme.primaryColor.withOpacity(0.2),
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
                              color: AppTheme.heavyStone.withOpacity(0.3),
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
                  const SizedBox(height: 60),
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
