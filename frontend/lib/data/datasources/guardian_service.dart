import '../../utils/input_validator.dart';
import 'base_service.dart';

/// 守望者服务
///
/// 提供灯火转赠、情绪洞察和守望者对话功能。
class GuardianService extends BaseService {
  @override
  String get serviceName => 'GuardianService';

  /// 获取守望者统计数据
  ///
  /// 返回灯火数量、守望天数等统计信息。
  Future<Map<String, dynamic>> getStats() async {
    final response = await get('/guardian/stats');
    return toMap(response);
  }

  /// 转赠灯火
  ///
  /// 将一盏灯火转赠给指定用户。
  ///
  /// [toUserId] 目标用户ID
  Future<Map<String, dynamic>> transferLamp(String toUserId) async {
    InputValidator.validateUUID(toUserId, '目标用户ID');
    final response = await post('/guardian/transfer-lamp', data: {
      'to_user_id': toUserId,
    });
    return toMap(response);
  }

  /// 获取情绪洞察
  ///
  /// 返回基于用户行为的情绪洞察分析。
  Future<Map<String, dynamic>> getEmotionInsights() async {
    final response = await get('/guardian/insights');
    return toMap(response);
  }

  /// 守望者对话
  ///
  /// 与守望者进行对话交流。
  ///
  /// [content] 对话内容，1-2000字符
  Future<Map<String, dynamic>> chat(String content) async {
    InputValidator.requireLength(content, '对话内容', min: 1, max: 2000);
    content = InputValidator.sanitizeText(content);
    final response = await post('/guardian/chat', data: {
      'content': content,
    });
    return toMap(response);
  }
}
