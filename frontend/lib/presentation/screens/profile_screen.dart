// 个人资料界面
//
// 展示用户信息、统计数据和个人中心功能入口。

import 'dart:async';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:image_picker/image_picker.dart';
import 'package:provider/provider.dart';

import '../providers/user_provider.dart';
import '../../data/datasources/account_service.dart';
import '../../data/datasources/user_service.dart';
import '../../data/datasources/vip_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';
import '../../utils/storage_util.dart';
import '../widgets/atmospheric_background.dart';
import 'help_screen.dart';
import 'vip_screen.dart';
import 'privacy_settings_screen.dart';
import 'emotion_calendar_screen.dart';
import 'notification_screen.dart';
import 'guardian_screen.dart';
import 'safe_harbor_screen.dart';
import 'my_stones_screen.dart';
import 'my_boats_screen.dart';
import 'my_ripples_screen.dart';
import 'emotion_trends_screen.dart';
import 'emotion_heatmap_screen.dart';
import 'received_boats_screen.dart';
import 'consultation_screen.dart';

/// 个人中心页面
///
/// 用户的个人信息管理入口，包含：
/// - 头像编辑（支持相册选取和裁剪）
/// - 昵称/签名修改
/// - 我的石头/涟漪/纸船统计与列表入口
/// - 情绪日历、热力图、趋势分析入口
/// - 守护者、安全港、心理咨询入口
/// - 隐私设置、灯火计划、帮助中心
/// - 登出操作
class ProfileScreen extends StatefulWidget {
  const ProfileScreen({super.key});

  @override
  State<ProfileScreen> createState() => _ProfileScreenState();
}

/// 个人中心页面状态管理
///
/// 初始化时并行加载用户信息、统计数据、完整资料和灯火状态，
/// 通过 WebSocket 监听石头/纸船/涟漪的增删事件实时刷新统计。
class _ProfileScreenState extends State<ProfileScreen> {
  final UserService _userService = sl<UserService>();
  final AccountService _accountService = sl<AccountService>();
  final WebSocketManager _wsManager = WebSocketManager();
  Map<String, dynamic>? _stats;
  String? _username;
  String? _nickname;
  String? _avatarUrl;
  String? _bio;
  String? _statsErrorMessage;
  String? _profileErrorMessage;
  String? _vipErrorMessage;
  bool _hasLight = false; // 灯是否点亮
  int _vipDaysLeft = 0;
  String? _currentUserId;
  final ImagePicker _picker = ImagePicker();

  // WebSocket 监听器
  void Function(Map<String, dynamic>)? _newStoneListener;
  void Function(Map<String, dynamic>)? _boatUpdateListener;
  void Function(Map<String, dynamic>)? _rippleUpdateListener;
  void Function(Map<String, dynamic>)? _stoneDeletedListener;
  void Function(Map<String, dynamic>)? _boatDeletedListener;
  void Function(Map<String, dynamic>)? _rippleDeletedListener;
  void Function(Map<String, dynamic>)? _reconnectedListener;

  @override
  void initState() {
    super.initState();
    _loadUserInfo();
    _loadUserStats();
    _loadFullProfile();
    _loadVIPStatus();
    _setupWebSocketListeners();
  }

  final _vipService = sl<VIPService>();

