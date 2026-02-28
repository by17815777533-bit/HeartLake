// 用户服务 - 用户信息查询和管理

import 'base_service.dart';
import '../../utils/input_validator.dart';

/// 用户信息查询与管理服务
///
/// 提供面向当前用户和其他用户的查询能力：
/// - 用户搜索（关键词匹配，带 XSS 过滤）
/// - 用户详情与统计数据
/// - 情绪热力图 / 情绪日历（个人情绪可视化数据源）
/// - 收到的纸船列表
/// - 通用文件上传（头像、音频、视频等）
class UserService extends BaseService {
  @override
  String get serviceName => 'UserService';

  /// 搜索用户
  ///
  /// 关键词经过 sanitize 处理后提交，长度限制 1~50 字符。
  /// 返回 `{success, users, total}`。
  Future<Map<String, dynamic>> searchUsers(String query) async {
    InputValidator.requireLength(query, '想找的人', min: 1, max: 50);
    query = InputValidator.sanitizeText(query);
    final response = await get('/users/search', queryParameters: {'q': query});

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'users': response.data?['users'] ?? [],
      'total': response.data?['total'] ?? 0,
    };
  }

  /// 获取用户信息
  Future<Map<String, dynamic>> getUserInfo(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await get('/users/$userId');

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'user': response.data,
    };
  }

  /// 获取用户统计
  Future<Map<String, dynamic>> getUserStats(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await get('/users/$userId/stats');
    if (!response.success) return toMap(response);
    return {'success': true, 'data': response.data};
  }

  /// 获取情绪热力图数据
  ///
  /// 返回当前用户近期的情绪分布热力图原始数据，
  /// 不走缓存以保证实时性。
  Future<Map<String, dynamic>> getEmotionHeatmap() async {
    final response = await get('/users/my/emotion-heatmap', useCache: false);
    if (!response.success) return toMap(response);
    return {'success': true, 'data': response.data};
  }

  /// 获取情绪日历数据
  ///
  /// 按年月查询当月每日的情绪记录，用于日历视图渲染。
  Future<Map<String, dynamic>> getEmotionCalendar(int year, int month) async {
    InputValidator.validateDateRange(year, month);
    final response = await get(
      '/users/my/emotion-calendar',
      queryParameters: {'year': year, 'month': month},
      useCache: false,
    );
    if (!response.success) return toMap(response);
    return {'success': true, 'data': response.data};
  }

  /// 获取我收到的纸船（分页）
  ///
  /// 纸船是其他用户对我的石头的匿名回应。
  Future<Map<String, dynamic>> getMyBoats(
      {int page = 1, int pageSize = 100}) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/users/my/boats',
        queryParameters: {'page': page, 'page_size': pageSize});
    if (!response.success) return toMap(response);
    return {'success': true, 'data': response.data};
  }

  /// 上传文件（头像、音频、视频等）
  ///
  /// 支持格式：jpg/jpeg/png/webp/gif/mp3/wav/aac/mp4。
  /// 通过 [ApiClient.uploadFile] 走 multipart 上传。
  Future<Map<String, dynamic>> uploadFile(dynamic file,
      {String? filename}) async {
    if (filename != null) {
      InputValidator.validateFileType(filename, const [
        'jpg',
        'jpeg',
        'png',
        'webp',
        'gif',
        'mp3',
        'wav',
        'aac',
        'mp4',
      ]);
    }
    try {
      final response = await client.uploadFile('/media/upload', file: file);
      if (response.statusCode == 200 && response.data['code'] == 0) {
        return {'success': true, 'data': response.data['data']};
      }
      return {'success': false, 'message': response.data['message'] ?? '上传失败'};
    } catch (e) {
      return {'success': false, 'message': '上传失败: $e'};
    }
  }
}
