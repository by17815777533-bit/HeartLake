// @file main.dart
// @brief 应用入口
// Created by 林子怡

import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'presentation/providers/theme_provider.dart';
import 'presentation/providers/user_provider.dart';
import 'presentation/providers/notification_provider.dart';
import 'data/datasources/cache_service.dart';
import 'data/datasources/websocket_manager.dart';
import 'data/datasources/api_client.dart';
import 'presentation/screens/splash_screen.dart';
import 'presentation/screens/home_screen.dart';
import 'utils/app_theme.dart';
import 'utils/app_config.dart';

/// 全局 navigatorKey，用于在非 Widget 上下文中导航
final GlobalKey<NavigatorState> navigatorKey = GlobalKey<NavigatorState>();

void main() {
  runZonedGuarded(() {
    WidgetsFlutterBinding.ensureInitialized();

    // 同步 Flutter 框架错误
    FlutterError.onError = (details) {
      FlutterError.presentError(details);
      debugPrint('Flutter Error: ${details.exceptionAsString()}');
    };

    // 捕获未处理的异步错误，避免白屏无提示
    WidgetsBinding.instance.platformDispatcher.onError = (error, stack) {
      debugPrint('Unhandled async error: $error');
      debugPrint(stack.toString());
      return true;
    };

    // 兜底错误组件，运行时异常时显示错误信息而非白屏
    ErrorWidget.builder = (FlutterErrorDetails details) {
      return Material(
        color: Colors.white,
        child: Center(
          child: Padding(
            padding: const EdgeInsets.all(20),
            child: Text(
              '页面渲染异常，请刷新重试\\n${details.exceptionAsString()}',
              style: const TextStyle(color: Colors.black87, fontSize: 14),
              textAlign: TextAlign.center,
            ),
          ),
        ),
      );
    };

    appConfig.initialize();
    CacheService().startAutoCleanup();

    // 注册401未授权回调 - token过期且刷新失败时跳转登录
    ApiClient().setOnUnauthorized(() {
      navigatorKey.currentState?.pushNamedAndRemoveUntil('/', (_) => false);
    });

    runApp(const HeartLakeApp());
  }, (error, stack) {
    debugPrint('runZonedGuarded caught: $error');
    debugPrint(stack.toString());
  });
}

class HeartLakeApp extends StatefulWidget {
  const HeartLakeApp({super.key});

  @override
  State<HeartLakeApp> createState() => _HeartLakeAppState();
}

class _HeartLakeAppState extends State<HeartLakeApp> {
  @override
  void dispose() {
    // P0-2 修复：App 销毁时清理 WebSocket 资源
    WebSocketManager().dispose();
    CacheService().dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => ThemeProvider()),
        ChangeNotifierProvider(create: (_) => UserProvider()),
        ChangeNotifierProvider(create: (_) => NotificationProvider()),
      ],
      child: Consumer<ThemeProvider>(
        builder: (context, themeProvider, child) {
          return MaterialApp(
            title: '心湖 Heart Lake',
            navigatorKey: navigatorKey,
            debugShowCheckedModeBanner: false,
            theme: AppTheme.lightTheme,
            darkTheme: AppTheme.darkTheme,
            themeMode: themeProvider.themeMode,
            home: const SplashScreen(),
            routes: {
              '/home': (context) => const HomeScreen(),
            },
          );
        },
      ),
    );
  }
}
