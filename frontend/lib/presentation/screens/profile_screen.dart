// @file profile_screen.dart
// @brief 个人资料界面
// Created by 林子怡

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/user_provider.dart';
import '../../utils/app_theme.dart';
import '../../utils/storage_util.dart';
import '../../data/datasources/user_service.dart';
import '../../data/datasources/auth_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../data/datasources/vip_service.dart';
import '../widgets/atmospheric_background.dart';
import 'package:image_picker/image_picker.dart';
import 'dart:io';
import 'auth_screen.dart';
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

class ProfileScreen extends StatefulWidget {
  const ProfileScreen({super.key});

  @override
  State<ProfileScreen> createState() => _ProfileScreenState();
}

class _ProfileScreenState extends State<ProfileScreen> {
  final UserService _userService = UserService();
  final WebSocketManager _wsManager = WebSocketManager();
  Map<String, dynamic>? _stats;
  String? _username;
  String? _nickname;
  String? _avatarUrl;
  String? _bio;
  bool _hasLight = false; // 灯是否点亮
  int _vipDaysLeft = 0;
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
    if (!mounted) return;

    // 监听新石头（更新投石计数）
    _newStoneListener = (data) async {
      final userId = await StorageUtil.getUserId();
      final stoneUserId = data['stone']?['user_id'] ?? data['user_id'];
      if (stoneUserId == userId && mounted) {
        _loadUserStats(silent: true);
      }
    };
    _wsManager.on('new_stone', _newStoneListener!);

    // 监听纸船更新
    _boatUpdateListener = (data) {
      if (mounted) _loadUserStats(silent: true);
    };
    _wsManager.on('boat_update', _boatUpdateListener!);
    _wsManager.on('new_boat', _boatUpdateListener!);

    // 监听涟漪更新
    _rippleUpdateListener = (data) {
      if (mounted) _loadUserStats(silent: true);
    };
    _wsManager.on('ripple_update', _rippleUpdateListener!);
    _wsManager.on('new_ripple', _rippleUpdateListener!);

    // 监听删除事件，刷新统计
    _stoneDeletedListener = (data) {
      if (mounted) _loadUserStats(silent: true);
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener!);

    _boatDeletedListener = (data) {
      if (mounted) _loadUserStats(silent: true);
    };
    _wsManager.on('boat_deleted', _boatDeletedListener!);

    _rippleDeletedListener = (data) {
      if (mounted) _loadUserStats(silent: true);
    };
    _wsManager.on('ripple_deleted', _rippleDeletedListener!);

    // 断线重连后自动刷新所有数据
    _reconnectedListener = (data) {
      if (mounted) _refreshAll();
    };
    _wsManager.on('reconnected', _reconnectedListener!);
  }

  void _removeWebSocketListeners() {
    if (_newStoneListener != null) {
      _wsManager.off('new_stone', _newStoneListener!);
    }
    if (_boatUpdateListener != null) {
      _wsManager.off('boat_update', _boatUpdateListener!);
      _wsManager.off('new_boat', _boatUpdateListener!);
    }
    if (_rippleUpdateListener != null) {
      _wsManager.off('ripple_update', _rippleUpdateListener!);
      _wsManager.off('new_ripple', _rippleUpdateListener!);
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
        final result = await _userService.getUserStats(userId);
        if (result['success'] == true && mounted) {
          setState(() => _stats = result['data']);
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

      final result = await _userService.getUserInfo(userId);
      if (result['success'] == true) {
        final userData = result['user'] as Map<String, dynamic>?;
        if (mounted && userData != null) {
          setState(() {
            _avatarUrl = userData['avatar_url'];
            _bio = userData['bio'];
            _nickname = userData['nickname'];
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
      final result = await _userService.uploadFile(file);

      if (result['success'] == true) {
        final data = result['data'] as Map<String, dynamic>?;
        final avatarUrl = data?['url'];

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
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final stonesCount = _stats?['stones_count']?.toString() ?? '0';
    final boatsReceived = _stats?['boats_received']?.toString() ?? '0';
    final boatsSent = _stats?['boats_sent']?.toString() ?? '0';
    final joinDays = (_stats?['join_days']?.toString() ?? '0');

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
            backgroundColor: Colors.white,
            child: ListView(
              padding: EdgeInsets.only(
                top: MediaQuery.of(context).padding.top + kToolbarHeight + 16,
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
                              const Icon(Icons.edit, size: 16, color: AppTheme.textTertiary),
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
                              color: _bio != null ? AppTheme.textSecondary : AppTheme.textTertiary,
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
                        leading: const Icon(Icons.show_chart, color: AppTheme.skyBlue),
                        title: const Text('情绪趋势'),
                        subtitle: const Text('查看情绪变化轨迹', style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const EmotionTrendsScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.grid_view_rounded, color: AppTheme.skyBlue),
                        title: const Text('情绪热力图'),
                        subtitle: const Text('查看情绪密度与洞察', style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const EmotionHeatmapScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.water, color: AppTheme.skyBlue),
                        title: const Text('我的涟漪'),
                        subtitle: const Text('查看发出的涟漪', style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const MyRipplesScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.lightbulb_outline, color: Color(0xFFFFB74D)),
                        title: const Text('守护者'),
                        subtitle: const Text('守护心湖的温暖与灯火', style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const GuardianScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.spa, color: AppTheme.secondaryColor),
                        title: const Text('安全港湾'),
                        subtitle: const Text('心理支持与自助资源', style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const SafeHarborScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.psychology, color: AppTheme.skyBlue),
                        title: const Text('心理咨询'),
                        subtitle: const Text('预约专业咨询师', style: TextStyle(fontSize: 12)),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const ConsultationScreen())),
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
                        leading: const Icon(Icons.shield_outlined, color: AppTheme.skyBlue),
                        title: const Text('隐私与安全'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const PrivacySettingsScreen())),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.help_outline, color: AppTheme.skyBlue),
                        title: const Text('帮助与反馈'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const HelpScreen())),
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
                            // 先移除 WebSocket 监听器
                            _removeWebSocketListeners();

                            // 断开 WebSocket
                            _wsManager.disconnect();

                            // 清除本地存储
                            await StorageUtil.clearToken();
                            AuthService().logout();

                            // 清除用户状态
                            if (context.mounted) {
                              await Provider.of<UserProvider>(context, listen: false)
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
        ],
      ),
    );
  }

  Widget _buildStatCard(
      BuildContext context, String label, String value, IconData icon,
      {VoidCallback? onTap}) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final card = Card(
      color: isDark ? const Color(0xFF1B2838).withValues(alpha: 0.9) : Colors.white.withValues(alpha: 0.9),
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
                    color: isDark ? const Color(0xFFE8EAED) : AppTheme.textPrimary,
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
