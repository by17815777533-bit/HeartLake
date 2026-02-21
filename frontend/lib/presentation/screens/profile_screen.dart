// @file profile_screen.dart
// @brief 个人资料界面
// Created by 林子怡

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/user_provider.dart';
import '../../utils/app_theme.dart';
import '../../utils/storage_util.dart';
import '../../data/datasources/api_client.dart';
import '../../data/datasources/auth_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../data/datasources/vip_service.dart';
import '../widgets/sky_scaffold.dart';
import 'package:image_picker/image_picker.dart';
import 'dart:io';
import 'dart:async';
import '../../data/datasources/media_service.dart';
import 'auth_screen.dart';
import 'help_screen.dart';
import 'vip_screen.dart';
import 'emotion_calendar_screen.dart';
import 'notification_screen.dart';
import 'guardian_screen.dart';
import 'my_stones_screen.dart';
import 'my_boats_screen.dart';
import 'received_boats_screen.dart';

class ProfileScreen extends StatefulWidget {
  const ProfileScreen({super.key});

  @override
  State<ProfileScreen> createState() => _ProfileScreenState();
}

class _ProfileScreenState extends State<ProfileScreen> {
  final ApiClient _apiClient = ApiClient();
  final WebSocketManager _wsManager = WebSocketManager();
  Map<String, dynamic>? _stats;
  String? _username;
  String? _nickname;
  String? _avatarUrl;
  String? _bio;
  bool _hasLight = false; // 灯是否点亮
  int _vipDaysLeft = 0;
  final ImagePicker _picker = ImagePicker();
  Timer? _statsDebounceTimer;

  void _debouncedLoadStats() {
    _statsDebounceTimer?.cancel();
    _statsDebounceTimer = Timer(const Duration(milliseconds: 500), () {
      if (mounted) _loadUserStats(silent: true);
    });
  }

  // WebSocket 监听器
  late void Function(Map<String, dynamic>) _newStoneListener;
  late void Function(Map<String, dynamic>) _boatUpdateListener;
  late void Function(Map<String, dynamic>) _rippleUpdateListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;
  late void Function(Map<String, dynamic>) _boatDeletedListener;
  late void Function(Map<String, dynamic>) _rippleDeletedListener;

  @override
  void initState() {
    super.initState();
    _loadUserInfo();
    _loadUserStats();
    _loadFullProfile();
    _loadVIPStatus();
    _setupWebSocketListeners();
  }

  final _vipService = VIPService();

  Future<void> _loadVIPStatus() async {
    try {
      final status = await _vipService.getVIPStatus();
      if (mounted) {
        setState(() {
          _hasLight = status['is_vip'] ?? false;
          _vipDaysLeft = status['days_left'] ?? 0;
        });
      }
    } catch (e) {
      if (kDebugMode) { debugPrint('Error loading VIP status: $e'); }
    }
  }

  Future<void> _refreshAll() async {
    await Future.wait([_loadUserStats(), _loadFullProfile(), _loadVIPStatus()]);
  }

  void _setupWebSocketListeners() async {
    await _wsManager.connect();

    // 监听新石头（更新投石计数）
    _newStoneListener = (data) async {
      final userId = await StorageUtil.getUserId();
      final stoneUserId = data['stone']?['user_id'] ?? data['user_id'];
      if (stoneUserId == userId && mounted) {
        _debouncedLoadStats();
      }
    };
    _wsManager.on('new_stone', _newStoneListener);

    // 监听纸船更新
    _boatUpdateListener = (data) async {
      // 当收到纸船时刷新统计
      if (mounted) {
        _debouncedLoadStats();
      }
    };
    _wsManager.on('boat_update', _boatUpdateListener);
    _wsManager.on('new_boat', _boatUpdateListener);

    // 监听涟漪更新
    _rippleUpdateListener = (data) async {
      if (mounted) {
        _debouncedLoadStats();
      }
    };
    _wsManager.on('ripple_update', _rippleUpdateListener);
    _wsManager.on('new_ripple', _rippleUpdateListener);

    // 监听删除事件，刷新统计
    _stoneDeletedListener = (data) {
      if (mounted) _debouncedLoadStats();
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener);

    _boatDeletedListener = (data) {
      if (mounted) _debouncedLoadStats();
    };
    _wsManager.on('boat_deleted', _boatDeletedListener);

    _rippleDeletedListener = (data) {
      if (mounted) _debouncedLoadStats();
    };
    _wsManager.on('ripple_deleted', _rippleDeletedListener);
  }

