// @file emotion_pulse_service.dart
// @brief 情绪脉搏服务 - 封装情绪脉搏和边缘AI状态API

import 'base_service.dart';

class EmotionPulseService extends BaseService {
  @override
  String get serviceName => 'EmotionPulse';

  /// 获取当前情绪脉搏数据
  Future<Map<String, dynamic>> getEmotionPulse() async {
    final resp = await get<dynamic>('/emotion-pulse');
    if (resp.success && resp.data is Map<String, dynamic>) {
      return resp.data as Map<String, dynamic>;
    }
    return {};
  }

  /// 获取边缘AI引擎状态
  Future<Map<String, dynamic>> getEdgeAIStatus() async {
    final resp = await get<dynamic>('/edge-ai/status');
    if (resp.success && resp.data is Map<String, dynamic>) {
      return resp.data as Map<String, dynamic>;
    }
    return {};
  }

  /// 提交情绪快照（用于脉搏追踪）
  Future<bool> submitEmotionSnapshot({
    required String mood,
    double? intensity,
    String? note,
  }) async {
    final resp = await post<dynamic>('/emotion-pulse/snapshot', data: {
      'mood': mood,
      if (intensity != null) 'intensity': intensity,
      if (note != null) 'note': note,
    });
    return resp.success;
  }

  /// 获取情绪脉搏历史趋势
  Future<List<Map<String, dynamic>>> getPulseHistory({int days = 7}) async {
    final resp = await get<dynamic>('/emotion-pulse/history',
        queryParameters: {'days': days});
    if (resp.success && resp.data != null) {
      if (resp.data is List) {
        return (resp.data as List).cast<Map<String, dynamic>>();
      }
      if (resp.data is Map<String, dynamic>) {
        final data = resp.data as Map<String, dynamic>;
        for (final key in ['history', 'pulses', 'data']) {
          if (data[key] is List) {
            return (data[key] as List).cast<Map<String, dynamic>>();
          }
        }
      }
    }
    return [];
  }
}
