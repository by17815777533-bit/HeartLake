/// HeartLake应用主入口
///
/// 作为Flutter应用的启动入口，负责：
/// - 全局异常捕获和错误处理
/// - 依赖注入容器初始化
/// - 缓存服务启动
/// - 认证状态管理
/// - Provider状态管理树构建

import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'presentation/providers/theme_provider.dart';
import 'presentation/providers/user_provider.dart';
import 'presentation/providers/notification_provider.dart';
import 'presentation/providers/stone_provider.dart';
import 'presentation/providers/friend_provider.dart';
import 'providers/edge_ai_provider.dart';
import 'data/datasources/cache_service.dart';
import 'data/datasources/websocket_manager.dart';
import 'data/datasources/api_client.dart';
import 'utils/app_theme.dart';
import 'utils/app_config.dart';
import 'package:go_router/go_router.dart';
import 'di/service_locator.dart';
import 'router/app_router.dart';

/// 应用主入口
///
/// 使用runZonedGuarded捕获所有未处理的异常，配置全局错误处理器。
void main() {
  runZonedGuarded(() {
    WidgetsFlutterBinding.ensureInitialized();

    // 捕获Flutter框架内的同步错误
    FlutterError.onError = (details) {
      FlutterError.presentError(details);
      debugPrint('Flutter Error: ${details.exceptionAsString()}');
    };

    // 捕获异步错误
    WidgetsBinding.instance.platformDispatcher.onError = (error, stack) {
      debugPrint('Unhandled async error: $error');
      debugPrint(stack.toString());
      return true;
    };

    // 自定义错误页面，避免白屏
    ErrorWidget.builder = (FlutterErrorDetails details) {
      return Material(
        color: Colors.white,
        child: Center(
          child: Padding(
            padding: const EdgeInsets.all(20),
            child: Text(
              '页面渲染异常，请刷新重试\n${details.exceptionAsString()}',
              style: const TextStyle(color: Colors.black87, fontSize: 14),
              textAlign: TextAlign.center,
            ),
          ),
        ),
      );
    };

    // 初始化依赖注入容器
    setupServiceLocator();

    appConfig.initialize();
    CacheService().startAutoCleanup();

    // 设置401未授权回调，跳转到登录页
    ApiClient().setOnUnauthorized(() {
      rootNavigatorKey.currentState?.pushNamedAndRemoveUntil('/', (_) => false);
    });

    runApp(const HeartLakeApp());
  }, (error, stack) {
    debugPrint('runZonedGuarded caught: $error');
    debugPrint(stack.toString());
  });
}

/// HeartLake应用根组件
///
/// 使用MultiProvider管理全局状态，包括主题、用户、通知、石头、好友和EdgeAI。
class HeartLakeApp extends StatefulWidget {
  const HeartLakeApp({super.key});

  @override
  State<HeartLakeApp> createState() => _HeartLakeAppState();
}

/// HeartLake应用状态
///
/// 管理路由器生命周期和资源清理。
class _HeartLakeAppState extends State<HeartLakeApp> {
  late final GoRouter _router;

  @override
  void initState() {
    super.initState();
    _router = createRouter();
  }

  @override
  void dispose() {
    // 清理WebSocket连接和缓存
    WebSocketManager().dispose();
    CacheService().dispose();
    _router.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => ThemeProvider()),
        ChangeNotifierProvider(create: (_) => UserProvider()),
        ChangeNotifierProvider(create: (_) => NotificationProvider()),
        ChangeNotifierProvider(create: (_) => StoneProvider()),
        ChangeNotifierProvider(create: (_) => FriendProvider()),
        ChangeNotifierProvider(create: (_) => EdgeAIProvider()),
      ],
      child: Consumer<ThemeProvider>(
        builder: (context, themeProvider, child) {
          return MaterialApp.router(
            title: '心湖 Heart Lake',
            debugShowCheckedModeBanner: false,
            theme: AppTheme.lightTheme,
            darkTheme: AppTheme.darkTheme,
            themeMode: themeProvider.themeMode,
            routerConfig: _router,
          );
        },
      ),
    );
  }
}