  void _removeWebSocketListeners() {
    _wsManager.off('new_stone', _newStoneListener);
    _wsManager.off('boat_update', _boatUpdateListener);
    _wsManager.off('new_boat', _boatUpdateListener);
    _wsManager.off('ripple_update', _rippleUpdateListener);
    _wsManager.off('new_ripple', _rippleUpdateListener);
    _wsManager.off('stone_deleted', _stoneDeletedListener);
    _wsManager.off('boat_deleted', _boatDeletedListener);
    _wsManager.off('ripple_deleted', _rippleDeletedListener);
  }

  @override
  void dispose() {
    _statsDebounceTimer?.cancel();
    _removeWebSocketListeners();
    super.dispose();
  }

  Future<void> _loadUserInfo() async {
    try {
      final username = await StorageUtil.getUsername();
      final nickname = await StorageUtil.getNickname();
      if (mounted) {
        setState(() {
          _username = username;
          _nickname = nickname;
        });
      }
    } catch (e) {
      if (kDebugMode) { debugPrint('Error loading user info: $e'); }
    }
  }

  Future<void> _loadUserStats({bool silent = false}) async {
    try {
      final token = await StorageUtil.getToken();
      final userId = await StorageUtil.getUserId();
      if (token != null && userId != null) {
        final response = await _apiClient.get('/users/$userId/stats');
        if (response.statusCode == 200 && response.data['code'] == 0 && mounted) {
          setState(() => _stats = response.data['data']);
        }
      }
    } catch (e) {
      if (kDebugMode) { debugPrint('Error loading user stats: $e'); }
    }
  }

  Future<void> _loadFullProfile() async {
    try {
      final userId = await StorageUtil.getUserId();
      if (userId == null) return;

      final response = await _apiClient.get('/users/$userId');
      if (response.statusCode == 200 && response.data['code'] == 0) {
        final userData = response.data['data'];
        if (mounted) {
          setState(() {
            _avatarUrl = userData['avatar_url'];
            _bio = userData['bio'];
            _nickname = userData['nickname'];
            // 如果本地也有更新，保存一下
            if (userData['nickname'] != null) {
              StorageUtil.saveNickname(userData['nickname']);
            }
          });
        }
      }
    } catch (e) {
      if (kDebugMode) { debugPrint('Error loading full profile: $e'); }
    }
  }

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
      final mediaService = MediaService();
      final result = await mediaService.uploadMedia(file);

