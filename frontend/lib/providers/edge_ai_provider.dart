// @file edge_ai_provider.dart
// @brief EdgeAI Provider - 管理本地AI推理状态

import 'package:flutter/foundation.dart';
import '../edge_ai/emotion_classifier.dart';
import '../data/datasources/edge_ai_service.dart';

class EdgeAIProvider extends ChangeNotifier {
  static final EdgeAIProvider _instance = EdgeAIProvider._();
  factory EdgeAIProvider() => _instance;
  EdgeAIProvider._();

  final LocalDPClassifier _classifier = LocalDPClassifier(epsilon: 2.0);
  final EdgeAIService _edgeService = EdgeAIService();
  bool _isReady = false;
  Map<String, double>? _lastResult;
  String? _lastEmotion;

  bool get isReady => _isReady;
  Map<String, double>? get lastResult => _lastResult;
  String? get lastEmotion => _lastEmotion;
  Map<String, dynamic> get privacyInfo => _classifier.privacyInfo;

  Future<void> initialize() async {
    if (_isReady) return;
    await _classifier.loadModel();
    _isReady = true;
    notifyListeners();
  }

  Future<Map<String, double>> classifyText(String text) async {
    if (!_isReady) await initialize();
    final features = _textToFeatures(text);
    final result = await _classifier.classifyWithPrivacy(features);
    _lastResult = result;
    _lastEmotion = _classifier.getTopEmotion(result);
    notifyListeners();
    return result;
  }

  Float32List _textToFeatures(String text) {
    final features = Float32List(24);
    final runes = text.runes.toList();
    for (var i = 0; i < runes.length && i < 20; i++) {
      features[i] = runes[i] / 65536.0;
    }
    features[20] = text.length / 200.0;
    features[21] =
        runes.where((r) => r > 0x4E00 && r < 0x9FFF).length /
        (runes.length + 1);
    features[22] = text.contains(RegExp(r'[!！？?]')) ? 1.0 : 0.0;
    features[23] = text.contains(RegExp(r'[。，、；]')) ? 0.5 : 0.0;
    return features;
  }

  static const Map<String, String> emotionLabels = {
    'happy': '开心',
    'sad': '悲伤',
    'angry': '愤怒',
    'fearful': '焦虑',
    'surprised': '惊喜',
    'neutral': '平静',
  };

  String getEmotionLabel(String emotion) => emotionLabels[emotion] ?? emotion;

  /// 后端情感分析
  Future<Map<String, dynamic>?> analyzeRemote(String text) async {
    try {
      final resp = await _edgeService.analyzeSentiment(text);
      if (resp.success && resp.data != null) {
        return resp.data as Map<String, dynamic>;
      }
    } catch (_) {}
    return null;
  }

  /// 内容审核
  Future<bool> moderateContent(String text) async {
    try {
      final resp = await _edgeService.moderateContent(text);
      if (resp.success && resp.data != null) {
        final data = resp.data as Map<String, dynamic>;
        return data['safe'] == true;
      }
    } catch (_) {}
    return true; // 审核失败时放行
  }

  @override
  void dispose() {
    _classifier.dispose();
    super.dispose();
  }
}
