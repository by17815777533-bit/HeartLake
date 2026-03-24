import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/app_config.dart';

void main() {
  group('AppConfig local gateway defaults', () {
    test('desktop development api defaults to gateway origin', () {
      expect(appConfig.apiBaseUrl, 'http://localhost:3000/api');
    });

    test('desktop development websocket defaults to gateway origin', () {
      expect(appConfig.wsUrl, 'ws://localhost:3000/ws/broadcast');
    });
  });
}
