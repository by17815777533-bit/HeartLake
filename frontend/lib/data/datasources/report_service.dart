// 举报服务 - 内容举报处理

import '../../utils/input_validator.dart';
import 'base_service.dart';
class ReportService extends BaseService {
  @override
  String get serviceName => 'ReportService';

  Future<Map<String, dynamic>> createReport({
    required String targetType,
    required String targetId,
    required String reason,
    String? description,
  }) async {
    InputValidator.requireInList(targetType, const [
      'stone', 'boat', 'user', 'message', 'comment',
    ], '举报目标类型');
    InputValidator.requireNonEmpty(targetId, '举报目标ID');
    InputValidator.requireLength(reason, '举报原因', min: 2, max: 200);
    if (description != null) {
      InputValidator.requireLength(description, '举报描述', max: 1000);
    }
    final response = await post('/reports', data: {
      'target_type': targetType,
      'target_id': targetId,
      'reason': reason,
      if (description != null) 'description': description,
    });

    if (!response.success) return toMap(response);

    return {
      ...toMap(response),
      'report_id': response.data?['report_id'],
    };
  }
  Future<Map<String, dynamic>> getMyReports({
    int page = 1,
    int pageSize = 20,
  }) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/reports/my', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });

    if (!response.success) return toMap(response);

    return {
      ...toMap(response),
      'reports': response.data?['reports'],
      'total': response.data?['total'],
    };
  }
}
