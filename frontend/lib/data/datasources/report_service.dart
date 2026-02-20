// @file report_service.dart
// @brief 举报服务 - 内容举报处理
// Created by 王璐瑶

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
    final response = await post('/reports', data: {
      'target_type': targetType,
      'target_id': targetId,
      'reason': reason,
      if (description != null) 'description': description,
    });

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'report_id': response.data?['report_id'],
    };
  }
  Future<Map<String, dynamic>> getMyReports({
    int page = 1,
    int pageSize = 20,
  }) async {
    final response = await get('/reports/my', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'reports': response.data?['reports'],
      'total': response.data?['total'],
    };
  }
}
