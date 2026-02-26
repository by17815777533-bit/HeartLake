// @file lake_god_service.dart
// @brief 湖神AI对话服务 - 对接 EdgeAIController 的湖神端点（有消息持久化+历史记录）
import 'base_service.dart';
import '../../utils/input_validator.dart';

class LakeGodService extends BaseService {
  @override
  String get serviceName => 'LakeGodService';

  // 湖神支持的情绪类型白名单
  static const _allowedEmotions = [
    'happy', 'sad', 'angry', 'anxious', 'calm', 'confused',
    'hopeful', 'lonely', 'grateful', 'neutral', 'fearful',
    'surprised', 'disgusted', 'depressed', 'excited',
  ];

  /// 发送消息给湖神
  /// 后端 POST /api/lake-god/chat
  /// 返回: { "code": 0, "data": { "reply": "...", "emotion": {...}, "rag_context": [...] } }
  Future<Map<String, dynamic>> sendMessage(String content, {String? emotion, double? emotionScore}) async {
    InputValidator.requireLength(content, '消息内容', min: 1, max: 2000);
    content = InputValidator.sanitizeText(content);
    if (emotion != null) {
      InputValidator.validateEnum(emotion, _allowedEmotions, '情绪类型');
    }
    if (emotionScore != null) {
      InputValidator.requireDoubleRange(emotionScore, '情绪分数', min: 0.0, max: 1.0);
    }
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
