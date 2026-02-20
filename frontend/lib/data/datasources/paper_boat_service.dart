// @file paper_boat_service.dart
// @brief 漂流纸船服务 - 纸船漂流功能
// Created by 王璐瑶

import 'base_service.dart';

class PaperBoatService extends BaseService {
  @override
  String get serviceName => 'PaperBoatService';

  /// 放漂纸船
  Future<Map<String, dynamic>> sendBoat({
    required String content,
    required String mood,
    String driftMode = 'random',
    String? receiverId,
    String boatStyle = 'paper',
  }) async {
    final response = await post('/boats/drift', data: {
      'content': content,
      'mood': mood,
      'drift_mode': driftMode,
      if (receiverId != null) 'receiver_id': receiverId,
      'boat_style': boatStyle,
    });
    return toMap(response);
  }

  /// 回复石头（创建关联纸船）
  Future<Map<String, dynamic>> replyToStone({
    required String stoneId,
    required String content,
    required String mood,
  }) async {
    final response = await post('/boats/reply', data: {
      'stone_id': stoneId,
      'content': content,
      'mood': mood,
    });
    return toMap(response);
  }

  /// 捞纸船
  Future<Map<String, dynamic>> catchBoat() async {
    final response = await post('/boats/catch');
    return toMap(response);
  }

  /// 回应纸船
  Future<Map<String, dynamic>> respondToBoat(String boatId, String content) async {
    final response = await post('/boats/$boatId/respond', data: {
      'content': content,
    });
    return toMap(response);
  }

  /// 扔回水中
  Future<Map<String, dynamic>> releaseBoat(String boatId) async {
    final response = await post('/boats/$boatId/release');
    return toMap(response);
  }

  /// 获取纸船详情
  Future<Map<String, dynamic>> getBoatDetail(String boatId) async {
    final response = await get('/boats/$boatId');
    return toMap(response);
  }

  /// 获取我发送的纸船
  Future<Map<String, dynamic>> getMySentBoats({int page = 1, int pageSize = 20}) async {
    final response = await get('/boats/sent', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    return toMap(response);
  }

  /// 获取我收到的纸船
  Future<Map<String, dynamic>> getMyReceivedBoats({int page = 1, int pageSize = 20}) async {
    final response = await get('/boats/received', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    return toMap(response);
  }

  /// 获取漂流中纸船数量
  Future<Map<String, dynamic>> getDriftingCount() async {
    final response = await get('/boats/drifting/count');
    return toMap(response);
  }

  /// 获取纸船状态
  Future<Map<String, dynamic>> getBoatStatus(String boatId) async {
    final response = await get('/boats/$boatId/status');
    return toMap(response);
  }
}
