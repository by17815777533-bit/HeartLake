// @file lake_god_service.dart
// @brief 湖神聊天服务 - 通过GuardianController调用DualMemoryRAG
import 'base_service.dart';

class LakeGodService extends BaseService {
  @override
  String get serviceName => 'LakeGodService';

  Future<Map<String, dynamic>> sendMessage(String content, {String? emotion, double? emotionScore}) async {
    final resp = await post('/guardian/chat', data: {
      'content': content,
      if (emotion != null) 'emotion': emotion,
      if (emotionScore != null) 'emotion_score': emotionScore,
    });
    return toMap(resp);
  }

  Future<Map<String, dynamic>> getMessages() async {
    return {'success': true, 'data': []};
  }
}
