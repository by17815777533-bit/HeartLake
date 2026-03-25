// WebSocket 管理器

import 'dart:async';
import 'dart:convert';
import 'dart:math' as math;
import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:flutter/foundation.dart';
import '../../utils/storage_util.dart';
import '../../utils/app_config.dart';
import '../../utils/payload_contract.dart';
import 'cache_service.dart';
import 'api_client.dart';

abstract class WebSocketClient {
  void on(String eventType, void Function(Map<String, dynamic>) listener);
  void off(String eventType, [void Function(Map<String, dynamic>)? listener]);
}

abstract class RealtimeClient extends WebSocketClient {
  Future<bool> connect();
  void disconnect();
}

abstract class RoomSubscriptionClient extends RealtimeClient {
  void joinRoom(String room);
  void leaveRoom(String room);
}

/// WebSocket 广播管理器（单例），负责实时消息推送
///
/// 核心机制：
/// - 握手阶段通过 URL query 传递 token 完成鉴权
/// - 房间采用引用计数，多页面订阅同一房间不会互相误退
/// - 断线后指数退避自动重连，离线期间消息暂存队列
/// - 心跳检测由后端驱动（30s ping），客户端回 pong 并监测半开连接
class WebSocketManager implements RoomSubscriptionClient {
  static final WebSocketManager _instance = WebSocketManager._internal();
  factory WebSocketManager() => _instance;
  WebSocketManager._internal();

  WebSocketChannel? _channel;
  final Map<String, List<void Function(Map<String, dynamic>)>> _listeners = {};
  final List<String> _offlineQueue = [];

  /// 房间引用计数，防止多页面订阅同一房间时互相误退
  final Map<String, int> _roomRefCounts = <String, int>{};
  Timer? _heartbeatTimer;
  Timer? _reconnectTimer;
  bool _isConnected = false;
  bool _shouldReconnect = true;
  DateTime? _lastPongTime;
  int _reconnectAttempts = 0;
  static const int _maxReconnectAttempts = 10;
  static const int _maxQueueSize = 50;

  /// 防止并发重连竞态的 Completer
  Completer<bool>? _connectCompleter;
  Completer<bool>? _authReadyCompleter;

  bool get isConnected => _isConnected;

  void _reportRealtimeError(
    Object error,
    StackTrace stackTrace,
    String context,
  ) {
    FlutterError.reportError(
      FlutterErrorDetails(
        exception: error,
        stack: stackTrace,
        library: 'heartlake',
        context: ErrorDescription(context),
      ),
    );
  }

  void _emitErrorEvent(
    String message, {
    String? code,
    Object? error,
  }) {
    _emit('error', {
      'type': 'error',
      'message': message,
      if (code != null) 'code': code,
      if (error != null) 'details': error.toString(),
    });
  }

  void _enqueueOfflineMessage(String message) {
    if (_offlineQueue.length >= _maxQueueSize) {
      _offlineQueue.removeAt(0);
    }
    _offlineQueue.add(message);
  }

  bool _sendRawMessage(
    String message, {
    bool queueOnFailure = false,
  }) {
    if (!_isConnected || _channel == null) {
      if (queueOnFailure) {
        _enqueueOfflineMessage(message);
        unawaited(connect());
      }
      return false;
    }

    try {
      _channel!.sink.add(message);
      return true;
    } catch (error, stackTrace) {
      _reportRealtimeError(
        error,
        stackTrace,
        'WebSocketManager._sendRawMessage',
      );
      if (queueOnFailure) {
        _enqueueOfflineMessage(message);
      }
      _emitErrorEvent(
        '实时消息发送失败，正在重连',
        code: 'send_failed',
        error: error,
      );
      _closeConnection(allowReconnect: true);
      _scheduleReconnect();
      return false;
    }
  }

  /// 校验 token 格式，兼容 PASETO v4 和 JWT，过滤损坏的缓存值
  String? _normalizeToken(String? raw) {
    if (raw == null) return null;
    final token = raw.trim();
    if (token.isEmpty) return null;
    // 兼容 PASETO v4 和 JWT 两种格式
    final looksLikePaseto =
        token.startsWith('v4.local.') || token.startsWith('v4.public.');
    final looksLikeJwt = token.startsWith('eyJ');
    if (!looksLikePaseto && !looksLikeJwt) return null;
    return token;
  }

