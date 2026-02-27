// StoneCard状态控制器

import 'package:flutter/material.dart';
import '../../../domain/entities/stone.dart';
import '../../../data/datasources/interaction_service.dart';
import '../../../data/datasources/websocket_manager.dart';
import '../../../di/service_locator.dart';
import '../../../utils/storage_util.dart';

/// StoneCard 状态控制器 - 管理业务逻辑和WebSocket监听
class StoneCardController {
  final Stone stone;
  final VoidCallback? onStateChanged;

  final InteractionService _interactionService = sl<InteractionService>();

  bool hasRippled = false;
  int localRipplesCount = 0;
  int localBoatsCount = 0;
  String? currentUserId;

  late Function(Map<String, dynamic>) _rippleUpdateListener;
  late Function(Map<String, dynamic>) _boatUpdateListener;
  late Function(Map<String, dynamic>) _boatDeletedListener;
  late Function(Map<String, dynamic>) _rippleDeletedListener;

  StoneCardController({required this.stone, this.onStateChanged}) {
    localRipplesCount = stone.rippleCount;
    localBoatsCount = stone.boatCount;
    hasRippled = stone.hasRippled;
  }

  Future<void> init() async {
    currentUserId = await StorageUtil.getUserId();
    _setupWebSocketListeners();
    onStateChanged?.call();
  }

  void _setupWebSocketListeners() {
    final ws = WebSocketManager();

    _rippleUpdateListener = (data) {
      final stoneId = data['stone_id'] ?? data['ripple']?['stone_id'];
      if (stoneId == stone.stoneId) {
        final triggeredBy = data['triggered_by']?.toString();
        final serverCount = data['ripple_count'];
        if (serverCount is int && triggeredBy != currentUserId) {
          localRipplesCount = serverCount;
          onStateChanged?.call();
        }
      }
    };
    ws.on('ripple_update', _rippleUpdateListener);

    _boatUpdateListener = (data) {
      final stoneId = data['stone_id'] ?? data['boat']?['stone_id'];
      if (stoneId == stone.stoneId) {
        final triggeredBy = data['triggered_by']?.toString();
        final serverCount = data['boat_count'];
        if (serverCount is int && triggeredBy != currentUserId) {
          localBoatsCount = serverCount;
          onStateChanged?.call();
        }
      }
    };
    ws.on('boat_update', _boatUpdateListener);

    _boatDeletedListener = (data) {
      final stoneId = data['stone_id'] ?? data['boat']?['stone_id'];
      if (stoneId == stone.stoneId) {
        final triggeredBy = data['triggered_by']?.toString();
        final serverCount = data['boat_count'];
        if (serverCount is int && triggeredBy != currentUserId) {
          localBoatsCount = serverCount;
          onStateChanged?.call();
        }
      }
    };
    ws.on('boat_deleted', _boatDeletedListener);

    _rippleDeletedListener = (data) {
      final stoneId = data['stone_id'];
      if (stoneId == stone.stoneId) {
        final triggeredBy = data['triggered_by']?.toString();
        final serverCount = data['ripple_count'];
        if (serverCount is int && triggeredBy != currentUserId) {
          localRipplesCount = serverCount;
          onStateChanged?.call();
        }
      }
    };
    ws.on('ripple_deleted', _rippleDeletedListener);
  }

  void dispose() {
    final ws = WebSocketManager();
    ws.off('ripple_update', _rippleUpdateListener);
    ws.off('boat_update', _boatUpdateListener);
    ws.off('boat_deleted', _boatDeletedListener);
    ws.off('ripple_deleted', _rippleDeletedListener);
  }

  Future<Map<String, dynamic>> createRipple() async {
    if (hasRippled) {
      return {
        'success': true,
        'already_rippled': true,
        'ripple_count': localRipplesCount,
      };
    }

    hasRippled = true;
    localRipplesCount++;
    onStateChanged?.call();

    final result = await _interactionService.createRipple(stone.stoneId);
    if (!result['success']) {
      hasRippled = false;
      if (localRipplesCount > 0) {
        localRipplesCount--;
      }
      onStateChanged?.call();
      return result;
    }

    final dynamic serverCount = result['ripple_count'] ??
        (result['data'] is Map ? result['data']['ripple_count'] : null);
    if (serverCount is int) {
      localRipplesCount = serverCount;
    }

    final alreadyRippled = result['already_rippled'] == true ||
        (result['data'] is Map && result['data']['already_rippled'] == true);
    if (alreadyRippled) {
      hasRippled = true;
    }
    onStateChanged?.call();
    return result;
  }

  Future<Map<String, dynamic>> deleteStone() async {
    return await _interactionService.deleteStone(stone.stoneId);
  }

  Future<Map<String, dynamic>> createBoat(String content) async {
    return await _interactionService.createBoat(
      stoneId: stone.stoneId,
      content: content,
    );
  }

  Future<Map<String, dynamic>> getBoats(
      {int page = 1, int pageSize = 10}) async {
    return await _interactionService.getBoats(stone.stoneId,
        page: page, pageSize: pageSize);
  }

  Future<Map<String, dynamic>> createConnection() async {
    return await _interactionService.createConnectionByStone(stone.stoneId);
  }

  void updateCounts(int ripples, int boats) {
    localRipplesCount = ripples;
    localBoatsCount = boats;
    onStateChanged?.call();
  }

  void syncFromStone(Stone newStone) {
    if (stone.stoneId != newStone.stoneId ||
        stone.rippleCount != newStone.rippleCount ||
        stone.boatCount != newStone.boatCount ||
        stone.hasRippled != newStone.hasRippled) {
      localRipplesCount = newStone.rippleCount;
      localBoatsCount = newStone.boatCount;
      hasRippled = newStone.hasRippled;
      onStateChanged?.call();
    }
  }

  bool get isAuthor => currentUserId != null && currentUserId == stone.userId;
}
