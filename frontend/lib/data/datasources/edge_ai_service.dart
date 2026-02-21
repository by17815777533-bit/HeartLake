// @file edge_ai_service.dart
// @brief EdgeAI引擎服务 - 封装情感分析、内容审核、情绪脉搏等API
import 'base_service.dart';

class EdgeAIService extends BaseService {
  @override
  String get serviceName => 'EdgeAIService';

  /// 获取EdgeAI引擎状态
  Future<ServiceResponse> getStatus() async {
    return get('/edge-ai/status');
  }

  /// 获取性能指标
  Future<ServiceResponse> getMetrics() async {
    return get('/edge-ai/metrics');
  }

  /// 本地情感分析
  Future<ServiceResponse> analyzeSentiment(String text) async {
    return post('/edge-ai/analyze', data: {'text': text});
  }

  /// 内容审核
  Future<ServiceResponse> moderateContent(String text) async {
    return post('/edge-ai/moderate', data: {'text': text});
  }

  /// 获取情绪脉搏（实时情绪分布）
  Future<ServiceResponse> getEmotionPulse() async {
    return get('/edge-ai/emotion-pulse');
  }

  /// 获取隐私预算
  Future<ServiceResponse> getPrivacyBudget() async {
    return get('/edge-ai/privacy-budget');
  }

  /// 向量语义搜索
  Future<ServiceResponse> vectorSearch(String query, {int topK = 10}) async {
    return post('/edge-ai/vector-search', data: {
      'query': query,
      'top_k': topK,
    });
  }
}
