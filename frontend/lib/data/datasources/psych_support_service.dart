import 'base_service.dart';
import '../../utils/input_validator.dart';
import '../../utils/mood_colors.dart';

/// 心理支持服务，对接安全港湾模块，提供热线、工具、推荐资源等接口
class PsychSupportService extends BaseService {
  @override
  String get serviceName => 'PsychSupport';

  /// 允许传入的情绪类型
  static const _allowedMoods = MoodColors.supportedMoodKeys;

  /// 获取心理援助热线列表
  Future<Map<String, dynamic>> getHotlines() async {
    final response = await get('/safe-harbor/hotlines');
    return toMap(response);
  }

  /// 获取自助工具列表
  Future<Map<String, dynamic>> getTools() async {
    final response = await get('/safe-harbor/tools');
    return toMap(response);
  }

  /// 获取安全港湾引导语
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
    return toMap(response);
  }
}
