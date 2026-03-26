import '../../utils/input_validator.dart';
import '../../utils/payload_contract.dart';
import 'base_service.dart';

/// 守望者服务
///
/// 提供灯火转赠、情绪洞察和守望者对话功能。
class GuardianService extends BaseService {
  @override
  String get serviceName => 'GuardianService';

  Map<String, dynamic> _normalizeGuardianPayload(dynamic raw) {
    if (raw is! Map) {
      throw StateError('Guardian success payload is not a map');
    }

    final payload = normalizePayloadContract(
      Map<String, dynamic>.from(raw.cast<String, dynamic>()),
    );
    final role = payload['role'];
    if (role != null) {
      payload['role_name'] = role;
      payload['roleName'] = role;
    }
    return payload;
  }

  /// 获取守望者统计数据
  ///
  /// 返回灯火数量、守望天数等统计信息。
  Future<Map<String, dynamic>> getStats() async {
    final response = await get('/guardian/stats');
    if (!response.success) return toMap(response);

    final payload = _normalizeGuardianPayload(response.data);
    if (payload.isEmpty) {
      throw StateError('Guardian stats success payload is empty');
    }
    return {
      ...toMap(response),
      'data': payload,
      ...payload,
    };
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
    if (!response.success) return toMap(response);

    final payload = _normalizeGuardianPayload(response.data);
    if (payload['target_user_id'] == null) {
      payload['target_user_id'] = toUserId;
      payload['targetUserId'] = toUserId;
      payload['to_user_id'] = toUserId;
      payload['toUserId'] = toUserId;
    }
    return {
      ...toMap(response),
      'data': payload,
      ...payload,
    };
  }

  /// 获取情绪洞察
  ///
  /// 返回基于用户行为的情绪洞察分析。
  Future<Map<String, dynamic>> getEmotionInsights() async {
    final response = await get('/guardian/insights');
    if (!response.success) return toMap(response);

    final payload = _normalizeGuardianPayload(response.data);
    return {
      ...toMap(response),
      'data': payload,
    };
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
    if (!response.success) return toMap(response);

    final payload = _normalizeGuardianPayload(response.data);
    final reply = payload['reply'] ?? payload['response'] ?? payload['content'];
    if (reply != null) {
      payload['reply'] = reply;
      payload['response'] = reply;
      payload['content'] = reply;
    }

    return {
      ...toMap(response),
      'data': payload,
      'reply': payload['reply'],
      'response': payload['response'],
    };
  }
}
