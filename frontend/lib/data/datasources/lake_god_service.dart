// @file lake_god_service.dart
// @brief 湖神AI对话服务 - 对接 EdgeAIController 的湖神端点（有消息持久化+历史记录）
import 'base_service.dart';

class LakeGodService extends BaseService {
  @override
  String get serviceName => 'LakeGodService';

  /// 发送消息给湖神
  /// 后端 POST /api/lake-god/chat
  /// 返回: { "code": 0, "data": { "reply": "...", "emotion": {...}, "rag_context": [...] } }
  Future<Map<String, dynamic>> sendMessage(String content, {String? emotion, double? emotionScore}) async {
    final resp = await post('/lake-god/chat', data: {
      'content': content,
      if (emotion != null) 'emotion': emotion,
      if (emotionScore != null) 'emotion_score': emotionScore,
    });
    return toMap(resp);
  }

  /// 获取历史消息
  /// 后端 GET /api/lake-god/history
  /// 返回: { "code": 0, "data": { "messages": [{ "role": "user"/"assistant", "content": "...", "created_at": "..." }] } }
  Future<Map<String, dynamic>> getMessages() async {
    final resp = await get('/lake-god/history');
    return toMap(resp);
  }
}