  void _reportUiError(
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

  void _showMessage(String message) {
    final messenger = ScaffoldMessenger.maybeOf(context);
    if (messenger == null) return;
    messenger
      ..hideCurrentSnackBar()
      ..showSnackBar(SnackBar(content: Text(message)));
  }

  String _resolveMessage(
    Map<String, dynamic> result,
    String fallback,
  ) {
    final message = result['message']?.toString().trim();
    if (message != null && message.isNotEmpty) {
      return message;
    }
    return fallback;
  }

  String _extractErrorMessage(Object error, String fallback) {
    final raw = error
        .toString()
        .replaceFirst(RegExp(r'^Bad state:\s*'), '')
        .replaceFirst(RegExp(r'^Exception:\s*'), '');
    final message = raw.trim();
    return message.isEmpty ? fallback : message;
  }

  int? _extractIntField(String key) {
    final rawValue = _stats?[key];
    if (rawValue is int) return rawValue;
    if (rawValue is num) return rawValue.toInt();
    if (rawValue is String) return int.tryParse(rawValue.trim());
    return null;
  }

  String _statText(String key) {
    final value = _extractIntField(key);
    if (value == null) return '—';
    return value.toString();
  }

  bool _matchesCurrentUserId(dynamic value) {
    final currentUserId = _currentUserId?.trim();
    if (currentUserId == null || currentUserId.isEmpty) return false;
    return value?.toString().trim() == currentUserId;
  }

  bool _shouldRefreshStatsForEvent(
    Map<String, dynamic> payload,
  ) {
    final currentUserId = _currentUserId?.trim();
    if (currentUserId == null || currentUserId.isEmpty) {
      return false;
    }

    final normalized = normalizePayloadContract(payload);
    final stonePayload = normalized['stone'];
    final stone = stonePayload is Map
        ? normalizePayloadContract(stonePayload)
        : const <String, dynamic>{};
    final candidateIds = [
      normalized['stone_owner_id'],
      normalized['stone_user_id'],
      normalized['user_id'],
      normalized['sender_id'],
      normalized['from_user_id'],
      normalized['to_user_id'],
      normalized['target_user_id'],
      normalized['triggered_by'],
      stone['stone_owner_id'],
      stone['stone_user_id'],
      stone['user_id'],
      stone['author_id'],
    ];
    if (candidateIds.any(_matchesCurrentUserId)) {
      return true;
    }
    return false;
  }

  /// 查询灯火（VIP）状态：是否点亮、剩余天数
  Future<void> _loadVIPStatus({bool showFeedback = false}) async {
    try {
      final status = await _vipService.getVIPStatus();
      if (status['success'] != true) {
        final message = _resolveMessage(status, '加载灯火状态失败');
        _reportUiError(
          StateError(message),
          StackTrace.current,
          'ProfileScreen._loadVIPStatus',
        );
        if (showFeedback && mounted) {
          _showMessage(message);
        }
        if (mounted) {
          setState(() => _vipErrorMessage = message);
        }
        return;
      }

      final payload = (status['data'] is Map<String, dynamic>)
          ? status['data'] as Map<String, dynamic>
          : const <String, dynamic>{};
      if (mounted) {
        setState(() {
          _hasLight = payload['is_vip'] == true;
          _vipDaysLeft = (payload['days_left'] as num?)?.toInt() ?? 0;
          _vipErrorMessage = null;
        });
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'ProfileScreen._loadVIPStatus');
      if (mounted) {
        setState(() => _vipErrorMessage = '加载灯火状态失败，请稍后重试');
      }
      if (showFeedback && mounted) {
        _showMessage('加载灯火状态失败，请稍后重试');
      }
    }
  }

  /// 并行刷新统计、资料和灯火状态
  Future<void> _refreshAll() async {
    await Future.wait([
      _loadUserStats(),
      _loadFullProfile(showFeedback: true),
      _loadVIPStatus(showFeedback: true),
    ]);
  }

  /// 注册 WebSocket 事件监听器，实时更新统计数据
  ///
  /// 监听事件包括：新石头、纸船增删、涟漪增删、断线重连
  void _setupWebSocketListeners() async {
    final userProvider = Provider.of<UserProvider>(context, listen: false);
    try {
      await _wsManager.connect();
      if (!mounted) return;

      _currentUserId = userProvider.userId ?? await StorageUtil.getUserId();

      // 监听新石头（更新投石计数）
      _newStoneListener = (data) {
        if (!mounted) return;
        if (_shouldRefreshStatsForEvent(data)) {
          unawaited(_loadUserStats(silent: true));
        }
      };
      _wsManager.on('new_stone', _newStoneListener!);

      // 监听纸船更新
      _boatUpdateListener = (data) {
        if (!mounted) return;
        if (_shouldRefreshStatsForEvent(
          data,
        )) {
          unawaited(_loadUserStats(silent: true));
        }
      };
      _wsManager.on('boat_update', _boatUpdateListener!);

      // 监听涟漪更新
      _rippleUpdateListener = (data) {
        if (!mounted) return;
        if (_shouldRefreshStatsForEvent(
          data,
        )) {
          unawaited(_loadUserStats(silent: true));
        }
      };
      _wsManager.on('ripple_update', _rippleUpdateListener!);

      // 监听删除事件，刷新统计
      _stoneDeletedListener = (data) {
        if (!mounted) return;
        if (_shouldRefreshStatsForEvent(data)) {
          unawaited(_loadUserStats(silent: true));
        }
      };
      _wsManager.on('stone_deleted', _stoneDeletedListener!);

      _boatDeletedListener = (data) {
        if (!mounted) return;
        if (_shouldRefreshStatsForEvent(
          data,
        )) {
          unawaited(_loadUserStats(silent: true));
        }
      };
      _wsManager.on('boat_deleted', _boatDeletedListener!);

      _rippleDeletedListener = (data) {
        if (!mounted) return;
        if (_shouldRefreshStatsForEvent(
          data,
        )) {
          unawaited(_loadUserStats(silent: true));
        }
      };
      _wsManager.on('ripple_deleted', _rippleDeletedListener!);

      // 断线重连后自动刷新所有数据
      _reconnectedListener = (data) {
        if (!mounted) return;
        unawaited(_refreshAll());
      };
      _wsManager.on('reconnected', _reconnectedListener!);
    } catch (error, stackTrace) {
      _reportUiError(
        error,
        stackTrace,
        'ProfileScreen._setupWebSocketListeners',
      );
    }
  }

  /// 移除所有 WebSocket 监听器，防止内存泄漏
  void _removeWebSocketListeners() {
    if (_newStoneListener != null) {
      _wsManager.off('new_stone', _newStoneListener!);
    }
    if (_boatUpdateListener != null) {
      _wsManager.off('boat_update', _boatUpdateListener!);
    }
    if (_rippleUpdateListener != null) {
      _wsManager.off('ripple_update', _rippleUpdateListener!);
    }
    if (_stoneDeletedListener != null) {
      _wsManager.off('stone_deleted', _stoneDeletedListener!);
    }
    if (_boatDeletedListener != null) {
      _wsManager.off('boat_deleted', _boatDeletedListener!);
    }
    if (_rippleDeletedListener != null) {
      _wsManager.off('ripple_deleted', _rippleDeletedListener!);
    }
    if (_reconnectedListener != null) {
      _wsManager.off('reconnected', _reconnectedListener!);
    }
  }

  @override
  void dispose() {
    _removeWebSocketListeners();
    super.dispose();
  }

  /// 从本地存储读取用户名和昵称
  Future<void> _loadUserInfo({bool showFeedback = false}) async {
    try {
      final username = await StorageUtil.getUsername();
      final nickname = await StorageUtil.getNickname();
      if (mounted) {
        setState(() {
          _username = username;
          _nickname = nickname;
        });
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'ProfileScreen._loadUserInfo');
      if (showFeedback && mounted) {
        _showMessage('读取本地用户信息失败，请稍后重试');
      }
    }
  }

  /// 加载用户统计数据（投石数、收发纸船数、相伴天数）
  ///
  /// [silent] 为 true 时不显示加载状态，用于后台静默刷新
  Future<void> _loadUserStats({bool silent = false}) async {
    final userProvider = Provider.of<UserProvider>(context, listen: false);
    try {
      final token = await StorageUtil.getToken();
      final userId = userProvider.userId ?? await StorageUtil.getUserId();
      _currentUserId = userId;
      if (token == null || userId == null || userId.isEmpty) {
        _reportUiError(
          StateError('缺少有效登录态，无法加载个人统计'),
          StackTrace.current,
          'ProfileScreen._loadUserStats',
        );
        if (!silent && mounted) {
          _showMessage('登录状态异常，请重新进入页面');
        }
        if (mounted) {
          setState(() => _statsErrorMessage = '登录状态异常，无法刷新个人统计');
        }
        return;
      }

      final result = await _userService.getUserStats(userId);
      if (result['success'] != true) {
        final message = _resolveMessage(result, '加载个人统计失败');
        _reportUiError(
          StateError(message),
          StackTrace.current,
          'ProfileScreen._loadUserStats',
        );
        if (!silent && mounted) {
          _showMessage(message);
        }
        if (mounted) {
          setState(() => _statsErrorMessage = message);
        }
        return;
      }

      final payload = result['data'];
      if (payload is! Map) {
        _reportUiError(
          StateError('个人统计响应缺少 data 对象'),
          StackTrace.current,
          'ProfileScreen._loadUserStats',
        );
        if (!silent && mounted) {
          _showMessage('加载个人统计失败，请稍后重试');
        }
        if (mounted) {
          setState(() => _statsErrorMessage = '加载个人统计失败，请稍后重试');
        }
        return;
      }

      if (mounted) {
        setState(
          () {
            _stats = Map<String, dynamic>.from(payload.cast<String, dynamic>());
            _statsErrorMessage = null;
          },
        );
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'ProfileScreen._loadUserStats');
      if (mounted) {
        setState(() => _statsErrorMessage = '加载个人统计失败，请稍后重试');
      }
      if (!silent && mounted) {
        _showMessage('加载个人统计失败，请稍后重试');
      }
    }
  }

  /// 从后端拉取完整用户资料（头像、签名等），并同步昵称到本地存储
  Future<void> _loadFullProfile({bool showFeedback = false}) async {
    try {
      final result = await _accountService.getAccountInfo();
      final payload = result['data'];
      if (payload is! Map) {
        _reportUiError(
          StateError('个人资料响应缺少 data 对象'),
          StackTrace.current,
          'ProfileScreen._loadFullProfile',
        );
        if (showFeedback && mounted) {
          _showMessage('加载个人资料失败，请稍后重试');
        }
        if (mounted) {
          setState(() => _profileErrorMessage = '加载个人资料失败，请稍后重试');
        }
        return;
      }

      final userData =
          Map<String, dynamic>.from(payload.cast<String, dynamic>());
      final resolvedNickname = userData['nickname']?.toString().trim();
      final resolvedAvatarUrl = userData['avatar_url']?.toString();
      final resolvedBio = userData['bio']?.toString();
      if (resolvedNickname != null && resolvedNickname.isNotEmpty) {
        await StorageUtil.saveNickname(resolvedNickname);
      }

      if (!mounted) return;
      setState(() {
        _username = userData['username']?.toString() ?? _username;
        _avatarUrl =
            resolvedAvatarUrl?.isNotEmpty == true ? resolvedAvatarUrl : null;
        _bio = resolvedBio;
        _nickname =
            resolvedNickname?.isNotEmpty == true ? resolvedNickname : _nickname;
        _profileErrorMessage = null;
      });
      Provider.of<UserProvider>(context, listen: false).updateUser(
        nickname:
            resolvedNickname?.isNotEmpty == true ? resolvedNickname : null,
        avatarUrl: resolvedAvatarUrl,
        bio: resolvedBio,
      );
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'ProfileScreen._loadFullProfile');
      final message = _extractErrorMessage(error, '加载个人资料失败，请稍后重试');
      if (mounted) {
        setState(() => _profileErrorMessage = message);
      }
      if (showFeedback && mounted) {
        _showMessage(message);
      }
    }
  }

