# HeartLake 心湖 - Flutter 移动端

匿名情感交流治愈系社交平台的移动客户端。

## 技术栈

- Flutter 3.19+
- Dart 3.3+
- 状态管理：Provider / Riverpod
- 网络请求：Dio
- 本地存储：SharedPreferences + SQLite
- 推送通知：Firebase Cloud Messaging
- 加密：E2E端到端加密

## 功能模块

- 🪨 石头广场 — 浏览和投放匿名石头
- 🌊 涟漪互动 — 对石头进行回应
- 🚣 纸船私信 — 匿名深度交流
- 👥 石友系统 — 基于共鸣的友谊关系
- 🧠 情绪追踪 — 个人情绪变化可视化
- 🔍 同频共鸣 — 语义相似度匹配
- 🛡️ 隐私保护 — 差分隐私 + E2E加密

## 环境配置

### 前置要求
- Flutter SDK >= 3.19
- Dart SDK >= 3.3
- Android Studio / Xcode

### 运行

```bash
flutter pub get
flutter run
```

### 构建

```bash
# Android
flutter build apk --release

# iOS
flutter build ios --release
```

## 项目结构

```
lib/
├── main.dart              # 应用入口
├── app/                   # 应用配置（路由、主题）
├── features/              # 功能模块
│   ├── auth/              # 认证
│   ├── stone/             # 石头
│   ├── ripple/            # 涟漪
│   ├── boat/              # 纸船
│   ├── friend/            # 好友
│   └── profile/           # 个人中心
├── core/                  # 核心层（网络、存储、加密）
├── shared/                # 共享组件和工具
└── l10n/                  # 国际化
```

## 设计规范

- Material Design 3
- 治愈系配色（湖蓝 + 暖白）
- 支持深色模式
- 无障碍访问支持
