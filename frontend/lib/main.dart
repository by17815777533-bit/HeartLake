// @file main.dart
// @brief 应用入口
// Created by 林子怡

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'presentation/providers/theme_provider.dart';
import 'presentation/providers/user_provider.dart';
import 'presentation/providers/notification_provider.dart';
import 'data/datasources/cache_service.dart';
import 'data/datasources/websocket_manager.dart';
import 'presentation/screens/splash_screen.dart';
import 'presentation/screens/home_screen.dart';
import 'utils/app_theme.dart';
import 'utils/app_config.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  appConfig.initialize();
  cacheService.startAutoCleanup();
  runApp(const HeartLakeApp());
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
    cacheService.dispose();
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
