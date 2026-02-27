// 声明式路由配置 - 基于 go_router 实现路由守卫与深度链接

import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import '../domain/entities/stone.dart';
import '../presentation/screens/splash_screen.dart';
import '../presentation/screens/home_screen.dart';
import '../presentation/screens/onboarding_screen.dart';
import '../presentation/screens/auth_screen.dart';
import '../presentation/screens/profile_screen.dart';
import '../presentation/screens/notification_screen.dart';
import '../presentation/screens/stone_detail_screen.dart';
import '../presentation/screens/my_stones_screen.dart';
import '../presentation/screens/my_boats_screen.dart';
import '../presentation/screens/my_ripples_screen.dart';
import '../presentation/screens/received_boats_screen.dart';
import '../presentation/screens/emotion_heatmap_screen.dart';
import '../presentation/screens/emotion_calendar_screen.dart';
import '../presentation/screens/emotion_trends_screen.dart';
import '../presentation/screens/guardian_screen.dart';
import '../presentation/screens/safe_harbor_screen.dart';
import '../presentation/screens/consultation_screen.dart';
import '../presentation/screens/vip_screen.dart';
import '../presentation/screens/help_screen.dart';
import '../presentation/screens/privacy_settings_screen.dart';
import '../presentation/screens/lake_god_chat_screen.dart';
import '../presentation/screens/personalized_screen.dart';
import '../presentation/screens/friends_screen.dart';
import '../presentation/screens/friend_chat_screen.dart';
import '../presentation/screens/temp_friends_screen.dart';
import '../presentation/screens/user_detail_screen.dart';
import '../presentation/screens/discover_screen.dart';
import '../presentation/screens/lake_feed_screen.dart';
import '../utils/storage_util.dart';
import '../data/datasources/api_client.dart';

/// 全局 navigatorKey，保持与旧代码兼容（ApiClient 401 回调等场景）
final GlobalKey<NavigatorState> rootNavigatorKey = GlobalKey<NavigatorState>();

/// 路由路径常量，避免硬编码字符串
class AppRoutes {
  static const String splash = '/';
  static const String home = '/home';
  static const String onboarding = '/onboarding';
  static const String auth = '/auth';
  static const String profile = '/profile';
  static const String notifications = '/notifications';
  static const String stoneDetail = '/stone-detail';
  static const String myStones = '/my-stones';
  static const String myBoats = '/my-boats';
  static const String myRipples = '/my-ripples';
  static const String receivedBoats = '/received-boats';
  static const String emotionHeatmap = '/emotion-heatmap';
  static const String emotionCalendar = '/emotion-calendar';
  static const String emotionTrends = '/emotion-trends';
  static const String guardian = '/guardian';
  static const String safeHarbor = '/safe-harbor';
  static const String consultation = '/consultation';
  static const String vip = '/light';
  static const String help = '/help';
  static const String privacySettings = '/privacy-settings';
  static const String lakeGodChat = '/lake-god-chat';
  static const String personalized = '/personalized';
  static const String friends = '/friends';
  static const String friendChat = '/friend-chat';
  static const String tempFriends = '/temp-friends';
  static const String userDetail = '/user-detail';
  static const String discover = '/discover';
  static const String lakeFeed = '/lake-feed';
}

