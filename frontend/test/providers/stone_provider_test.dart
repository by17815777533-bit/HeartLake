import 'package:flutter/foundation.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/domain/entities/stone.dart';

/// StoneProvider 依赖 StoneService/CacheService/WebSocketManager 单例，无法直接实例化。
/// 提取核心状态管理逻辑进行测试。

class StoneState with ChangeNotifier {
  List<Stone> _stones = [];
  bool _isLoading = false;
  bool _isLoadingMore = false;
  bool _hasMore = true;
  int _currentPage = 1;
  String? _errorMessage;

  List<Stone> get stones => List.unmodifiable(_stones);
  bool get isLoading => _isLoading;
  bool get isLoadingMore => _isLoadingMore;
  bool get hasMore => _hasMore;
  int get currentPage => _currentPage;
  String? get errorMessage => _errorMessage;

  Future<void> loadStones(Future<Map<String, dynamic>> Function(int page) fetchFn) async {
    if (_isLoading) return;
    _isLoading = true;
    _errorMessage = null;
    notifyListeners();

    try {
      final result = await fetchFn(1);
      if (result['success'] == true) {
        _stones = List<Stone>.from(result['stones'] ?? []);
        _currentPage = 1;
        final pagination = result['pagination'] as Map<String, dynamic>?;
        _hasMore = pagination?['has_more'] ?? false;
      } else {
        _errorMessage = result['message'] ?? '加载失败';
      }
    } catch (e) {
      _errorMessage = '加载失败: $e';
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  Future<void> loadMore(Future<Map<String, dynamic>> Function(int page) fetchFn) async {
    if (_isLoadingMore || !_hasMore) return;
    _isLoadingMore = true;
    notifyListeners();

    try {
      final nextPage = _currentPage + 1;
      final result = await fetchFn(nextPage);
      if (result['success'] == true) {
        final newStones = List<Stone>.from(result['stones'] ?? []);
        _stones = [..._stones, ...newStones];
        _currentPage = nextPage;
        final pagination = result['pagination'] as Map<String, dynamic>?;
        _hasMore = pagination?['has_more'] ?? false;
      }
    } catch (_) {
    } finally {
      _isLoadingMore = false;
      notifyListeners();
    }
  }

  Future<Map<String, dynamic>> createStone(Future<Map<String, dynamic>> Function() createFn) async {
    try {
      final result = await createFn();
      if (result['success'] == true) {
        // 投石成功后刷新列表（实际中会触发 refresh）
      }
      return result;
    } catch (e) {
      return {'success': false, 'message': '投石失败: $e'};
    }
  }

  Future<Map<String, dynamic>> deleteStone(String stoneId, Future<Map<String, dynamic>> Function(String) deleteFn) async {
    try {
      final result = await deleteFn(stoneId);
      if (result['success'] == true) {
        _stones = _stones.where((s) => s.stoneId != stoneId).toList();
        notifyListeners();
      }
      return result;
    } catch (e) {
      return {'success': false, 'message': '删除失败: $e'};
    }
  }

  void handleNewStone(Map<String, dynamic> data) {
    try {
      final stone = Stone.fromJson(data);
      _stones = [stone, ..._stones];
      notifyListeners();
    } catch (_) {}
  }

  void handleRippleUpdate(Map<String, dynamic> data) {
    final stoneId = data['stone_id']?.toString();
    final rippleCount = data['ripple_count'];
    if (stoneId == null) return;
    final idx = _stones.indexWhere((s) => s.stoneId == stoneId);
    if (idx >= 0 && rippleCount != null) {
      _stones[idx] = _stones[idx].copyWith(rippleCount: rippleCount);
      notifyListeners();
    }
  }

  void handleBoatUpdate(Map<String, dynamic> data) {
    final stoneId = data['stone_id']?.toString();
    final boatCount = data['boat_count'];
    if (stoneId == null) return;
    final idx = _stones.indexWhere((s) => s.stoneId == stoneId);
    if (idx >= 0 && boatCount != null) {
      _stones[idx] = _stones[idx].copyWith(boatCount: boatCount);
      notifyListeners();
    }
  }

  void handleStoneDeleted(Map<String, dynamic> data) {
    final stoneId = data['stone_id']?.toString();
    if (stoneId == null) return;
    _stones = _stones.where((s) => s.stoneId != stoneId).toList();
    notifyListeners();
  }

  void clear() {
    _stones = [];
    _currentPage = 1;
    _hasMore = true;
    _errorMessage = null;
    _isLoading = false;
    _isLoadingMore = false;
    notifyListeners();
  }
}

Stone _makeStone(String id, {int rippleCount = 0, int boatCount = 0}) {
  return Stone(
    stoneId: id,
    userId: 'u1',
    content: '内容$id',
    stoneType: 'medium',
    stoneColor: '#7A92A3',
    createdAt: DateTime(2026, 1, 1),
    rippleCount: rippleCount,
    boatCount: boatCount,
  );
}

void main() {
  late StoneState state;

  setUp(() {
    state = StoneState();
  });

  group('Initial state', () {
    test('should have empty stones', () {
      expect(state.stones, isEmpty);
    });

    test('should not be loading', () {
      expect(state.isLoading, false);
    });

    test('should not be loading more', () {
      expect(state.isLoadingMore, false);
    });

    test('should have more by default', () {
      expect(state.hasMore, true);
    });

    test('should be on page 1', () {
      expect(state.currentPage, 1);
    });

    test('should have no error', () {
      expect(state.errorMessage, isNull);
    });
  });

  group('loadStones', () {
    test('should load stones successfully', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1'), _makeStone('s2')],
        'pagination': {'has_more': true},
      });

      expect(state.stones.length, 2);
      expect(state.stones[0].stoneId, 's1');
      expect(state.hasMore, true);
      expect(state.isLoading, false);
      expect(state.errorMessage, isNull);
    });

    test('should handle empty result', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': <Stone>[],
        'pagination': {'has_more': false},
      });

