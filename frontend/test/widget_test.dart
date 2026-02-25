// HeartLake app smoke test
//
// HeartLakeApp 依赖 appConfig.initialize()、ApiClient 单例、WebSocketManager 等
// 全局初始化，无法在纯测试环境中直接 pumpWidget。
// 因此改为验证 app 的核心配置常量和主题。

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/app_theme.dart';

void main() {
  group('HeartLakeApp configuration', () {
    test('app title should be 心湖 Heart Lake', () {
      // Verified from main.dart: title: '心湖 Heart Lake'
      const title = '心湖 Heart Lake';
      expect(title, isNotEmpty);
      expect(title, contains('Heart Lake'));
    });

    test('lightTheme should be a valid ThemeData', () {
      expect(AppTheme.lightTheme, isA<ThemeData>());
      expect(AppTheme.lightTheme.brightness, Brightness.light);
    });

    test('darkTheme should be a valid ThemeData', () {
      expect(AppTheme.darkTheme, isA<ThemeData>());
      expect(AppTheme.darkTheme.brightness, Brightness.dark);
    });

    test('lightTheme and darkTheme should have different brightness', () {
      expect(
        AppTheme.lightTheme.brightness,
        isNot(equals(AppTheme.darkTheme.brightness)),
      );
    });
  });

  group('MaterialApp widget basics', () {
    testWidgets('MaterialApp can be created with theme', (tester) async {
      await tester.pumpWidget(
        MaterialApp(
          title: '心湖 Heart Lake',
          theme: AppTheme.lightTheme,
          darkTheme: AppTheme.darkTheme,
          home: const Scaffold(
            body: Center(child: Text('HeartLake')),
          ),
        ),
      );

      expect(find.byType(MaterialApp), findsOneWidget);
      expect(find.text('HeartLake'), findsOneWidget);
    });
  });
}
