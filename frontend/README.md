# HeartLake — Flutter 移动端

匿名情感社交平台的跨平台移动客户端，基于 Flutter 3.19+ 构建。

## 技术栈

| 类别 | 技术 |
|------|------|
| 框架 | Flutter 3.19+ / Dart 3.3+ |
| 状态管理 | Provider (ThemeProvider / UserProvider / NotificationProvider) |
| 网络请求 | Dio + WebSocket |
| 本地存储 | SharedPreferences |
| 边缘AI | 三层情感融合（规则 + 词典 + 统计）+ 本地差分隐私 |
| 主题 | Material Design 3, 深色/浅色双模式 |

## 项目结构

```
lib/
├── main.dart                              # 应用入口, MaterialApp + MultiProvider + 路由
├── data/
│   ├── datasources/                       # 服务层（22 个数据源服务）
│   │   ├── api_client.dart                # HTTP 客户端 (Dio 封装, Token 管理, 拦截器)
│   │   ├── websocket_manager.dart         # WebSocket 连接管理 (心跳/重连/事件分发)
│   │   ├── base_service.dart              # 服务基类 (ServiceResponse<T> 通用封装)
│   │   ├── auth_service.dart              # PASETO 认证
│   │   ├── account_service.dart           # 账户管理
│   │   ├── stone_service.dart             # 心石 CRUD
│   │   ├── interaction_service.dart       # 涟漪互动
│   │   ├── friend_service.dart            # 好友系统
│   │   ├── temp_friend_service.dart       # 临时好友
│   │   ├── guardian_service.dart          # 守护者系统
│   │   ├── lake_god_service.dart          # 湖神 AI 对话
│   │   ├── vip_service.dart               # VIP 关怀
│   │   ├── recommendation_service.dart    # 推荐服务
│   │   ├── ai_recommendation_service.dart # AI 推荐服务
│   │   ├── consultation_service.dart      # 心理咨询
│   │   ├── psych_support_service.dart     # 心理支持资源
│   │   ├── edge_ai_service.dart           # 边缘 AI 数据源
│   │   ├── notification_service.dart      # 通知服务
│   │   ├── user_service.dart              # 用户信息
│   │   ├── report_service.dart            # 举报
│   │   └── cache_service.dart             # 本地缓存
│   └── repositories/                      # 仓储层（预留）
├── domain/
│   └── entities/                          # 领域实体
│       ├── stone.dart                     # 心石
│       ├── user.dart                      # 用户
│       ├── mood.dart                      # 情绪类型
│       ├── emotion_type.dart              # 情绪枚举
│       └── api_response.dart              # API 响应封装
├── edge_ai/                               # 边缘 AI 模块
│   ├── edge_ai.dart                       # EdgeAI 主入口
│   ├── emotion_classifier.dart            # 三层情感分类器（规则+词典+统计）
│   └── local_dp.dart                      # 本地差分隐私噪声注入
├── presentation/
│   ├── screens/                           # 29 个页面
│   │   ├── splash_screen.dart             # 启动页
│   │   ├── onboarding_screen.dart         # 引导页
│   │   ├── auth_screen.dart               # 登录/注册
│   │   ├── home_screen.dart               # 主页框架 (5 个底部 Tab)
│   │   ├── lake_screen.dart               # 观湖 - 心石列表 + WebSocket 实时更新
│   │   ├── lake_feed_screen.dart          # 湖面信息流
│   │   ├── discover_screen.dart           # 湖底 - 热门/搜索/心情发现
│   │   ├── publish_screen.dart            # 投石 - 内容+类型+颜色+心情
│   │   ├── friends_screen.dart            # 好友列表 + WebSocket 实时更新
│   │   ├── profile_screen.dart            # 个人中心
│   │   ├── stone_detail_screen.dart       # 石头详情 + 涟漪列表
│   │   ├── friend_chat_screen.dart        # 好友聊天
│   │   ├── lake_god_chat_screen.dart      # 湖神 AI 对话
│   │   ├── received_boats_screen.dart     # 收到的纸船
│   │   ├── my_boats_screen.dart           # 我的纸船
│   │   ├── my_stones_screen.dart          # 我的心石
│   │   ├── my_ripples_screen.dart         # 我的涟漪
│   │   ├── emotion_calendar_screen.dart   # 情绪日历（月度视图）
│   │   ├── emotion_heatmap_screen.dart    # 情绪热力图（独立页面）
│   │   ├── emotion_trends_screen.dart     # 情绪趋势分析
│   │   ├── guardian_screen.dart           # 守护者
│   │   ├── vip_screen.dart                # VIP 关怀
│   │   ├── safe_harbor_screen.dart        # 安全港湾
│   │   ├── consultation_screen.dart       # 心理咨询（E2E 加密）
│   │   ├── personalized_screen.dart       # 个性化推荐
│   │   ├── temp_friends_screen.dart       # 临时好友
│   │   ├── user_detail_screen.dart        # 用户详情
│   │   ├── notification_screen.dart       # 通知中心
│   │   └── help_screen.dart               # 帮助
│   ├── widgets/                           # 可复用组件
│   │   ├── stone_card/                    # 心石卡片（模块化重构版）
│   │   │   ├── stone_card.dart            # 主组件
│   │   │   ├── stone_card_header.dart     # 头部
│   │   │   ├── stone_card_content.dart    # 内容
│   │   │   ├── stone_card_actions.dart    # 操作栏
│   │   │   └── stone_card_controller.dart # 控制器
│   │   ├── similar_stones_section.dart    # 相似心石区块
│   │   ├── ai_content_preview.dart        # AI 内容预览
│   │   ├── emotion_heatmap.dart           # 情绪热力图组件
│   │   ├── emotion_insights_card.dart     # 情绪洞察卡片
│   │   ├── emotion_pulse_widget.dart      # 情绪脉搏组件
│   │   ├── atmospheric_background.dart    # 大气背景动效
│   │   ├── water_background.dart          # 水面背景动效
│   │   ├── deep_dive_layer.dart           # 深潜分层浏览
│   │   ├── privacy_badge.dart             # 隐私保护徽章
│   │   ├── report_dialog.dart             # 举报对话框
│   │   ├── psych_support_dialog.dart      # 心理支持对话框
│   │   ├── shimmer_loading.dart           # 骨架屏加载
│   │   ├── status_view.dart               # 状态视图
│   │   └── animations/                    # 动画组件
│   │       ├── ripple_effect.dart         # 涟漪效果
│   │       └── staggered_list.dart        # 交错列表动画
│   └── providers/                         # 状态管理
│       ├── edge_ai_provider.dart          # 边缘 AI 状态
│       ├── friend_provider.dart           # 好友关系状态
│       ├── notification_provider.dart     # 通知状态
│       ├── stone_provider.dart            # 心石流状态
│       ├── theme_provider.dart            # 主题状态
│       └── user_provider.dart             # 用户状态
└── utils/                                 # 工具类
    ├── app_theme.dart                     # 主题配置
    ├── app_config.dart                    # 应用配置
    ├── app_logger.dart                    # 日志工具
    ├── mood_colors.dart                   # 情绪色彩映射
    ├── error_handler.dart                 # 错误处理 (Result<T> monad)
    ├── storage_util.dart                  # 本地存储
    └── animation_utils.dart               # 动画工具
```

