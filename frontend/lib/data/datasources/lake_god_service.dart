// @file lake_god_service.dart
// @brief 湖神AI对话服务 - 对接 /api/edge-ai/lake-god/chat 和 /api/edge-ai/lake-god/history
import 'base_service.dart';

class LakeGodService extends BaseService {
  @override
  String get serviceName => 'LakeGodService';

  Future<Map<String, dynamic>> sendMessage(String content) async {
    final resp =
        await post('/edge-ai/lake-god/chat', data: {'content': content});
    return toMap(resp);
  }

  Future<Map<String, dynamic>> getMessages() async {
    final resp = await get('/edge-ai/lake-god/history');
    return toMap(resp);
  }
}
