import 'base_service.dart';
import '../../utils/input_validator.dart';
import '../../utils/payload_contract.dart';

/// EdgeAI引擎客户端服务
///
/// 封装普通用户可访问的端侧智能接口，提供情感分析、隐私预算查询
/// 和社区情绪脉搏等功能。管理后台专用端点由admin模块单独调用。
class EdgeAIService extends BaseService {
  @override
  String get serviceName => 'EdgeAIService';

  Map<String, dynamic> _normalizeEdgePayload(dynamic raw) {
    if (raw is! Map) {
      throw StateError('EdgeAI payload is not a map');
    }

    final payload = normalizePayloadContract(
      Map<String, dynamic>.from(raw.cast<String, dynamic>()),
    );
    final status = payload['status'] ?? payload['engine_status'];
    if (status != null) {
      payload['status'] = status;
      payload['engine_status'] = status;
    }
    return payload;
  }

  Map<String, dynamic> _requirePayload(
    dynamic raw, {
    required String endpoint,
  }) {
    final payload = _normalizeEdgePayload(raw);
    if (payload.isEmpty) {
      throw StateError('$endpoint payload is empty');
    }
    return payload;
  }

  /// 查询EdgeAI引擎运行状态
  ///
  /// 返回引擎的运行状态信息。
  Future<Map<String, dynamic>> getStatus() async {
    final response = await get('/edge-ai/status');
    if (!response.success) return toMap(response);

    final payload = _requirePayload(
      response.data,
      endpoint: '/edge-ai/status',
    );
    return {
      ...toMap(response),
      'data': payload,
    };
  }

  /// 调用后端情感分析接口
  ///
  /// [text] 待分析文本，1-5000字符
  /// [preferOnnx] 是否优先使用ONNX推理，默认true
  Future<Map<String, dynamic>> analyzeSentiment(
    String text, {
    bool preferOnnx = true,
  }) async {
    InputValidator.requireLength(text, '分析文本', min: 1, max: 5000);
    text = InputValidator.sanitizeText(text);
    final response = await post('/edge-ai/analyze', data: {
      'text': text,
      'prefer_onnx': preferOnnx,
      'analysis_mode': preferOnnx ? 'onnx_preferred' : 'balanced',
    });
    if (!response.success) return toMap(response);

    final payload = _requirePayload(
      response.data,
      endpoint: '/edge-ai/analyze',
    );
    return {
      ...toMap(response),
      'data': payload,
      ...payload,
    };
  }

  /// 查询差分隐私预算
  ///
  /// 返回当前用户的差分隐私预算消耗情况。
  Future<Map<String, dynamic>> getPrivacyBudget() async {
    final response = await get('/edge-ai/privacy-budget', useCache: false);
    if (!response.success) return toMap(response);

    final payload = _requirePayload(
      response.data,
      endpoint: '/edge-ai/privacy-budget',
    );
    return {
      ...toMap(response),
      'data': payload,
      ...payload,
    };
  }

  /// 获取社区情绪脉搏
  ///
  /// 返回社区整体的情绪分布和趋势数据。
  Future<Map<String, dynamic>> getEmotionPulse() async {
    final response = await get('/edge-ai/emotion-pulse', useCache: false);
    if (!response.success) return toMap(response);

    final payload = _requirePayload(
      response.data,
      endpoint: '/edge-ai/emotion-pulse',
    );
    return {
      ...toMap(response),
      'data': payload,
      ...payload,
    };
  }
}