  /// 从相册选取图片并上传为头像，成功后同步到 UserProvider
  Future<void> _pickAndUploadAvatar() async {
    try {
      final XFile? image = await _picker.pickImage(
        source: ImageSource.gallery,
        maxWidth: 800,
        maxHeight: 800,
        imageQuality: 85,
      );

      if (image == null) return;

      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('正在上传头像...')),
      );

      final file = File(image.path);
      final fileSize = await file.length();
      final result = await _accountService.uploadAvatar(
        file,
        filename: file.path.split('/').last,
        fileSize: fileSize,
      );

      final payload = result['data'];
      if (payload is! Map) {
        _reportUiError(
          StateError('头像上传成功但响应缺少 data 对象'),
          StackTrace.current,
          'ProfileScreen._pickAndUploadAvatar',
        );
        if (mounted) {
          _showMessage('头像更新失败，请稍后重试');
        }
        return;
      }

      final data = Map<String, dynamic>.from(payload.cast<String, dynamic>());
      final avatarUrl =
          data['avatar_url']?.toString() ?? data['avatarUrl']?.toString();
      if (avatarUrl == null || avatarUrl.isEmpty) {
        final message = _resolveMessage(result, '头像上传成功但未返回头像地址');
        _reportUiError(
          StateError(message),
          StackTrace.current,
          'ProfileScreen._pickAndUploadAvatar',
        );
        if (mounted) {
          _showMessage(message);
        }
        return;
      }

