// @file splash_screen.dart
// @brief 启动页面 - 光遇 Sky 视觉风格
// Created by 林子怡

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'dart:async';
import 'dart:io';
import 'package:dio/dio.dart';
import 'home_screen.dart';
import '../widgets/journey_effects/glow_particles.dart';
import '../../utils/app_theme.dart';
import '../../utils/animation_utils.dart';
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
  late Animation<double> _scaleAnimation;
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

    _scaleAnimation = Tween<double>(begin: 0.8, end: 1.0).animate(
      CurvedAnimation(parent: _controller, curve: Curves.easeOutCubic),
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
      SkyPageRoute(page: const HomeScreen()),
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
          // 全屏星空渐变背景
          Positioned.fill(
            child: Container(
              decoration: const BoxDecoration(
                gradient: LinearGradient(
                  begin: Alignment.topCenter,
                  end: Alignment.bottomCenter,
                  colors: AppTheme.nightGradient,
                ),
              ),
            ),
          ),
          // 粒子效果层
          const Positioned.fill(child: GlowParticles()),
          // 主内容
          Center(
            child: FadeTransition(
              opacity: _fadeAnimation,
              child: ScaleTransition(
                scale: _scaleAnimation,
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    // 心形 Logo - 金色光晕效果
                    Container(
                      width: 120,
                      height: 120,
                      decoration: BoxDecoration(
                        color: AppTheme.nightDeep.withValues(alpha: 0.6),
                        shape: BoxShape.circle,
                        border: Border.all(
                          color: AppTheme.candleGlow.withValues(alpha: 0.7),
                          width: 2,
                        ),
                        boxShadow: [
                          BoxShadow(
                            color: AppTheme.candleGlow.withValues(alpha: 0.4),
                            blurRadius: 30,
                            spreadRadius: 8,
                          ),
                          BoxShadow(
                            color: AppTheme.candleGlow.withValues(alpha: 0.15),
                            blurRadius: 60,
                            spreadRadius: 20,
                          ),
                        ],
                      ),
                      child: Icon(
                        Icons.water_drop,
                        size: 60,
                        color: AppTheme.candleGlow.withValues(alpha: 0.9),
                      ),
                    ),
                    const SizedBox(height: 32),
                    // "心湖" 金色渐变文字
                    ShaderMask(
                      shaderCallback: (bounds) => const LinearGradient(
                        colors: AppTheme.warmGradient,
                      ).createShader(bounds),
                      child: const Text(
                        '心湖',
                        style: TextStyle(
                          fontSize: 48,
                          fontWeight: FontWeight.bold,
                          color: Colors.white,
                          letterSpacing: 4,
                        ),
                      ),
                    ),
                    const SizedBox(height: 8),
                    // 副标题
                    const Text(
                      'Sky · HeartLake',
                      style: TextStyle(
                        fontSize: 16,
                        color: AppTheme.darkTextSecondary,
                        letterSpacing: 6,
                      ),
                    ),
                    const SizedBox(height: 60),
                    // 金色加载指示器
                    SizedBox(
                      width: 32,
                      height: 32,
                      child: CircularProgressIndicator(
                        strokeWidth: 2.5,
                        valueColor: AlwaysStoppedAnimation<Color>(
                          AppTheme.candleGlow.withValues(alpha: 0.8),
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