      if (result['success']) {
        final avatarUrl = result['data']['url'];

        // 更新个人资料
        final authService = AuthService();
        final updateResult = await authService.updateProfile(avatarUrl: avatarUrl);

        if (mounted) {
          if (updateResult['success']) {
            setState(() {
              _avatarUrl = avatarUrl;
            });
            // 同步到 UserProvider
            Provider.of<UserProvider>(context, listen: false).updateUser(avatarUrl: avatarUrl);
            ScaffoldMessenger.of(context).showSnackBar(
              const SnackBar(content: Text('头像更新成功')),
            );
          } else {
            ScaffoldMessenger.of(context).showSnackBar(
              SnackBar(content: Text(updateResult['message'] ?? '更新失败')),
            );
          }
        }
      } else {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text(result['message'] ?? '上传失败')),
          );
        }
      }
    } catch (e) {
      if (kDebugMode) { debugPrint('Error picking avatar: $e'); }
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('选择图片失败')),
        );
      }
    }
  }

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
              final userProvider = Provider.of<UserProvider>(context, listen: false);
              final authService = AuthService();
              final result = await authService.updateProfile(bio: newBio);

              if (mounted) {
                if (result['success']) {
                  setState(() {
                    _bio = newBio;
                  });
                  // 同步到 UserProvider
                  userProvider.updateUser(bio: newBio);
                  messenger.showSnackBar(
                    const SnackBar(content: Text('签名已更新')),
                  );
                } else {
                  messenger.showSnackBar(
                    SnackBar(content: Text(result['message'] ?? '更新失败')),
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
    final stonesCount = _stats?['stones_count']?.toString() ?? '0';
    final boatsReceived = _stats?['boats_received']?.toString() ?? '0';
    final boatsSent = _stats?['boats_sent']?.toString() ?? '0';
    final joinDays = (_stats?['join_days']?.toString() ?? '0');

    return SkyScaffold(
      showParticles: true,
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
      body: SafeArea(
        child: RefreshIndicator(
          onRefresh: _refreshAll,
          color: Colors.blue[900],
          backgroundColor: Colors.white,
          child: ListView(
            padding: const EdgeInsets.only(
              top: 16,
              bottom: 20,
              left: 16,
              right: 16,
            ),
            children: [
                // 个人信息卡片
                Card(
                  color: Colors.white.withValues(alpha: 0.9),
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
                                backgroundColor: AppTheme.skyBlue.withValues(alpha: 0.3),
                                backgroundImage: (_avatarUrl != null && _avatarUrl!.isNotEmpty) ? NetworkImage(_avatarUrl!) : null,
                                child: _avatarUrl == null
                                    ? const Icon(Icons.person, size: 45, color: Colors.white)
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
                                  child: const Icon(Icons.camera_alt, size: 16, color: Colors.white),
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
                                _nickname ?? _username ?? '心湖用户',
                                style: const TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                              ),
                              const SizedBox(width: 4),
                              const Icon(Icons.edit, size: 16, color: Colors.grey),
                            ],
                          ),
                        ),
                        const SizedBox(height: 8),
                        // 签名
                        GestureDetector(
                          onTap: _showEditBioDialog,
                          child: Text(
                            _bio ?? '点击添加个性签名...',
                            style: TextStyle(
                              color: _bio != null ? Colors.grey[700] : Colors.grey,
                              fontStyle: _bio == null ? FontStyle.italic : FontStyle.normal,
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
                    Expanded(child: _buildStatCard(context, '投石', stonesCount, Icons.water_drop,
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const MyStonesScreen())))),
                    Expanded(child: _buildStatCard(context, '收到纸船', boatsReceived, Icons.sailing,
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const ReceivedBoatsScreen())))),
                  ],
                ),
                const SizedBox(height: 8),
                Row(
                  children: [
                    Expanded(child: _buildStatCard(context, '发送纸船', boatsSent, Icons.send,
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const MyBoatsScreen())))),
                    Expanded(child: _buildStatCard(context, '相伴', '$joinDays天', Icons.favorite)),
                  ],
                ),
                const SizedBox(height: 12),

                // 我的心湖
                Card(
                  child: Column(
                    children: [
                      ListTile(
                        leading: Icon(_hasLight ? Icons.lightbulb : Icons.lightbulb_outline,
                            color: _hasLight ? const Color(0xFFFFD54F) : Colors.grey),
                        title: Text(_hasLight ? '灯已点亮' : '灯'),
                        subtitle: Text(_hasLight ? '灯火将燃$_vipDaysLeft天' : '温暖时刻自动点亮',
                            style: const TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const VIPScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.calendar_month, color: AppTheme.skyBlue),
                        title: const Text('情绪日历'),
                        subtitle: const Text('记录每日心情变化', style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const EmotionCalendarScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.lightbulb_outline, color: Color(0xFFFFB74D)),
                        title: const Text('点灯人'),
                        subtitle: const Text('守护心湖的温暖', style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const GuardianScreen())),
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
                        leading: const Icon(Icons.notifications_outlined, color: AppTheme.skyBlue),
                        title: const Text('消息通知'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const NotificationScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.help_outline, color: AppTheme.skyBlue),
                        title: const Text('帮助与反馈'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const HelpScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.lock_outline, color: AppTheme.skyBlue),
                        title: const Text('修改密码'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: _showChangePasswordDialog,
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
                        leading: const Icon(Icons.person_remove, color: Colors.orange),
                        title: const Text('注销账号', style: TextStyle(color: Colors.orange)),
                        trailing: const Icon(Icons.chevron_right, color: Colors.orange),
                        onTap: _showDeleteAccountDialog,
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.logout, color: AppTheme.errorColor),
                        title: const Text('退出登录', style: TextStyle(color: AppTheme.errorColor)),
                        trailing: const Icon(Icons.chevron_right, color: AppTheme.errorColor),
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
                            // 清除本地存储
                            await StorageUtil.clearToken();
                            ApiClient().clearToken();

                            // 清除用户状态
                            if (context.mounted) {
                              Provider.of<UserProvider>(context, listen: false)
                                  .logout();
                            }

                            // 跳转到登录页面
                            if (context.mounted) {
                              Navigator.of(context).pushAndRemoveUntil(
                                MaterialPageRoute(
                                  builder: (context) => const AuthScreen(),
                                ),
                                (route) => false,
                              );
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
        ),
    );
  }

  Widget _buildStatCard(
      BuildContext context, String label, String value, IconData icon,
      {VoidCallback? onTap}) {
    final card = Card(
      color: Colors.white.withValues(alpha: 0.9),
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
                    color: AppTheme.textPrimary,
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

  void _showChangePasswordDialog() {
    final oldPasswordController = TextEditingController();
    final newPasswordController = TextEditingController();
    final confirmPasswordController = TextEditingController();
    bool obscureOld = true;
    bool obscureNew = true;
    bool obscureConfirm = true;

    void disposeControllers() {
      oldPasswordController.dispose();
      newPasswordController.dispose();
      confirmPasswordController.dispose();
    }

    showDialog(
      context: context,
      builder: (context) => StatefulBuilder(
        builder: (context, setState) => AlertDialog(
          title: const Text('修改密码'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: oldPasswordController,
                obscureText: obscureOld,
                decoration: InputDecoration(
                  labelText: '旧密码',
                  suffixIcon: IconButton(
                    icon: Icon(
                        obscureOld ? Icons.visibility : Icons.visibility_off),
                    onPressed: () => setState(() => obscureOld = !obscureOld),
                  ),
                ),
              ),
              const SizedBox(height: 16),
              TextField(
                controller: newPasswordController,
                obscureText: obscureNew,
                decoration: InputDecoration(
                  labelText: '新密码',
                  hintText: '至少6个字符',
                  suffixIcon: IconButton(
                    icon: Icon(
                        obscureNew ? Icons.visibility : Icons.visibility_off),
                    onPressed: () => setState(() => obscureNew = !obscureNew),
                  ),
                ),
              ),
              const SizedBox(height: 16),
              TextField(
                controller: confirmPasswordController,
                obscureText: obscureConfirm,
                decoration: InputDecoration(
                  labelText: '确认新密码',
                  suffixIcon: IconButton(
                    icon: Icon(obscureConfirm
                        ? Icons.visibility
                        : Icons.visibility_off),
                    onPressed: () =>
                        setState(() => obscureConfirm = !obscureConfirm),
                  ),
                ),
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('取消'),
            ),
            TextButton(
              onPressed: () async {
                final oldPassword = oldPasswordController.text;
                final newPassword = newPasswordController.text;
                final confirmPassword = confirmPasswordController.text;
                final messenger = ScaffoldMessenger.of(context);

                if (oldPassword.isEmpty || newPassword.isEmpty) {
                  messenger.showSnackBar(
                    const SnackBar(content: Text('请填写所有字段')),
                  );
                  return;
                }

                if (newPassword.length < 6) {
                  messenger.showSnackBar(
                    const SnackBar(content: Text('新密码至少6个字符')),
                  );
                  return;
                }

                if (newPassword != confirmPassword) {
                  messenger.showSnackBar(
                    const SnackBar(content: Text('两次密码输入不一致')),
                  );
                  return;
                }

                Navigator.pop(context);

                final authService = AuthService();
                final result = await authService.changePassword(
                  oldPassword: oldPassword,
                  newPassword: newPassword,
                );

                if (mounted) {
                  if (result['success']) {
                    messenger.showSnackBar(
                      const SnackBar(content: Text('密码修改成功')),
                    );
                  } else {
                    messenger.showSnackBar(
                      SnackBar(content: Text(result['message'] ?? '修改失败')),
                    );
                  }
                }
              },
              child: const Text('确定'),
            ),
          ],
        ),
      ),
    ).then((_) => disposeControllers());
  }

  void _showDeleteAccountDialog() {
    final passwordController = TextEditingController();
    final confirmationController = TextEditingController();
    bool obscurePassword = true;

    void disposeControllers() {
      passwordController.dispose();
      confirmationController.dispose();
    }

    showDialog(
      context: context,
      builder: (context) => StatefulBuilder(
        builder: (context, setState) => AlertDialog(
          title: const Text(
            '注销账号',
            style: TextStyle(color: Colors.red),
          ),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text(
                '⚠️ 警告：注销账号将永久删除您的所有数据',
                style: TextStyle(
                    color: Colors.red,
                    fontSize: 14,
                    fontWeight: FontWeight.bold),
              ),
              const SizedBox(height: 12),
              const Text(
                '包括：\n• 所有投放的石头\n• 所有纸船\n• 聊天记录\n• 好友关系',
                style: TextStyle(fontSize: 13),
              ),
              const SizedBox(height: 16),
              TextField(
                controller: passwordController,
                obscureText: obscurePassword,
                decoration: InputDecoration(
                  labelText: '请输入密码确认',
                  suffixIcon: IconButton(
                    icon: Icon(obscurePassword
                        ? Icons.visibility
                        : Icons.visibility_off),
                    onPressed: () =>
                        setState(() => obscurePassword = !obscurePassword),
                  ),
                ),
              ),
              const SizedBox(height: 16),
              TextField(
                controller: confirmationController,
                decoration: const InputDecoration(
                  labelText: '输入 DELETE 确认注销',
                  hintText: 'DELETE',
                ),
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('取消'),
            ),
            TextButton(
              onPressed: () async {
                final password = passwordController.text;
                final confirmation = confirmationController.text;
                final messenger = ScaffoldMessenger.of(context);
                final navigator = Navigator.of(context);

                if (password.isEmpty || confirmation != 'DELETE') {
                  messenger.showSnackBar(
                    const SnackBar(content: Text('请正确填写所有字段')),
                  );
                  return;
                }

                navigator.pop();

                final authService = AuthService();
                final result = await authService.deleteAccount(
                  password: password,
                  confirmation: confirmation,
                );

                if (mounted) {
                  if (result['success']) {
                    messenger.showSnackBar(
                      const SnackBar(content: Text('账号已注销')),
                    );
                    navigator.pushAndRemoveUntil(
                      MaterialPageRoute(
                          builder: (context) => const AuthScreen()),
                      (route) => false,
                    );
                  } else {
                    messenger.showSnackBar(
                      SnackBar(content: Text(result['message'] ?? '注销失败')),
                    );
                  }
                }
              },
              child: const Text('确定', style: TextStyle(color: Colors.red)),
            ),
          ],
        ),
      ),
    ).then((_) => disposeControllers());
  }

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
          maxLength: 100,
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
              final userProvider = Provider.of<UserProvider>(context, listen: false);
              final authService = AuthService();
              final result = await authService.updateNickname(newNickname);

              if (!mounted) return;

              if (result['success']) {
                if (mounted) {
                  setState(() {
                    _nickname = newNickname;
                  });
                  await StorageUtil.saveNickname(newNickname);
                  // 同步到 UserProvider
                  userProvider.updateUser(nickname: newNickname);
                  if (mounted) {
                    messenger.showSnackBar(
                      const SnackBar(content: Text('昵称修改成功')),
                    );
                  }
                }
              } else {
                messenger.showSnackBar(
                  SnackBar(content: Text(result['message'] ?? '修改失败')),
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