      if (mounted) {
        setState(() {
          _avatarUrl = avatarUrl;
        });
        Provider.of<UserProvider>(context, listen: false)
            .updateUser(avatarUrl: avatarUrl);
        _showMessage('头像更新成功');
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'ProfileScreen._pickAndUploadAvatar');
      final message = _extractErrorMessage(error, '选择或上传图片失败，请稍后重试');
      if (mounted) {
        _showMessage(message);
      }
    }
  }

  /// 弹出个性签名编辑弹窗，保存后同步到后端和 UserProvider
  void _showEditBioDialog() {
    final controller = TextEditingController(text: _bio ?? '');
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('编辑个性签名'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(
            hintText: '写下你的心情...',
            border: OutlineInputBorder(),
          ),
          maxLines: 3,
          maxLength: 100,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('取消'),
          ),
          FilledButton(
            onPressed: () async {
              final newBio = controller.text.trim();
              Navigator.pop(context);

              if (newBio == _bio) return;

              final messenger = ScaffoldMessenger.of(context);
              final userProvider =
                  Provider.of<UserProvider>(context, listen: false);
              final success = await userProvider.updateProfile(bio: newBio);

              if (mounted) {
                if (success) {
                  setState(() {
                    _bio = userProvider.user?.bio;
                  });
                  messenger.showSnackBar(
                    const SnackBar(content: Text('签名已更新')),
                  );
                } else {
                  messenger.showSnackBar(
                    SnackBar(
                      content: Text(userProvider.errorMessage ?? '更新失败，请稍后重试'),
                    ),
                  );
                }
              }
            },
            child: const Text('保存'),
          ),
        ],
      ),
    ).then((_) => controller.dispose());
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final hasAvatar = _avatarUrl?.trim().isNotEmpty == true;
    final stonesCount = _statText('stones_count');
    final boatsReceived = _statText('boats_received');
    final boatsSent = _statText('boats_sent');
    final joinDaysValue = _extractIntField('join_days');
    final joinDays = joinDaysValue == null ? '—' : '$joinDaysValue天';
    final displayName = (_nickname?.trim().isNotEmpty == true)
        ? _nickname!
        : (_username?.trim().isNotEmpty == true)
            ? _username!
            : (_profileErrorMessage != null ? '资料未加载' : '心湖用户');
    final bioText =
        _bio ?? (_profileErrorMessage != null ? '个人资料暂未刷新' : '点击添加个性签名...');

    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        title: const Text('倒影',
            style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        centerTitle: true,
        backgroundColor: Colors.transparent,
        elevation: 0,
        scrolledUnderElevation: 0,
        foregroundColor: Colors.white,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _refreshAll,
          ),
        ],
      ),
      body: Stack(
        children: [
          // 倒影背景
          const Positioned.fill(
            child: SceneTransitionBackground(
              scene: LakeScene.reflection,
              child: SizedBox.expand(),
            ),
          ),

          // 内容区域
          RefreshIndicator(
            onRefresh: _refreshAll,
            color: AppTheme.lakeDeep,
            backgroundColor: isDark ? AppTheme.nightSurface : Colors.white,
            child: ListView(
              padding: EdgeInsets.only(
                top: MediaQuery.of(context).padding.top + kToolbarHeight + 16,
                bottom: 20,
                left: 16,
                right: 16,
              ),
              children: [
                if (_statsErrorMessage != null ||
                    _profileErrorMessage != null ||
                    _vipErrorMessage != null)
                  Card(
                    color: const Color(0xFFFFF4E5),
                    child: Padding(
                      padding: const EdgeInsets.all(14),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          const Row(
                            children: [
                              Icon(Icons.warning_amber_rounded,
                                  color: Color(0xFFB26A00)),
                              SizedBox(width: 8),
                              Text(
                                '部分资料未刷新',
                                style: TextStyle(
                                  fontWeight: FontWeight.w600,
                                  color: Color(0xFF7A4A00),
                                ),
                              ),
                            ],
                          ),
                          const SizedBox(height: 8),
                          if (_profileErrorMessage != null)
                            Text(_profileErrorMessage!,
                                style:
                                    const TextStyle(color: Color(0xFF7A4A00))),
                          if (_statsErrorMessage != null)
                            Text(_statsErrorMessage!,
                                style:
                                    const TextStyle(color: Color(0xFF7A4A00))),
                          if (_vipErrorMessage != null)
                            Text(_vipErrorMessage!,
                                style:
                                    const TextStyle(color: Color(0xFF7A4A00))),
                        ],
                      ),
                    ),
                  ),
                if (_statsErrorMessage != null ||
                    _profileErrorMessage != null ||
                    _vipErrorMessage != null)
                  const SizedBox(height: 12),
                // 个人信息卡片
                Card(
                  color: isDark
                      ? AppTheme.nightSurface.withValues(alpha: 0.9)
                      : Colors.white.withValues(alpha: 0.9),
                  child: Padding(
                    padding: const EdgeInsets.all(20),
                    child: Column(
                      children: [
                        // 头像
                        GestureDetector(
                          onTap: _pickAndUploadAvatar,
                          child: Stack(
                            children: [
                              CircleAvatar(
                                radius: 45,
                                backgroundColor:
                                    AppTheme.skyBlue.withValues(alpha: 0.3),
                                backgroundImage: hasAvatar
                                    ? NetworkImage(_avatarUrl!)
                                    : null,
                                child: !hasAvatar
                                    ? const Icon(Icons.person,
                                        size: 45, color: Colors.white)
                                    : null,
                              ),
                              Positioned(
                                right: 0,
                                bottom: 0,
                                child: Container(
                                  padding: const EdgeInsets.all(4),
                                  decoration: const BoxDecoration(
                                    color: AppTheme.skyBlue,
                                    shape: BoxShape.circle,
                                  ),
                                  child: const Icon(Icons.camera_alt,
                                      size: 16, color: Colors.white),
                                ),
                              ),
                            ],
                          ),
                        ),
                        const SizedBox(height: 12),
                        // 昵称
                        GestureDetector(
                          onTap: _showEditNicknameDialog,
                          child: Row(
                            mainAxisSize: MainAxisSize.min,
                            children: [
                              Text(
                                displayName,
                                style: const TextStyle(
                                    fontSize: 20, fontWeight: FontWeight.bold),
                              ),
                              const SizedBox(width: 4),
                              const Icon(Icons.edit,
                                  size: 16, color: AppTheme.textTertiary),
                            ],
                          ),
                        ),
                        const SizedBox(height: 8),
                        // 签名
                        GestureDetector(
                          onTap: _showEditBioDialog,
                          child: Text(
                            bioText,
                            style: TextStyle(
                              color: _bio != null
                                  ? AppTheme.textSecondary
                                  : AppTheme.textTertiary,
                              fontStyle: _bio == null
                                  ? FontStyle.italic
                                  : FontStyle.normal,
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
                const SizedBox(height: 12),

                // 统计数据
                Row(
                  children: [
                    Expanded(
                        child: _buildStatCard(
                            context, '投石', stonesCount, Icons.water_drop,
                            onTap: () => Navigator.push(
                                context,
                                MaterialPageRoute(
                                    builder: (_) => const MyStonesScreen())))),
                    Expanded(
                        child: _buildStatCard(
                            context, '收到纸船', boatsReceived, Icons.sailing,
                            onTap: () => Navigator.push(
                                context,
                                MaterialPageRoute(
                                    builder: (_) =>
                                        const ReceivedBoatsScreen())))),
                  ],
                ),
                const SizedBox(height: 8),
                Row(
                  children: [
                    Expanded(
                        child: _buildStatCard(
                            context, '发送纸船', boatsSent, Icons.send,
                            onTap: () => Navigator.push(
                                context,
                                MaterialPageRoute(
                                    builder: (_) => const MyBoatsScreen())))),
                    Expanded(
                        child: _buildStatCard(
                            context, '相伴', '$joinDays天', Icons.favorite)),
                  ],
                ),
                const SizedBox(height: 12),

                // 我的心湖
                Card(
                  child: Column(
                    children: [
                      ListTile(
                        leading: Icon(
                            _hasLight
                                ? Icons.lightbulb
                                : Icons.lightbulb_outline,
                            color: _hasLight
                                ? const Color(0xFFFFD54F)
                                : Colors.grey),
                        title: Text(
                          _vipErrorMessage != null
                              ? '灯火状态未刷新'
                              : (_hasLight ? '灯已点亮' : '灯'),
                        ),
                        subtitle: Text(
                            _vipErrorMessage != null
                                ? _vipErrorMessage!
                                : (_hasLight
                                    ? '灯火将燃$_vipDaysLeft天'
                                    : '温暖时刻自动点亮'),
                            style: const TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => const VIPScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.calendar_month,
                            color: AppTheme.skyBlue),
                        title: const Text('情绪日历'),
                        subtitle: const Text('记录每日心情变化',
                            style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => const EmotionCalendarScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.show_chart,
                            color: AppTheme.skyBlue),
                        title: const Text('情绪趋势'),
                        subtitle: const Text('查看情绪变化轨迹',
                            style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => const EmotionTrendsScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.grid_view_rounded,
                            color: AppTheme.skyBlue),
                        title: const Text('情绪热力图'),
                        subtitle: const Text('查看情绪密度与关怀提示',
                            style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => const EmotionHeatmapScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading:
                            const Icon(Icons.water, color: AppTheme.skyBlue),
                        title: const Text('我的涟漪'),
                        subtitle: const Text('查看发出的涟漪',
                            style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => const MyRipplesScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.lightbulb_outline,
                            color: Color(0xFFFFB74D)),
                        title: const Text('守护者'),
                        subtitle: const Text('守护心湖的温暖与灯火',
                            style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => const GuardianScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.spa,
                            color: AppTheme.secondaryColor),
                        title: const Text('安全港湾'),
                        subtitle: const Text('心理支持与自助资源',
                            style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => const SafeHarborScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.psychology,
                            color: AppTheme.skyBlue),
                        title: const Text('心理咨询'),
                        subtitle: const Text('预约专业咨询师',
                            style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => const ConsultationScreen())),
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 12),

                // 设置
                Card(
                  child: Column(
                    children: [
                      ListTile(
                        leading: const Icon(Icons.notifications_outlined,
                            color: AppTheme.skyBlue),
                        title: const Text('消息通知'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => const NotificationScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.shield_outlined,
                            color: AppTheme.skyBlue),
                        title: const Text('隐私与安全'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => const PrivacySettingsScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.help_outline,
                            color: AppTheme.skyBlue),
                        title: const Text('帮助与反馈'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => const HelpScreen())),
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 12),

                // 账号
                Card(
                  child: Column(
                    children: [
                      ListTile(
                        leading: const Icon(Icons.logout,
                            color: AppTheme.errorColor),
                        title: const Text('退出登录',
                            style: TextStyle(color: AppTheme.errorColor)),
                        trailing: const Icon(Icons.chevron_right,
                            color: AppTheme.errorColor),
                        onTap: () async {
                          final confirm = await showDialog<bool>(
                            context: context,
                            builder: (context) => AlertDialog(
                              title: const Text('退出登录'),
                              content: const Text('确定要退出当前账号吗？'),
                              actions: [
                                TextButton(
                                  onPressed: () =>
                                      Navigator.pop(context, false),
                                  child: const Text('取消'),
                                ),
                                TextButton(
                                  onPressed: () => Navigator.pop(context, true),
                                  child: const Text(
                                    '确定',
                                    style:
                                        TextStyle(color: AppTheme.errorColor),
                                  ),
                                ),
                              ],
                            ),
                          );

                          if (confirm == true && context.mounted) {
                            // 先移除 WebSocket 监听器
                            _removeWebSocketListeners();

                            // 断开 WebSocket
                            _wsManager.disconnect();

                            // 清除用户状态与本地凭证
                            if (context.mounted) {
                              await Provider.of<UserProvider>(context,
                                      listen: false)
                                  .logout();
                            }

                            // 跳转到登录页面
                            if (context.mounted) {
                              context.go('/auth');
                            }
                          }
                        },
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  /// 构建统计数据卡片，可选点击跳转到对应列表页
  Widget _buildStatCard(
      BuildContext context, String label, String value, IconData icon,
      {VoidCallback? onTap}) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final card = Card(
      color: isDark
          ? const Color(0xFF1B2838).withValues(alpha: 0.9)
          : Colors.white.withValues(alpha: 0.9),
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(16),
        side: BorderSide(
          color: AppTheme.skyBlue.withValues(alpha: 0.3),
          width: 2,
        ),
      ),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            Icon(
              icon,
              color: AppTheme.skyBlue,
              size: 28,
            ),
            const SizedBox(height: 8),
            Text(
              value,
              style: Theme.of(context).textTheme.displayMedium?.copyWith(
                    color:
                        isDark ? const Color(0xFFE8EAED) : AppTheme.textPrimary,
                    fontWeight: FontWeight.bold,
                    fontSize: 20,
                  ),
            ),
            const SizedBox(height: 4),
            Text(
              label,
              style: Theme.of(context).textTheme.bodyMedium,
            ),
          ],
        ),
      ),
    );

    if (onTap != null) {
      return InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(16),
        child: card,
      );
    }
    return card;
  }

  /// 弹出昵称修改弹窗，保存后同步到后端、本地存储和 UserProvider
  void _showEditNicknameDialog() {
    final controller = TextEditingController(text: _nickname ?? '');

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('修改昵称'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(
            labelText: '新昵称',
            hintText: '请输入新昵称',
          ),
          maxLength: 20,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('取消'),
          ),
          FilledButton(
            onPressed: () async {
              final newNickname = controller.text.trim();
              final messenger = ScaffoldMessenger.of(context);
              final navigator = Navigator.of(context);

              if (newNickname.isEmpty) {
                messenger.showSnackBar(
                  const SnackBar(content: Text('昵称不能为空')),
                );
                return;
              }

              navigator.pop();

              // 调用更新昵称API
              final userProvider =
                  Provider.of<UserProvider>(context, listen: false);
              final success = await userProvider.updateNickname(newNickname);

              if (!mounted) return;

              if (success) {
                final savedNickname =
                    userProvider.nickname?.trim().isNotEmpty == true
                        ? userProvider.nickname!.trim()
                        : newNickname;
                if (mounted) {
                  setState(() {
                    _nickname = savedNickname;
                  });
                  if (mounted) {
                    messenger.showSnackBar(
                      const SnackBar(content: Text('昵称修改成功')),
                    );
                  }
                }
              } else {
                messenger.showSnackBar(
                  SnackBar(
                    content: Text(userProvider.errorMessage ?? '修改失败，请稍后重试'),
                  ),
                );
              }
            },
            child: const Text('确定'),
          ),
        ],
      ),
    ).then((_) => controller.dispose());
  }
}