/// 应用路由配置
///
/// 使用 go_router 实现：
/// - 声明式路由定义
/// - 路由守卫（未登录重定向到启动页）
/// - 深度链接支持
/// - 页面过渡动画
GoRouter createRouter() {
  return GoRouter(
    navigatorKey: rootNavigatorKey,
    initialLocation: AppRoutes.splash,
    debugLogDiagnostics: false,
    redirect: _routeGuard,
    routes: [
      // 启动页 - 处理登录检查和初始导航
      GoRoute(
        path: AppRoutes.splash,
        builder: (context, state) => const SplashScreen(),
      ),

      // 引导页
      GoRoute(
        path: AppRoutes.onboarding,
        builder: (context, state) => const OnboardingScreen(),
      ),

      // 认证页
      GoRoute(
        path: AppRoutes.auth,
        builder: (context, state) => const AuthScreen(),
      ),

      // 主页（底部导航）
      GoRoute(
        path: AppRoutes.home,
        builder: (context, state) => const HomeScreen(),
      ),

      // 通知
      GoRoute(
        path: AppRoutes.notifications,
        builder: (context, state) => const NotificationScreen(),
      ),

      // 石头详情 - 通过 extra 传递 Stone 对象
      GoRoute(
        path: AppRoutes.stoneDetail,
        builder: (context, state) {
          final stone = state.extra as Stone;
          return StoneDetailScreen(stone: stone);
        },
      ),

      // 个人中心子页面
      GoRoute(
        path: AppRoutes.profile,
        builder: (context, state) => const ProfileScreen(),
      ),
      GoRoute(
        path: AppRoutes.myStones,
        builder: (context, state) => const MyStonesScreen(),
      ),
      GoRoute(
        path: AppRoutes.myBoats,
        builder: (context, state) => const MyBoatsScreen(),
      ),
      GoRoute(
        path: AppRoutes.myRipples,
        builder: (context, state) => const MyRipplesScreen(),
      ),
      GoRoute(
        path: AppRoutes.receivedBoats,
        builder: (context, state) => const ReceivedBoatsScreen(),
      ),
      GoRoute(
        path: AppRoutes.emotionHeatmap,
        builder: (context, state) => const EmotionHeatmapScreen(),
      ),
      GoRoute(
        path: AppRoutes.emotionCalendar,
        builder: (context, state) => const EmotionCalendarScreen(),
      ),
      GoRoute(
        path: AppRoutes.emotionTrends,
        builder: (context, state) => const EmotionTrendsScreen(),
      ),
      GoRoute(
        path: AppRoutes.guardian,
        builder: (context, state) => const GuardianScreen(),
      ),
      GoRoute(
        path: AppRoutes.safeHarbor,
        builder: (context, state) => const SafeHarborScreen(),
      ),
      GoRoute(
        path: AppRoutes.consultation,
        builder: (context, state) => const ConsultationScreen(),
      ),
      GoRoute(
        path: AppRoutes.vip,
        builder: (context, state) => const VIPScreen(),
      ),
      // 兼容旧地址：/vip -> 灯火页
      GoRoute(
        path: '/vip',
        builder: (context, state) => const VIPScreen(),
      ),
      GoRoute(
        path: AppRoutes.help,
        builder: (context, state) => const HelpScreen(),
      ),
      GoRoute(
        path: AppRoutes.privacySettings,
        builder: (context, state) => const PrivacySettingsScreen(),
      ),
      GoRoute(
        path: AppRoutes.lakeGodChat,
        builder: (context, state) => const LakeGodChatScreen(),
      ),
      GoRoute(
        path: AppRoutes.personalized,
        builder: (context, state) => const PersonalizedScreen(),
      ),

      // 好友相关
      GoRoute(
        path: AppRoutes.friends,
        builder: (context, state) => const FriendsScreen(),
      ),
      // 好友聊天 - 通过 extra 传递 {friendId, friendName}
      GoRoute(
        path: AppRoutes.friendChat,
        builder: (context, state) {
          final params = state.extra as Map<String, dynamic>;
          return FriendChatScreen(
            friendId: params['friendId'] as String,
            friendName: params['friendName'] as String?,
          );
        },
      ),
      GoRoute(
        path: AppRoutes.tempFriends,
        builder: (context, state) => const TempFriendsScreen(),
      ),
      // 用户详情 - 通过 extra 传递 {userId, nickname}
      GoRoute(
        path: AppRoutes.userDetail,
        builder: (context, state) {
          final params = state.extra as Map<String, dynamic>;
          return UserDetailScreen(
            userId: params['userId'] as String,
            nickname: params['nickname'] as String?,
          );
        },
      ),

      // 发现
      GoRoute(
        path: AppRoutes.discover,
        builder: (context, state) => const DiscoverScreen(),
      ),
      GoRoute(
        path: AppRoutes.lakeFeed,
        builder: (context, state) => const LakeFeedScreen(),
      ),
    ],
  );
}

/// 路由守卫 - 控制未登录用户的访问权限
///
/// 逻辑：
/// 1. 启动页、引导页、认证页始终放行
/// 2. 其他页面检查 token，无 token 则重定向到启动页
Future<String?> _routeGuard(BuildContext context, GoRouterState state) async {
  final path = state.matchedLocation;

  // 这些页面不需要登录
  const publicPaths = [
    AppRoutes.splash,
    AppRoutes.onboarding,
    AppRoutes.auth,
  ];
  if (publicPaths.contains(path)) return null;

  // 检查是否有 token
  String? token;
  try {
    token = await StorageUtil.getToken();
  } catch (_) {
    // Web 环境安全存储不可用时，回退到内存态 token。
    token = ApiClient().token;
  }

  token ??= ApiClient().token;
  if (token == null || token.isEmpty) {
    return AppRoutes.splash;
  }

  return null;
}