  /// 连接 WebSocket
  /// 后端在握手阶段要求 URL query 带 token，否则会立即断开连接
  @override
  Future<bool> connect() async {
    if (_isConnected) return true;

    // 如果已有连接正在进行，等待其结果而不是发起新连接
    if (_connectCompleter != null && !_connectCompleter!.isCompleted) {
      return _connectCompleter!.future;
    }

    _connectCompleter = Completer<bool>();
    _authReadyCompleter = Completer<bool>();

    try {
      // 优先使用内存态 token，SecureStorage 在 Web 端可能读到旧值
      _shouldReconnect = true;
      var token = _normalizeToken(ApiClient().token);
      token ??= _normalizeToken(await StorageUtil.getToken());
      if (token == null || token.isEmpty) {
        // 登录态未就绪，延迟重连
        _scheduleReconnect();
        _connectCompleter!.complete(false);
        return false;
      }

      final baseUri = Uri.parse(appConfig.wsUrl);
      final uri = baseUri.replace(
        queryParameters: <String, String>{
          ...baseUri.queryParameters,
          'token': token,
        },
      );

      _channel = WebSocketChannel.connect(uri);
      _channel!.stream.listen(_onMessage,
          onError: _onError, onDone: _onDone, cancelOnError: false);

      final ready = await _authReadyCompleter!.future.timeout(
        const Duration(seconds: 6),
        onTimeout: () => false,
      );
      _connectCompleter!.complete(ready);
      if (!ready) {
        _closeConnection(allowReconnect: true);
        _scheduleReconnect();
      }
      return ready;
    } catch (e) {
      _reportRealtimeError(
        e,
        StackTrace.current,
        'WebSocketManager.connect',
      );
      if (_authReadyCompleter != null && !_authReadyCompleter!.isCompleted) {
        _authReadyCompleter!.complete(false);
      }
      _connectCompleter!.complete(false);
      _emitErrorEvent('实时连接失败，正在重试', code: 'connect_failed', error: e);
      _scheduleReconnect();
      return false;
    }
  }

  /// 断开连接
  @override
  void disconnect() {
    _closeConnection(allowReconnect: false);
  }

  void _closeConnection({required bool allowReconnect}) {
    _shouldReconnect = allowReconnect;
    _isConnected = false;
    _lastPongTime = null;
    _connectCompleter = null;
    if (_authReadyCompleter != null && !_authReadyCompleter!.isCompleted) {
      _authReadyCompleter!.complete(false);
    }
    _authReadyCompleter = null;
    _heartbeatTimer?.cancel();
    _reconnectTimer?.cancel();
    try {
      _channel?.sink.close();
    } catch (error, stackTrace) {
      _reportRealtimeError(
        error,
        stackTrace,
        'WebSocketManager._closeConnection',
      );
    }
    _channel = null;
  }

  /// 释放所有资源，包括连接、监听器和离线队列
  void dispose() {
    disconnect();
    _listeners.clear();
    _offlineQueue.clear();
    _roomRefCounts.clear();
  }

  /// 加入指定房间，首次订阅时发送 join 指令，后续仅增加引用计数
  @override
  void joinRoom(String room) {
    final normalizedRoom = room.trim();
    if (normalizedRoom.isEmpty) return;
    final current = _roomRefCounts[normalizedRoom] ?? 0;
    _roomRefCounts[normalizedRoom] = current + 1;
    if (!_isConnected) {
      unawaited(connect());
      return;
    }
    // 首次订阅时发送 join，后续仅增加引用计数
    if (current == 0) {
      send({'type': 'join', 'room': normalizedRoom});
    }
  }

  /// 离开指定房间，引用计数归零时才真正发送 leave 指令
  @override
  void leaveRoom(String room) {
    final normalizedRoom = room.trim();
    if (normalizedRoom.isEmpty) return;
    final current = _roomRefCounts[normalizedRoom];
    if (current == null) return;
    if (current > 1) {
      _roomRefCounts[normalizedRoom] = current - 1;
      return;
    }
    _roomRefCounts.remove(normalizedRoom);
    if (_isConnected) {
      send({'type': 'leave', 'room': normalizedRoom});
    }
  }

  /// 注册事件监听器
  @override
  void on(String eventType, void Function(Map<String, dynamic>) listener) {
    _listeners[eventType] ??= [];
    if (_listeners[eventType]!.contains(listener)) {
      return;
    }
    _listeners[eventType]!.add(listener);
  }

  /// 移除事件监听器，不传 listener 则移除该事件的全部监听
  @override
  void off(String eventType, [void Function(Map<String, dynamic>)? listener]) {
    if (listener == null) {
      _listeners.remove(eventType);
    } else {
      _listeners[eventType]?.remove(listener);
      if (_listeners[eventType]?.isEmpty == true) {
        _listeners.remove(eventType);
      }
    }
  }

  /// 触发事件回调，遍历前复制列表以避免并发修改
  void _emit(String eventType, Map<String, dynamic> data) {
    final callbacks = List<void Function(Map<String, dynamic>)>.from(
        _listeners[eventType] ?? []);
    for (final listener in callbacks) {
      try {
        listener(data);
      } catch (error, stackTrace) {
        _reportRealtimeError(
          error,
          stackTrace,
          'WebSocketManager._emit($eventType)',
        );
      }
    }
  }

