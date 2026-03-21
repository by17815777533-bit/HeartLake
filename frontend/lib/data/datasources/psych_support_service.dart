import 'base_service.dart';
import '../../utils/input_validator.dart';
import '../../utils/mood_colors.dart';
import '../../utils/payload_contract.dart';
import 'social_payload_normalizer.dart';

/// 心理支持服务，对接安全港湾模块，提供热线、工具、推荐资源等接口
class PsychSupportService extends BaseService {
  @override
  String get serviceName => 'PsychSupport';

  /// 允许传入的情绪类型
  static const _allowedMoods = MoodColors.supportedMoodKeys;

  Map<String, dynamic> _normalizeSupportItem(Map raw) {
    final item = normalizePayloadContract(
      Map<String, dynamic>.from(raw.cast<String, dynamic>()),
    );

    final title = item['title'] ?? item['name'];
    if (title != null) {
      item['title'] = title;
      item['name'] = title;
    }

    final description =
        item['description'] ?? item['desc'] ?? item['content'];
    if (description != null) {
      item['description'] = description;
      item['desc'] = description;
      item['content'] = description;
    }

    return item;
  }

  Map<String, dynamic> _normalizePromptPayload(dynamic raw) {
    if (raw is Map) {
      final payload = normalizePayloadContract(
        Map<String, dynamic>.from(raw.cast<String, dynamic>()),
      );
      final prompt =
          payload['prompt'] ?? payload['text'] ?? payload['message'] ?? payload['title'] ?? payload['content'];
      if (prompt != null) {
        payload['prompt'] = prompt;
        payload['text'] = prompt;
        payload['message'] = prompt;
        payload['title'] = prompt;
        payload['content'] = prompt;
      }
      return payload;
    }

    final text = raw?.toString().trim();
    if (text == null || text.isEmpty) {
      return const <String, dynamic>{};
    }
    return {
      'prompt': text,
      'text': text,
      'message': text,
      'title': text,
      'content': text,
    };
  }

  /// 获取心理援助热线列表
  Future<Map<String, dynamic>> getHotlines() async {
    final response = await get('/safe-harbor/hotlines');
    if (!response.success) return toMap(response);

    final hotlines = extractNormalizedList(
      response.data,
      itemNormalizer: _normalizeSupportItem,
      listKeys: const ['hotlines'],
    );

    return {
      ...toMap(response),
      'data': hotlines,
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'hotlines',
        items: hotlines,
      ),
    };
  }

  /// 获取自助工具列表
  Future<Map<String, dynamic>> getTools() async {
    final response = await get('/safe-harbor/tools');
    if (!response.success) return toMap(response);

    final tools = extractNormalizedList(
      response.data,
      itemNormalizer: _normalizeSupportItem,
      listKeys: const ['tools'],
    );

    return {
      ...toMap(response),
      'data': tools,
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'tools',
        items: tools,
      ),
    };
  }

  /// 获取安全港湾引导语
  Future<Map<String, dynamic>> getPrompt() async {
    final response = await get('/safe-harbor/prompt');
    if (!response.success) return toMap(response);

    final payload = _normalizePromptPayload(response.data);
    return {
      ...toMap(response),
      'data': payload,
      ...payload,
    };
  }

  /// 获取安全港湾资源列表
  Future<Map<String, dynamic>> getResources() async {
    final response = await get('/safe-harbor/resources');
    if (!response.success) return toMap(response);

    final resources = extractNormalizedList(
      response.data,
      itemNormalizer: _normalizeSupportItem,
      listKeys: const ['resources'],
    );

    return {
      ...toMap(response),
      'data': resources,
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'resources',
        items: resources,
      ),
    };
  }

  /// 根据情绪推荐资源
  Future<Map<String, dynamic>> recommendResources({String? mood}) async {
    if (mood != null) {
      InputValidator.validateEnum(mood, _allowedMoods, '情绪类型');
    }
    final response = await get('/safe-harbor/recommend',
        queryParameters:
            mood != null ? {'emotion': mood, 'mood': mood} : null);
    if (!response.success) return toMap(response);

    final resources = extractNormalizedList(
      response.data,
      itemNormalizer: _normalizeSupportItem,
      listKeys: const ['resources', 'recommendations'],
    );

    return {
      ...toMap(response),
      'data': resources,
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'resources',
        items: resources,
      ),
    };
  }

  /// 记录访问（用于统计用户使用安全港湾的频率）
  Future<Map<String, dynamic>> recordAccess(
      {required String resourceId}) async {
    InputValidator.validateUUID(resourceId, '资源ID');
    final response =
        await post('/safe-harbor/access', data: {'resource_id': resourceId});
    return toMap(response);
  }

  /// 获取访问历史
  Future<Map<String, dynamic>> getAccessHistory() async {
    final response = await get('/safe-harbor/access/history');
    if (!response.success) return toMap(response);

    final history = extractNormalizedList(
      response.data,
      itemNormalizer: _normalizeSupportItem,
      listKeys: const ['history', 'access_history', 'records'],
    );

    return {
      ...toMap(response),
      'data': history,
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'history',
        items: history,
      ),
    };
  }
}
