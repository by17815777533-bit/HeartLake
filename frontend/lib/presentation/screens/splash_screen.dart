// @file splash_screen.dart
// @brief 启动页面 - Material Design 3 风格
// Created by 林子怡

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'dart:async';
import 'dart:io';
import 'package:dio/dio.dart';
import 'home_screen.dart';
import '../../utils/animation_utils.dart';
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
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final glowColor = isDark ? AppTheme.primaryLightColor : AppTheme.primaryColor;

    return Scaffold(
      body: Stack(
        children: [
          Positioned.fill(
            child: DecoratedBox(
              decoration: BoxDecoration(
                gradient: LinearGradient(
                  begin: Alignment.topCenter,
                  end: Alignment.bottomCenter,
                  colors: isDark
                      ? AppTheme.nightGradient
                      : const [
                          Color(0xFFFFF9F3),
                          Color(0xFFFFEEDC),
                          Color(0xFFEAF1FF),
                        ],
                ),
              ),
            ),
          ),
          Positioned(
            top: -120,
            left: -60,
            child: _buildOrb(
              size: 280,
              color: glowColor.withValues(alpha: isDark ? 0.2 : 0.24),
            ),
          ),
          Positioned(
            right: -70,
            top: -90,
            child: _buildOrb(
              size: 220,
              color: (isDark ? AppTheme.secondaryColor : AppTheme.secondaryColor)
                  .withValues(alpha: isDark ? 0.18 : 0.16),
            ),
          ),
          Positioned.fill(
            child: IgnorePointer(
              child: Stack(
                children: const [
                  _StarPoint(alignment: Alignment(-0.82, -0.72), size: 3),
                  _StarPoint(alignment: Alignment(-0.2, -0.86), size: 2),
                  _StarPoint(alignment: Alignment(0.56, -0.7), size: 3),
                  _StarPoint(alignment: Alignment(0.84, -0.5), size: 2),
                  _StarPoint(alignment: Alignment(-0.7, -0.16), size: 2),
                  _StarPoint(alignment: Alignment(0.28, -0.28), size: 2),
                  _StarPoint(alignment: Alignment(0.74, 0.04), size: 3),
                ],
              ),
            ),
          ),
          Center(
            child: FadeTransition(
              opacity: _fadeAnimation,
              child: ScaleTransition(
                scale: _scaleAnimation,
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Container(
                      width: 128,
                      height: 128,
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        gradient: LinearGradient(
                          begin: Alignment.topLeft,
                          end: Alignment.bottomRight,
                          colors: [
                            Colors.white.withValues(alpha: isDark ? 0.2 : 0.85),
                            Colors.white.withValues(alpha: isDark ? 0.06 : 0.48),
                          ],
                        ),
                        border: Border.all(
                          color: glowColor.withValues(alpha: isDark ? 0.38 : 0.26),
                          width: 1.6,
                        ),
                        boxShadow: [
                          BoxShadow(
                            color: glowColor.withValues(alpha: isDark ? 0.2 : 0.24),
                            blurRadius: 36,
                            spreadRadius: 2,
                          ),
                        ],
                      ),
                      child: Icon(
                        Icons.water_drop,
                        size: 64,
                        color: isDark ? AppTheme.primaryLightColor : AppTheme.primaryLightColor,
                      ),
                    ),
                    const SizedBox(height: 30),
                    ShaderMask(
                      shaderCallback: (rect) {
                        return const LinearGradient(
                          colors: AppTheme.warmGradient,
                        ).createShader(rect);
                      },
                      child: const Text(
                        '心湖',
                        style: TextStyle(
                          fontSize: 50,
                          fontWeight: FontWeight.w700,
                          color: Colors.white,
                          letterSpacing: 4,
                        ),
                      ),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      'HeartLake · Sky Mood',
                      style: TextStyle(
                        fontSize: 14,
                        color: isDark
                            ? AppTheme.textSecondary.withValues(alpha: 0.9)
                            : AppTheme.textSecondary.withValues(alpha: 0.85),
                        letterSpacing: 3.5,
                      ),
                    ),
                    const SizedBox(height: 56),
                    SizedBox(
                      width: 34,
                      height: 34,
                      child: CircularProgressIndicator(
                        strokeWidth: 2.5,
                        valueColor: AlwaysStoppedAnimation<Color>(
                          isDark ? AppTheme.primaryLightColor : AppTheme.primaryLightColor,
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

  Widget _buildOrb({
    required double size,
    required Color color,
  }) {
    return Container(
      width: size,
      height: size,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        gradient: RadialGradient(
          colors: [color, color.withValues(alpha: 0)],
        ),
      ),
    );
  }
}

class _StarPoint extends StatelessWidget {
  final Alignment alignment;
  final double size;

  const _StarPoint({
    required this.alignment,
    required this.size,
  });

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final color = isDark ? AppTheme.primaryLightColor : AppTheme.primaryColor;
    return Align(
      alignment: alignment,
      child: Container(
        width: size,
        height: size,
        decoration: BoxDecoration(
          shape: BoxShape.circle,
          color: color.withValues(alpha: isDark ? 0.5 : 0.35),
          boxShadow: [
            BoxShadow(
              color: color.withValues(alpha: isDark ? 0.48 : 0.3),
              blurRadius: 8,
              spreadRadius: 0.3,
            ),
          ],
        ),
      ),
    );
  }
}
