// @file websocket_manager.dart
// @brief WebSocket广播管理器 - 仅用于实时通知

import 'dart:async';
import 'dart:convert';
import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:flutter/foundation.dart';
import '../../utils/storage_util.dart';
import '../../utils/app_config.dart';

class WebSocketManager {
  static final WebSocketManager _instance = WebSocketManager._internal();
  factory WebSocketManager() => _instance;
  WebSocketManager._internal();

  WebSocketChannel? _channel;
  final Map<String, List<Function(Map<String, dynamic>)>> _listeners = {};
  final List<String> _offlineQueue = [];
  Timer? _heartbeatTimer;
  Timer? _reconnectTimer;
  bool _isConnected = false;
  int _reconnectAttempts = 0;
  static const int _maxReconnectAttempts = 10;
  static const int _maxQueueSize = 50;

  /// 用 Completer 防止并发重连竞态
  Completer<bool>? _connectCompleter;

  bool get isConnected => _isConnected;

  /// 连接 WebSocket
  /// P0-1 修复：不在 URL 中传输 token，改为连接后通过首条消息认证
  Future<bool> connect() async {
    if (_isConnected) return true;

    // 如果已有连接正在进行，等待其结果而不是发起新连接
    if (_connectCompleter != null && !_connectCompleter!.isCompleted) {
      return _connectCompleter!.future;
    }

    _connectCompleter = Completer<bool>();

    try {
      final token = await StorageUtil.getToken();
      if (token == null) {
        _connectCompleter!.complete(false);
        return false;
      }

      final baseUrl = appConfig.apiBaseUrl.replaceFirst('/api', '');
      final wsUrl = baseUrl.replaceFirst('http://', 'ws://').replaceFirst('https://', 'wss://');
      // 通过 URL 参数传递 token 进行认证（后端在 handleNewConnection 中验证）
      final uri = Uri.parse('$wsUrl/ws/broadcast?token=$token');

      _channel = WebSocketChannel.connect(uri);
      _channel!.stream.listen(_onMessage, onError: _onError, onDone: _onDone, cancelOnError: false);

      // 标记为已连接（WebSocket 通道已建立）
      _isConnected = true;
      _reconnectAttempts = 0;
      _startHeartbeat();
      _flushQueue();
      _emit('connected', {'type': 'connected'});
      _connectCompleter!.complete(true);
      return true;
    } catch (e) {
      if (kDebugMode) { debugPrint('WebSocket连接失败: $e'); }
      _connectCompleter!.complete(false);
      _scheduleReconnect();
      return false;
    }
  }

  /// 断开连接
  void disconnect() {
    _isConnected = false;
    _connectCompleter = null;
    _heartbeatTimer?.cancel();
    _reconnectTimer?.cancel();
    _channel?.sink.close();
    _channel = null;
  }

  /// P0-2 修复：添加 dispose 方法，清理所有资源
  void dispose() {
    disconnect();
    _listeners.clear();
    _offlineQueue.clear();
  }

  void on(String eventType, Function(Map<String, dynamic>) listener) {
    _listeners[eventType] ??= [];
    _listeners[eventType]!.add(listener);
  }

  void off(String eventType, [Function(Map<String, dynamic>)? listener]) {
    if (listener == null) {
      _listeners.remove(eventType);
    } else {
      _listeners[eventType]?.remove(listener);
    }
  }

  /// P0-3 修复：遍历前复制列表，避免并发修改错误
  void _emit(String eventType, Map<String, dynamic> data) {
    final callbacks = List<Function(Map<String, dynamic>)>.from(_listeners[eventType] ?? []);
    for (final listener in callbacks) {
      try {
        listener(data);
      } catch (e) {
        if (kDebugMode) { debugPrint('事件处理错误: $e'); }
      }
    }
  }

  void _onMessage(dynamic message) {
    try {
      final data = jsonDecode(message as String) as Map<String, dynamic>;
      final type = data['type'] as String?;
      if (type != null) _emit(type, data);
    } catch (e) {
      if (kDebugMode) { debugPrint('消息解析失败: $e'); }
    }
  }

  void _onError(error) {
    _isConnected = false;
    _emit('error', {'type': 'error', 'message': error.toString()});
    _scheduleReconnect();
  }

  void _onDone() {
    _isConnected = false;
    _heartbeatTimer?.cancel();
    _emit('disconnected', {'type': 'disconnected'});
    _scheduleReconnect();
  }

  void send(Map<String, dynamic> data) {
    final message = jsonEncode(data);
    if (_isConnected && _channel != null) {
      _channel!.sink.add(message);
    } else if (_offlineQueue.length < _maxQueueSize) {
      _offlineQueue.add(message);
    }
  }

  void _flushQueue() {
    if (!_isConnected || _channel == null) return;
    while (_offlineQueue.isNotEmpty) {
      _channel!.sink.add(_offlineQueue.removeAt(0));
    }
  }

  void _scheduleReconnect() {
    if (_reconnectAttempts >= _maxReconnectAttempts) return;
    // 如果已有重连定时器在等待，不重复调度
    if (_reconnectTimer?.isActive == true) return;
    _reconnectAttempts++;
    final delay = Duration(seconds: (1 << _reconnectAttempts).clamp(1, 30));
    _reconnectTimer = Timer(delay, () async {
      await connect();
    });
  }

  void _startHeartbeat() {
    _heartbeatTimer?.cancel();
    _heartbeatTimer = Timer.periodic(const Duration(seconds: 30), (_) {
      if (_isConnected && _channel != null) {
        try {
          _channel!.sink.add(jsonEncode({'type': 'ping'}));
        } catch (e) {
          if (kDebugMode) { debugPrint('心跳发送失败: $e'); }
          _isConnected = false;
          _scheduleReconnect();
        }
      }
    });
  }
}
