// @file edge_ai_service.dart
// @brief EdgeAI引擎服务 - 封装用户可用的EdgeAI端点
//
// 注意：以下端点为管理后台专用（AdminAuthFilter），普通用户不可调用：
//   - /api/admin/edge-ai/moderate   （内容审核 - 后端在createStone/lakeGodChat中已内置）
//   - /api/admin/edge-ai/metrics    （性能指标）
//   - /api/admin/edge-ai/emotion-pulse （情绪脉搏）
//   - /api/admin/edge-ai/vector-search （向量搜索）
//   - /api/admin/edge-ai/federated/aggregate （联邦学习聚合）
//   - /api/admin/edge-ai/config     （配置管理）
import 'base_service.dart';

class EdgeAIService extends BaseService {
  @override
  String get serviceName => 'EdgeAIService';

  /// 获取EdgeAI引擎状态
  Future<Map<String, dynamic>> getStatus() async {
    final response = await get('/edge-ai/status');
    return toMap(response);
  }

  /// 本地情感分析
  Future<Map<String, dynamic>> analyzeSentiment(String text) async {
    final response = await post('/edge-ai/analyze', data: {'text': text});
    return toMap(response);
  }

  /// 获取隐私预算
  Future<Map<String, dynamic>> getPrivacyBudget() async {
    final response = await get('/edge-ai/privacy-budget');
    return toMap(response);
  }

  /// 获取社区情绪脉搏
  Future<Map<String, dynamic>> getEmotionPulse() async {
    final response = await get('/edge-ai/emotion-pulse');
    return toMap(response);
  }
}
