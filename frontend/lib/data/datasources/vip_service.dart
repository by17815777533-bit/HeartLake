import 'base_service.dart';
import '../../utils/input_validator.dart';
import '../../utils/payload_contract.dart';
import 'social_payload_normalizer.dart';

/// 灯火（VIP）服务，管理会员状态、权益查询和心理咨询预约
class VIPService extends BaseService {
  @override
  String get serviceName => '灯火服务';

  Map<String, dynamic> _normalizeVipPayload(dynamic raw) {
    if (raw is! Map) return const <String, dynamic>{};

    final payload = normalizePayloadContract(
      Map<String, dynamic>.from(raw.cast<String, dynamic>()),
    );
    final expiresAt = payload['expires_at'] ?? payload['expiresAt'];
    if (expiresAt != null) {
      payload['expires_at'] = expiresAt;
      payload['expiresAt'] = expiresAt;
    }
    return payload;
  }

  /// 获取当前用户的灯火会员状态
  Future<Map<String, dynamic>> getVIPStatus() async {
    final response = await get('/vip/status');
    if (!response.success) return toMap(response);

    final payload = _normalizeVipPayload(response.data);
    return {
      ...toMap(response),
      'data': payload,
      ...payload,
    };
  }

  /// 获取VIP权益列表
  Future<Map<String, dynamic>> getPrivileges() async {
    final response = await get('/vip/privileges');
    if (!response.success) return toMap(response);

    final privileges = extractNormalizedList(
      response.data,
      itemNormalizer: (raw) => normalizePayloadContract(
        Map<String, dynamic>.from(raw.cast<String, dynamic>()),
      ),
      listKeys: const ['privileges'],
    );
    final payload = response.data is Map
        ? _normalizeVipPayload(response.data)
        : const <String, dynamic>{};

    return {
      ...toMap(response),
      'data': {
        ...payload,
        'privileges': privileges,
        'items': privileges,
        'list': privileges,
      },
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'privileges',
        items: privileges,
      ),
    };
  }

  /// 检查是否有免费心理咨询额度
  Future<bool> hasFreeCounselingQuota() async {
    final response = await get('/vip/counseling/check');
    if (response.success) {
      final payload = _normalizeVipPayload(response.data);
      return payload['has_quota'] == true;
    }
    return false;
  }

  /// 预约心理咨询
  Future<Map<String, dynamic>> bookCounseling({
    required String appointmentTime,
    bool isFreeVIP = false,
  }) async {
    InputValidator.validateFutureISO8601(appointmentTime, '预约时间');
    final response = await post('/vip/counseling/book', data: {
      'appointment_time': appointmentTime,
      'is_free_vip': isFreeVIP,
    });
    if (!response.success) return toMap(response);

    final payload = _normalizeVipPayload(response.data);
    return {
      ...toMap(response),
      'data': payload,
      ...payload,
    };
  }

  /// 获取AI评论频率（小时）
  Future<double> getAICommentFrequency() async {
    final response = await get('/vip/ai-comment-frequency');
    if (response.success) {
      final payload = _normalizeVipPayload(response.data);
      final frequency = payload['frequency_hours'];
      if (frequency is num) return frequency.toDouble();
      return double.tryParse(frequency?.toString() ?? '') ?? 2.0;
    }
    return 2.0;
  }

  /// 检查用户是否是VIP
  Future<bool> isVIP() async {
    final result = await getVIPStatus();
    if (result['success'] == true) {
      final data = result['data'];
      return (data is Map) ? (data['is_vip'] ?? false) : false;
    }
    return false;
  }

  /// 获取VIP剩余天数
  Future<int> getVIPDaysLeft() async {
    final result = await getVIPStatus();
    if (result['success'] == true) {
      final data = result['data'];
      return (data is Map) ? (data['days_left'] ?? 0) : 0;
    }
    return 0;
  }
}