  void _onMessage(dynamic message) {
    try {
      if (message is! String) return;
      final decoded = jsonDecode(message);
      if (decoded is! Map) return;
      final data = normalizePayloadContract(decoded);
      final type = data['type']?.toString();
      // 收到后端 ping 时回 pong 并更新时间戳
      if (type == 'ping') {
        _lastPongTime = DateTime.now();
        if (_channel != null) {
          _sendRawMessage(jsonEncode({'type': 'pong'}));
        }
        return;
      }
      if (type == 'pong') {
        _lastPongTime = DateTime.now();
        return;
      }
      if (type == 'auth_success') {
        _completeAuthReady(data);
        return;
      }
      if (type != null) _emit(type, data);
    } catch (error, stackTrace) {
      _reportRealtimeError(
        error,
        stackTrace,
        'WebSocketManager._onMessage',
      );
      _emitErrorEvent('实时消息解析失败', code: 'message_parse_failed', error: error);
    }
  }

  void _onError(error) {
    _isConnected = false;
    if (_authReadyCompleter != null && !_authReadyCompleter!.isCompleted) {
      _authReadyCompleter!.complete(false);
    }
    _heartbeatTimer?.cancel();
    _emit('error', {'type': 'error', 'message': error.toString()});
    if (_shouldReconnect) {
      _scheduleReconnect();
    }
  }

  void _onDone() {
    _isConnected = false;
    if (_authReadyCompleter != null && !_authReadyCompleter!.isCompleted) {
      _authReadyCompleter!.complete(false);
    }
    _heartbeatTimer?.cancel();
    _emit('disconnected', {'type': 'disconnected'});
    if (_shouldReconnect) {
      _scheduleReconnect();
    }
  }

  void _completeAuthReady(Map<String, dynamic> payload) {
    final wasReconnect = _reconnectAttempts > 0;
    _isConnected = true;
    _lastPongTime = DateTime.now();
    if (wasReconnect) {
      CacheService().removeByPrefix('stones_');
      CacheService().removeByPrefix('stone_');
      _emit('reconnected', {'type': 'reconnected', ...payload});
    }
    _reconnectAttempts = 0;
    _startHeartbeat();
    _rejoinRooms();
    _flushQueue();
    _emit('connected', {'type': 'connected', ...payload});
    if (_authReadyCompleter != null && !_authReadyCompleter!.isCompleted) {
      _authReadyCompleter!.complete(true);
    }
  }

  /// 发送消息，离线时暂存到队列（上限 50 条）
  void send(Map<String, dynamic> data) {
    final message = jsonEncode(data);
    _sendRawMessage(message, queueOnFailure: true);
  }

  /// 冲刷离线队列中暂存的消息
  void _flushQueue() {
    if (!_isConnected || _channel == null) return;
    while (_offlineQueue.isNotEmpty) {
      final message = _offlineQueue.first;
      final sent = _sendRawMessage(message, queueOnFailure: true);
      if (!sent) {
        break;
      }
      _offlineQueue.removeAt(0);
    }
  }

  /// 重连后重新加入之前订阅的所有房间
  void _rejoinRooms() {
    if (!_isConnected || _channel == null || _roomRefCounts.isEmpty) return;
    for (final room in _roomRefCounts.keys) {
      _sendRawMessage(jsonEncode({'type': 'join', 'room': room}));
    }
  }

  /// 公开重连方法：重置计数器并重新连接
  Future<bool> reconnect() async {
    _closeConnection(allowReconnect: true);
    _reconnectAttempts = 0;
    return connect();
  }

  void _scheduleReconnect() {
    if (!_shouldReconnect) return;
    if (_reconnectTimer?.isActive == true) return;
    _reconnectAttempts++;
    // 前10次指数退避，之后每60秒尝试一次
    final baseDelay = _reconnectAttempts <= _maxReconnectAttempts
        ? Duration(seconds: (1 << _reconnectAttempts).clamp(1, 30))
        : const Duration(seconds: 60);
    final jitterFactor = 0.75 + math.Random().nextDouble() * 0.5;
    final delay = Duration(
      milliseconds: (baseDelay.inMilliseconds * jitterFactor).round(),
    );
    _reconnectTimer = Timer(delay, () async {
      await connect();
    });
  }

  void _startHeartbeat() {
    _heartbeatTimer?.cancel();
    _lastPongTime = DateTime.now();
    // 不主动发 ping，由后端每 30 秒发 ping，Flutter 回 pong
    // 定时器仅检查是否超过 90 秒未收到后端 ping，判定半开连接
    _heartbeatTimer = Timer.periodic(const Duration(seconds: 30), (_) {
      if (_isConnected && _channel != null) {
        final now = DateTime.now();
        if (_lastPongTime != null &&
            now.difference(_lastPongTime!).inSeconds > 90) {
          _emitErrorEvent('实时连接心跳超时，正在重连', code: 'heartbeat_timeout');
          _isConnected = false;
          _heartbeatTimer?.cancel();
          try {
            _channel?.sink.close();
          } catch (error, stackTrace) {
            _reportRealtimeError(
              error,
              stackTrace,
              'WebSocketManager._startHeartbeat',
            );
          }
          _channel = null;
          _scheduleReconnect();
        }
      }
    });
  }
}
