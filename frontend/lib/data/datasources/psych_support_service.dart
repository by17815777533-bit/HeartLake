// 心理支持服务 - 获取心理援助资源

import 'base_service.dart';
import '../../utils/input_validator.dart';

class PsychSupportService extends BaseService {
  @override
  String get serviceName => 'PsychSupport';

  // 情绪类型白名单
  static const _allowedMoods = [
    'happy', 'sad', 'angry', 'anxious', 'calm', 'confused',
    'hopeful', 'lonely', 'grateful', 'neutral', 'fearful',
    'surprised', 'disgusted', 'depressed', 'excited',
  ];

  // 资源类型白名单
  static const _allowedResourceTypes = [
    'hotline', 'tool', 'article', 'video', 'exercise',
    'meditation', 'breathing', 'journal', 'prompt',
  ];

  Future<Map<String, dynamic>> getHotlines() async {
    final response = await get('/safe-harbor/hotlines');
    return toMap(response);
  }

  Future<Map<String, dynamic>> getTools() async {
    final response = await get('/safe-harbor/tools');
    return toMap(response);
  }

  Future<Map<String, dynamic>> getPrompt() async {
    final response = await get('/safe-harbor/prompt');
    return toMap(response);
  }

  /// 获取安全港湾资源列表
  Future<Map<String, dynamic>> getResources() async {
    final response = await get('/safe-harbor/resources');
    return toMap(response);
  }

  /// 根据情绪推荐资源
  Future<Map<String, dynamic>> recommendResources({String? mood}) async {
    if (mood != null) {
      InputValidator.validateEnum(mood, _allowedMoods, '情绪类型');
    }
    final response = await get('/safe-harbor/recommend',
        queryParameters: mood != null ? {'mood': mood} : null);
    return toMap(response);
  }

  /// 记录访问（用于统计用户使用安全港湾的频率）
  Future<Map<String, dynamic>> recordAccess({required String resourceType}) async {
    InputValidator.validateEnum(resourceType, _allowedResourceTypes, '资源类型');
    final response =
        await post('/safe-harbor/access', data: {'resource_type': resourceType});
    return toMap(response);
  }

  /// 获取访问历史
  Future<Map<String, dynamic>> getAccessHistory() async {
    final response = await get('/safe-harbor/access/history');
    return toMap(response);
  }
}
