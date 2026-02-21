// @file stone_repository.dart
// @brief 石头数据仓库
// Created by 王璐瑶

import '../../domain/entities/stone.dart';
import '../datasources/stone_service.dart';
import 'base_repository.dart';

/// Stone 数据仓库
class StoneRepository extends BaseRepository {
  final StoneService _stoneService = StoneService();

  Future<List<Stone>> getStones({int page = 1, int pageSize = 20}) async {
    final cacheKey = 'stones_${page}_$pageSize';

    final cached = cacheService.get<List<Stone>>(cacheKey);
    if (cached != null) return cached;

    final result = await _stoneService.getStones(page: page, pageSize: pageSize);
    if (result['success'] == true) {
      final items = result['stones'] as List? ?? [];
      final stones = items.map((e) => Stone.fromJson(e is Map<String, dynamic> ? e : (e as Stone).toJson())).toList();
      cacheService.set(cacheKey, stones, ttl: const Duration(minutes: 2));
      return stones;
    }
    return [];
  }

  Future<Stone?> getStoneById(String stoneId) async {
    final cacheKey = 'stone_$stoneId';

    final cached = cacheService.get<Stone>(cacheKey);
    if (cached != null) return cached;

    final result = await _stoneService.getStoneDetail(stoneId);
    if (result['success'] == true && result['stone'] != null) {
      final stone = Stone.fromJson(result['stone'] as Map<String, dynamic>);
      cacheService.set(cacheKey, stone, ttl: const Duration(minutes: 5));
      return stone;
    }
    return null;
  }

  Future<List<Stone>> getMyStones({int page = 1, int pageSize = 20}) async {
    final result = await _stoneService.getMyStones(page: page, pageSize: pageSize);
    if (result['success'] == true) {
      final items = result['stones'] as List? ?? [];
      return items.map((e) => Stone.fromJson(e is Map<String, dynamic> ? e : (e as Stone).toJson())).toList();
    }
    return [];
  }

  Future<Stone?> createStone({
    required String content,
    required String stoneType,
    required String stoneColor,
    List<String>? tags,
    String? moodType,
    bool isAnonymous = true,
  }) async {
    final result = await _stoneService.createStone(
      content: content,
      stoneType: stoneType,
      stoneColor: stoneColor,
      tags: tags,
      moodType: moodType,
      isAnonymous: isAnonymous,
    );
    if (result['success'] == true && result['stone_id'] != null) {
      invalidateCache('stones_');
      return null; // createStone 只返回 stone_id，不返回完整 stone
    }
    return null;
  }

  Future<bool> deleteStone(String stoneId) async {
    final result = await _stoneService.deleteStone(stoneId);
    if (result['success'] == true) {
      cacheService.remove('stone_$stoneId');
      invalidateCache('stones_');
      return true;
    }
    return false;
  }
}