      expect(state.stones, isEmpty);
      expect(state.hasMore, false);
    });

    test('should handle failure', () async {
      await state.loadStones((page) async => {
        'success': false,
        'message': '网络错误',
      });

      expect(state.stones, isEmpty);
      expect(state.errorMessage, '网络错误');
      expect(state.isLoading, false);
    });

    test('should handle exception', () async {
      await state.loadStones((page) async => throw Exception('boom'));

      expect(state.errorMessage, contains('加载失败'));
      expect(state.isLoading, false);
    });

    test('should use default error message', () async {
      await state.loadStones((page) async => {'success': false});

      expect(state.errorMessage, '加载失败');
    });

    test('should not double-load', () async {
      var callCount = 0;
      // Start first load
      final future1 = state.loadStones((page) async {
        callCount++;
        await Future.delayed(const Duration(milliseconds: 50));
        return {'success': true, 'stones': <Stone>[], 'pagination': {'has_more': false}};
      });
      // Try second load immediately
      final future2 = state.loadStones((page) async {
        callCount++;
        return {'success': true, 'stones': <Stone>[], 'pagination': {'has_more': false}};
      });
      await Future.wait([future1, future2]);
      expect(callCount, 1);
    });

    test('should replace existing stones on reload', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': false},
      });
      expect(state.stones.length, 1);

      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s2'), _makeStone('s3')],
        'pagination': {'has_more': false},
      });
      expect(state.stones.length, 2);
      expect(state.stones[0].stoneId, 's2');
    });

    test('should notify listeners during load', () async {
      var notifyCount = 0;
      state.addListener(() => notifyCount++);

      await state.loadStones((page) async => {
        'success': true,
        'stones': <Stone>[],
        'pagination': {'has_more': false},
      });

      expect(notifyCount, greaterThanOrEqualTo(2)); // start + end
    });

    test('should handle null pagination', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
      });

      expect(state.hasMore, false);
    });

    test('should handle null stones', () async {
      await state.loadStones((page) async => {
        'success': true,
        'pagination': {'has_more': false},
      });

      expect(state.stones, isEmpty);
    });
  });

  group('loadMore', () {
    test('should append stones', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': true},
      });

      await state.loadMore((page) async => {
        'success': true,
        'stones': [_makeStone('s2')],
        'pagination': {'has_more': false},
      });

      expect(state.stones.length, 2);
      expect(state.currentPage, 2);
      expect(state.hasMore, false);
    });

    test('should not load more when hasMore is false', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': false},
      });

      var called = false;
      await state.loadMore((page) async {
        called = true;
        return {'success': true, 'stones': <Stone>[]};
      });

      expect(called, false);
    });

    test('should not double-load more', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': true},
      });

      var callCount = 0;
      final f1 = state.loadMore((page) async {
        callCount++;
        await Future.delayed(const Duration(milliseconds: 50));
        return {'success': true, 'stones': [_makeStone('s2')], 'pagination': {'has_more': false}};
      });
      final f2 = state.loadMore((page) async {
        callCount++;
        return {'success': true, 'stones': [_makeStone('s3')], 'pagination': {'has_more': false}};
      });
      await Future.wait([f1, f2]);
      expect(callCount, 1);
    });

    test('should handle failure silently', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': true},
      });

      await state.loadMore((page) async => {'success': false});

      expect(state.stones.length, 1);
      expect(state.isLoadingMore, false);
    });

    test('should handle exception silently', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': true},
      });

      await state.loadMore((page) async => throw Exception('fail'));

      expect(state.stones.length, 1);
      expect(state.isLoadingMore, false);
    });

    test('should increment page correctly', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': true},
      });
      expect(state.currentPage, 1);

      await state.loadMore((page) async {
        expect(page, 2);
        return {'success': true, 'stones': [_makeStone('s2')], 'pagination': {'has_more': true}};
      });
      expect(state.currentPage, 2);

      await state.loadMore((page) async {
        expect(page, 3);
        return {'success': true, 'stones': [_makeStone('s3')], 'pagination': {'has_more': false}};
      });
      expect(state.currentPage, 3);
    });
  });

  group('createStone', () {
    test('should return success result', () async {
      final result = await state.createStone(() async => {'success': true, 'stone_id': 's1'});
      expect(result['success'], true);
      expect(result['stone_id'], 's1');
    });

    test('should return failure result', () async {
      final result = await state.createStone(() async => {'success': false, 'message': '内容违规'});
      expect(result['success'], false);
    });

    test('should handle high_risk response', () async {
      final result = await state.createStone(() async => {
        'success': false,
        'high_risk': true,
        'message': '检测到高危内容',
      });
      expect(result['success'], false);
      expect(result['high_risk'], true);
    });

    test('should handle exception', () async {
      final result = await state.createStone(() async => throw Exception('网络错误'));
      expect(result['success'], false);
      expect(result['message'], contains('投石失败'));
    });
  });

  group('deleteStone', () {
    test('should remove stone from list on success', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1'), _makeStone('s2'), _makeStone('s3')],
        'pagination': {'has_more': false},
      });

      final result = await state.deleteStone('s2', (id) async => {'success': true});
      expect(result['success'], true);
      expect(state.stones.length, 2);
      expect(state.stones.any((s) => s.stoneId == 's2'), false);
    });

    test('should not remove stone on failure', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': false},
      });

      await state.deleteStone('s1', (id) async => {'success': false});
      expect(state.stones.length, 1);
    });

    test('should handle exception', () async {
      final result = await state.deleteStone('s1', (id) async => throw Exception('fail'));
      expect(result['success'], false);
    });

    test('should handle deleting non-existent stone', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': false},
      });

      final result = await state.deleteStone('s999', (id) async => {'success': true});
      expect(result['success'], true);
      expect(state.stones.length, 1);
    });
  });

  group('handleNewStone (WebSocket)', () {
    test('should prepend new stone', () {
      state.handleNewStone({
        'stone_id': 'ws1',
        'user_id': 'u1',
        'content': 'WebSocket石头',
        'stone_type': 'light',
        'stone_color': '#FFF',
        'created_at': '2026-01-01T00:00:00Z',
      });

      expect(state.stones.length, 1);
      expect(state.stones[0].stoneId, 'ws1');
    });

    test('should prepend to existing list', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': false},
      });

      state.handleNewStone({
        'stone_id': 'ws1',
        'user_id': 'u1',
        'content': '新石头',
        'stone_type': 'light',
        'stone_color': '#FFF',
        'created_at': '2026-01-01T00:00:00Z',
      });

      expect(state.stones.length, 2);
      expect(state.stones[0].stoneId, 'ws1');
    });

    test('should handle invalid data gracefully', () {
      state.handleNewStone({'invalid': 'data'});
      // Should not crash, stone_id defaults to ''
      expect(state.stones.length, 1);
    });

    test('should notify listeners', () {
      var notified = false;
      state.addListener(() => notified = true);
      state.handleNewStone({
        'stone_id': 'ws1',
        'user_id': 'u1',
        'content': '内容',
        'stone_type': 'medium',
        'stone_color': '#000',
        'created_at': '2026-01-01',
      });
      expect(notified, true);
    });
  });

  group('handleRippleUpdate (WebSocket)', () {
    test('should update ripple count', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1', rippleCount: 5)],
        'pagination': {'has_more': false},
      });

      state.handleRippleUpdate({'stone_id': 's1', 'ripple_count': 10});
      expect(state.stones[0].rippleCount, 10);
    });

    test('should ignore unknown stone_id', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1', rippleCount: 5)],
        'pagination': {'has_more': false},
      });

      state.handleRippleUpdate({'stone_id': 's999', 'ripple_count': 10});
      expect(state.stones[0].rippleCount, 5);
    });

    test('should ignore null stone_id', () {
      state.handleRippleUpdate({'ripple_count': 10});
      // Should not crash
    });

    test('should ignore null ripple_count', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1', rippleCount: 5)],
        'pagination': {'has_more': false},
      });

      state.handleRippleUpdate({'stone_id': 's1'});
      expect(state.stones[0].rippleCount, 5);
    });
  });

  group('handleBoatUpdate (WebSocket)', () {
    test('should update boat count', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1', boatCount: 3)],
        'pagination': {'has_more': false},
      });

      state.handleBoatUpdate({'stone_id': 's1', 'boat_count': 7});
      expect(state.stones[0].boatCount, 7);
    });

    test('should ignore unknown stone_id', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1', boatCount: 3)],
        'pagination': {'has_more': false},
      });

      state.handleBoatUpdate({'stone_id': 's999', 'boat_count': 7});
      expect(state.stones[0].boatCount, 3);
    });

    test('should ignore null stone_id', () {
      state.handleBoatUpdate({'boat_count': 7});
    });

    test('should ignore null boat_count', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1', boatCount: 3)],
        'pagination': {'has_more': false},
      });

      state.handleBoatUpdate({'stone_id': 's1'});
      expect(state.stones[0].boatCount, 3);
    });
  });

  group('handleStoneDeleted (WebSocket)', () {
    test('should remove stone', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1'), _makeStone('s2')],
        'pagination': {'has_more': false},
      });

      state.handleStoneDeleted({'stone_id': 's1'});
      expect(state.stones.length, 1);
      expect(state.stones[0].stoneId, 's2');
    });

    test('should ignore unknown stone_id', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': false},
      });

      state.handleStoneDeleted({'stone_id': 's999'});
      expect(state.stones.length, 1);
    });

    test('should ignore null stone_id', () {
      state.handleStoneDeleted({});
    });
  });

  group('clear', () {
    test('should reset all state', () async {
      await state.loadStones((page) async => {
        'success': true,
        'stones': [_makeStone('s1')],
        'pagination': {'has_more': true},
      });

      state.clear();

      expect(state.stones, isEmpty);
      expect(state.currentPage, 1);
      expect(state.hasMore, true);
      expect(state.errorMessage, isNull);
      expect(state.isLoading, false);
      expect(state.isLoadingMore, false);
    });

    test('should notify listeners', () {
      var notified = false;
      state.addListener(() => notified = true);
      state.clear();
      expect(notified, true);
    });
  });
}
