import 'base_service.dart';
import '../../utils/input_validator.dart';
import '../../utils/mood_colors.dart';
import 'social_payload_normalizer.dart';

/// 湖神对话服务，对接 EdgeAI 的湖神聊天端点
///
/// 支持带情绪上下文的对话，消息在后端持久化并可查询历史。
class LakeGodService extends BaseService {
  @override
  String get serviceName => 'LakeGodService';

  /// 允许传入的情绪类型
  static const _allowedEmotions = MoodColors.supportedMoodKeys;

  /// 发送消息给湖神，可附带当前情绪类型和情绪分数作为上下文
  Future<Map<String, dynamic>> sendMessage(String content,
      {String? emotion, double? emotionScore}) async {
    InputValidator.requireLength(content, '消息内容', min: 1, max: 2000);
    content = InputValidator.sanitizeText(content);
    if (emotion != null) {
      InputValidator.validateEnum(emotion, _allowedEmotions, '情绪类型');
    }
    if (emotionScore != null) {
      InputValidator.requireDoubleRange(emotionScore, '情绪分数',
          min: 0.0, max: 1.0);
    }
    final resp = await post('/lake-god/chat', data: {
      'content': content,
      if (emotion != null) 'emotion': emotion,
      if (emotionScore != null) 'emotion_score': emotionScore,
    });
    if (!resp.success) return toMap(resp);

    final payload = resp.data is Map
        ? Map<String, dynamic>.from((resp.data as Map).cast<String, dynamic>())
        : <String, dynamic>{};
    final reply = payload['reply'] ?? payload['response'] ?? payload['content'];
    if (reply != null) {
      payload['reply'] = reply;
      payload['response'] = reply;
      payload['content'] = reply;
    }

    return {
      ...toMap(resp),
      'data': payload,
      'reply': payload['reply'],
      'response': payload['response'],
    };
  }

  /// 获取与湖神的历史对话记录
  Future<Map<String, dynamic>> getMessages() async {
    final resp = await get('/lake-god/history');
    if (!resp.success) return toMap(resp);

    final messages = extractNormalizedList(
      resp.data,
      itemNormalizer: normalizeMessagePayload,
      listKeys: const ['messages'],
    );
    return {
      ...toMap(resp),
      'data': {
        'messages': messages,
      },
      ...buildCollectionEnvelope(
        resp.data,
        primaryKey: 'messages',
        items: messages,
      ),
    };
  }
}
