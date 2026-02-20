// @file stone_service.dart
// @brief 石头服务 - 负责石头的发布、获取、编辑、删除
// Created by 王璐瑶

import 'package:flutter/foundation.dart';
import '../../domain/entities/stone.dart';
import 'base_service.dart';

/// 石头服务 - 负责发布、获取、编辑、删除石头
class StoneService extends BaseService {
  @override
  String get serviceName => 'StoneService';

  // 发布石头
  Future<Map<String, dynamic>> createStone({
    required String content,
    required String stoneType,
    required String stoneColor,
    String? moodType,
    bool isAnonymous = true,
    List<String>? tags,
  }) async {
    final response = await post('/stones', data: {
      'content': content,
      'stone_type': stoneType,
      'stone_color': stoneColor,
      'is_anonymous': isAnonymous,
      if (moodType != null) 'mood_type': moodType,
      if (tags != null) 'tags': tags,
    });

    // 高危内容检测 - 必须在success检查之前
    if (response.code == 403) {
      return {
        'success': false,
        'high_risk': true,
        'message': response.message,
        'help_tip': response.data?['message'],
      };
    }

    if (!response.success) {
      return toMap(response);
    }

    return {
      'success': true,
      'stone_id': response.data?['stone_id'],
    };
  }

  // 获取石头列表（观湖）
  Future<Map<String, dynamic>> getStones({
    int page = 1,
    int pageSize = 20,
    String sort = 'latest',
  }) async {
    final response = await get('/lake/stones', queryParameters: {
      'page': page,
      'page_size': pageSize,
      'sort': sort,
    });

    if (!response.success) {
      return toMap(response);
    }

    final data = response.data;
    final items = data?['stones'] as List? ?? [];
    final List<Stone> stones = [];
    for (final json in items) {
      try {
        stones.add(Stone.fromJson(json));
      } catch (e) {
        debugPrint('跳过无法解析的石头: $e');
      }
    }

    return {
      'success': true,
      'stones': stones,
      'pagination': _buildPagination(data),
    };
  }

  // 获取我的石头
  Future<Map<String, dynamic>> getMyStones({
    int page = 1,
    int pageSize = 20,
  }) async {
    final response = await get('/stones/my', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });

    if (!response.success) {
      return toMap(response);
    }

    final data = response.data;
    final items = data?['stones'] as List? ?? [];
    final List<Stone> stones = [];
    for (final json in items) {
      try {
        stones.add(Stone.fromJson(json));
      } catch (e) {
        debugPrint('跳过无法解析的石头: $e');
      }
    }

    return {
      'success': true,
      'stones': stones,
      'pagination': _buildPagination(data),
    };
  }

  // 获取湖面气象
  Future<Map<String, dynamic>> getLakeWeather() async {
    final response = await get('/lake/weather');

    if (!response.success) {
      return toMap(response);
    }

    return {
      'success': true,
      'weather': response.data,
    };
  }

  // 删除石头
  Future<Map<String, dynamic>> deleteStone(String stoneId) async {
    final response = await delete('/stones/$stoneId');
    return toMap(response);
  }

  // 构建分页信息（API返回的分页字段在data顶层，不是嵌套的pagination对象）
  Map<String, dynamic> _buildPagination(Map<String, dynamic>? data) {
    final page = data?['page'] ?? 1;
    final totalPages = data?['total_pages'] ?? 1;
    final total = data?['total'] ?? 0;
    final pageSize = data?['page_size'] ?? 20;
    return {
      'page': page,
      'total_pages': totalPages,
      'total': total,
      'page_size': pageSize,
      'has_more': page < totalPages,
    };
  }

  // 获取石头详情
  Future<Map<String, dynamic>> getStoneDetail(String stoneId) async {
    final response = await get('/stones/$stoneId');
    if (!response.success) {
      return toMap(response);
    }
    return {
      'success': true,
      'stone': response.data,
    };
  }
}
