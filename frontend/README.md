# 移动端说明

当前移动端是 Flutter 客户端，直接连接现网网关，负责匿名登录、内容互动、关系链、情绪链和 AI 互动链。

## 当前入口

- 公网入口：`http://121.41.195.165`
- API：`http://121.41.195.165/api`
- WebSocket：`ws://121.41.195.165/ws/broadcast`
- release 包：`frontend/build/app/outputs/flutter-apk/app-release.apk`
- 当前 APK SHA-256：`9654d5facf294ab1c0d21e6ce6f73728d346977994fe911a23be1de02553ac31`

## 本地开发

```bash
cd frontend
flutter pub get
flutter run
```

## 构建与检查

```bash
cd frontend
flutter analyze
flutter build apk --release
```

## 当前启动链

### 入口文件

- `lib/main.dart`
- `lib/router/app_router.dart`
- `lib/utils/app_config.dart`
- `lib/di/service_locator.dart`

### 启动顺序

1. `runZonedGuarded` 包裹全局异常。
2. 注册 `FlutterError.onError`、`platformDispatcher.onError`、`ErrorWidget.builder`。
3. 执行 `setupServiceLocator()`。
4. 执行 `appConfig.initialize()`。
5. 启动 `CacheService().startAutoCleanup()`。
6. 配置 `ApiClient` 的 token 刷新和 401 处理。
7. 挂载 `MultiProvider`。

## 当前环境变量

- `PUBLIC_ORIGIN`
- `API_BASE_URL`
- `WS_URL`
- `PRODUCTION_PUBLIC_ORIGIN`
- `DEV_API_URL`
- `ANDROID_API_HOST`
- `IOS_API_HOST`
- `ENABLE_CERT_PINNING`
- `CERT_SHA256_PINS`

## 当前依赖注入

`service_locator.dart` 当前注册：

- `AuthService`
- `UserService`
- `AccountService`
- `StoneService`
- `InteractionService`
- `FriendService`
- `TempFriendService`
- `NotificationService`
- `GuardianService`
- `ReportService`
- `EdgeAIService`
- `AIRecommendationService`
- `RecommendationService`
- `LakeGodService`
- `ConsultationService`
- `PsychSupportService`
- `VIPService`

## 当前 Provider 状态链

- `ThemeProvider`
- `UserProvider`
- `NotificationProvider`
- `StoneProvider`
- `FriendProvider`
- `EdgeAIProvider`

## 当前数据源结构

### 账号与认证

- `auth_service.dart`
- `account_service.dart`
- `user_service.dart`

### 内容与互动

- `stone_service.dart`
- `interaction_service.dart`
- `notification_service.dart`

### 关系链

- `friend_service.dart`
- `temp_friend_service.dart`
- `guardian_service.dart`

### AI 与推荐

- `ai_recommendation_service.dart`
- `recommendation_service.dart`
- `edge_ai_service.dart`
- `lake_god_service.dart`

### 关怀与增值

- `consultation_service.dart`
- `psych_support_service.dart`
- `vip_service.dart`

### 网络与基础件

- `api_client.dart`
- `websocket_manager.dart`
- `cache_service.dart`
- `payload_contract.dart`
- `storage_util.dart`

## 当前页面范围

- 匿名登录与账号恢复
- 湖面流、石头详情、投石
- 涟漪、纸船、通知
- 好友、临时好友、守护
- 情绪日历、热力图、脉搏
- 推荐、AI 情绪分析、湖神、安全港、VIP、咨询

## 当前路由分组

### 公开路由

- `/`
- `/onboarding`
- `/auth`

### 登录后路由

- `/home`
- `/notifications`
- `/stone-detail`
- `/profile`
- `/my-stones`
- `/my-boats`
- `/my-ripples`
- `/received-boats`
- `/emotion-heatmap`
- `/emotion-calendar`
- `/emotion-trends`
- `/guardian`
- `/safe-harbor`
- `/consultation`
- `/light`
- `/help`
- `/privacy-settings`
- `/lake-god-chat`
- `/personalized`
- `/friends`
- `/friend-chat`
- `/temp-friends`
- `/user-detail`
- `/discover`
- `/lake-feed`

路由守卫当前通过 `StorageUtil.getToken()` 和 `StorageUtil.getUserId()` 判断是否放行。

## 当前网络与存储

- `api_client.dart`：Dio base URL、token 注入、401 刷新、缓存配合、证书 pinning。
- `websocket_manager.dart`：query `token` 鉴权、离线队列、房间引用计数、自动重连、心跳。
- `payload_contract.dart`：字段镜像、ID 提取、事件 payload 归一。
- `storage_util.dart`：敏感数据走 `FlutterSecureStorage`，普通数据走 `SharedPreferences`。

## 当前约束

- 发布版默认直连当前生产网关，不允许回落到占位域名。
- 数据源负责协议归一和错误抛出。
- Provider 负责状态编排，不在页面里拼接口分叉。
- WebSocket 建连必须携带 token。

## 相关手册

- [../docs/04_技术实现全景手册.md](../docs/04_技术实现全景手册.md)
- [../docs/02_API与实时链路手册.md](../docs/02_API与实时链路手册.md)
- [../docs/05_API接口全量清单.md](../docs/05_API接口全量清单.md)
