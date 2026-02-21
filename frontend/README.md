# HeartLake -- Flutter 移动端

匿名情感社交平台的跨平台移动客户端，基于 Flutter 3.19+ 构建。

## 技术栈

| 类别 | 技术 |
|------|------|
| 框架 | Flutter 3.19+ / Dart 3.3+ |
| 状态管理 | Provider |
| 网络请求 | Dio + WebSocket |
| 本地存储 | SharedPreferences |
| 边缘AI | TFLite (情感分类 + 本地差分隐私) |
| 主题 | Material Design 3, 深色/浅色双模式 |

## 项目结构

```
lib/
├── main.dart                          # 应用入口, MaterialApp + 路由
├── data/
│   ├── datasources/                   # 服务层（认证、心石、纸船、好友、推荐、媒体等）
│   │   ├── api_client.dart            # HTTP客户端 (Dio封装, Token管理)
│   │   ├── websocket_manager.dart     # WebSocket连接管理 (心跳/重连/事件分发)
│   │   ├── stone_service.dart         # 心石CRUD
│   │   ├── paper_boat_service.dart    # 纸船CRUD
│   │   ├── friend_service.dart        # 好友系统
│   │   ├── interaction_service.dart   # 涟漪互动
│   │   ├── auth_service.dart          # PASETO认证
│   │   ├── account_service.dart       # 账户管理
│   │   ├── guardian_service.dart      # 守护者系统
│   │   ├── lake_god_service.dart      # 湖神AI对话
│   │   ├── vip_service.dart           # VIP关怀
│   │   ├── media_service.dart         # 媒体上传
│   │   ├── report_service.dart        # 举报
│   │   ├── temp_friend_service.dart   # 临时好友
│   │   ├── user_service.dart          # 用户信息
│   │   ├── cache_service.dart         # 本地缓存
│   │   └── base_service.dart          # 服务基类
│   └── repositories/                  # 仓储层（预留）
├── domain/
│   └── entities/                      # 领域实体
│       ├── stone.dart                 # 心石
│       ├── paper_boat.dart            # 纸船
│       ├── user.dart                  # 用户
│       └── mood.dart                  # 情绪类型
├── edge_ai/                           # 边缘AI模块
│   └── local_dp.dart                  # 本地差分隐私
├── presentation/
│   ├── screens/                       # 多页面业务流（观湖/湖底/投石/好友/倒影等）
│   │   ├── splash_screen.dart         # 启动页
│   │   ├── auth_screen.dart           # 登录/注册
│   │   ├── home_screen.dart           # 主页框架 (5个底部Tab)
│   │   ├── lake_screen.dart           # 观湖 - 心石列表 + WebSocket实时更新
│   │   ├── discover_screen.dart       # 湖底 - 热门/搜索/心情发现
│   │   ├── publish_screen.dart        # 投石 - 内容+类型+颜色+心情
│   │   ├── friends_screen.dart        # 好友列表 + WebSocket实时更新
│   │   ├── profile_screen.dart        # 个人中心
│   │   ├── stone_detail_screen.dart   # 石头详情 + 涟漪列表
│   │   ├── chat_screen.dart           # 纸船聊天
│   │   ├── friend_chat_screen.dart    # 好友聊天
│   │   ├── lake_god_chat_screen.dart  # 湖神AI对话
│   │   ├── received_boats_screen.dart # 收到的纸船
│   │   ├── my_boats_screen.dart       # 我的纸船
│   │   ├── my_stones_screen.dart      # 我的心石
│   │   ├── emotion_calendar_screen.dart # 情绪日历 + 热力图
│   │   ├── guardian_screen.dart       # 守护者
│   │   ├── vip_screen.dart            # VIP关怀
│   │   ├── temp_friends_screen.dart   # 临时好友
│   │   ├── user_detail_screen.dart    # 用户详情
│   │   ├── notification_screen.dart   # 通知中心
│   │   └── help_screen.dart           # 帮助
│   ├── widgets/                       # 复用组件（SkyScaffold/SkyGlassCard 等）
│   │   ├── stone_card.dart            # 心石卡片
│   │   ├── deep_dive_layer.dart       # 深潜分层浏览
│   │   ├── emotion_heatmap.dart       # 情绪热力图
│   │   ├── emotion_insights_card.dart # 情绪洞察卡片
│   │   ├── privacy_badge.dart         # 隐私保护徽章
│   │   ├── report_dialog.dart         # 举报对话框
│   │   ├── shimmer_loading.dart       # 骨架屏加载
│   │   ├── image_preview.dart         # 图片预览
│   │   └── status_view.dart           # 状态视图
│   └── providers/                     # 状态管理
└── utils/                             # 工具类
    ├── app_theme.dart                 # 主题配置
    └── mood_colors.dart               # 情绪色彩
```

## 核心功能

### 五大主页面 (底部导航)

| Tab | 页面 | 功能 |
|-----|------|------|
| 观湖 | LakeScreen | 心石瀑布流, WebSocket实时推送(8种事件), 下拉刷新, 分页加载 |
| 湖底 | DiscoverScreen | 热门内容, 语义搜索, 按心情发现, 三Tab切换 |
| 投石 | PublishScreen | 内容编辑, 石头类型(轻/中/重), 颜色选择, 心情标签, 匿名开关 |
| 好友 | FriendsScreen | 好友列表, 搜索, WebSocket实时更新(好友请求/接受/移除) |
| 倒影 | ProfileScreen | 个人信息, 情绪日历入口, 我的心石/纸船, 设置 |

### WebSocket 实时通信

WebSocketManager 管理全局连接，支持：
- 自动重连 (指数退避)
- 心跳保活 (30秒间隔)
- 事件分发: new_stone, boat_update, new_boat, ripple_update, new_ripple, stone_deleted, boat_deleted, ripple_deleted, friend_accepted, friend_removed, friend_request

### 边缘AI

- 本地情感分类器 (TFLite)
- 本地差分隐私噪声注入
- 离线可用，保护用户隐私

### 情绪可视化

- 情绪日历: 月度视图 + 热力图 + 情绪洞察
- 水面动效: 根据社区情绪动态变化
- 深潜分层: 按情绪深度分层浏览内容

## 开发

```bash
# 安装依赖
flutter pub get

# 开发运行
flutter run

# 运行测试
flutter test

# 构建发布版
flutter build apk --release    # Android
flutter build ios --release    # iOS
```

## 统计

- 24 个页面（含聊天、守护者、推荐与情绪分析）
- 19 个可复用 Widget（Sky 系列基础组件 + 业务组件）
- 20 个数据源服务（账号、内容、互动、推荐、媒体、VIP 等）
- 3 个核心领域实体
