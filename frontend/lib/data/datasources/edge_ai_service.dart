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
  Future<ServiceResponse> getStatus() async {
    return get('/edge-ai/status');
  }

  /// 本地情感分析
  Future<ServiceResponse> analyzeSentiment(String text) async {
    return post('/edge-ai/analyze', data: {'text': text});
  }

  /// 获取隐私预算
  Future<ServiceResponse> getPrivacyBudget() async {
    return get('/edge-ai/privacy/budget');
  }
}