## 核心功能

### 五大主页面（底部导航）

| Tab | 页面 | 功能 |
|-----|------|------|
| 观湖 | LakeScreen | 心石瀑布流, WebSocket 实时推送, 下拉刷新, 分页加载 |
| 湖底 | DiscoverScreen | 热门内容, 语义搜索, 按心情发现, 三 Tab 切换 |
| 投石 | PublishScreen | 内容编辑, 石头类型(轻/中/重), 颜色选择, 心情标签, 匿名开关 |
| 好友 | FriendsScreen | 好友列表, 搜索, WebSocket 实时更新 |
| 倒影 | ProfileScreen | 个人信息, 情绪日历入口, 我的心石/纸船, 设置 |

### WebSocket 实时通信

WebSocketManager 管理全局连接，支持：
- 自动重连（指数退避）
- 心跳保活（30 秒间隔）
- 事件分发: new_stone, boat_update, new_boat, ripple_update, new_ripple, stone_deleted, boat_deleted, ripple_deleted, friend_accepted, friend_removed, friend_request

### 边缘 AI

- 三层情感融合分类器（规则引擎 + 情感词典 + 统计模型）
- 本地差分隐私噪声注入（Laplace 机制）
- 情绪脉搏实时追踪
- 离线可用，保护用户隐私

### 情绪可视化

- 情绪日历：月度视图，按日期展示情绪变化
- 情绪热力图：独立页面，直观展示情绪分布
- 情绪趋势：折线图展示长期情绪走向
- 水面动效：根据社区情绪动态变化
- 深潜分层：按情绪深度分层浏览内容

### 心理关怀

- 湖神 AI 对话：基于双记忆 RAG 的智能陪伴
- 安全港湾：心理支持资源
- 心理咨询：E2E 加密的咨询会话
- 守护者系统：社区互助守望

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

| 指标 | 数值 |
|------|------|
| 代码行数 | 26,950 行 |
| 页面数 | 29 |
| 可复用 Widget | 24 |
| 数据源服务 | 22 |
| 领域实体 | 5 |
| 动画组件 | 3 |
| 工具类 | 8 |
