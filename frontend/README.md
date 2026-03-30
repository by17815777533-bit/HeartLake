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

## 当前环境变量

- `PUBLIC_ORIGIN`
- `API_BASE_URL`
- `WS_URL`
- `PRODUCTION_PUBLIC_ORIGIN`

## 当前页面范围

- 匿名登录与账号恢复
- 湖面流、石头详情、投石
- 涟漪、纸船、通知
- 好友、临时好友、守护
- 情绪日历、热力图、脉搏
- 推荐、AI 情绪分析、湖神、安全港、VIP、咨询

## 当前约束

- 发布版默认直连当前生产网关，不允许回落到占位域名。
- 数据源负责协议归一和错误抛出。
- Provider 负责状态编排，不在页面里拼接口分叉。
- WebSocket 建连必须携带 token。

## 详细代码手册

- [../docs/12_移动端代码地图与状态链手册.md](../docs/12_移动端代码地图与状态链手册.md)
- [../docs/02_API与实时链路手册.md](../docs/02_API与实时链路手册.md)
