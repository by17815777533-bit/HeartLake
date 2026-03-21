// 依赖注入容器
//
// 基于get_it统一管理Service实例，所有Service注册为惰性单例。

import 'package:get_it/get_it.dart';
import '../data/datasources/auth_service.dart';
import '../data/datasources/stone_service.dart';
import '../data/datasources/friend_service.dart';
import '../data/datasources/temp_friend_service.dart';
import '../data/datasources/interaction_service.dart';
import '../data/datasources/consultation_service.dart';
import '../data/datasources/user_service.dart';
import '../data/datasources/account_service.dart';
import '../data/datasources/guardian_service.dart';
import '../data/datasources/report_service.dart';
import '../data/datasources/notification_service.dart';
import '../data/datasources/lake_god_service.dart';
import '../data/datasources/vip_service.dart';
import '../data/datasources/psych_support_service.dart';
import '../data/datasources/ai_recommendation_service.dart';
import '../data/datasources/recommendation_service.dart';
import '../data/datasources/edge_ai_service.dart';

/// 全局ServiceLocator实例
final GetIt sl = GetIt.instance;

/// 初始化依赖注入容器
///
/// 所有Service注册为惰性单例，首次访问时创建，后续复用同一实例。
/// 在main()中调用一次即可。
void setupServiceLocator() {
  // 认证与用户
  sl.registerLazySingleton<AuthService>(() => AuthService());
  sl.registerLazySingleton<UserService>(() => UserService());
  sl.registerLazySingleton<AccountService>(() => AccountService());

  // 核心业务
  sl.registerLazySingleton<StoneService>(() => StoneService());
  sl.registerLazySingleton<InteractionService>(() => InteractionService());
  sl.registerLazySingleton<FriendService>(() => FriendService());
  sl.registerLazySingleton<TempFriendService>(() => TempFriendService());

  // 社交与通知
  sl.registerLazySingleton<NotificationService>(() => NotificationService());
  sl.registerLazySingleton<GuardianService>(() => GuardianService());
  sl.registerLazySingleton<ReportService>(() => ReportService());

  // AI 与推荐
  sl.registerLazySingleton<EdgeAIService>(() => EdgeAIService());
  sl.registerLazySingleton<AIRecommendationService>(() => AIRecommendationService());
  sl.registerLazySingleton<RecommendationService>(() => RecommendationService());
  sl.registerLazySingleton<LakeGodService>(() => LakeGodService());

  // 咨询与心理支持
  sl.registerLazySingleton<ConsultationService>(() => ConsultationService());
  sl.registerLazySingleton<PsychSupportService>(() => PsychSupportService());
  sl.registerLazySingleton<VIPService>(() => VIPService());
}
